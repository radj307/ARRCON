/**
 * @file	net.hpp
 * @author	Tiiffi (Updated & modified for C++ by radj307)
 * @brief	Contains all of the raw networking functions used by the rcon namespace.
 *\n		These functions were originally created by Tiiffi for mcrcon: (https://github.com/Tiiffi/mcrcon)
 *\n		This is the original license:
 *\n
 *\n		Copyright (c) 2012-2021, Tiiffi <tiiffi at gmail>
 *\n
 *\n		This software is provided 'as-is', without any express or implied
 *\n		warranty. In no event will the authors be held liable for any damages
 *\n		arising from the use of this software.
 *\n
 *\n		Permission is granted to anyone to use this software for any purpose,
 *\n		including commercial applications, and to alter it and redistribute it
 *\n		freely, subject to the following restrictions:
 *\n
 *\n		  1. The origin of this software must not be misrepresented; you must not
 *\n		  claim that you wrote the original software. If you use this software
 *\n		  in a product, an acknowledgment in the product documentation would be
 *\n		  appreciated but is not required.
 *\n
 *\n		  2. Altered source versions must be plainly marked as such, and must not be
 *\n		  misrepresented as being the original software.
 *\n
 *\n		  3. This notice may not be removed or altered from any source
 *\n		  distribution.
 */
#pragma once
#include "packet.hpp"

#include <TermAPI.hpp>

#include <optional>
#include <string>
#include <winsock.h>
#include <sys/socket.h>

 /**
  * @namespace	net
  * @brief		Contains functions used to interact with sockets.
  */
namespace net {
#ifdef _WIN32
	inline void init_WSA(void) noexcept(false)
	{
		WSADATA wsaData;
		if (const auto rc{ WSAStartup(WINSOCK_VERSION, &wsaData) }; rc != 0 || LOBYTE(wsaData.wVersion) != LOBYTE(WINSOCK_VERSION) || HIBYTE(wsaData.wVersion) != HIBYTE(WINSOCK_VERSION))
			throw std::exception("WSAStartup failed with error code " + rc);
	}
#endif

	/**
	 * @brief		Close the socket & call WSACleanup.
	 * @param sd	The socket descriptor to close.
	 */
	inline void close_socket(const SOCKET& sd)
	{
	#ifdef _WIN32
		closesocket(sd);
		WSACleanup();
	#else
		close(sd);
	#endif
	}

	/**
	 * @brief	Retrieve the last reported socket error code.
	 * @returns	int
	 */
	inline int lastError()
	{
	#ifdef OS_WIN
		return WSAGetLastError();
	#else
		return -1;
	#endif
	}

	/**
	 * @brief		Connect the socket to the specified RCON server.
	 * @author		Tiiffi
	 * @param host	Target server IP address or hostname.
	 * @param port	Target server port.
	 * @returns		SOCKET
	*/
	inline SOCKET connect(const std::string& host, const std::string& port)
	{
		SOCKET sd;

		struct addrinfo* server_info, * p;

		struct addrinfo hints;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

	#ifdef _WIN32
		init_WSA();
	#endif

		int ret = getaddrinfo(host.c_str(), port.c_str(), &hints, &server_info);
		if (ret != 0)
			throw std::exception(("Name resolution failed with error code "s + std::to_string(ret) + " (Last Socket Error: "s + std::to_string(lastError()) + ")").c_str());

		// Go through the hosts and try to connect
		for (p = server_info; p != NULL; p = p->ai_next) {
			sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

			if (sd == -1)
				continue;

			ret = connect(sd, p->ai_addr, static_cast<int>(p->ai_addrlen));
			if (ret == SOCKET_ERROR) {
				close_socket(sd);
				continue;
			}
			break; // connection successful, break from loop
		}

		freeaddrinfo(server_info);

		if (p == NULL)
			throw std::exception(("Connection failed with error code "s + std::to_string(lastError())).c_str());

		return sd;
	}

	/**
	 * @brief			Send a packet to the specified socket.
	 * @param sd		Socket to use.
	 * @param packet	Packet to send to the server.
	 * @returns			bool
	 */
	inline bool send_packet(const SOCKET& sd, const packet::Packet& packet)
	{
		int len;
		int total = 0;	// bytes we've sent
		int bytesleft;	// bytes left to send 
		int ret = -1;

		bytesleft = len = packet.size + sizeof(int);

		auto spacket{ packet.serialize() };

		auto* ptr = &spacket;

		while (total < len) {
			ret = send(sd, (char*)ptr + total, bytesleft, 0);
			if (ret == -1) break;
			total += ret;
			bytesleft -= ret;
		}

		return ret != -1;
	}

	inline void flush(const SOCKET& sd)
	{
		fd_set set{ 1u, sd };
		auto rc{ select(NULL, &set, NULL, NULL, NULL) };
		while (rc != 0 && rc != SOCKET_ERROR) {
			if (recv(sd, std::unique_ptr<char>{}.get(), packet::PSIZE_MAX, 0) == 0)
				throw std::exception("Connection Lost! Last Error: " + lastError());
			rc = select(NULL, &set, NULL, NULL, NULL);
		}
	}

	/**
	 * @brief		Receive a single packet from the specified socket.
	 * @param sd	Socket to use.
	 * @returns		packet::Packet
	 */
	inline packet::Packet recv_packet(const SOCKET& sd)
	{
		int psize{ NULL };

		if (int ret{ recv(sd, (char*)&psize, sizeof(int), 0) }; ret == 0)
			throw std::exception("Connection Lost! Last Error: " + lastError());
		else if (ret != sizeof(int))
			throw std::exception("Invalid packet size: " + ret);

		if (psize < packet::PSIZE_MIN) {
			std::cerr << sys::term::warn << "Received unexpectedly small packet size: " << psize << std::endl;
		}
		else if (psize > packet::PSIZE_MAX) {
			std::cerr << sys::term::warn << "Received unexpectedly large packet size: " << psize << std::endl;
			flush(sd);
		}

		packet::serialized_packet spacket{ psize, NULL, NULL, { 0x00 } }; ///< create a serialized packet to receive data

		for (int received{ 0 }, ret{ 0 }; received < psize; received += ret) {
			ret = recv(sd, (char*)&spacket + sizeof(int) + received, psize - received, 0);
			if (ret == 0)
				throw std::exception("Connection Lost! Last Error: " + lastError());
		}

		return { spacket };
	}
}