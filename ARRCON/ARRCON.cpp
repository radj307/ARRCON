// CMake
#include "version.h"
#include "copyright.h"

// ARRCON
#include "net/rcon.hpp"
#include "config.hpp"
#include "helpers/print_input_prompt.h"
#include "helpers/bukkit-colors.h"
#include "helpers/FileLocator.hpp"

// 307lib
#include <opt3.hpp>					//< for commandline argument parser & manager
#include <color-sync.hpp>			//< for color::sync
#include <envpath.hpp>				//< for env::PATH
#include <hasPendingDataSTDIN.h>	//< for hasPendingDataSTDIN

// STL
#include <filesystem>	//< for std::filesystem
#include <iostream>		//< for standard io streams

// Global defaults
static constexpr char const* const DEFAULT_TARGET_HOST{ "127.0.0.1" };
static constexpr char const* const DEFAULT_TARGET_PORT{ "27015" };

struct print_help {
	std::string exeName;

	print_help(const std::string& exeName) : exeName{ exeName } {}

	friend std::ostream& operator<<(std::ostream& os, const print_help& h)
	{
		return os << h.exeName << " v" << ARRCON_VERSION_EXTENDED << " (" << ARRCON_COPYRIGHT << ")\n"
			<< "  A Robust Remote-CONsole (RCON) client designed for use with the Source RCON Protocol.\n"
			<< "  It is also compatible with similar protocols such as the one used by Minecraft.\n"
			<< '\n'
			<< "  Report compatibility issues here: https://github.com/radj307/ARRCON/issues/new?template=support-request.md\n"
			<< '\n'
			<< "USAGE:" << '\n'
			<< "  " << h.exeName << " [OPTIONS] [COMMANDS]\n"
			<< '\n'
			<< "  Some arguments take additional inputs, labeled with <angle brackets>." << '\n'
			<< "  Inputs that contain spaces must be enclosed with single (\') or double(\") quotation marks." << '\n'
			<< '\n'
			<< "TARGET SPECIFIER OPTIONS:\n"
			<< "  -H, --host <Host>           RCON Server IP/Hostname.  (Default: \"" << DEFAULT_TARGET_HOST << "\")" << '\n'
			<< "  -P, --port <Port>           RCON Server Port.         (Default: \"" << DEFAULT_TARGET_PORT << "\")" << '\n'
			<< "  -p, --pass <Pass>           RCON Server Password.     (Default: \"\")" << '\n'
			<< "  -R, --recall <Name>         Recalls saved [Host|Port|Pass] values from the hosts file." << '\n'
			<< "      --save   <Name>         Saves the specified [Host|Port|Pass] as \"<Name>\" in the hosts file." << '\n'
			<< "      --remove <Name>         Removes an entry from the hosts file." << '\n'
			<< "  -l, --list                  Lists the servers currently saved in the host file." << '\n'
			<< '\n'
			<< "OPTIONS:\n"
			<< "  -h, --help                  Shows this help display, then exits." << '\n'
			<< "  -v, --version               Prints the current version number, then exits." << '\n'
			<< "  -q, --quiet                 Silent/Quiet mode; prevents or minimizes console output. Use \"-qn\" for scripts." << '\n'
			<< "  -i, --interactive           Starts an interactive command shell after sending any scripted commands." << '\n'
			<< "  -e, --echo                  Enables command echo in oneshot mode." << '\n'
			<< "  -w, --wait <ms>             Sets the number of milliseconds to wait between sending each queued command. Default: 0" << '\n'
			<< "  -t, --timeout <ms>          Sets the number of milliseconds to wait for a response before timing out. Default: 3000" << '\n'
			<< "  -n, --no-color              Disables colorized console output." << '\n'
			<< "  -Q, --no-prompt             Disables the prompt in interactive mode." << '\n'
			<< "      --no-exit               Disables handling the \"exit\" keyword in interactive mode." << '\n'
			<< "      --allow-empty           Enables sending empty (whitespace-only) commands to the server in interactive mode." << '\n'
			<< "      --print-env             Prints all recognized environment variables, their values, and descriptions." << '\n'
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
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'P', "port"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'p', "pass", "password"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'S', 'R', "saved", "recall"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, "save", "save-host"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, "rm", "remove", "rm-host" "remove-host"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'w', "wait"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 't', "timeout"),
		opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'f', "file"),
	};

	// get the executable's location & name
	const auto& [programPath, programName] { env::PATH().resolve_split(argv[0]) };
	FileLocator locator{ programPath, std::filesystem::path{ programName }.replace_extension() };

	/// setup the log
	// log file stream
	std::ofstream logfs{ locator.from_extension(".log") };
	// log manager object
	Logger logManager{ logfs.rdbuf() };
	logManager.print_header();
	// write commandline to log
	{
		const auto argVec{ opt3::vectorize(argc, argv) };
		std::clog
			<< MessageHeader(LogLevel::Debug) << "Commandline Arguments: \""
			<< str::stringify_join(argVec.begin(), argVec.end(), ' ') << '\"'
			<< std::endl;
	}

	try {
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

		std::string programNameStr{ std::filesystem::path(programName).replace_extension().generic_string() };

		// --print-env
		if (args.check<opt3::Option>("print-env")) {
			const auto
				config_dir{ env::getvar(programNameStr + "_CONFIG_DIR") },
				hostname{ env::getvar(programNameStr + "_HOST") },
				port{ env::getvar(programNameStr + "_PORT") },
				password{ env::getvar(programNameStr + "_PASS") };
			std::cout << std::boolalpha
				<< "Environment Variables" << '\n'
				<< "  " << csync(color::yellow) << programNameStr << "_CONFIG_DIR" << csync() << '\n'
				<< "    Is Defined:     " << config_dir.has_value() << '\n'
				<< "    Current Value:  " << config_dir.value_or("") << '\n'
				<< "    Description:\n"
				<< "      Overrides the config file search location.\n"
				<< "      When this is set, config files in other directories on the search path are ignored.\n"
				<< '\n'
				<< "  " << csync(color::yellow) << programNameStr << "_HOST" << csync() << '\n'
				<< "    Is Defined:     " << hostname.has_value() << '\n'
				<< "    Current Value:  " << hostname.value_or("") << '\n'
				<< "    Description:\n"
				<< "      Overrides the target hostname, unless one is specified on the commandline with [-H|--host].\n"
				//<< "      When this is set, the " << csync(color::yellow) << "sDefaultHost" << csync() << " key in the INI will be ignored.\n"
				<< '\n'
				<< "  " << csync(color::yellow) << programNameStr << "_PORT" << csync() << '\n'
				<< "    Is Defined:     " << port.has_value() << '\n'
				<< "    Current Value:  " << port.value_or("") << '\n'
				<< "    Description:\n"
				<< "      Overrides the target port, unless one is specified on the commandline with [-P|--port].\n"
				//<< "      When this is set, the " << csync(color::yellow) << "sDefaultPort" << csync() << " key in the INI will be ignored.\n"
				<< '\n'
				<< "  " << csync(color::yellow) << programNameStr << "_PASS" << csync() << '\n'
				<< "    Is Defined:     " << password.has_value() << '\n'
				<< "    Description:\n"
				<< "      Overrides the target password, unless one is specified on the commandline with [-p|--pass].\n"
				//<< "      When this is set, the " << csync(color::yellow) << "sDefaultPass" << csync() << " key in the INI will be ignored.\n"
				;
			return 0;
		}

		/// determine the target server info & operate on the hosts file
		const auto hostsfile_path{ locator.from_extension(".hosts") };
		std::optional<config::SavedHosts> hostsfile;

		// --remove|--rm|--rm-host|--remove-host
		if (const auto& arg_removeHost{ args.getv_any<opt3::Option>("rm", "remove", "rm-host", "remove-host") }; arg_removeHost.has_value()) {
			if (!std::filesystem::exists(hostsfile_path))
				throw make_exception("The hosts file hasn't been created yet. (Use \"--save\" to create one)");

			// load the hosts file directly
			ini::INI ini(hostsfile_path);

			// remove the specified entry
			if (const auto it{ ini.find(arg_removeHost.value()) }; it != ini.end())
				ini.erase(it);
			else throw make_exception("The specified saved host \"", arg_removeHost.value(), "\" doesn't exist! (Use \"--list\" to see a list of saved hosts)");

			// save the hosts file
			if (ini.write(hostsfile_path)) {
				std::cout
					<< "Successfully removed \"" << csync(color::yellow) << arg_removeHost.value() << csync() << "\" from the hosts list.\n"
					<< "Saved hosts file to " << hostsfile_path << '\n'
					;
				return 0;
			}
			else throw make_exception("Failed to save hosts file to ", hostsfile_path, '!');
		}
		// --list|--list-hosts
		else if (args.check_any<opt3::Flag, opt3::Option>('l', "list", "list-hosts", "list-host")) {
			if (!std::filesystem::exists(hostsfile_path))
				throw make_exception("The hosts file hasn't been created yet. (Use \"--save-host\" to create one)");

			// load the hosts file
			if (!hostsfile.has_value())
				hostsfile = config::SavedHosts(hostsfile_path);

			if (hostsfile->empty())
				throw make_exception("The hosts file doesn't have any entries yet. (Use \"--save-host\" to create one)");

			// if quiet was specified, get the length of the longest saved host name
			size_t longestNameLength{};
			if (quiet) {
				for (const auto& [name, _] : *hostsfile) {
					if (name.size() > longestNameLength)
						longestNameLength = name.size();
				}
			}

			// print out the hosts list
			for (const auto& [name, info] : *hostsfile) {
				if (!quiet) {
					std::cout
						<< csync(color::yellow) << name << csync() << '\n'
						<< "    Hostname:  \"" << info.host << "\"\n"
						<< "    Port:      \"" << info.port << "\"\n"
						;
				}
				else {
					std::cout
						<< csync(color::yellow) << name << csync()
						<< indent(longestNameLength + 2, name.size())
						<< "( " << info.host << ':' << info.port << " )\n"
						;
				}
			}

			return 0;
		}

		net::rcon::target_info target{
			env::getvar(programNameStr + "_HOST").value_or(DEFAULT_TARGET_HOST),
			env::getvar(programNameStr + "_PORT").value_or(DEFAULT_TARGET_PORT),
			env::getvar(programNameStr + "_PASS").value_or("")
		};

		// -S|-R|--saved|--recall
		if (const auto& arg_saved{ args.getv_any<opt3::Flag, opt3::Option>('S', 'R', "saved", "recall") }; arg_saved.has_value()) {
			if (!std::filesystem::exists(hostsfile_path))
				throw make_exception("The hosts file hasn't been created yet. (Use \"--save\" to create one)");

			// load the hosts file
			if (!hostsfile.has_value())
				hostsfile = config::SavedHosts(hostsfile_path);

			// try getting the specified saved target's info
			if (const auto savedTarget{ hostsfile->get_host(arg_saved.value()) }; savedTarget.has_value()) {
				target = savedTarget.value();
			}
			else throw make_exception("The specified saved host \"", arg_saved.value(), "\" doesn't exist! (Use \"--list\" to see a list of saved hosts)");

			std::clog << MessageHeader(LogLevel::Debug) << "Recalled saved host information for \"" << arg_saved.value() << "\": " << target << std::endl;
		}
		// -H|--host|--hostname
		if (const auto& arg_hostname{ args.getv_any<opt3::Flag, opt3::Option>('H', "host", "hostname") }; arg_hostname.has_value())
			target.host = arg_hostname.value();
		// -P|--port
		if (const auto& arg_port{ args.getv_any<opt3::Flag, opt3::Option>('P', "port") }; arg_port.has_value())
			target.port = arg_port.value();
		// -p|--pass|--password
		if (const auto& arg_password{ args.getv_any<opt3::Flag, opt3::Option>('p', "pass", "password") }; arg_password.has_value())
			target.pass = arg_password.value();

		// --save|--save-host
		if (const auto& arg_saveHost{ args.getv_any<opt3::Option>("save", "save-host") }; arg_saveHost.has_value()) {
			// load the hosts file
			if (!hostsfile.has_value()) {
				hostsfile = std::filesystem::exists(hostsfile_path)
					? config::SavedHosts(hostsfile_path)
					: config::SavedHosts();
			}

			const bool exists{ hostsfile->contains(arg_saveHost.value()) };
			auto& entry{ (*hostsfile)[arg_saveHost.value()] };

			// break early if no changes will be made
			if (exists && entry == target) {
				std::cout << "Host \"" << csync(color::yellow) << arg_saveHost.value() << csync() << "\" was already saved with the specified server info.\n";
				return 0;
			}

			// set the target
			entry = target;

			// create directory structure
			if (!std::filesystem::exists(hostsfile_path))
				std::filesystem::create_directories(hostsfile_path.parent_path());

			// write to disk
			ini::INI ini;
			hostsfile->export_to(ini);
			if (ini.write(hostsfile_path)) {
				std::cout
					<< "Host \"" << csync(color::yellow) << arg_saveHost.value() << csync() << "\" was " << (exists ? "updated" : "created") << " with the specified server info.\n"
					<< "Saved hosts file to " << hostsfile_path << '\n'
					;
				return 0;
			}
			else throw make_exception("Failed to save hosts file to ", hostsfile_path, '!');
		}

		// initialize the client
		net::rcon::RconClient client;

		// connect to the server
		client.connect(target.host, target.port);

		// -t|--timeout
		client.set_timeout(args.castgetv_any<int, opt3::Flag, opt3::Option>([](auto&& arg) { return str::stoi(std::forward<decltype(arg)>(arg)); }, 't', "timeout").value_or(3000));
		// ^ this needs to be set AFTER connecting

		// authenticate with the server
		if (!client.authenticate(target.pass)) {
			throw ExceptionBuilder()
				.line("Authentication Error:  Incorrect Password!")
				.line("Target Hostname/IP:    ", target.host)
				.line("Target Port:           ", target.port)
				.line("Suggested Solutions:")
				.line("1.  Verify the password you entered is correct.")
				.line("2.  Make sure this is the correct target.")
				.build();
		}

		/// get commands from STDIN & the commandline
		std::vector<std::string> commands;
		if (hasPendingDataSTDIN()) {
			// get commands from STDIN
			for (std::string buf; std::getline(std::cin, buf);) {
				commands.emplace_back(buf);
			}
		}
		if (const auto parameters{ args.getv_all<opt3::Parameter>() };
			!parameters.empty()) {
			commands.insert(commands.end(), parameters.begin(), parameters.end());
		}

		const bool noPrompt{ args.check_any<opt3::Flag, opt3::Option>('Q', "no-prompt") };
		const bool echoCommands{ args.check_any<opt3::Flag, opt3::Option>('e', "echo") };

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

				if (echoCommands) {
					if (!noPrompt) // print the shell prompt
						print_input_prompt(std::cout, target.host, csync);
					// echo the command
					std::cout << command << '\n';
				}

				// execute the command and print the result
				std::cout << str::trim(client.command(command)) << std::endl;
			}
		}

		const bool disableExitKeyword{ args.check_any<opt3::Option>("no-exit") };
		const bool allowEmptyCommands{ args.check_any<opt3::Option>("allow-empty") };

		// Interactive mode
		if (commands.empty() || args.check_any<opt3::Flag, opt3::Option>('i', "interactive")) {
			if (!noPrompt) {
				std::cout << "Authentication Successful.\nUse <Ctrl + C>";
				if (!disableExitKeyword) std::cout << " or type \"exit\"";
				std::cout << " to quit.\n";
			}

			// interactive mode input loop
			while (true) {
				if (!quiet && !noPrompt) // print the shell prompt
					print_input_prompt(std::cout, target.host, csync);

				// get user input
				std::string str;
				std::getline(std::cin, str);

				// check for data remaining in the socket's buffer from previous commands
				if (const auto& buffer_size{ client.buffer_size() }; buffer_size > 0) {
					std::clog << MessageHeader(LogLevel::Warning) << "The buffer contains " << buffer_size << " unexpected bytes! Dumping the buffer to STDOUT." << std::endl;

					// print the buffered data before continuing
					std::cout << str::trim(net::rcon::bytes_to_string(client.flush())) << std::endl;
				}

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
					std::cerr << csync(color::orange) << "[empty response]" << csync() << '\n';
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
	} catch (std::exception const& ex) {
		// catch & log exceptions
		std::clog << MessageHeader(LogLevel::Fatal) << ex.what() << std::endl;
		throw; //< rethrow
	}
}
