#pragma once
// 307lib::TermAPI
#include <color-sync.hpp>

// STL
#include <string>	//< for std::string
#include <ostream>	//< for std::ostream

/**
 * @brief				Pretty-prints an input prompt (ex: "RCON@...>") to the specified output stream.
 * @param os			The output stream to print to.
 * @param hostname		The hostname text to show in the prompt.
 * @param csync			The terminal color synchronizer object to use.
 */
inline void print_input_prompt(std::ostream& os, std::string const& hostname, color::sync& csync)
{
	os << csync(color::green) << csync(color::bold) << "RCON@" << hostname << '>' << csync(color::reset_all) << ' ';
}
