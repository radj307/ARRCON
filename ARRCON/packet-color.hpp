/**
 * @file	packet-color.hpp
 * @author	radj307
 * @brief	Contains packet color handling & printing functions.
 */
#pragma once
#include "net/objects/packet.hpp"
#include <Sequence.hpp>
#include <color-values.h>
#include <setcolor.hpp>


namespace mc_color {
	inline ANSI::sequence to_sequence(const char& ch)
	{
		using namespace ANSI;
		switch (ch) {
		case '0': // black
			return color::setcolor(color::black);
		case '1': // dark blue
			return color::setcolor(color::dark_blue);
		case '2': // dark green
			return color::setcolor(color::dark_green);
		case '3': // dark aqua
			return color::setcolor(color::dark_cyan);
		case '4': // dark red
			return color::setcolor(color::dark_red);
		case '5': // dark purple
			return color::setcolor(color::dark_purple);
		case '6': // gold
			return color::setcolor(color::gold);
		case '7': // gray
			return color::setcolor(color::gray);
		case '8': // dark gray
			return color::setcolor(color::dark_gray);
		case '9': // blue
			return color::setcolor(color::blue);
		case 'a': // green
			return color::setcolor(color::green);
		case 'b': // aqua
			return color::setcolor(color::cyan);
		case 'c': // red
			return color::setcolor(color::red);
		case 'd': // light purple
			return color::setcolor(color::light_purple);
		case 'e': // yellow
			return color::setcolor(color::yellow);
		case 'f': // white
			return color::setcolor(color::white);
		case 'r': // reset
			return color::reset;
		case 'n': // underline
			return color::underline;
		case 'l': // bold
			return color::bold;
		case 'k': // obfuscated
		case 'm': [[fallthrough]]; // strikethrough
		case 'o': [[fallthrough]]; // italic
		default:
			return{};
		}
	}
}

/**
 * @brief			Packet stream insertion operator.
 * @param os		Output Stream.
 * @param packet	Packet instance.
 * @returns			std::ostream&
 */
inline std::ostream& operator<<(std::ostream& os, const net::packet::Packet& packet)
{
	if (Global.enable_bukkit_color_support) {
		for (auto ch{ packet.body.begin() }; ch != packet.body.end(); ++ch) {
			switch (*ch) {
			case -62: // discard first part of section sign when represented in ASCII
				break;
			case -89: // '�' // second part of ASCII section sign
				if (std::distance(ch, packet.body.end()) > 1ull)
					os << mc_color::to_sequence(*++ch);
				break;
			default:
				os << *ch;
				break;
			}
		}
	}
	else os << packet.body;
	if (!packet.body.empty() && packet.body.back() != '\n')
		os << '\n'; // print newline if packet doesn't already have one
	return os.flush();
}
