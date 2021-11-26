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
		args.typegetv<opt::Flag>('H').value_or(DEFAULT_HOST), // hostname
		args.typegetv<opt::Flag>('P').value_or(DEFAULT_PORT), // port
		args.typegetv<opt::Flag>('p').value_or("")            // password
	};
}

/**
 * @brief		Handle commandline arguments.
 * @param args	Arguments from main()
 */
inline void handle_args(const opt::ParamsAPI2& args)
{
	// help:
	if (args.check_any<opt::Flag, opt::Option>('h', "help")) {
		std::cout << Help(env::PATH().resolve_split(args.arg0().value()).second);
		std::exit(EXIT_SUCCESS);
	}
	// version:
	else if (args.check_any<opt::Flag, opt::Option>('v', "version")) {
		std::cout << DEFAULT_PROGRAM_NAME << " v" << VERSION << std::endl;
		std::exit(EXIT_SUCCESS);
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
		if (std::all_of(arg.value().begin(), arg.value().end(), isdigit))
			Global.command_delay = std::chrono::milliseconds(str::stoi(arg.value()));
		// check for appended unit
		else if (const auto dPos{ arg.value().find_first_not_of("0123456789") }; str::pos_valid(dPos)) {
			const auto& [num, unit] { str::tolower(str::split(arg.value(), dPos, true)) };
			if (!unit.empty() && unit.back() == 's' && std::all_of(num.begin(), num.end(), isdigit))
				switch (unit.front()) {
				case 's':
					Global.command_delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(str::stoi(num)));
					break;
				case 'm':
					Global.command_delay = std::chrono::milliseconds(str::stoi(num));
					break;
				case 'u':
					Global.command_delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::microseconds(str::stoi(num)));
					break;
				case 'n':
					Global.command_delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(str::stoi(num)));
					break;
				default:
					break;
				}
		}
	}
	// disable colors:
	if (const auto arg{ args.typegetv_any<opt::Option, opt::Flag>('n', "no-color") }; arg.has_value())
		Global.palette.setActive(false);
	// scriptfiles:
	for (auto& scriptfile : args.typegetv_all<opt::Option, opt::Flag>('f', "file"))
		Global.scriptfiles.emplace_back(scriptfile);
}
