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
	 * @param sock		Socket to use.
	 * @param passwd	RCON Password.
	 * @returns			bool
	 */
	inline bool authenticate(const SOCKET& sock, const std::string& passwd)
	{
		const auto pid{ packet::ID_Manager.get() };
		packet::rc_packet packet{ pid, packet::Type::SERVERDATA_AUTH, passwd };

		if (int ret{ net::send_packet(sock, packet) }; !ret)
			return false;

		if (const auto recpacket{ net::recv_packet(sock) }; recpacket.has_value())
			packet = recpacket.value();

		return packet.id == pid;
	}
	/**
	 * @brief			Send a command to the connected RCON server.
	 * @param sock		Socket to use.
	 * @param command	Command string to send.
	 * @returns			std::optional<packet::rc_packet>
	 */
	inline std::optional<packet::rc_packet> command(const SOCKET& sock, const std::string& command)
	{
		const auto pid{ packet::ID_Manager.get() };
		packet::rc_packet packet{ pid, packet::Type::SERVERDATA_EXECCOMMAND, command };

		net::send_packet(sock, packet);

		if (const auto recpacket{ net::recv_packet(sock) }; recpacket.has_value())
			packet = recpacket.value();

		if (packet.id != pid)
			return std::nullopt;

		return packet;
	}
}
