/**
 * @file	globals.h
 * @author	radj307
 * @brief	Contains global variables used in other files.
 */
#pragma once
#include "version.h"
#include "net/objects/HostInfo.hpp"

#include <color-values.h>
#include <palette.hpp>
#include <make_exception.hpp>
#include <INIRedux.hpp>
#include <env.hpp>

#include <thread>
#include <cmath>
#include <sys/socket.h>
#include <chrono>
#include <atomic>
#include <unistd.h>

 /// @brief	The default program name on each platform.
inline constexpr const auto DEFAULT_PROGRAM_NAME{ "ARRCON" };

/// @brief	SOCKET type compatible with winsock & posix
using SOCKET = unsigned long long;

#ifndef OS_WIN
#ifndef SOCKET_ERROR // Define SOCKET_ERROR macro
#define SOCKET_ERROR -1
#endif
#endif // #ifndef OS_WIN

#define ISSUE_REPORT_URL "https://github.com/radj307/ARRCON/issues"

/**
 * @enum	Color
 * @brief	Defines the color keys available to the color palette.
 *\n		To add or remove an entry, see the definition of Global::palette.
 */
enum class Color : unsigned char {
	WHITE,
	GRAY,
	RED,
	ORANGE,
	GREEN,
	BLUE,
	YELLOW,
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
	color::palette<Color> palette{
		std::make_pair(Color::GREEN, color::green),
		std::make_pair(Color::GRAY, color::gray),
		std::make_pair(Color::WHITE, color::white),
		std::make_pair(Color::RED, color::red),
		std::make_pair(Color::BLUE, color::cyan),
		std::make_pair(Color::YELLOW, color::intense_yellow),
		std::make_pair(Color::ORANGE, color::orange),
	};
	const net::HostInfo DEFAULT_TARGET{ // default target
		"localhost",
		"27015",
		""
	};
	net::HostInfo target{ DEFAULT_TARGET }; // active target

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

	/// @brief	Allows or disallows ARRCON from being able to create or delete files automatically, such as when the hostlist is empty.
	bool autoDeleteHostlist{ true };

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
	os << std::boolalpha
		<< "Environment Variables" << '\n'
		<< "  " << Global.palette.set(Color::YELLOW) << e.name_config_dir << Global.palette.reset() << '\n'
		<< "    Is Defined:     " << e.Values.config_dir.has_value() << '\n'
		<< "    Current Value:  " << e.Values.config_dir.value_or("") << '\n'
		<< "    Description:\n"
		<< "      Overrides the config file search location.\n"
		<< "      When this is set, config files in other directories on the search path are ignored.\n"
		<< '\n'
		<< "  " << Global.palette.set(Color::YELLOW) << e.name_host << Global.palette.reset() << '\n'
		<< "    Is Defined:     " << e.Values.hostname.has_value() << '\n'
		<< "    Current Value:  " << e.Values.hostname.value_or("") << '\n'
		<< "    Description:\n"
		<< "      Overrides the target hostname, unless one is specified on the commandline with [-H|--host].\n"
		<< "      When this is set, the " << Global.palette.set(Color::YELLOW) << "sDefaultHost" << Global.palette.reset() << " key in the INI will be ignored.\n"
		<< '\n'
		<< "  " << Global.palette.set(Color::YELLOW) << e.name_port << Global.palette.reset() << '\n'
		<< "    Is Defined:     " << e.Values.port.has_value() << '\n'
		<< "    Current Value:  " << e.Values.port.value_or("") << '\n'
		<< "    Description:\n"
		<< "      Overrides the target port, unless one is specified on the commandline with [-P|--port].\n"
		<< "      When this is set, the " << Global.palette.set(Color::YELLOW) << "sDefaultPort" << Global.palette.reset() << " key in the INI will be ignored.\n"
		<< '\n'
		<< "  " << Global.palette.set(Color::YELLOW) << e.name_pass << Global.palette.reset() << '\n'
		<< "    Is Defined:     " << e.Values.password.has_value() << '\n'
		<< "    Description:\n"
		<< "      Overrides the target password, unless one is specified on the commandline with [-p|--pass].\n"
		<< "      When this is set, the " << Global.palette.set(Color::YELLOW) << "sDefaultPass" << Global.palette.reset() << " key in the INI will be ignored.\n"
		;
	return os;
}

