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
#include <envpath.hpp>

#include <str.hpp>
#include <INI.hpp>

namespace config {
	/**
	 * @brief				Retrieve the path to the directory where config files are located, depending on the current Operating System.
	 * @param program_dir	The directory where the program is located.
	 * @returns				std::filesystem::path
	 */
	inline std::filesystem::path getDirPath(const std::filesystem::path& program_dir, const std::string_view& env_var_name)
	{
		if (const auto v{ env::getvar(env_var_name) }; v.has_value())
			return{ v.value() };

		#ifdef OS_WIN
		return program_dir;
		#else
		return std::filesystem::path("/usr/local/etc/");
		#endif
	}

	/**
	 * @namespace	header
	 * @brief		Contains constant definitions for the headers in the INI
	 */
	namespace header {
		inline constexpr const auto APPEARANCE{ "appearance" }, TIMING{ "timing" }, TARGET{ "target" }, MISCELLANEOUS{"miscellaneous"};
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
		Global.command_delay = to_ms(ini.getvs(header::TIMING, "iCommandDelay"), Global.command_delay);
		Global.receive_delay = to_ms(ini.getvs(header::TIMING, "iReceiveDelay"), Global.receive_delay);
		Global.select_timeout = to_ms(ini.getvs(header::TIMING, "iSelectTimeout"), Global.select_timeout);

		// Target Header:
		Global.target.hostname = ini.getvs(header::TARGET, "sDefaultHost").value_or(Global.target.hostname);
		Global.target.port = ini.getvs(header::TARGET, "sDefaultPort").value_or(Global.target.port);
		Global.target.password = ini.getvs(header::TARGET, "sDefaultPass").value_or(Global.target.password);
		Global.allow_no_args = ini.checkv(header::TARGET, "bAllowNoArgs", true);

		// Miscellaneous Header:
		Global.allow_exit = ini.checkv(header::MISCELLANEOUS, "bInteractiveAllowExitKeyword", true);

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
				<< "bAllowNoArgs = false\n"
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
				<< '\n'
				<< '[' << header::MISCELLANEOUS << ']' << '\n'
				<< "bInteractiveAllowExitKeyword = true\n"
				<< '\n';
		}
		else { // use current settings
			ss << std::boolalpha
				<< '[' << header::TARGET << ']' << '\n'
				<< "sDefaultHost = \"" << Global.target.hostname << "\"\n"
				<< "sDefaultPort = \"" << Global.target.port << "\"\n"
				<< "sDefaultPass = \"" << Global.target.password << "\"\n"
				<< "bAllowNoArgs = " << Global.allow_no_args << '\n'
				<< '\n'
				<< '[' << header::APPEARANCE << ']' << '\n'
				<< "bDisablePrompt = " << Global.no_prompt << '\n'
				<< "bDisableColors = " << Global.no_color << '\n'
				<< "sCustomPrompt = \"" << Global.custom_prompt << "\"\n"
				<< "bEnableBukkitColors = " << Global.enable_bukkit_color_support << '\n'
				<< '\n'
				<< '[' << header::TIMING << ']' << '\n'
				<< "iCommandDelay = " << Global.command_delay.count() << '\n'
				<< "iReceiveDelay = " << Global.receive_delay.count() << '\n'
				<< "iSelectTimeout = " << Global.select_timeout.count() << '\n'
				<< '\n'
				<< '[' << header::MISCELLANEOUS << ']' << '\n'
				<< "bInteractiveAllowExitKeyword = " << Global.allow_exit << '\n'
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
		HostList hosts{};

		const auto& get_target_info{ 
			[](const file::ini::INIContainer::SectionContent& sec) -> HostInfo {
				HostInfo info;
				for (const auto& [key, val] : sec) {
					if (key == "sHost")
						info.hostname = file::ini::to_string(val, false);
					else if (key == "sPort")
						info.port = file::ini::to_string(val, false);
					else if (key == "sPass")
						info.password = file::ini::to_string(val, false);
				}
				if (info.hostname.empty()) // fill in missing hostname
					info.hostname = (Global.target.hostname.empty() ? Global.DEFAULT_TARGET.hostname : Global.target.hostname);
				if (info.port.empty()) // fill in missing port
					info.port = (Global.target.port.empty() ? Global.DEFAULT_TARGET.port : Global.target.port);
				if (info.password.empty()) // fill in missing password
					info.password = (Global.target.password.empty() ? Global.DEFAULT_TARGET.password : Global.target.password);
				return info;
			}
		};

		for (const auto& [hostname, section] : file::INI{ path })
			hosts.insert_or_assign(hostname, get_target_info(section));

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
	 * @brief			Insert an entry into the given hostlist.
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

	/**
	 * @brief			Remove an entry from the given hostlist.
	 * @param hostlist	A reference to the hostlist.
	 * @param name		The name of the target entry to remove.
	 * @returns			bool
	 *\n				true	Successfully removed the given target from the hostlist.
	 *\n				false	Specified name doesn't exist.
	 */
	inline bool remove_host_from(HostList& hostlist, const std::string& name)
	{
		return hostlist.erase(name) == 1ull;
	}
}