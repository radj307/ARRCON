/**
 * @file	exceptions.hpp
 * @author	radj307
 * @brief	Contains the exception overloads used by the ARRCON project.
 */
#pragma once
#include "globals.h"

#include <make_exception.hpp>
#include <str.hpp>

#include <filesystem>

struct permission_except final : public ex::except { permission_except(auto&& message) : except(std::forward<decltype(message)>(message)) {} };
/// @brief	Make a file permission exception
static permission_except permission_exception(const std::string& function_name, const std::filesystem::path& path, const std::string& message)
{
	return ex::make_custom_exception<permission_except>(
		"File Permissions Error:  ", message, '\n',
		indent(10), "Target Path:     ", path, '\n',
		indent(10), "Function Name:   ", function_name, '\n',
		indent(10), "Suggested Solutions:\n",
		indent(10), "1.  Change the config directory by setting the \"", Global.env.name_config_dir, "\" environment variable.\n",
		indent(10), "2.  Change the permissions of the target directory/file to allow read/write with the current elevation level.\n",
#		ifdef OS_WIN
		indent(10), "3.  Close the terminal, and re-open it as an administrator."
#		else
		indent(10), "3.  Re-run the command with sudo."
#		endif
	);
}

struct socket_except final : public ex::except { socket_except(auto&& message) : except(std::forward<decltype(message)>(message)) {} };
/// @brief	Make a socket exception
static socket_except socket_exception(const std::string& function_name, const std::string& message, const int& errorCode, const std::string& errorMsg)
{
	return ex::make_custom_exception<socket_except>(
		"Socket Error:  ", message, '\n',
		indent(10), "Function Name:         ", function_name, '\n',
		indent(10), "Socket Error Code:     ", errorCode, '\n',
		indent(10), "Socket Error Message:  ", errorMsg
	);
}/// @brief	Make an inline socket exception
static socket_except socket_exception(const std::string& function_name, const std::string& message)
{
	return ex::make_custom_exception<socket_except>(function_name, ":  ", message);
}

struct connection_except final : public ex::except { connection_except(auto&& message) : except(std::forward<decltype(message)>(message)) {} };
/// @brief	Make a connection exception
static connection_except connection_exception(const std::string& function_name, const std::string& message, const std::string& host, const std::string& port, const int& errorCode, const std::string& errorMsg)
{
	return ex::make_custom_exception<connection_except>(
		"Connection Error:      ", message, '\n',
		indent(10), "Function Name:         ", function_name, '\n',
		indent(10), "Target Hostname/IP:    ", host, '\n',
		indent(10), "Target Port:           ", port, '\n',
		indent(10), "Socket Error Code:     ", errorCode, '\n',
		indent(10), "Socket Error Message:  ", errorMsg, '\n',
		indent(10), "Suggested Solutions:\n",
		indent(10), "1.  Verify that you're using the correct IP/hostname & port.\n",
		indent(10), "2.  Verify that the target is online, and that the server is running.\n",
		indent(10), "3.  Check your network connection.\n",
		indent(10), "4.  Reset your network adapter."
	);
}
static connection_except badpass_exception(const std::string& host, const std::string& port, const int& errorCode, const std::string& errorMsg)
{
	return ex::make_custom_exception<connection_except>(
		"Connection Error:      ", "Incorrect Password!", '\n',
		indent(10), "Target Hostname/IP:    ", host, '\n',
		indent(10), "Target Port:           ", port, '\n',
		indent(10), "Socket Error Code:     ", errorCode, '\n',
		indent(10), "Socket Error Message:  ", errorMsg, '\n',
		indent(10), "Suggested Solutions:\n",
		indent(10), "1.  Verify the password you entered is the correct password for this target.\n",
		indent(10), "2.  Make sure this is the correct target."
	);
}
