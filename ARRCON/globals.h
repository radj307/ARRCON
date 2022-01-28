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
#include <INI.hpp>
#include <env.hpp>

#include <thread>
#include <cmath>
#include <sys/socket.h>
#include <chrono>
#include <atomic>
#include <unistd.h>

 /// @brief	The default program name on each platform.
inline constexpr const auto DEFAULT_PROGRAM_NAME{
#ifdef OS_WIN
	"ARRCON.exe"
#else
	"ARRCON"
#endif
};

/// @brief	SOCKET type compatible with winsock & posix
using SOCKET = unsigned long long;

#ifndef OS_WIN
#ifndef SOCKET_ERROR // Define SOCKET_ERROR macro
#define SOCKET_ERROR -1
#endif
#endif // #ifndef OS_WIN

#define ISSUE_REPORT_URL "https://github.com/radj307/ARRCON/issues"



/**
 * @struct	HostInfo
 * @brief	Contains the connection info for a single target.
 */
struct HostInfo {
	std::string hostname, port, password;

	HostInfo() = default;
	HostInfo(const std::string& hostname, const std::string& port, const std::string& password) : hostname{ hostname }, port{ port }, password{ password } {}
	HostInfo(const file::INI::SectionContent& ini_section, const HostInfo& default_target)
	{
		// hostname:
		if (const auto host{ ini_section.find("sHost") }; host != ini_section.end())
			hostname = file::ini::to_string(host->second);
		else hostname = default_target.hostname;
		// port:
		if (const auto prt{ ini_section.find("sPort") }; prt != ini_section.end())
			port = file::ini::to_string(prt->second);
		else port = default_target.port;
		// password:
		if (const auto pass{ ini_section.find("sPass") }; pass != ini_section.end())
			password = file::ini::to_string(pass->second);
		else password = default_target.password;
	}
	HostInfo(const file::INI::SectionContent& ini_section) : HostInfo(ini_section, HostInfo()) {}

	/**
	 * @brief			Create a HostInfo struct containing values from the given optional overrides, or values from this HostInfo instance for any null overrides.
	 * @param ohost		Optional Override Hostname
	 * @param oport		Optional Override Port
	 * @param opass		Optional Override Password
	 * @returns			HostInfo
	 */
	HostInfo getWithOverride(const std::optional<std::string>& ohost, const std::optional<std::string>& oport, const std::optional<std::string>& opass) const
	{
		return{ ohost.value_or(hostname), oport.value_or(port), opass.value_or(password) };
	}

	operator file::INI::SectionContent() const
	{
		file::INI::SectionContent section;

		section.insert_or_assign("sHost", hostname);
		section.insert_or_assign("sPort", port);
		section.insert_or_assign("sPass", password);

		return section;
	}

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
	INI_KEY_HIGHLIGHT,	// used by some exceptions to highlight INI keys.
	NO_RESPONSE,		// interactive mode no response message
	ENV_VAR,			// --print-env
};

/**
 * @struct	Environment
 * @brief	Interface for interacting with environment variables.
 */
struct Environment {
	std::string name_config_dir, name_host, name_port, name_pass;

public:
	struct Values {
		std::optional<std::string> config_dir;
		std::optional<std::string> hostname;
		std::optional<std::string> port;
		std::optional<std::string> password;
	};
	Values Values;

	void load_all(const std::string& programFilename)
	{
		name_config_dir = programFilename + "_CONFIG_DIR";
		name_host = programFilename + "_HOST";
		name_port = programFilename + "_PORT";
		name_pass = programFilename + "_PASS";

		Values.config_dir = env::getvar(name_config_dir);
		Values.hostname = env::getvar(name_host);
		Values.port = env::getvar(name_port);
		Values.password = env::getvar(name_pass);
	}
};
static struct {
	/// @brief Color palette
	color::palette<UIElem> palette{
		std::make_pair(UIElem::TERM_PROMPT_NAME, color::green),
		std::make_pair(UIElem::TERM_PROMPT_ARROW, color::green),
		std::make_pair(UIElem::PACKET, color::white),
		std::make_pair(UIElem::COMMAND_ECHO, color::green),
		std::make_pair(UIElem::HOST_NAME, color::white),
		std::make_pair(UIElem::HOST_NAME_HIGHLIGHT, color::yellow),
		std::make_pair(UIElem::HOST_INFO, color::light_gray),
		std::make_pair(UIElem::INI_KEY_HIGHLIGHT, color::intense_yellow),
		std::make_pair(UIElem::NO_RESPONSE, color::orange),
		std::make_pair(UIElem::ENV_VAR, color::intense_yellow),
	};
	const HostInfo DEFAULT_TARGET{ // default target
		"localhost",
		"27015",
		""
	};
	HostInfo target{ DEFAULT_TARGET }; // active target

	Environment env;

