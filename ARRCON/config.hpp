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
	 * @brief				Retrieve the path to the directory where config files are located, depending on the current Operating System & whether or not the given environment variable name is set.
	 * @param program_dir	The directory where the program is located.
	 * @returns				std::filesystem::path
	 *\n					This can be the value of the ${env_var_name} environment variable, or the default path for the current OS.
	 *\n					Windows:	${program_dir}
	 *\n					Linux:		`~/.config/ARRCON`
	 */
	inline std::filesystem::path getDirPath(const std::filesystem::path& program_dir, const std::string_view& env_var_name)
	{
		if (const auto v{ env::getvar(env_var_name) }; v.has_value()) {
			std::filesystem::path path(v.value());
			return path;
		}

		#ifdef OS_WIN
		return program_dir;
		#else
		const auto home{ env::getvar("HOME") }; // get the home directory of the current user.
		if (!home.has_value())
			throw make_exception("Failed to retrieve the $HOME environment variable!");
		const std::filesystem::path default_linux_config_dir{ home.value() + "/.config/ARRCON" };
		// create directory if it doesn't exist
		std::filesystem::create_directories(default_linux_config_dir);
		return default_linux_config_dir;
		#endif
	}

	/**
	 * @namespace	header
	 * @brief		Contains constant definitions for the headers in the INI
	 */
	namespace header {
		inline constexpr const auto APPEARANCE{ "appearance" }, TIMING{ "timing" }, TARGET{ "target" }, MISCELLANEOUS{ "miscellaneous" };
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
		Global.enable_no_response_message = ini.checkv(header::MISCELLANEOUS, "bEnableNoResponseMessage", true);

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
				<< "bEnableNoResponseMessage = true\n"
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
				<< "bEnableNoResponseMessage = " << Global.enable_no_response_message << '\n'
				<< '\n';
		}
		return file::write_to(path, std::move(ss));
		#pragma warning (default:26800) // use of a moved-from object: ss
	}

	using HostList = file::INI;

	/**
	 * @brief		Read & parse a given hosts file.
	 * @param path	The location of the target hosts file.
	 * @returns		HostList
	 */
	inline HostList load_hostfile(const std::filesystem::path& path) noexcept(false)
	{
		return file::INI(path);
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
}


/**
 * @brief				Create a HostInfo object from an INI section.
 * @param ini_section	A reference to an INI section container.
 * @returns				HostInfo
 */
inline HostInfo to_hostinfo(const file::INI::SectionContent& ini_section)
{
	HostInfo info;

	// hostname:
	if (const auto host{ ini_section.find("sHost") }; host != ini_section.end())
		info.hostname = file::ini::to_string(host->second);
	else info.hostname = Global.DEFAULT_TARGET.hostname;
	// port:
	if (const auto port{ ini_section.find("sPort") }; port != ini_section.end())
		info.port = file::ini::to_string(port->second);
	else info.port = Global.DEFAULT_TARGET.port;
	// password:
	if (const auto pass{ ini_section.find("sPass") }; pass != ini_section.end())
		info.password = file::ini::to_string(pass->second);
	else info.password = Global.DEFAULT_TARGET.password;

	return info;
}

/**
 * @brief			Copy a hostinfo object's values into an INI section reference.
 * @param section	Reference to an existing INI section.
 * @param info		HostInfo to insert.
 * @returns			file::INI::SectionContent
 */
inline file::INI::SectionContent& from_hostinfo(file::INI::SectionContent& section, const HostInfo& info)
{
	section.insert_or_assign("sHost", info.hostname);
	section.insert_or_assign("sPort", info.port);
	section.insert_or_assign("sPass", info.password);

	return section;
}

/**
 * @brief		Create an INI section from a hostinfo object.
 * @param info	HostInfo to insert.
 * @returns		file::INI::SectionContent
 */
inline file::INI::SectionContent from_hostinfo(const HostInfo& info)
{
	file::INI::SectionContent section;
	return from_hostinfo(section, info);
}