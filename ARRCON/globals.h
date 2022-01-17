/**
 * @file	globals.h
 * @author	radj307
 * @brief	Contains global variables used in other files.
 */
#pragma once
#include "version.h"

#include <color-values.h>
#include <palette.hpp>
#include <make_exception.hpp>

#include <cmath>
#include <sys/socket.h>
#include <chrono>
#include <atomic>

inline constexpr const auto
DEFAULT_PROGRAM_NAME{ "ARRCON.exe" };

/**
 * @enum	UIElem
 * @brief	Defines various UI elements, used by the color palette to select appropriate colors.
 */
enum class UIElem : unsigned char {
	TERM_PROMPT_NAME,	// interactive mode prompt name
	TERM_PROMPT_ARROW,	// interactive mode prompt arrow '>'
	PACKET,				// interactive mode response
	COMMAND_ECHO,		// commandline mode command echo
	HOST_NAME,			// list-hosts command (name)
	HOST_NAME_HIGHLIGHT,// save-host command
	HOST_INFO,			// list-hosts command (hostname/port)
};

inline constexpr const auto MAX_DELAY{ std::chrono::hours(24) };

/// @brief	SOCKET type compatible with winsock & posix
using SOCKET = unsigned long long;

#ifndef OS_WIN
#ifndef SOCKET_ERROR // Define SOCKET_ERROR macro
#define SOCKET_ERROR -1
#endif
#endif // #ifndef OS_WIN

struct HostInfo {
	std::string hostname, port, password;
	friend std::ostream& operator<<(std::ostream& os, const HostInfo& hostinfo)
	{
		return (os
			<< "sHost = " << hostinfo.hostname << '\n'
			<< "sPort = " << hostinfo.port << '\n'
			<< "sPass = " << hostinfo.password << '\n'
			).flush();
	}
	bool operator==(const HostInfo& o) const { return hostname == o.hostname && port == o.port && password == o.password; }
	bool operator!=(auto&& o) const { return !operator==(std::forward<decltype(o)>(o)); }
};

static struct {
	/// @brief Color palette
	color::palette<UIElem> palette{
		std::make_pair(UIElem::TERM_PROMPT_NAME, color::setcolor{ color::green, color::format::BOLD }),
		std::make_pair(UIElem::TERM_PROMPT_ARROW, color::green),
		std::make_pair(UIElem::PACKET, color::white),
		std::make_pair(UIElem::COMMAND_ECHO, color::green),
		std::make_pair(UIElem::HOST_NAME, color::white),
		std::make_pair(UIElem::HOST_NAME_HIGHLIGHT, color::yellow),
		std::make_pair(UIElem::HOST_INFO, color::light_gray),
	};
	const HostInfo DEFAULT_TARGET{
		"localhost",
		"27015",
		""
	};
	HostInfo target{ DEFAULT_TARGET };

	/// @brief When true, response packets are not printed to the terminal
	bool quiet{ false };

	/// @brief When true, hides the prompt in interactive mode.
	bool no_prompt{ false };

	std::string custom_prompt{};

	/// @brief When true, the RCON socket is currently connected.
	bool connected{ false };

	/// @brief When true, support for minecraft bukkit colors is enabled, and the color mapped to UIElem::PACKET will have no effect.
	bool enable_bukkit_color_support{ true };

	/// @brief Delay between sending each command when using commandline mode.
	std::chrono::milliseconds command_delay{ 0ll };

	/// @brief Delay between receive calls. Changing this may break or fix multi-packet response handling. (Default is 10)
	std::chrono::milliseconds receive_delay{ 10ll };

	/// @brief amount of time before the select() function times out.
	std::chrono::milliseconds select_timeout{ 500ll };

	/// @brief Global socket connected to the RCON server.
	SOCKET socket{ static_cast<SOCKET>(SOCKET_ERROR) };

	/// @brief When true, interactive mode is started after running any commands specified on the commandline.
	bool force_interactive{ false };

	/// @brief When entries are present, the user specified at least one [-f|--file] option.
	std::vector<std::string> scriptfiles{};
} Global;

#ifdef OS_WIN
/**
 * @brief		Convert a std::chrono millisecond duration to a timeval struct.
 * @param ms	Duration in milliseconds
 * @returns		timeval
 */
inline timeval make_timeout(const std::chrono::milliseconds& ms)
{
	return{ static_cast<long>(static_cast<long double>(ms.count()) / 1000.0L), static_cast<long>(ms.count() * 1000L) };
}
#define SELECT(nfds, rd, wr, ex, timeout) select(nfds, rd, wr, ex, timeout)
#else // POSIX
/**
 * @brief		Convert a std::chrono millisecond duration to a timespec struct.
 * @param ms	Duration in milliseconds
 * @returns		timespec
 */
inline timespec make_timeout(const std::chrono::milliseconds& ms)
{
	return{ static_cast<long>(std::trunc(static_cast<long double>(ms.count()) / 1000.0L)), static_cast<long>(ms.count() * 1e+6) };
}
#define SELECT(nfds, rd, wr, ex, timeout) pselect(nfds, rd, wr, ex, timeout, nullptr)
#endif // #ifdef OS_WIN
