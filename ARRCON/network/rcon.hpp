/**
 * @file	rcon.hpp
 * @author	radj307
 * @brief	This header expands on the functions offered by net.hpp and provides specializations for the Source RCON Protocol.
 *\n		Contains the _authenticate()_ & _command()_ functions.
 */
#pragma once
#include "net.hpp"
#include "../packet-color.hpp"

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
	 * @returns			bool
	 */
	inline bool command(const SOCKET& sd, const std::string& command)
	{
		const auto pid{ packet::ID_Manager.get() };

		if (!net::send_packet(sd, { pid, packet::Type::SERVERDATA_EXECCOMMAND, command }))
			return false;

		const auto terminator_pid{ packet::ID_Manager.get() };
		bool wait_for_term{ false }; ///< true when terminator packet was sent successfully
		std::this_thread::sleep_for(Global.receive_delay); ///< allow some time for the server to respond
		auto p{ net::recv_packet(sd) }; ///< receive first packet
		if (!Global.quiet)
			std::cout << Global.palette.set(UIElem::PACKET) << p;

		// create a file descriptor set for select()
		fd_set socket_set;
		FD_ZERO(&socket_set);
		FD_SET(sd, &socket_set);

		const auto timeout{ make_timeout(Global.select_timeout) };

		// loop while 1 socket has pending data
		for (size_t i{ 0ull }; SELECT(sd + 1ll, &socket_set, nullptr, nullptr, &timeout) == 1; p = net::recv_packet(sd), ++i) {
			if (i == 0ull && net::send_packet(sd, { terminator_pid, packet::Type::SERVERDATA_RESPONSE_VALUE, "TERM" }))
				wait_for_term = true;
			if (wait_for_term && p.id == terminator_pid) {
				if (SELECT(sd + 1ll, &socket_set, nullptr, nullptr, &timeout) == 1)
					net::flush(sd, false); // flush any remaining packets
				break;
			}
			else if (!Global.quiet)
				std::cout << p.body; ///< don't print newlines automatically
			std::this_thread::sleep_for(Global.receive_delay);
			p = {}; ///< wipe existing packet
		}
		std::cout.flush() << Global.palette.reset();; ///< flush STDOUT & reset color (interrupts before color reset call are handled by sighandler so colors don't bleed out)
		return p.id == terminator_pid || !wait_for_term; // if the last received packet has the terminator's ID, or if the terminator wasn't set
	}
}
