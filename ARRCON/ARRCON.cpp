// CMake
#include "version.h"
#include "copyright.h"

// ARRCON
#include "net/rcon.hpp"
#include "config.hpp"
#include "helpers/print_input_prompt.h"
#include "helpers/bukkit-colors.h"

// 307lib
#include <opt3.hpp>					//< for commandline argument parser & manager
#include <color-sync.hpp>			//< for color::sync
#include <envpath.hpp>				//< for env::PATH
#include <hasPendingDataSTDIN.h>	//< for hasPendingDataSTDIN

// STL
#include <filesystem>	//< for std::filesystem
#include <iostream>		//< for standard io streams

struct print_help {
	std::string exeName;

	print_help(const std::string& exeName) : exeName{ exeName } {}

	friend std::ostream& operator<<(std::ostream& os, const print_help& h)
	{
		return os << h.exeName << " v" << ARRCON_VERSION_EXTENDED << " (" << ARRCON_COPYRIGHT << ")\n"
			<< "  A robust & powerful commandline Remote-CONsole (RCON) client designed for use with the Source RCON Protocol.\n"
			<< "  It is also compatible with similar protocols such as the one used by Minecraft.\n"
			<< '\n'
			<< "  Report compatibility issues here: https://github.com/radj307/ARRCON/issues/new?template=support-request.md\n"
			<< '\n'
			<< "USAGE:" << '\n'
			<< "  " << h.exeName << " [OPTIONS] [COMMANDS]\n"
			<< '\n'
			<< "  Some arguments take additional inputs, labeled with <angle brackets>." << '\n'
			<< "  Arguments that contain spaces must be enclosed with single (\') or double(\") quotation marks." << '\n'
			<< '\n'
			<< "TARGET SPECIFIER OPTIONS:\n"
			<< "  -H, --host  <Host>          RCON Server IP/Hostname." << '\n'//"  (Default: \"" /*<< Global.DEFAULT_TARGET.hostname*/ << "\")" << '\n'
			<< "  -P, --port  <Port>          RCON Server Port." << '\n'//"         (Default: \"" /*<< Global.DEFAULT_TARGET.port*/ << "\")" << '\n'
			<< "  -p, --pass  <Pass>          RCON Server Password." << '\n'
			//<< "  -S, --saved <Host>          Use a saved host's connection information, if it isn't overridden by arguments." << '\n'
			//<< "      --save-host <H>         Create a new saved host named \"<H>\" using the current [Host/Port/Pass] value(s)." << '\n'
			//<< "      --remove-host <H>       Remove an existing saved host named \"<H>\" from the list, then exit." << '\n'
			//<< "  -l, --list-hosts            Show a list of all saved hosts, then exit." << '\n'
			<< '\n'
			<< "OPTIONS:\n"
			<< "  -h, --help                  Show this help display, then exits." << '\n'
			<< "  -v, --version               Prints the current version number, then exits." << '\n'
			<< "  -q, --quiet                 Silent/Quiet mode; prevents or minimizes console output." << '\n'
			<< "  -i, --interactive           Starts an interactive command shell after sending any scripted commands." << '\n'
			<< "  -w, --wait <ms>             Sets the number of milliseconds to wait between sending each queued command. Default: 0" << '\n'
			<< "  -t, --timeout <ms>          Sets the number of milliseconds to wait for a response before timing out. Default: 3000" << '\n'
			<< "  -n, --no-color              Disables colorized console output." << '\n'
			<< "  -Q, --no-prompt             Disables the prompt in interactive mode and/or command echo in commandline mode." << '\n'
			<< "      --no-exit               Disables handling the \"exit\" keyword in interactive mode." << '\n'
			<< "      --allow-empty           Enables sending empty (whitespace-only) commands to the server in interactive mode." << '\n'
			//	<< "      --print-env             Prints all recognized environment variables, their values, and descriptions." << '\n'
			//	<< "      --write-ini             (Over)write the INI file with the default configuration values & exit." << '\n'
			//	<< "      --update-ini            Writes the current configuration values to the INI file, and adds missing keys." << '\n'
			//	<< "  -f, --file <file>           Load the specified file and run each line as a command." << '\n'
			;
	}
};

// terminal color synchronizer
color::sync csync{};

int main_impl(const int, char**);

int main(const int argc, char** argv)
{
	try {
		return main_impl(argc, argv);
	} catch (std::exception const& ex) {
		std::cerr << csync.get_fatal() << ex.what() << std::endl;

		// try getting a stacktrace for this exception and print it
		if (const auto* trace = boost::get_error_info<traced_exception>(ex))
			std::cerr << "Stacktrace:\n" << *trace;

		return 1;
	} catch (...) {
		std::cerr << csync.get_fatal() << "An undefined error occurred!" << std::endl;
		return 1;
	}
}

