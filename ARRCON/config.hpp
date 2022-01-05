/**
 * @file	config.hpp
 * @author	radj307
 * @brief	Contains the config namespace, and the configuration object that reads the INI config file.
 */
#pragma once
#include "globals.h"
#include <sysarch.h>

#define INI_USE_EXPERIMENTAL
#include <str.hpp>
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
			Global.no_prompt = ini.checkv(HEADER_APPEARANCE, "bDisablePrompt", true);
			Global.enable_bukkit_color_support = str::string_to_bool<bool>(ini.getvs(HEADER_APPEARANCE, "bEnableBukkitColors").value_or(""));
			if (ini.checkv(HEADER_APPEARANCE, "bDisableColors", "true", false)) {
				Global.palette.setActive(false);
				Global.enable_bukkit_color_support = false;
			}
			Global.custom_prompt = ini.getvs(HEADER_APPEARANCE, "sCustomPrompt").value_or("");
			// Timing:
			const auto to_ms{ [](const std::optional<std::string>& str, const std::chrono::milliseconds& def) -> std::chrono::milliseconds { return ((str.has_value() && std::all_of(str.value().begin(), str.value().end(), isdigit)) ? std::chrono::milliseconds(str::stoi(str.value())) : def); } };
			Global.command_delay = to_ms(ini.getvs(HEADER_TIMING, "iCommandDelay"), 0ms);
			Global.receive_delay = to_ms(ini.getvs(HEADER_TIMING, "iReceiveDelay"), 10ms);
			Global.select_timeout = to_ms(ini.getvs(HEADER_TIMING, "iSelectTimeout"), 500ms);
			// Target:
			Global.target.hostname = ini.getvs(HEADER_TARGET, "sDefaultHost").value_or(Global.target.hostname);
			Global.target.port = ini.getvs(HEADER_TARGET, "sDefaultPort").value_or(Global.target.port);
			Global.target.password = ini.getvs(HEADER_TARGET, "sDefaultPass").value_or(Global.target.password);
		}
	}
	
	inline WINCONSTEXPR std::string MakeHeader(const std::string_view& name) { return { std::string{ " "s += name } + " "s }; }

	inline auto write_default_config(const std::string& filename)
	{
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
			<< "bEnableBukkitColors = true\n"
			<< '\n'
			<< MakeHeader(HEADER_TIMING)
			<< "iCommandDelay = 0\n"
			<< "iReceiveDelay = 10\n"
			<< "iSelectTimeout = 500\n"
			<< '\n';
		return file::write(filename, std::move(ss), false);
	}

	using HostList = std::unordered_map<std::string, HostInfo>;

	inline HostList read_hostfile(const std::string& filename) noexcept(false)
	{
		const auto hostfile{ read_config(filename) };
		HostList hosts{};
		for (auto& [name, targetinfo] : hostfile) {
			if (!targetinfo.empty())
				hosts.insert_or_assign(name, HostInfo{ str::strip_line(hostfile.getvs(name, "sHost").value_or(Global.DEFAULT_TARGET.hostname), ";#", "\""), str::strip_line(hostfile.getvs(name, "sPort").value_or(Global.DEFAULT_TARGET.port), "#;", "\""), str::strip_line(hostfile.getvs(name, "sPass").value_or(Global.DEFAULT_TARGET.password), ";#", "\"") });
		}
		return hosts;
	}

	/**
	 * @brief			Insert a given target into the hostlist
	 * @param hostlist	A reference to the host list.
	 * @param name		Name to save host info as.
	 * @param info		Host info to save.
	 * @returns			char
	 *\n				0		Host wasn't changed
	 *\n				1		Host was updated
	 *\n				2		Host was added
	 */
	inline char save_hostinfo(HostList& hostlist, const std::string& name, const HostInfo& info)
	{
		if (const auto existing{ hostlist.find(name) }; existing != hostlist.end()) { // host already exists
			if (existing->second == info)
				return 0; // host info is already set to this target
			// update host info with the given target
			existing->second = info;
			return 1;
		}
		// add new host
		hostlist.insert(std::move(std::make_pair(name, info)));
		return 2;
	}

	/// @brief HostList insertion operator used for writing it to a file.
	inline std::ostream& operator<<(std::ostream& os, const HostList& hostlist)
	{
		for (auto& [name, hostinfo] : hostlist) {
			os
				<< '[' << name << "]\n"
				<< hostinfo << '\n';
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
		std::stringstream ss;
		ss << hostlist;
		return file::write(filename, std::move(ss), false);
	}
}