#pragma once
#include "globals.h"

// STL
#include <string>	//< for std::string
#include <ostream>	//< for std::ostream

struct InputPrompt {
	std::string hostname;
	bool enable;

	InputPrompt(std::string const& hostname, bool const enable) : hostname{ hostname }, enable{ enable } {}

	friend std::ostream& operator<<(std::ostream& os, const InputPrompt& p)
	{
		if (!p.enable)
			return os;

		return os << csync(color::green) << csync(color::bold) << "RCON@" << p.hostname << '>' << csync(color::reset_all) << ' ';
	}
};