int main_impl(const int argc, char** argv)
{
	const opt3::ArgManager args{ argc, argv,
		// define capturing args:
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'H', "host", "hostname"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'S', "saved"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'P', "port"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'p', "pass", "password"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'w', "wait"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 't', "timeout"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'f', "file"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, "save-host"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, "remove-host"),
	};

	// get the executable's location & name
	const auto& [programPath, programName] { env::PATH().resolve_split(argv[0]) };
	config::Locator locator{ programPath, std::filesystem::path{ programName }.replace_extension() };

	/// setup the log
	// log file stream
	std::ofstream logfs(locator.from_extension(".log"));
	// log manager object
	Logger logManager{ logfs.rdbuf() };

	// -h|--help
	if (args.empty() || args.check_any<opt3::Flag, opt3::Option>('h', "help")) {
		std::cout << print_help(programName.generic_string());
		return 0;
	}

	// -q|--quiet
	const bool quiet{ args.check_any<opt3::Flag, opt3::Option>('q', "quiet") };

	// -v|--version
	if (args.check_any<opt3::Flag, opt3::Option>('v', "version")) {
		if (!quiet) std::cout << "ARRCON v";
		std::cout << ARRCON_VERSION_EXTENDED;
		if (!quiet) std::cout << std::endl << ARRCON_COPYRIGHT;
		std::cout << std::endl;
		return 0;
	}

	// -n|--no-color
	csync.setEnabled(!args.check_any<opt3::Flag, opt3::Option>('n', "no-color"));

	// TODO: Move this elsewhere vvv
	static constexpr char const* const DEFAULT_TARGET_HOST{ "127.0.0.1" };
	static constexpr char const* const DEFAULT_TARGET_PORT{ "27015" };
	static constexpr char const* const DEFAULT_TARGET_PASS{ "" };

	// get the target server info
	const std::string target_host{ args.getv_any<opt3::Flag, opt3::Option>('H', "host", "hostname").value_or(DEFAULT_TARGET_HOST) };
	const std::string target_port{ args.getv_any<opt3::Flag, opt3::Option>('P', "port").value_or(DEFAULT_TARGET_PORT) };
	const std::string target_pass{ args.getv_any<opt3::Flag, opt3::Option>('p', "pass", "password").value_or(DEFAULT_TARGET_PASS) };

	// validate & log the target host information
	std::clog << MessageHeader(LogLevel::Info) << "Target Host: \"" << target_host << ':' << target_port << '\"' << std::endl;

	// initialize and connect the client
	net::rcon::RconClient client{ target_host, target_port };

	// -t|--timeout
	client.set_timeout(args.castgetv_any<int, opt3::Flag, opt3::Option>([](auto&& arg) { return str::stoi(std::forward<decltype(arg)>(arg)); }, 't', "timeout").value_or(3000));

	// authenticate with the server
	if (!client.authenticate(target_pass)) {
		throw make_exception("Authentication failed due to incorrect password!");
	}

	// get the list of commands from the commandline
	std::vector<std::string> commands;

	if (hasPendingDataSTDIN()) {
		// get commands from STDIN
		for (std::string buf; std::getline(std::cin, buf);) {
			commands.emplace_back(buf);
		}
	}
	// get commands from the commandline
	if (const auto parameters{ args.getv_all<opt3::Parameter>() };
		!parameters.empty()) {
		commands.insert(commands.end(), parameters.begin(), parameters.end());
	}

	const bool disablePromptAndEcho{ args.check_any<opt3::Flag, opt3::Option>('Q', "no-prompt") };

	// Oneshot Mode
	if (!commands.empty()) {
		// get the command delay, if one was specified
		std::chrono::milliseconds commandDelay;
		bool useCommandDelay{ false };
		if (const auto waitArg{ args.getv_any<opt3::Flag, opt3::Option>('w', "wait") }; waitArg.has_value()) {
			commandDelay = std::chrono::milliseconds{ str::tonumber<uint64_t>(waitArg.value()) };
			useCommandDelay = true;
		}

		// oneshot mode
		bool fst{ true };
		for (const auto& command : commands) {
			// wait for the specified number of milliseconds
			if (useCommandDelay) {
				if (fst) fst = false;
				else std::this_thread::sleep_for(commandDelay);
			}

			if (!quiet) {
				if (!disablePromptAndEcho) // print the shell prompt
					print_input_prompt(std::cout, target_host, csync);
				// echo the command
				std::cout << command << '\n';
			}

			// execute the command and print the result
			std::cout << str::trim(client.command(command)) << std::endl;
		}
	}

	bool disableExitKeyword{ args.check_any<opt3::Option>("no-exit") };
	bool allowEmptyCommands{ args.check_any<opt3::Option>("allow-empty") };

	// Interactive mode
	if (commands.empty() || args.check_any<opt3::Flag, opt3::Option>('i', "interactive")) {
		if (!disablePromptAndEcho) {
			std::cout << "Authentication Successful.\nUse <Ctrl + C>";
			if (!disableExitKeyword) std::cout << " or type \"exit\"";
			std::cout << " to quit.\n";
		}

		// interactive mode input loop
		while (true) {
			if (!quiet && !disablePromptAndEcho) // print the shell prompt
				print_input_prompt(std::cout, target_host, csync);

			// get user input
			std::string str;
			std::getline(std::cin, str);

			// validate the input
			if (!allowEmptyCommands && str::trim(str).empty()) {
				std::cerr << csync(color::cyan) << "[not sent: empty]" << csync() << '\n';
				continue;
			}
			// check for the exit keyword
			else if (!disableExitKeyword && str == "exit")
				break; //< exit on keyword input

			// send the command and get the response
			str = str::trim(client.command(str));

			if (str.empty()) {
				// response is empty
				std::cerr << csync(color::orange) << "[no response]" << csync() << '\n';
			}
			else {
				// replace minecraft bukkit color codes with ANSI sequences
				str = mc_color::replace_color_codes(str);

				// print the response
				std::cout << str << std::endl;
			}
		}
	}

	return 0;
}
