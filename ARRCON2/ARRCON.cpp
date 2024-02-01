// CMake
#include "version.h"
#include "copyright.h"

// ARRCON
#include "net/rcon.hpp"
#include "config.hpp"

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
			<< "  -H, --host  <Host>          RCON Server IP/Hostname.  (Default: \"" /*<< Global.DEFAULT_TARGET.hostname*/ << "\")" << '\n'
			<< "  -P, --port  <Port>          RCON Server Port.         (Default: \"" /*<< Global.DEFAULT_TARGET.port*/ << "\")" << '\n'
			<< "  -p, --pass  <Pass>          RCON Server Password." << '\n'
			<< "  -S, --saved <Host>          Use a saved host's connection information, if it isn't overridden by arguments." << '\n'
			<< "      --save-host <H>         Create a new saved host named \"<H>\" using the current [Host/Port/Pass] value(s)." << '\n'
			<< "      --remove-host <H>       Remove an existing saved host named \"<H>\" from the list, then exit." << '\n'
			<< "  -l, --list-hosts            Show a list of all saved hosts, then exit." << '\n'
			<< '\n'
			<< "OPTIONS:\n"
			<< "  -h, --help                  Show this help display, then exits." << '\n'
			<< "  -v, --version               Prints the current version number, then exits." << '\n'
			<< "  -q, --quiet                 Silent/Quiet mode; prevents or minimizes console output." << '\n'
			<< "  -i, --interactive           Starts an interactive command shell after sending any scripted commands." << '\n'
			<< "  -w, --wait <ms>             Wait for \"<ms>\" milliseconds between sending each command in oneshot mode." << '\n'
			<< "  -n, --no-color              Disable colorized console output." << '\n'
			<< "  -Q, --no-prompt             Disables the prompt in interactive mode, and command echo in commandline mode." << '\n'
			<< "      --print-env             Prints all recognized environment variables, their values, and descriptions." << '\n'
			<< "      --write-ini             (Over)write the INI file with the default configuration values & exit." << '\n'
			<< "      --update-ini            Writes the current configuration values to the INI file, and adds missing keys." << '\n'
			<< "  -f, --file <file>           Load the specified file and run each line as a command." << '\n'
			;
	}
};

// terminal color synchronizer
color::sync csync{};

void main_impl(opt3::ArgManager const&);

int main(const int argc, char** argv)
{
	// swap the std::clog buffer
	const auto original_clog_rdbuf{ std::clog.rdbuf() };

	// TODO: implement this properly:
	std::clog.rdbuf(nullptr);

	int rc{ -1 };

	try {
		const opt3::ArgManager args{ argc, argv,
			// define capturing args:
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'H', "host", "hostname"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'S', "saved"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'P', "port"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'p', "pass", "password"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'w', "wait"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'f', "file"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, "save-host"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, "remove-host"),
		};

		// get the executable's location & name
		const auto& [programPath, programName] { env::PATH().resolve_split(argv[0]) };

		// -h|--help
		if (args.empty() || args.check_any<opt3::Flag, opt3::Option>('h', "help")) {
			std::cout << print_help(programName.generic_string());
			rc = 0;
			goto BREAK;
		}

		main_impl(args); //< INVOKE MAIN IMPLEMENTATION

		rc = 0;
	} catch (std::exception const& ex) {
		std::cerr << csync.get_fatal() << ex.what() << std::endl;
		rc = 1;
	} catch (...) {
		std::cerr << csync.get_fatal() << "An undefined exception occurred!" << std::endl;
		rc = 1;
	}

BREAK:
	std::clog.rdbuf(original_clog_rdbuf);

	return rc;
}

struct InputPrompt {
	std::string hostname;
	bool enable;

	InputPrompt(std::string const& hostname, bool const enable) : hostname{ hostname }, enable{ enable } {}

	friend std::ostream& operator<<(std::ostream& os, const InputPrompt& p)
	{
		if (!p.enable)
			return os;

		return os << csync(color::green) << csync(color::bold) << "RCON@" << p.hostname << '>' << csync(color::reset_all) << ' ';
	}
};

void main_impl(opt3::ArgManager const& args)
{
	// -q|--quiet
	const bool quiet{ args.check_any<opt3::Flag, opt3::Option>('q', "quiet") };

	// -v|--version
	if (args.check_any<opt3::Flag, opt3::Option>('v', "version")) {
		if (!quiet) std::cout << "ARRCON v";
		std::cout << ARRCON_VERSION_EXTENDED;
		if (!quiet) std::cout << std::endl << ARRCON_COPYRIGHT;
		std::cout << std::endl;
		return;
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

	// authenticate with the server
	if (!client.authenticate(target_pass)) {
		throw make_exception("Authentication failed due to incorrect password!");
	}

	// get the list of commands from the commandline
	std::vector<std::string> commands;

	// get commands from STDIN
	if (hasPendingDataSTDIN()) {
		for (std::string buf; std::getline(std::cin, buf);) {
			commands.emplace_back(buf);
		}
	}
	// get commands from the commandline
	if (const auto parameters{ args.getv_all<opt3::Parameter>() };
		!parameters.empty()) {
		commands.insert(commands.end(), parameters.begin(), parameters.end());
	}

	// prompt printer object "RCON@...>"
	InputPrompt prompt{ target_host, !args.check_any<opt3::Flag, opt3::Option>('Q', "no-prompt") };

	const bool commandsProvided{ !commands.empty() };
	if (commandsProvided) {
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

			// print the prompt & echo the command
			if (!quiet)
				std::cout << prompt << command << '\n';

			// execute the command and print the result
			std::cout << str::trim(client.command(command)) << std::endl;
		}
	}
	if (!commandsProvided || args.check_any<opt3::Flag, opt3::Option>('i', "interactive")) {
		if (prompt.enable) {
			std::cout << "Authentication Successful.\nUse <Ctrl + C> ";

			std::cout << "to quit.\n";
		}

		// interactive mode
		while (true) {
			if (!quiet) std::cout << prompt;

			std::string input;
			std::getline(std::cin, input);

			// execute the command and print the result
			std::cout << str::trim(client.command(input)) << std::endl;
		}
	}
}
