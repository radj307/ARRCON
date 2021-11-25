#pragma once
#include "globals.h"
#include "rcon.hpp"

namespace mode {
	// Batch execute all command parameters mode
	inline size_t batch(const std::vector<std::string>& commands)
	{
		size_t count{ 0ull };
		for (auto& cmd : commands) {
			rcon::command(g_socket, cmd);
			std::this_thread::sleep_for(std::chrono::milliseconds(g_command_delay));
		}
		return count;
	}

	// interactive mode
	inline void interactive(const SOCKET& sock, const std::string& prompt = "RCON")
	{
		using namespace std::chrono_literals;
		std::cout << "Authentication Successful.\nUse <Ctrl + C> or type \"exit\" to exit.\n";
		const auto prompt_str{ str::stringify(g_palette.set(UIElem::TERM_PROMPT_NAME), prompt, g_palette.reset(UIElem::TERM_PROMPT_ARROW), '>', g_palette.reset(), ' ') };

		while (g_connected) {
			// wait for the listener thread to finish its operation
			while (g_wait_for_listener) { std::this_thread::sleep_for(50us); }

			{
				std::scoped_lock<std::mutex> lock(g_mutex); // lock the mutex
				while (!packet::Queue.empty()) {
					const auto p{ packet::Queue.pop() };
					const auto p_empty{ p.body.empty() };
					if (p_empty)
						std::cout << g_palette.set(UIElem::PACKET_EMPTY) << "[ No Response ]" << g_palette.reset() << std::endl;
					else if (!g_quiet) {
						// print out the packet
						std::cout << g_palette.set(UIElem::PACKET) << p << g_palette.reset();
						if (p.body.back() != '\n')
							std::cout << std::endl;
					}
					std::this_thread::sleep_for(50us); // sleep for 50us
				}
			} // mutex is unlocked

			std::string command;

			std::cout << prompt_str;
			std::getline(std::cin, command);

			if (const auto lc_com{ str::tolower(command) }; lc_com == "exit")
				break;

			if (command.size() > 0 && g_connected) {
				g_wait_for_listener = true;
				rcon::command(sock, command);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		}
	}

}