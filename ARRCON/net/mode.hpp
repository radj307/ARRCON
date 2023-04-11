/**
 * @file	mode.hpp
 * @author	radj307
 * @brief	Contains the mode namespace.
 */
#pragma once
#include <sysarch.h>
#include <term.hpp>
#include "../globals.h"
#include "rcon.hpp"

#include <str.hpp>

#include <iostream>	///< for std::cout & std::cerr
#include <thread>	///< for this_thread::sleep_for
#include <signal.h>	///< for signal handling
#include <unistd.h>	///< for signal handling
#ifndef OS_WIN
#include <cstring>	///< for something I forgot
#endif // ^ POSIX

 /**
  * @brief		Handler function for OS signals. Passed to sigaction/signal to intercept interrupts and shut down the socket correctly.
  * @param sig	The signal thrown to prompt the calling of this function.
  */
#ifdef OS_WIN
BOOL WINAPI sighandler(DWORD sig) noexcept
{
	switch (sig) {
	case CTRL_C_EVENT:
		Global.connected.store(false);
		return TRUE;
	default:
		return FALSE;
	}
}
#else
void sighandler(int sig) noexcept
{
	Global.connected.store(false);
}
#endif

inline std::istream& readline(std::istream& is, std::string& s)
{
	bool cancel{ false };
	while (!cancel) {
		if (term::kbhit()) {
			const auto c{ term::getch() };
			switch (c) {
			case '\n':
				cancel = true;
				[[fallthrough]];
			default:
				s += static_cast<char>(c);
				break;
			}
		}
	}
	return is;
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
				std::cout << Global.custom_prompt << Global.palette.set(Color::GREEN) << cmd << Global.palette.reset() << '\n';
			count += static_cast<int>(net::rcon::command(Global.socket, cmd)); // 0 or 1, command returns a boolean
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
	#ifdef OS_WIN
		if (!SetConsoleCtrlHandler(sighandler, TRUE))
			throw make_exception("Failed to install Windows Control+C handler!");
	#else
		// Register the interrupt handler:
		struct sigaction action {};
		struct sigaction oldAction;
		//memset(&action, 0, sizeof(action));
		action.sa_handler = sighandler;
		sigemptyset(&action.sa_mask);
		action.sa_flags = 0;

		sigaction(SIGINT, &action, &oldAction);
	#endif

		bool hasTriedAutoAdjustingTimeout{ false };

		// Begin interactive session:
		if (!Global.no_prompt) {
			std::cout << "Authentication Successful.\nUse <Ctrl + C> ";
			if (Global.allow_exit)
				std::cout << "or type \"exit\" ";
			std::cout << "to quit.\n";
		}

		// check std::cin.good() for CTRL+C
		while (Global.connected && std::cin.good()) {
			std::cout << Global.custom_prompt;

			std::string command;
			std::getline(std::cin, command);

			if (Global.allow_exit && command == "exit")
				break;

			if (Global.connected.load() && std::cin.good()) {
				if (!command.empty()) {
					if (!net::rcon::command(sd, command) && Global.enable_no_response_message && !Global.quiet) {
						// nothing received:
						if (!hasTriedAutoAdjustingTimeout && Global.auto_adjust_timeouts) {
							if (const auto maxTime{ Global.select_timeout * 10 }, time{ net::wait_for_packet(sd, maxTime) };
								time != maxTime) {
								Global.select_timeout = std::chrono::milliseconds{ math::CeilToNearestMultiple(time.count(), Global.select_timeout.count()) } + Global.select_timeout;
								hasTriedAutoAdjustingTimeout = true;
							}
						}
						std::cerr << Global.palette.set(Color::ORANGE) << "[no response]" << Global.palette.reset() << '\n';
					}
				}
				else std::cerr << Global.palette.set(Color::BLUE) << "[not sent: empty]" << Global.palette.reset() << '\n';
			}
			else std::cerr << Global.palette.set(Color::RED) << "[not sent: disconnected]" << '\n';
		}
		// Flush & reset colors once done.
		(std::cout << Global.palette.reset()).flush();
	}
}