/**
 * @file	config.hpp
 * @author	radj307
 * @brief	Contains I/O and processing functions related to ARRCON's configuration files.
 *\n		Includes both the INI config, and the hosts config.
 */
#pragma once
#include "globals.h"
#include <sysarch.h>

#include <fileutil.hpp>

#include <str.hpp>
#include <INI.hpp>

namespace config {
	/**
	 * @namespace	header
	 * @brief		Contains constant definitions for the headers in the INI
	 */
	namespace header {
		inline constexpr const auto APPEARANCE{ "appearance" }, TIMING{ "timing" }, TARGET{ "target" };
	}

	/**
	 * @brief		Read the INI config and apply its settings to the Global object.
	 * @param path	The location of the target config.
	 */
	inline bool load_ini(const std::filesystem::path& path)
	{
		if (!file::exists(path))
			return false;

		// Read the ini:
		const auto ini{ file::INI(path) };

		if (ini.empty())
			return false;

		// Appearance Header:
		Global.no_prompt = ini.checkv(header::APPEARANCE, "bDisablePrompt", true);
		Global.enable_bukkit_color_support = str::string_to_bool<bool>(ini.getvs(header::APPEARANCE, "bEnableBukkitColors").value_or(""));
		if (ini.checkv(header::APPEARANCE, "bDisableColors", "true", false)) {
			Global.palette.setActive(false);
			Global.enable_bukkit_color_support = false;
		}
		Global.custom_prompt = ini.getvs(header::APPEARANCE, "sCustomPrompt").value_or("");

		// Timing Header:
		using namespace std::chrono_literals;
		const auto to_ms{ [](const std::optional<std::string>& str, const std::chrono::milliseconds& def) -> std::chrono::milliseconds { return ((str.has_value() && std::all_of(str.value().begin(), str.value().end(), isdigit)) ? std::chrono::milliseconds(str::stoi(str.value())) : def); } };
		Global.command_delay = to_ms(ini.getvs(header::TIMING, "iCommandDelay"), 0ms);
		Global.receive_delay = to_ms(ini.getvs(header::TIMING, "iReceiveDelay"), 10ms);
		Global.select_timeout = to_ms(ini.getvs(header::TIMING, "iSelectTimeout"), 500ms);

		// Target Header:
		Global.target.hostname = ini.getvs(header::TARGET, "sDefaultHost").value_or(Global.target.hostname);
		Global.target.port = ini.getvs(header::TARGET, "sDefaultPort").value_or(Global.target.port);
		Global.target.password = ini.getvs(header::TARGET, "sDefaultPass").value_or(Global.target.password);

		return true;
	}

	/**
	 * @brief				Save the INI configuration file.
	 * @param path			Location to save config.
	 * @param use_defaults	When true, the current configuration is ignored and the output will be static.
	 * @returns				bool
	 *\n					true	Success
	 *\n					false	Failed
	 */
	inline bool save_ini(const std::filesystem::path& path, const bool& use_defaults = true)
	{
		#pragma warning (disable:26800) // use of a moved-from object: ss
		std::stringstream ss{};
		if (use_defaults) {
			ss // use defaults
				<< '[' << header::TARGET << ']' << '\n'
				<< "sDefaultHost = \"localhost\"\n"
				<< "sDefaultPort = \"27015\"\n"
				<< "sDefaultPass = \"\"\n"
				<< '\n'
				<< '[' << header::APPEARANCE << ']' << '\n'
				<< "bDisablePrompt = false\n"
				<< "bDisableColors = false\n"
				<< "sCustomPrompt = \"\"\n"
				<< "bEnableBukkitColors = true\n"
				<< '\n'
				<< '[' << header::TIMING << ']' << '\n'
				<< "iCommandDelay = 0\n"
				<< "iReceiveDelay = 10\n"
				<< "iSelectTimeout = 500\n"
				<< '\n';
		}
		else { // use current settings
			ss << std::boolalpha
				<< '[' << header::TARGET << ']' << '\n'
				<< "sDefaultHost = \"" << Global.target.hostname << "\"\n"
				<< "sDefaultPort = \"" << Global.target.port << "\"\n"
				<< "sDefaultPass = \"" << Global.target.password << "\"\n"
				<< '\n'
				<< '[' << header::APPEARANCE << ']' << '\n'
				<< "bDisablePrompt = " << Global.no_prompt << '\n'
				<< "bDisableColors = " << Global.palette.isActive() << '\n'
				<< "sCustomPrompt = \"" << Global.custom_prompt << "\"\n"
				<< "bEnableBukkitColors = " << Global.enable_bukkit_color_support << '\n'
				<< '\n'
				<< '[' << header::TIMING << ']' << '\n'
				<< "iCommandDelay = " << Global.command_delay.count() << '\n'
				<< "iReceiveDelay = " << Global.receive_delay.count() << '\n'
				<< "iSelectTimeout = " << Global.select_timeout.count() << '\n'
				<< '\n';
		}
		return file::write_to(path, std::move(ss));
		#pragma warning (default:26800) // use of a moved-from object: ss
	}

	/// @brief	Contains the list of user-saved hosts.
	using HostList = std::unordered_map<std::string, HostInfo>;

	/// @brief HostList insertion operator used when writing to file.
	inline std::ostream& operator<<(std::ostream& os, const HostList& hostlist)
	{
		for (auto& [name, hostinfo] : hostlist)
			os << '[' << name << "]\n" << hostinfo << '\n';
		return os;
	}

	/**
	 * @brief		Read & parse a given hosts file.
	 * @param path	The location of the target hosts file.
	 * @returns		HostList
	 */
	inline HostList load_hostfile(const std::filesystem::path& path) noexcept(false)
	{
		const file::INI hostfile(path);
		HostList hosts{};
		hosts.reserve(hostfile.countHeaders());
		for (auto& [name, targetinfo] : hostfile) {
			if (!targetinfo.empty())
				hosts.insert_or_assign(name, HostInfo{ str::strip_line(hostfile.getvs(name, "sHost").value_or(Global.DEFAULT_TARGET.hostname), ";#", "\""), str::strip_line(hostfile.getvs(name, "sPort").value_or(Global.DEFAULT_TARGET.port), "#;", "\""), str::strip_line(hostfile.getvs(name, "sPass").value_or(Global.DEFAULT_TARGET.password), ";#", "\"") });
		}
		return hosts;
	}

	/**
	 * @brief			Write the given hostlist to a file.
	 * @param hostlist	Host list to write.
	 * @param filename	Target filename to write to.
	 * @returns			bool
	 */
	inline auto save_hostfile(const HostList& hostlist, const std::filesystem::path& path)
	{
		#pragma warning (disable:26800) // use of a moved-from object: ss
		std::stringstream ss;
		ss << hostlist;
		return file::write_to(path, std::move(ss));
		#pragma warning (default:26800) // use of a moved-from object: ss
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
	inline char add_host_to(HostList& hostlist, const std::string& name, const HostInfo& info)
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
}