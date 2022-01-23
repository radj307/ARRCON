/**
 * @file	mode.hpp
 * @author	radj307
 * @brief	Contains the mode namespace.
 */
#pragma once
#include <sysarch.h>
#include "globals.h"
#include "network/rcon.hpp"

#include <str.hpp>

#include <signal.h>	///< signal handling
#include <unistd.h>
#ifndef OS_WIN // include <cstring> on POSIX
#include <cstring>
#endif

 /**
  * @brief		Handler function for OS signals. Passed to sigaction/signal to intercept interrupts and shut down the socket correctly.
  * @param sig	The signal thrown to prompt the calling of this function.
  */
inline void sighandler(int sig) noexcept
{
	Global.connected.store(false);
	printf("\n");
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
			if (!Global.quiet && !Global.no_prompt)
				std::cout << Global.custom_prompt << Global.palette.set(UIElem::COMMAND_ECHO) << cmd << Global.palette.reset() << '\n';
			count += static_cast<int>(rcon::command(Global.socket, cmd)); // 0 or 1, command returns a boolean
			std::this_thread::sleep_for(Global.command_delay);
		}
		return count;
	}

	/**
	 * @brief								Prompts the user for input & handles an interactive session.
	 * @param sd							Connected RCON socket descriptor.
	 * @param close_socket_before_return	When true, the socket is disconnected before returning.
	 */
	inline void interactive(const SOCKET& sd)
	{
		// Register the interrupt handler:
		struct sigaction action;
		memset(&action, 0, sizeof(action));
		action.sa_handler = sighandler;
		sigaction(SIGINT, &action, 0);

		// Begin interactive session:
		if (!Global.no_prompt) {
			std::cout << "Authentication Successful.\nUse <Ctrl + C> ";
			if (Global.allow_exit)
				std::cout << "or type \"exit\" ";
			std::cout << "to quit.\n";
		}

		while (Global.connected && std::cin.good()) {
			std::cout << Global.custom_prompt;

			std::string command;
			std::getline(std::cin, command);

			if (Global.allow_exit && str::tolower(command) == "exit")
				break;

			if (Global.connected.load() && !command.empty()) {
				rcon::command(sd, command);
				//std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		}
		// Flush & reset colors once done.
		(std::cout << Global.palette.reset()).flush();
	}

}