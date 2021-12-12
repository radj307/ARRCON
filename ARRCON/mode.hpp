/**
 * @file	mode.hpp
 * @author	radj307
 * @brief	Contains the mode namespace.
 */
#pragma once
#include "globals.h"
#include "rcon.hpp"

#include <signal.h>				///< signal handling
#include <unistd.h>

 /**
  * @brief		Handler function for OS signals. Passed to sigaction/signal to intercept interrupts and shut down the socket correctly.
  * @param sig	The signal thrown to prompt the calling of this function.
  */
inline void sighandler(int sig) noexcept
{
	Global.connected = false;
}
/**
 * @namespace	mode
 * @brief		Contains all of the mode functions.
 */
namespace mode {

	/**
	 * @brief			Execute a list of commands.
	 * @param commands	List of commands to execute, in order.
	 * @returns size_t	Number of commands successfully executed.
	 */
	inline size_t commandline(const std::vector<std::string>& commands)
	{
		size_t count{ 0ull };
		for (auto& cmd : commands) {
			if (!Global.quiet)
				std::cout << Global.custom_prompt << Global.palette.set(UIElem::COMMAND_ECHO) << cmd << Global.palette.reset() << '\n';
			count += !!rcon::command(Global.socket, cmd);
			std::this_thread::sleep_for(Global.command_delay);
		}
		return count;
	}

	/**
	 * @brief			Prompts the user for input & handles an interactive session.
	 * @param sd		Connected RCON socket
	 * @param prompt	Custom prompt string, not including the arrow.
	 */
	inline void interactive(const SOCKET& sd)
	{
		struct sigaction action;

		memset(&action, 0, sizeof(action));
		action.sa_handler = sighandler;

		sigaction(SIGINT, &action, 0); // initialize signal handler for loop

		if (!Global.no_prompt)
			std::cout << "Authentication Successful.\nUse <Ctrl + C> or type \"exit\" to exit.\n";
		while (Global.connected && std::cin.good()) {
			std::cout << Global.custom_prompt;

			std::string command;
			std::getline(std::cin, command);

			if (str::tolower(command) == "exit")
				break;

			if (Global.connected && !command.empty()) {
				rcon::command(sd, command);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		}
		(std::cout << Global.palette.reset()).flush();
	}

}