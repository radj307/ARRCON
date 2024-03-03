#pragma once
#include <string>	//< for std::string

namespace net::rcon {
	struct target_info {
		std::string host;
		std::string port;
		std::string pass;

		friend bool operator==(target_info const& a, target_info const& b)
		{
			return a.host == b.host && a.port == b.port && a.pass == b.pass;
		}

		friend std::ostream& operator<<(std::ostream& os, const target_info& t)
		{
			return os << t.host << ':' << t.port;
		}
	};
}
