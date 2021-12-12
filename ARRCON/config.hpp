/**
 * @file	config.hpp
 * @author	radj307
 * @brief	Contains the config namespace, and the configuration object that reads the INI config file.
 */
#pragma once
#include "globals.h"
#include <sysarch.h>

#include <INI.hpp>

namespace config {
	inline file::ini::INI read_config(const std::string& filename) noexcept(false)
	{
		if (file::exists(filename))
			return file::ini::INI(filename);
		throw make_exception("Cannot read \"", filename, "\" because it doesn't exist!");
	}

	inline constexpr const auto HEADER_APPEARANCE{ "appearance" }, HEADER_TIMING{ "timing" }, HEADER_TARGET{ "target" };

	inline auto apply_config(const std::string& filename)
	{
		using namespace std::chrono_literals;
		if (const auto ini{ read_config(filename) }; !ini.empty()) {
			// Appearance:
			Global.no_prompt = str::string_to_bool<bool>(ini.getv(HEADER_APPEARANCE, "bDisablePrompt").value_or(""));
			if (ini.checkv(HEADER_APPEARANCE, "bDisableColors", "true", false))
				Global.palette.setActive(false);
			Global.custom_prompt = ini.getv(HEADER_APPEARANCE, "sCustomPrompt").value_or("");
			Global.enable_bukkit_color_support = str::string_to_bool<bool>(ini.getv(HEADER_APPEARANCE, "bEnableBukkitColors").value_or(""));
			// Timing:
			const auto to_ms{ [](const std::optional<std::string>& str, const std::chrono::milliseconds& def) -> std::chrono::milliseconds { return ((str.has_value() && std::all_of(str.value().begin(), str.value().end(), isdigit)) ? std::chrono::milliseconds(str::stoi(str.value())) : def); } };
			Global.command_delay = to_ms(ini.getv(HEADER_TIMING, "iCommandDelay"), 0ms);
			Global.receive_delay = to_ms(ini.getv(HEADER_TIMING, "iReceiveDelay"), 10ms);
			Global.select_timeout = to_ms(ini.getv(HEADER_TIMING, "iSelectTimeout"), 500ms);
			// Target:
			Global.DEFAULT_HOST = ini.getv(HEADER_TARGET, "sDefaultHost").value_or(Global.DEFAULT_HOST);
			Global.DEFAULT_PORT = ini.getv(HEADER_TARGET, "sDefaultPort").value_or(Global.DEFAULT_PORT);
			Global.DEFAULT_PASS = ini.getv(HEADER_TARGET, "sDefaultPass").value_or(Global.DEFAULT_PASS);
		}
	}

	namespace _internal {
		struct MakeHeader {
			const std::string _str;
			_CONSTEXPR MakeHeader(const std::string& str) : _str{ str } {}
			friend std::ostream& operator<<(std::ostream& os, const MakeHeader& h) { return os << '[' << h._str << ']' << '\n'; }
		};
	}

	inline auto write_default_config(const std::string& filename, const bool& append = false)
	{
		using namespace _internal;
		std::stringstream ss;
		ss
			<< MakeHeader(HEADER_TARGET)
			<< "sDefaultHost = 127.0.0.1\n"
			<< "sDefaultPort = 27015\n"
			<< "sDefaultPass = \n"
			<< '\n'
			<< MakeHeader(HEADER_APPEARANCE)
			<< "bDisablePrompt = false\n"
			<< "bDisableColors = false\n"
			<< "sCustomPrompt =\n"
			<< "bEnableBukkitColors = false\n"
			<< '\n'
			<< MakeHeader(HEADER_TIMING)
			<< "iCommandDelay = 0\n"
			<< "iReceiveDelay = 10\n"
			<< "iSelectTimeout = 500\n"
			<< '\n';

		return file::write(filename, ss.rdbuf(), append);
	}

	struct HostInfo {
		std::string hostname, port, password;
		friend std::ostream& operator<<(std::ostream& os, const HostInfo& hostinfo)
		{
			return (os
				<< "sHost = " << hostinfo.hostname << '\n'
				<< "sPort = " << hostinfo.port << '\n'
				<< "sPass = " << hostinfo.password << '\n'
				).flush();
		}
	};

	using HostList = std::unordered_map<std::string, HostInfo>;

	inline HostList read_hostfile(const std::string& filename) noexcept(false)
	{
		const auto hostfile{ read_config(filename) };
		HostList hosts{};
		for (auto& [name, targetinfo] : hostfile) {
			const auto getv{ [&targetinfo](const std::string& var_name) -> std::optional<std::string> {
				if (const auto target{targetinfo.find(var_name)}; target != targetinfo.end())
					return target->second;
				return std::nullopt;
			} };
			if (!targetinfo.empty())
				hosts.insert_or_assign(name, HostInfo{ getv("sHost").value_or(Global.DEFAULT_HOST), getv("sPort").value_or(Global.DEFAULT_PORT), getv("sPass").value_or(Global.DEFAULT_PASS) });
		}
		return hosts;
	}

	/**
	 * @brief			Insert a given target into the hostlist
	 * @param hostlist	A reference to the host list.
	 * @param name		Name to save host info as.
	 * @param info		Host info to save.
	 * @returns			std::pair<iterator, bool>
	 */
	inline auto save_hostinfo(HostList& hostlist, const std::string& name, const HostInfo& info)
	{
		return hostlist.insert_or_assign(name, info);
	}

	/// @brief HostList insertion operator used for writing it to a file.
	inline std::ostream& operator<<(std::ostream& os, const HostList& hostlist)
	{
		for (auto& [name, hostinfo] : hostlist) {
			os
				<< '[' << name << "]\n"
				<< "hostname = " << hostinfo.hostname << '\n'
				<< "port = " << hostinfo.port << '\n'
				<< "password = " << hostinfo.password << "\n\n";
		}
		return os;
	}

	/**
	 * @brief			Write the given hostlist to a file.
	 * @param hostlist	Host list to write.
	 * @param filename	Target filename to write to.
	 * @returns			bool
	 */
	inline auto write_hostfile(const HostList& hostlist, const std::string& filename)
	{
		return file::write(filename, hostlist, false);
	}
}