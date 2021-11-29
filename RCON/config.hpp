/**
 * @file	config.hpp
 * @author	radj307
 * @brief	Contains the config namespace, and the configuration object that reads the INI config file.
 */
#pragma once
#include "globals.h"

#define USE_DEPRECATED_INI
#include <INI.hpp>

namespace config {
	inline file::ini::INI read_config(const std::string& filename) noexcept(false)
	{
		return (file::exists(filename) ? file::ini::INI(filename) : file::ini::INI{});
	}

	inline constexpr const auto HEADER_APPEARANCE{ "appearance" }, HEADER_TIMING{ "timing" }, HEADER_TARGET{ "target" };

	inline auto apply_config(const std::string& filename)
	{
		using namespace std::chrono_literals;
		if (const auto ini{ read_config(filename) }; !ini.empty()) {
			// Appearance:
			Global.no_prompt = str::string_to_bool(ini.getv("appearance", "bDisablePrompt").value_or("")).value_or(false);
			if (ini.checkv("appearance", "bDisableColors", "true", false))
				Global.palette.setActive(false);
			Global.custom_prompt = ini.getv("appearance", "sCustomPrompt").value_or("");
			// Timing:
			const auto to_ms{ [](const std::optional<std::string>& str, const std::chrono::milliseconds& def) -> std::chrono::milliseconds { return ((str.has_value() && std::all_of(str.value().begin(), str.value().end(), isdigit)) ? std::chrono::milliseconds(str::stoi(str.value())) : def); } };
			Global.command_delay = to_ms(ini.getv("timing", "iCommandDelay"), 0ms);
			Global.receive_delay = to_ms(ini.getv("timing", "iReceiveDelay"), 10ms);
			Global.select_timeout = to_ms(ini.getv("timing", "iSelectTimeout"), 500ms);
			// Target:
			Global.DEFAULT_HOST = ini.getv("target", "sHost").value_or(Global.DEFAULT_HOST);
			Global.DEFAULT_PORT = ini.getv("target", "sPort").value_or(Global.DEFAULT_PORT);
			Global.DEFAULT_PASS = ini.getv("target", "sPass").value_or(Global.DEFAULT_PASS);
		}
	}

	namespace _internal {
		struct MakeHeader {
			const std::string _str;
			constexpr MakeHeader(const std::string& str) : _str{ str } {}
			friend std::ostream& operator<<(std::ostream& os, const MakeHeader& h) { return os << '[' << h._str << ']' << '\n'; }
		};
	}

	inline auto write_default_config(const std::string& filename, const bool& append = false)
	{
		using namespace _internal;
		std::stringstream ss;
		ss
			<< MakeHeader(HEADER_TARGET)
			<< "sHost =\n"
			<< "sPort =\n"
			<< "sPass =\n"
			<< '\n'
			<< MakeHeader(HEADER_APPEARANCE)
			<< "bDisablePrompt = false\n"
			<< "bDisableColors = false\n"
			<< "sCustomPrompt =\n"
			<< '\n'
			<< MakeHeader(HEADER_TIMING)
			<< "iCommandDelay = 0\n"
			<< "iReceiveDelay = 10\n"
			<< "iSelectDelay = 500\n"
			<< '\n';

		return file::write(filename, ss, append);
	}
}