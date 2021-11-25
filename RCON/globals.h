/**
 * @file	globals.h
 * @author	radj307
 * @brief	Contains global variables used in other files.
 */
#pragma once
#include <ColorPalette.hpp>
#include <sys/socket.h>
#include <mutex>

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

} Global;

/// @brief Used to set terminal colors throughout the program.
inline static color::ColorPalette<UIElem> g_palette{
	std::make_pair(UIElem::TERM_PROMPT_NAME, color::setcolor{ color::green, color::Layer::FOREGROUND, color::FormatFlag::BOLD }),
	std::make_pair(UIElem::TERM_PROMPT_ARROW, color::green),
	std::make_pair(UIElem::PACKET, color::white),
};

inline constexpr const auto
DEFAULT_HOST{ "localhost" },
DEFAULT_PORT{ "27015" };
/// @brief When true, response packets are not printed to the terminal
static bool g_quiet{ false };
/// @brief When true, the RCON socket is currently connected.
static bool g_connected{ false };
/// @brief Global socket connected to the RCON server.
static SOCKET g_socket{ static_cast<SOCKET>(-1) };
/// @brief Delay in seconds between each command when using batch mode.
static unsigned g_command_delay{ 0u };

static std::mutex g_mutex;

static bool g_wait_for_listener{ false };


