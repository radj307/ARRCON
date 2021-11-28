#pragma once
#include "globals.h"
#include "Help.hpp"
#include <ParamsAPI2.hpp>
#include <env.hpp>

/**
 * @brief		Retrieve the user's specified connection target.
 * @param args	Arguments from main().
 * @returns		std::tuple<std::string, std::string, std::string>
 *\n			0	RCON Hostname
 *\n			1	RCON Port
 *\n			2	RCON Password
 */
inline std::tuple<std::string, std::string, std::string> get_server_info(const opt::ParamsAPI2& args)
{
	return{
		args.typegetv<opt::Flag>('H').value_or(Global.DEFAULT_HOST), // hostname
		args.typegetv<opt::Flag>('P').value_or(Global.DEFAULT_PORT), // port
		args.typegetv<opt::Flag>('p').value_or(Global.DEFAULT_PASS)  // password
	};
}

/**
 * @brief		Handle commandline arguments.
 * @param args	Arguments from main()
 */
inline void handle_args(const opt::ParamsAPI2& args, const std::string& program_name)
{
	// help:
	if (args.check_any<opt::Flag, opt::Option>('h', "help")) {
		std::cout << Help(program_name);
		std::exit(EXIT_SUCCESS);
	}
	// version:
	else if (args.check_any<opt::Flag, opt::Option>('v', "version")) {
		std::cout << DEFAULT_PROGRAM_NAME << " v" << VERSION << std::endl;
		std::exit(EXIT_SUCCESS);
	}
	// write-ini:
	if (args.check_any<opt::Option>("write-ini")) {
		if (!Global.ini_path.empty() && config::write_default_config(Global.ini_path)) {
			std::cout << "Successfully wrote to config: \"" << Global.ini_path << '\"' << std::endl;
			std::exit(EXIT_SUCCESS);
		}
		else throw std::exception(("I/O operation failed: \""s + Global.ini_path + "\" couldn't be written to."s).c_str());
	}
	// quiet:
	if (args.check_any<opt::Option, opt::Flag>('q', "quiet"))
		Global.quiet = true;
	// no-prompt
	if (args.check_any<opt::Option>("no-prompt"))
		Global.no_prompt = true;
	// force interactive:
	if (args.check_any<opt::Option, opt::Flag>('i', "interactive"))
		Global.force_interactive = true;
	// command delay:
	if (const auto arg{ args.typegetv_any<opt::Flag, opt::Option>('d', "delay") }; arg.has_value()) {
		if (std::all_of(arg.value().begin(), arg.value().end(), isdigit)) {
			if (const auto t{ std::chrono::milliseconds(std::abs(str::stoll(arg.value()))) }; t <= MAX_DELAY)
				Global.command_delay = t;
			else throw std::exception(("Cannot set a delay value longer than "s + std::to_string(MAX_DELAY.count()) + " hours!"s).c_str());
		}
		else throw std::exception(("Invalid delay value given: \""s + arg.value() + "\", expected an integer."s).c_str());
	}
	// disable colors:
	if (const auto arg{ args.typegetv_any<opt::Option, opt::Flag>('n', "no-color") }; arg.has_value())
		Global.palette.setActive(false);
	// scriptfiles:
	for (auto& scriptfile : args.typegetv_all<opt::Option, opt::Flag>('f', "file"))
		Global.scriptfiles.emplace_back(scriptfile);
}
