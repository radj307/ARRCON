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

 /// @def TABSPACE @brief Used to indent lines that follow a term:: message prefix.
#define TABSPACE "        "

struct permission_except final : public ex::except { permission_except(auto&& message) : except(std::forward<decltype(message)>(message)) {} };
/// @brief	Make a file permission exception
static permission_except permission_exception(const std::string& function_name, const std::filesystem::path& path, const std::string& message)
{
	return ex::make_custom_exception<permission_except>(
		"File Permissions Error:  ", message, '\n',
		TABSPACE"Target Path:     ", path, '\n',
		TABSPACE"Function Name:   ", function_name, '\n',
		TABSPACE"Suggested Solutions:\n",
		TABSPACE"1.  Change the config directory by setting the \"", Global.env.name_config_dir, "\" environment variable.\n",
		TABSPACE"2.  Change the permissions of the target directory/file to allow read/write with the current elevation level.\n"
		#ifdef OS_WIN
		TABSPACE"3.  Close the terminal, and re-open it as an administrator."
		#else
		TABSPACE"3.  Re-run the command with sudo."
		#endif
		);
}

struct socket_except final : public ex::except { socket_except(auto&& message) : except(std::forward<decltype(message)>(message)) {} };
/// @brief	Make a socket exception
static socket_except socket_exception(const std::string& function_name, const std::string& message, const int& errorCode, const std::string& errorMsg)
{
	return ex::make_custom_exception<socket_except>(
		"Socket Error:  ", message, '\n',
		TABSPACE"Function Name:         ", function_name, '\n',
		TABSPACE"Socket Error Code:     ", errorCode, '\n',
		TABSPACE"Socket Error Message:  ", errorMsg, '\n'
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
		"Connection Error:  ", message, '\n',
		TABSPACE"Function Name:         ", function_name, '\n',
		TABSPACE"Target Hostname/IP:    ", host, '\n',
		TABSPACE"Target Port:           ", port, '\n',
		TABSPACE"Socket Error Code:     ", errorCode, '\n',
		TABSPACE"Socket Error Message:  ", errorMsg, '\n',
		TABSPACE"Suggested Solutions:\n",
		TABSPACE"1.  Verify that you're using the correct IP/hostname & port.\n",
		TABSPACE"2.  Verify that the target is online.\n",
		TABSPACE"3.  Check your network connection."
		);
}
static connection_except badpass_exception(const std::string& host, const std::string& port, const int& errorCode, const std::string& errorMsg)
{
	return ex::make_custom_exception<connection_except>(
		"Connection Error:  ", "Incorrect Password!", '\n',
		TABSPACE"Target Hostname/IP:    ", host, '\n',
		TABSPACE"Target Port:           ", port, '\n',
		TABSPACE"Socket Error Code:     ", errorCode, '\n',
		TABSPACE"Socket Error Message:  ", errorMsg, '\n',
		TABSPACE"Suggested Solutions:\n",
		TABSPACE"1.  Verify the password you entered is the correct password for this target.\n"
		TABSPACE"2.  Make sure this is the correct target.\n"
		);
}
