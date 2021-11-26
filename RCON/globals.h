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
VERSION{ "1.0.0" },
DEFAULT_PROGRAM_NAME{ "RCON.exe" },
DEFAULT_HOST{ "localhost" },
DEFAULT_PORT{ "27015" };

enum class UIElem : unsigned char {
	TERM_PROMPT_NAME,	// interactive mode prompt name
	TERM_PROMPT_ARROW,	// interactive mode prompt arrow '>'
	PACKET,				// interactive mode response
	PACKET_EMPTY,		// interactive mode empty response [ No Response ]
};

static struct {
	color::ColorPalette<UIElem> palette{
		std::make_pair(UIElem::TERM_PROMPT_NAME, color::setcolor{ color::green, color::Layer::FOREGROUND, color::FormatFlag::BOLD }),
		std::make_pair(UIElem::TERM_PROMPT_ARROW, color::green),
		std::make_pair(UIElem::PACKET, color::white),
		std::make_pair(UIElem::PACKET_EMPTY, color::light_gray),
	};
	/// @brief When true, response packets are not printed to the terminal
	bool quiet{ false };

	/// @brief When true, hides the prompt in interactive mode.
	bool no_prompt{ false };

	/// @brief When true, the RCON socket is currently connected.
	bool connected{ false };

	/// @brief Delay between sending each command when using commandline mode.
	std::chrono::milliseconds command_delay{ 0u };

	/// @brief Global socket connected to the RCON server.
	SOCKET socket{ static_cast<SOCKET>(SOCKET_ERROR) };

	/// @brief When true, interactive mode is started after running any commands specified on the commandline.
	bool force_interactive{ false };

	/// @brief When entries are present, the user specified at least one [-f|--file] option.
	std::vector<std::string> scriptfiles{};
} Global;

