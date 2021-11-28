/**
 * @file	globals.h
 * @author	radj307
 * @brief	Contains global variables used in other files.
 */
#pragma once
#include <ColorPalette.hpp>

#include <sys/socket.h>
#include <chrono>

inline constexpr const auto
VERSION{ "1.1.0" },
DEFAULT_PROGRAM_NAME{ "ARRCON.exe" };

/**
 * @enum	UIElem
 * @brief	Defines various UI elements, used by the color palette to select appropriate colors.
 */
enum class UIElem : unsigned char {
	TERM_PROMPT_NAME,	// interactive mode prompt name
	TERM_PROMPT_ARROW,	// interactive mode prompt arrow '>'
	PACKET,				// interactive mode response
	PACKET_EMPTY,		// interactive mode empty response [ No Response ]
};

inline constexpr const auto MAX_DELAY{ std::chrono::hours(24) };

static struct {
	/// @brief Color palette
	color::ColorPalette<UIElem> palette{
		std::make_pair(UIElem::TERM_PROMPT_NAME, color::setcolor{ color::green, color::Layer::FOREGROUND, color::FormatFlag::BOLD }),
		std::make_pair(UIElem::TERM_PROMPT_ARROW, color::green),
		std::make_pair(UIElem::PACKET, color::white),
		std::make_pair(UIElem::PACKET_EMPTY, color::light_gray),
	};
	std::string
		DEFAULT_HOST{ "localhost" },
		DEFAULT_PORT{ "27015" },
		DEFAULT_PASS{ "" };


	/// @brief When true, response packets are not printed to the terminal
	bool quiet{ false };

	/// @brief When true, hides the prompt in interactive mode.
	bool no_prompt{ false };

	std::string custom_prompt{};

	/// @brief When true, the RCON socket is currently connected.
	bool connected{ false };

	/// @brief Delay between sending each command when using commandline mode.
	std::chrono::milliseconds command_delay{ 0ll };

	/// @brief Delay between receive calls. Changing this may break or fix multi-packet response handling. (Default is 10)
	std::chrono::milliseconds receive_delay{ 10ll };

	/// @brief amount of time before the select() function times out.
	std::chrono::milliseconds select_timeout{ 500ll };

	std::string ini_path;

	/// @brief Global socket connected to the RCON server.
	SOCKET socket{ static_cast<SOCKET>(SOCKET_ERROR) };

	/// @brief When true, interactive mode is started after running any commands specified on the commandline.
	bool force_interactive{ false };

	/// @brief When entries are present, the user specified at least one [-f|--file] option.
	std::vector<std::string> scriptfiles{};
} Global;

