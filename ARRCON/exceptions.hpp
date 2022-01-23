/**
 * @file	exceptions.hpp
 * @author	radj307
 * @brief	Contains the exception overloads used by the ARRCON project.
 */
#pragma once
#include "globals.h"

#include <make_exception.hpp>
#include <str.hpp>
#include <TermAPI.hpp>

#include <filesystem>

struct permission_except final : public ex::except { permission_except(auto&& message) : except(std::forward<decltype(message)>(message)) {} };
/// @brief	Make a file permission exception
static permission_except permission_exception(const std::string& function_name, const std::filesystem::path& path, const std::string& message)
{
	return ex::make_custom_exception<permission_except>(
		        "File Permissions Error:  ", message, '\n',
		"        Target Path:             ", path, '\n',
		"        Function Name:           ", function_name, '\n',
		"        Suggested Solutions:\n",
		"        1.  Change the config directory by setting the \"", Global.EnvVar_CONFIG_DIR, "\" environment variable.\n",
		"        2.  Change the permissions of the target directory/file to allow read/write with the current elevation level.\n"
		#ifdef OS_WIN
		"        3.  Close the terminal, and re-open it as an administrator."
		#else
		"        3.  Re-run the command with sudo."
		#endif
	);
}

struct socket_except final : public ex::except { socket_except(auto&& message) : except(std::forward<decltype(message)>(message)) {} };
/// @brief	Make a socket exception
static socket_except socket_exception(const std::string& function_name, const std::string& message, const int& errorCode, const std::string& errorMsg)
{
	return ex::make_custom_exception<socket_except>(
		message, '\n',
		"        Function Name:      ", function_name, '\n',
		"        Last Socket Error:  ", '[', errorCode, "] ", errorMsg
	);
}/// @brief	Make a socket exception
static socket_except socket_exception(const std::string& function_name, const std::string& message)
{
	return ex::make_custom_exception<socket_except>(function_name, ":  ", message);
}

struct connection_except final : public ex::except { connection_except(auto&& message) : except(std::forward<decltype(message)>(message)) {} };
/// @brief	Make a connection exception
static connection_except connection_exception(const std::string& function_name, const std::string& message, const std::string& host, const std::string& port, const int& errorCode, const std::string& errorMsg)
{
	return ex::make_custom_exception<connection_except>(
		message, '\n',
		"        Function Name:       ", function_name, '\n',
		"        Target Hostname/IP:  ", host, '\n',
		"        Target Port:         ", port, '\n',
		"        Last Socket Error:   [", errorCode, "] ", errorMsg,
		"        Suggested Solutions:\n",
		"        1.  Verify that you're using the correct IP/hostname & port.\n",
		"        2.  Verify that the target is online.\n"
		"        3.  Check your network connection."
	);
}

struct print_warning {
	template<var::Streamable<std::ostream>... Ts>
	print_warning(Ts&&... message) : message{ str::stringify(std::forward<Ts>(message)...) } {}
	friend std::ostream& operator<<(std::ostream& os, const print_warning& warn)
	{
		return os << term::get_warn(!Global.no_color) << warn.message;
	}
private:
	std::string message;
};