	/// @brief	When true, response packets are not printed to the terminal
	bool quiet{ false };

	/// @brief	When true, hides the prompt in interactive mode.
	bool no_prompt{ false };

	/// @brief	When true, disables ANSI color escape sequences
	bool no_color{ false };

	/// @brief	When true & no arguments are specified, try to connect to the default target.
	bool allow_no_args{ false };

	/// @brief	When true, the user can type "exit" in interactive mode to exit. Otherwise, they must use CTRL+C
	bool allow_exit{ true };

	/// @brief	When true, a message is printed to STDOUT when a command didn't provoke any response from the server, indicating it was invalid.
	bool enable_no_response_message{ true };

	/// @brief	The name of the environment variable to check for the config directory
	std::string EnvVar_CONFIG_DIR{ std::string(DEFAULT_PROGRAM_NAME) + "_CONFIG_DIR" };

	std::string custom_prompt{};

	/// @brief	When true, the RCON socket is currently connected.
	std::atomic<bool> connected{ false };

	/// @brief	When true, support for minecraft bukkit colors is enabled, and the color mapped to UIElem::PACKET will have no effect.
	bool enable_bukkit_color_support{ true };

	/// @brief	Delay between sending each command when using commandline mode.
	std::chrono::milliseconds command_delay{ 0ll };

	/// @brief	Delay between receive calls. Changing this may break or fix multi-packet response handling. (Default is 10)
	std::chrono::milliseconds receive_delay{ 10ll };

	/// @brief	Amount of time before the select() function times out.
	std::chrono::milliseconds select_timeout{ 250ll };

	/// @brief	Global socket connected to the RCON server.
	SOCKET socket{ static_cast<SOCKET>(SOCKET_ERROR) };

	/// @brief	When true, interactive mode is started after running any commands specified on the commandline.
	bool force_interactive{ false };

	/// @brief	When entries are present, the user specified at least one [-f|--file] option.
	std::vector<std::string> scriptfiles{};
} Global;

inline std::ostream& operator<<(std::ostream& os, const Environment& e)
{
	os << "Environment Variables"
		<< "  " << Global.palette.set(UIElem::ENV_VAR) << e.name_config_dir << Global.palette.reset() << '\n'
		<< "    Current Value:  " << e.Values.config_dir.value_or("") << '\n'
		<< "    Description:\n"
		<< "      Overrides the config file search location.\n"
		<< "      When this is set, config files in other directories on the search path are ignored.\n"
		<< '\n'
		<< "  " <<  Global.palette.set(UIElem::ENV_VAR) << e.name_host << Global.palette.reset() << '\n'
		<< "    Current Value:  " << e.Values.hostname.value_or("") << '\n'
		<< "    Description:\n"
		<< "      Overrides the target hostname, unless one is specified on the commandline with [-H|--host].\n"
		<< "      When this is set, the " << Global.palette.set(UIElem::INI_KEY_HIGHLIGHT) << "sDefaultHost" << Global.palette.reset() << " key in the INI will be ignored.\n"
		<< '\n'
		<< "  " << Global.palette.set(UIElem::ENV_VAR) << e.name_port << Global.palette.reset() << '\n'
		<< "    Current Value:  " << e.Values.port.value_or("") << '\n'
		<< "    Description:\n"
		<< "      Overrides the target port, unless one is specified on the commandline with [-P|--port].\n"
		<< "      When this is set, the " << Global.palette.set(UIElem::INI_KEY_HIGHLIGHT) << "sDefaultPort" << Global.palette.reset() << " key in the INI will be ignored.\n"
		<< '\n'
		<< "  " << Global.palette.set(UIElem::ENV_VAR) << e.name_pass << Global.palette.reset() << '\n'
		<< "    Is Defined:     " << std::boolalpha << e.Values.password.has_value() << '\n'
		<< "    Description:\n"
		<< "      Overrides the target password, unless one is specified on the commandline with [-p|--pass].\n"
		<< "      When this is set, the " << Global.palette.set(UIElem::INI_KEY_HIGHLIGHT) << "sDefaultPass" << Global.palette.reset() << " key in the INI will be ignored.\n"
		;
	return os;
}

// Use platform-specific select function
#ifdef OS_WIN
/**
 * @brief		Convert a std::chrono millisecond duration to a timeval struct.
 * @param ms	Duration in milliseconds
 * @returns		timeval
 */
inline timeval make_timeout(const std::chrono::milliseconds& ms)
{
	return{ 0L, static_cast<long>(static_cast<double>(ms.count()) * 1000L) };
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
	return{ 0L, static_cast<long>(static_cast<double>(ms.count()) * 1000L) };
}
#define SELECT(nfds, rd, wr, ex, timeout) pselect(nfds, rd, wr, ex, timeout, nullptr)
#endif // #ifdef OS_WIN

