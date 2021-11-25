/**
 * @file	globals.h
 * @author	radj307
 * @brief	Contains global variables used in other files.
 */
#pragma once
#include <ColorPalette.hpp>
#include <sys/socket.h>

enum class UIElem : unsigned char {
	TERM_PROMPT_NAME,	// interactive mode prompt name
	TERM_PROMPT_ARROW,	// interactive mode prompt arrow '>'
	PACKET,
};

/// @brief Used to set terminal colors throughout the program.
inline static color::ColorPalette<UIElem> g_palette{
	std::make_pair(UIElem::TERM_PROMPT_NAME, color::setcolor{ color::green, color::Layer::FOREGROUND, color::FormatFlag::BOLD }),
	std::make_pair(UIElem::TERM_PROMPT_ARROW, color::green),
	std::make_pair(UIElem::PACKET, color::white),
};

inline constexpr const auto
DEFAULT_HOST{ "localhost" },
DEFAULT_PORT{ "27015" };
/// @brief When true, response packets are not printed to the terminal in batch mode.
static bool g_quiet{ false };
/// @brief When true, the RCON socket is currently connected.
static bool g_connected{ false };
/// @brief Global socket connected to the RCON server.
static SOCKET g_socket{ static_cast<SOCKET>(-1) };
/// @brief Delay in seconds between each command when using batch mode.
static int g_command_delay{ 0 };
