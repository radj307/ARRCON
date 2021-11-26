/**
 * @file	mode.hpp
 * @author	radj307
 * @brief	Contains the mode namespace.
 */
#pragma once
#include "globals.h"
#include "rcon.hpp"

 /**
  * @namespace	mode
  * @brief		Contains all of the mode functions.
  */
namespace mode {
	/**
	 * @brief			Execute a list of commands.
	 * @param commands	List of commands to execute, in order.
	 * @returns			size_t
	 */
	inline size_t commandline(const std::vector<std::string>& commands)
	{
		size_t count{ 0ull };
		for (auto& cmd : commands) {
			rcon::command(Global.socket, cmd);
			std::this_thread::sleep_for(Global.command_delay);
		}
		return count;
	}

	/**
	 * @brief			Prompts the user for input & handles an interactive session.
	 * @param sd		Connected RCON socket
	 * @param prompt	Custom prompt string, not including the arrow.
	 */
	inline void interactive(const SOCKET& sd, const std::string& prompt = "RCON")
	{
		if (!Global.no_prompt)
			std::cout << "Authentication Successful.\nUse <Ctrl + C> or type \"exit\" to exit.\n";
		const auto prompt_str{ str::stringify(Global.palette.set(UIElem::TERM_PROMPT_NAME), prompt, Global.palette.reset(UIElem::TERM_PROMPT_ARROW), '>', Global.palette.reset(), ' ') };

		while (Global.connected) {
			std::string command;

			if (!Global.no_prompt)
				std::cout << prompt_str;
			std::getline(std::cin, command);

			if (const auto lc_com{ str::tolower(command) }; lc_com == "exit")
				break;

			if (Global.connected && !command.empty()) {
				rcon::command(sd, command);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		}
	}

}