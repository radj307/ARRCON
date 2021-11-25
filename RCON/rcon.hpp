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
	 * @brief			Wraps the net::send_packet & net::recv_packet functions so multi-packet responses are handled correctly
	 * @param sd		RCON Socket descriptor.
	 * @param packet	Packet to send to the server.
	 * @returns			size_t
	 *					Contains the number of valid packets received from the server in response.
	 */
	inline size_t send_wrapper(const SOCKET& sd, const packet::Packet& packet)
	{
		using namespace net;
		using namespace std::chrono_literals;

		if (send_packet(sd, packet)) { ///< send packet
			size_t received_count{ 0ull };
			// send a terminator packet so we know when the server is done responding
			const auto term_pid{ packet::ID_Manager.get() };
			send_packet(sd, { term_pid, packet::Type::SERVERDATA_RESPONSE_VALUE, "TERMINATOR" });


			//std::this_thread::sleep_for(50ms);
			bool received_term{ false };
			for (auto p{ recv_packet(sd) }; !received_term; p = recv_packet(sd)) {
				if (!g_quiet)
					std::cout << p.body;
				received_term = p.id == term_pid && p.type == packet::Type::SERVERDATA_RESPONSE_VALUE && p.body == "TERMINATOR";
				if (received_term)
					break;
				else if (p.id == packet.id) {
					++received_count;
				}
				else
					std::cerr << sys::term::warn << "Received unexpected packet ID: " << p.id << " (Expected: " << packet.id << ")" << std::endl;

				std::this_thread::sleep_for(50ms);
			}

			return received_count;
		}
		else throw std::exception(("Command \""s + packet.body + "\" failed!"s).c_str());
		std::cout << std::endl;
		return 0ull;
	}
	/**
	 * @brief			Authenticate with the connected RCON server.
	 * @param sock		Socket to use.
	 * @param passwd	RCON Password.
	 * @returns			bool
	 */
	inline bool authenticate(const SOCKET& sock, const std::string& passwd)
	{
		const auto pid{ packet::ID_Manager.get() };
		packet::Packet packet{ pid, packet::Type::SERVERDATA_AUTH, passwd };

		if (net::send_packet(sock, packet)) {
			packet = net::recv_packet(sock);
			return packet.id == pid;
		}
		return false;

	}
	/**
	 * @brief			Send a command to the connected RCON server.
	 * @param sock		Socket to use.
	 * @param command	Command string to send.
	 * @returns			std::optional<packet::Packet>
	 */
	inline bool command(const SOCKET& sock, const std::string& command)
	{
		const auto pid{ packet::ID_Manager.get() };

		if (!net::send_packet(sock, { pid, packet::Type::SERVERDATA_EXECCOMMAND, command }))
			return false;

	#ifdef MULTITHREADING
		return true;
	#else
		const auto terminator_pid{ packet::ID_Manager.get() };
		bool wait_for_term{ false };
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // allow some time for the server to respond
		auto p{ net::recv_packet(sock) };

		std::cout << p;
		fd_set socket_set{ 1u, sock };
		const TIMEVAL timeout{ 0L, 500L };
		for (size_t i{ 0ull }; select(NULL, &socket_set, NULL, NULL, &timeout) == 1; p = net::recv_packet(sock), ++i) {
			if (i == 0ull && net::send_packet(sock, { terminator_pid, packet::Type::SERVERDATA_RESPONSE_VALUE, "TERM" }))
				wait_for_term = true;
			if (wait_for_term && p.id == terminator_pid) {
				net::flush(sock);
				break;
			}
			else std::cout << p.body; ///< don't print newlines automatically
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		std::cout << std::endl; ///< print newline & flush STDOUT
		return p.id == terminator_pid || !wait_for_term; // if the last received packet has the terminator's ID, or if the terminator wasn't set
	#endif
	}
}
