/**
 * @file	bukkit-colors.hpp
 * @author	radj307
 * @brief	Helper functions for minecraft bukkit's color syntax.
 */
#pragma once
 // 307lib::TermAPI
#include <Sequence.hpp>		//< for ANSI::Sequence
#include <color-values.h>	//< for color codes
#include <setcolor.hpp>		//< for term::setcolor

namespace mc_color {
	inline bool color_code_to_sequence(char const ch, std::string& sequence)
	{
		switch (ch) {
		case '0': // black
			sequence = color::setcolor(color::black);
			return true;
		case '1': // dark blue
			sequence = color::setcolor(color::dark_blue);
			return true;
		case '2': // dark green
			sequence = color::setcolor(color::dark_green);
			return true;
		case '3': // dark aqua
			sequence = color::setcolor(color::dark_cyan);
			return true;
		case '4': // dark red
			sequence = color::setcolor(color::dark_red);
			return true;
		case '5': // dark purple
			sequence = color::setcolor(color::dark_purple);
			return true;
		case '6': // gold
			sequence = color::setcolor(color::gold);
			return true;
		case '7': // gray
			sequence = color::setcolor(color::gray);
			return true;
		case '8': // dark gray
			sequence = color::setcolor(color::dark_gray);
			return true;
		case '9': // blue
			sequence = color::setcolor(color::blue);
			return true;
		case 'a': // green
			sequence = color::setcolor(color::green);
			return true;
		case 'b': // aqua
			sequence = color::setcolor(color::cyan);
			return true;
		case 'c': // red
			sequence = color::setcolor(color::red);
			return true;
		case 'd': // light purple
			sequence = color::setcolor(color::light_purple);
			return true;
		case 'e': // yellow
			sequence = color::setcolor(color::yellow);
			return true;
		case 'f': // white
			sequence = color::setcolor(color::white);
			return true;
		case 'r': // reset
			sequence = color::reset;
			return true;
		case 'n': // underline
			sequence = color::underline;
			return true;
		case 'l': // bold
			sequence = color::bold;
			return true;
		case 'k': // obfuscated
			return true;
		case 'm': // strikethrough
			return true;
		case 'o': // italic
			return true;
		default:
			return{};
		}
		return false;
	}

	// The ASCII section sign character(s)
#define SECTION_SIGN "§"

	/**
	 * @brief			Replaces Minecraft Bukkit color codes in the specified
	 *					 message with the corresponding ANSI escape sequence.
	 * @param message -	The string to replace the bukkit color codes in.
	 * @returns			The converted message string.
	 */
	inline std::string replace_color_codes(std::string message)
	{
		for (size_t pos{ message.rfind(SECTION_SIGN) }, lastPos{ std::string::npos };
			 pos != std::string::npos;
			 lastPos = pos, pos = message.rfind(SECTION_SIGN, pos - 1)) {
			// We need to check the last pos to prevent previous blank
			//  replacement sequences from interfering with this one.
			// For example, "§§oo" should output "§o", not ""
			if (pos + 2 >= message.size() || pos + 2 == lastPos)
				continue;

			const auto it{ message.begin() + pos };

			if (std::string colorSequence;
				color_code_to_sequence(*(it + 2), colorSequence)) {
				message.replace(it, it + 3, colorSequence);
			}
		}

		return message;
	}
}
