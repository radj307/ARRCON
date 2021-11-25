#pragma once
#include "globals.h"
#include "rcon.hpp"

namespace mode {
	// Batch execute all command parameters mode
	inline int batch(const std::vector<std::string>& commands)
	{
		int count{ 0 };
		for (auto& cmd : commands) {
			const auto packet{ rcon::command(g_socket, cmd) };
			if (packet.has_value()) {
				++count;
				if (!g_quiet)
					std::cout << packet.value();
			}
			if (g_command_delay != 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(g_command_delay));
		}
		return count;
	}

	// interactive mode
	inline void interactive(const SOCKET& sock, const std::string& prompt = "RCON")
	{
		std::cout << "Authentication Successful.\nUse <Ctrl+C> or type \"exit\" to exit." << std::endl;
		while (g_connected) {
			std::cout << g_palette.set(UIElem::TERM_PROMPT_NAME) << prompt << g_palette.reset(UIElem::TERM_PROMPT_ARROW) << '>' << g_palette.reset() << ' ';

			std::string command;
			std::getline(std::cin, command);

			if (const auto lc_com{ str::tolower(command) }; lc_com == "exit" || !g_connected)
				break;

			if (command.size() > 0 && g_connected) {
				const auto rc{ rcon::command(sock, command) };
				if (rc.has_value() && !g_quiet) {
					std::cout << rc.value();
				}
			}
		}
	}

}