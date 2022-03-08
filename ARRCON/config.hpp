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

/**
 * @namespace	config
 * @brief		Contains functions and objects used to interact with ARRCON's configuration files.
 */
namespace config {	
	/**
	 * @namespace	header
	 * @brief		Contains constants that correspond to the names of headers in ARRCON's configuration file.
	 */
	namespace header {
		inline constexpr const auto 
			// Appearance-related keys
			APPEARANCE{ "appearance" }, 
			// Timing-related keys
			TIMING{ "timing" }, 
			// Target-related keys
			TARGET{ "target" }, 
			// Misc keys
			MISCELLANEOUS{ "miscellaneous" };
	}

	/**
	 * @class	Locator
	 * @brief	Used to locate ARRCON's config files.
	 */
	class Locator {
		std::filesystem::path program_location;
		std::string name_no_ext;
		std::filesystem::path env_path;
		std::filesystem::path home_path;

	public:
		Locator(const std::filesystem::path& program_dir, const std::string& program_name_no_extension) : program_location{ program_dir }, name_no_ext{ program_name_no_extension }, env_path{ env::getvar(name_no_ext + "_CONFIG_DIR").value_or("") }, home_path{ env::get_home() } {}

		/**
		 * @brief		Retrieves the target location of the given file extension appended to the program name. (Excluding extension, if applicable.)
		 * @param ext	The file extension of the target file.
		 * @returns		std::filesystem::path
		 *\n			This is NOT guaranteed to exist! If no valid config file was found, the .config directory in the user's home directory is returned.
		 */
		std::filesystem::path from_extension(const std::string& ext) const
		{
			if (ext.empty())
				throw make_exception("Empty extension passed to Locator::from_extension()!");
			std::string target{ name_no_ext + ((ext.front() != '.') ? ("." + ext) : ext) };
			std::filesystem::path path;
			// 1:  check the environment
			if (!env_path.empty()) {
				path = env_path / target;
				return path;
			}
			// 2:  check the program directory. (support portable versions by checking this before the user's home dir)
			if (path = program_location / target; file::exists(path))
				return path;
			// 3:  user's home directory:
			path = home_path / ".config" / name_no_ext / target;
			return path; // return even if it doesn't exist
		}
	};

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
		Global.autoDeleteHostlist = ini.checkv(header::MISCELLANEOUS, "bAutoDeleteHostlist", true);

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
		std::filesystem::create_directories(std::filesystem::path(path).remove_filename());
		#pragma warning (disable:26800) // use of a moved-from object: ss
		std::stringstream ss{};
		if (use_defaults) {
			ss // use defaults
				<< '[' << ::config::header::TARGET << ']' << '\n'
				<< "sDefaultHost = \"localhost\"\n"
				<< "sDefaultPort = \"27015\"\n"
				<< "sDefaultPass = \"\"\n"
				<< "bAllowNoArgs = false\n"
				<< '\n'
				<< '[' << ::config::header::APPEARANCE << ']' << '\n'
				<< "bDisablePrompt = false\n"
				<< "bDisableColors = false\n"
				<< "sCustomPrompt = \"\"\n"
				<< "bEnableBukkitColors = true\n"
				<< '\n'
				<< '[' << ::config::header::TIMING << ']' << '\n'
				<< "iCommandDelay = 0\n"
				<< "iReceiveDelay = 10\n"
				<< "iSelectTimeout = 500\n"
				<< '\n'
				<< '[' << ::config::header::MISCELLANEOUS << ']' << '\n'
				<< "bInteractiveAllowExitKeyword = true\n"
				<< "bEnableNoResponseMessage = true\n"
				<< "bAutoDeleteHostlist = true\n"
				<< '\n';
		}
		else { // use current settings
			ss << std::boolalpha
				<< '[' << ::config::header::TARGET << ']' << '\n'
				<< "sDefaultHost = \"" << Global.target.hostname << "\"\n"
				<< "sDefaultPort = \"" << Global.target.port << "\"\n"
				<< "sDefaultPass = \"" << Global.target.password << "\"\n"
				<< "bAllowNoArgs = " << Global.allow_no_args << '\n'
				<< '\n'
				<< '[' << ::config::header::APPEARANCE << ']' << '\n'
				<< "bDisablePrompt = " << Global.no_prompt << '\n'
				<< "bDisableColors = " << Global.no_color << '\n'
				<< "sCustomPrompt = \"" << Global.custom_prompt << "\"\n"
				<< "bEnableBukkitColors = " << Global.enable_bukkit_color_support << '\n'
				<< '\n'
				<< '[' << ::config::header::TIMING << ']' << '\n'
				<< "iCommandDelay = " << Global.command_delay.count() << '\n'
				<< "iReceiveDelay = " << Global.receive_delay.count() << '\n'
				<< "iSelectTimeout = " << Global.select_timeout.count() << '\n'
				<< '\n'
				<< '[' << ::config::header::MISCELLANEOUS << ']' << '\n'
				<< "bInteractiveAllowExitKeyword = " << Global.allow_exit << '\n'
				<< "bEnableNoResponseMessage = " << Global.enable_no_response_message << '\n'
				<< "bAutoDeleteHostlist = " << Global.autoDeleteHostlist << '\n'
				<< '\n';
		}
		return file::write_to(path, std::move(ss));
		#pragma warning (default:26800) // use of a moved-from object: ss
	}

	/**
	 * @brief			Write the given hostlist to a file.
	 * @param hostlist	Host list to write.
	 * @param filename	Target filename to write to.
	 * @returns			bool
	 */
	inline auto save_hostfile(const net::HostList& hostlist, const std::filesystem::path& path)
	{
		std::filesystem::create_directories(std::filesystem::path(path).remove_filename());
		#pragma warning (disable:26800) // use of a moved-from object: ss
		std::stringstream ss;
		ss << hostlist;
		return file::write_to(path, std::move(ss), false);
		#pragma warning (default:26800) // use of a moved-from object: ss
	}
}
