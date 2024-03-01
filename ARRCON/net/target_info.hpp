#pragma once
#include <string>	//< for std::string

namespace net::rcon {
	struct target_info {
		std::string host;
		std::string port;
		std::string pass;

		friend std::ostream& operator<<(std::ostream& os, const target_info& t)
		{
			return os << t.host << ':' << t.port;
		}
	};
}
