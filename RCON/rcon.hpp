/**
 * @file	rcon.hpp
 * @author	radj307
 * @brief	Contains the rcon namespace.
 *\n		__Functions:__
 *\n		- authenticate
 *\n			Used to authenticate with the connected RCON server.
 *\n		- command
 *\n			Used to send a command to the connected RCON server.
 */
#pragma once
#include "net.hpp"

 /**
  * @namespace	rcon
  * @brief		Contains functions used to interact with the RCON server.
  */
namespace rcon {
	/**
	 * @brief			Authenticate with the connected RCON server.
	 * @param sd		Socket to use.
	 * @param passwd	RCON Password.
	 * @returns			bool
	 */
	inline bool authenticate(const SOCKET& sd, const std::string& pass)
	{
		const auto pid{ packet::ID_Manager.get() };
		packet::Packet packet{ pid, packet::Type::SERVERDATA_AUTH, pass };

		if (net::send_packet(sd, packet)) {
			packet = net::recv_packet(sd);
			return packet.id == pid;
		}
		return false;
	}
	/**
	 * @brief			Send a command to the connected RCON server.
	 * @param sd		Socket to use.
	 * @param command	Command string to send.
	 * @returns			std::optional<packet::Packet>
	 */
	inline bool command(const SOCKET& sd, const std::string& command)
	{
		const auto pid{ packet::ID_Manager.get() };

		if (!net::send_packet(sd, { pid, packet::Type::SERVERDATA_EXECCOMMAND, command }))
			return false;

		const auto terminator_pid{ packet::ID_Manager.get() };
		bool wait_for_term{ false }; ///< true when terminator packet was sent successfully
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); ///< allow some time for the server to respond
		auto p{ net::recv_packet(sd) }; ///< receive first packet
		std::cout << p;

		// init required vars for the select function
		fd_set socket_set{ 1u, sd };
		const TIMEVAL timeout{ 0L, 500L };

		// loop while 1 socket has pending data
		for (size_t i{ 0ull }; select(NULL, &socket_set, NULL, NULL, &timeout) == 1; p = net::recv_packet(sd), ++i) {
			if (i == 0ull && net::send_packet(sd, { terminator_pid, packet::Type::SERVERDATA_RESPONSE_VALUE, "TERM" }))
				wait_for_term = true;
			if (wait_for_term && p.id == terminator_pid) {
				net::flush(sd);
				break;
			}
			else std::cout << p.body; ///< don't print newlines automatically
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			p = {}; ///< wipe existing packet
		}
		std::cout << std::endl; ///< print newline & flush STDOUT
		return p.id == terminator_pid || !wait_for_term; // if the last received packet has the terminator's ID, or if the terminator wasn't set
	}
}
