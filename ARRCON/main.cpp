#include "globals.h"
#include "mode.hpp"				///< RCON client modes
#include "config.hpp"			///< INI functions
#include "Help.hpp"				///< CLI usage instructions

#include <make_exception.hpp>
#include <ParamsAPI2.hpp>
#include <fileio.hpp>			///< file I/O functions
#include <TermAPI.hpp>			///< file I/O functions
#include <hasPendingDataSTDIN.h>

#include <signal.h>				///< signal handling
#include <unistd.h>

// undefine unistd macros for fileio.hpp compat
#undef read
#undef write

struct permission_except final : public except {
	permission_except(auto&& message) : except(std::forward<decltype(message)>(message)) {}
	void update_message(const std::filesystem::path& path)
	{
		message = str::stringify(
			"File Permissions Error:  ", message, " at location ", path, '\n',
			"        Suggested Solutions:\n",
			"        1.  Change the config directory by setting the \"", Global.EnvVar_CONFIG_DIR, "\" environment variable.\n",
			"        2.  Change the permissions of the target directory/file to allow read/write with the current elevation level.\n"
			#ifdef OS_WIN
			"        3.  Close the terminal, and re-open it as an administrator.\n"
			#else
			"        3.  Re-run the command with sudo.\n"
			#endif
		);
	}
};

/**
 * @brief		Retrieve the user's specified connection target.
 * @param args	Arguments from main().
 * @returns		std::tuple<std::string, std::string, std::string>
 *\n			0	RCON Hostname
 *\n			1	RCON Port
 *\n			2	RCON Password
 */
inline HostInfo get_target_info(const opt::ParamsAPI2& args, const config::HostList& hostlist)
{
	const auto
		host{ args.typegetv_any<opt::Flag, opt::Option>('H', "host") },
		port{ args.typegetv_any<opt::Flag, opt::Option>('P', "port") },
		pass{ args.typegetv_any<opt::Flag, opt::Option>('p', "pass") };

	if (host.has_value()) { // check saved hosts first
		const auto it{ std::find_if(hostlist.begin(), hostlist.end(), [&host](auto&& kvpr) {
			return kvpr.first == host.value();
		}) };
		if (it != hostlist.end())
			return it->second;
	}

	return{
		host.value_or(Global.DEFAULT_TARGET.hostname),
		port.value_or(Global.DEFAULT_TARGET.port), // port
		pass.value_or(Global.DEFAULT_TARGET.password)  // password
	};
}

/**
 * @brief		Handle commandline arguments.
 * @param args	Arguments from main()
 */
inline void handle_args(const opt::ParamsAPI2& args, config::HostList& hosts, const HostInfo& target, const std::filesystem::path& ini_path, const std::filesystem::path& hostfile_path)
{
	const auto do_list_hosts{ args.check<opt::Option>("list-hosts") };
	// remove-host
	if (const auto remove_hosts{ args.typegetv_all<opt::Option>("remove-host") }; !remove_hosts.empty()) {
		for (const auto& name : remove_hosts) {
			if (config::remove_host_from(hosts, name)) // removed target successfully
				std::cout << term::get_msg(!Global.no_color) << "Removed " << Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT) << name << Global.palette.reset() << std::endl;
			else
				std::cerr << term::get_error(!Global.no_color) << "Hostname \"" << Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT) << name << Global.palette.reset() << " doesn't exist!" << std::endl;
		}

		if (config::save_hostfile(hosts, hostfile_path)) // print a success message or throw failure exception
			std::cout << term::get_msg(!Global.no_color) << "Saved modified hostlist to " << hostfile_path << std::endl;
		else {
			auto exception{ make_custom_exception<permission_except>("Failed to write modified hostfile to disk!") };
			exception.update_message(hostfile_path);
			throw exception;
		}

		if (!do_list_hosts) std::exit(EXIT_SUCCESS);
	}
	// save-host
	else if (const auto save_host{ args.typegetv<opt::Option>("save-host") }; save_host.has_value()) {
		switch (config::add_host_to(hosts, save_host.value(), target)) {
		case 0: // Host already exists, and has the same target
			throw make_exception("Host ", Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT), save_host.value(), Global.palette.reset(), " is already set to ", target.hostname, ':', target.port, '\n');
		case 1: // Host already exists, but with a different target
			std::cout << term::msg << "Updated " << Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT) << save_host.value() << Global.palette.reset() << ": " << target.hostname << ':' << target.port << '\n';
			break;
		case 2: // Added new host
			std::cout << term::get_msg(!Global.no_color) << "Added host: " << Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT) << save_host.value() << Global.palette.reset() << " " << target.hostname << ':' << target.port << '\n';
			break;
		default:
			throw make_exception("Received an undefined return value while saving host!");
		}

		if (config::save_hostfile(hosts, hostfile_path)) // print a success message or throw failure exception
			std::cout << term::get_msg(!Global.no_color) << "Saved modified hostlist to " << hostfile_path << std::endl;
		else {
			auto exception{ make_custom_exception<permission_except>("Failed to write modified hostfile to disk!") };
			exception.update_message(hostfile_path);
			throw exception;
		}

		if (!do_list_hosts) std::exit(EXIT_SUCCESS);
	}
	// list all hosts
	if (do_list_hosts) {
		if (hosts.empty()) {
			std::cerr << term::get_warn(!Global.no_color) << "No hosts were found." << std::endl;
			std::exit(1);
		}
		const auto longest_name{ [&hosts]() {
			size_t longest{0ull};
			for (auto& [name, _] : hosts)
				if (const auto sz{ name.size() }; sz > longest)
					longest = sz;
			return longest + 2ull;
		}() };
		for (const auto& [name, info] : hosts)
			std::cout << Global.palette.set(UIElem::HOST_NAME) << name << Global.palette.reset() << str::VIndent(longest_name, name.size()) << Global.palette.set(UIElem::HOST_INFO) << "( " << info.hostname << ':' << info.port << " )" << Global.palette.reset() << '\n';
		std::exit(EXIT_SUCCESS);
	}

	// write-ini:
	if (args.check_any<opt::Option>("write-ini")) {
		if (!ini_path.empty() && config::save_ini(ini_path)) {
			std::cout << term::get_msg(!Global.no_color) << "Successfully wrote config: " << ini_path << std::endl;
			std::exit(EXIT_SUCCESS);
		}
		else {
			auto exception{ make_custom_exception<permission_except>("Failed to write INI file to disk!") };
			exception.update_message(ini_path);
			throw exception;
		}
	}
	// force interactive:
	if (args.check_any<opt::Option, opt::Flag>('i', "interactive"))
		Global.force_interactive = true;
	// quiet:
	if (args.check_any<opt::Option, opt::Flag>('q', "quiet"))
		Global.quiet = true;
	// no-prompt
	if (args.check_any<opt::Flag, opt::Option>('Q', "no-prompt"))
		Global.no_prompt = true;
	// command delay:
	if (const auto arg{ args.typegetv_any<opt::Flag, opt::Option>('d', "delay") }; arg.has_value()) {
		if (std::all_of(arg.value().begin(), arg.value().end(), isdigit))
			Global.command_delay = std::chrono::milliseconds(std::abs(str::stoll(arg.value())));
		else throw make_exception("Invalid delay value given: \"", arg.value(), "\", expected an integer.");
	}
	// scriptfiles:
	for (auto& scriptfile : args.typegetv_all<opt::Option, opt::Flag>('f', "file"))
		Global.scriptfiles.emplace_back(scriptfile);
}

/**
 * @brief			Reads the target file and returns a vector of command strings for each valid line.
 * @param filename	Target Filename
 * @returns			std::vector<std::string>
 */
inline std::vector<std::string> read_script_file(std::string filename, const env::PATH& pathvar)
{
	if (!file::exists(filename)) // if the filename doesn't exist, try to resolve it from the PATH
		filename = pathvar.resolve(filename, { ".txt" }).generic_string();
	if (!file::exists(filename)) // if the resolved filename still doesn't exist, throw
		std::cerr << term::warn << "Couldn't find file: \"" << filename << "\"\n";
	// read the file, parse it if the stream didn't fail
	else if (auto fss{ file::read(filename) }; !fss.fail()) {
		std::vector<std::string> commands;
		commands.reserve(file::count(fss, '\n') + 1ull);

		for (std::string line{}; std::getline(fss, line, '\n'); )
			if (line = str::strip_line(line, "#;"); !line.empty())
				commands.emplace_back(line);

		commands.shrink_to_fit();
		return commands;
	}
	return{};
}
/**
 * @brief		Retrieves a list of all user-specified commands to be sent to the RCON server, in order.
 * @param args	All commandline arguments.
 * @returns		std::vector<std::string>
 */
inline std::vector<std::string> get_commands(const opt::ParamsAPI2& args, const env::PATH& pathvar)
{
	std::vector<std::string> commands{ args.typegetv_all<opt::Parameter>() }; // Arg<std::string> is implicitly convertable to std::string

	// Check for piped data
	if (hasPendingDataSTDIN()) {
		for (std::string ln{}; str::getline(std::cin, ln, '\n'); ln.clear()) {
			ln = str::strip_line(ln); // remove preceeding & trailing whitespace
			if (!ln.empty())
				commands.emplace_back(ln); // read all available lines from STDIN into the commands list
		}
	}

	// read all user-specified files
	for (auto& file : Global.scriptfiles) {
		if (const auto script_commands{ read_script_file(file, pathvar) }; !script_commands.empty()) {
			if (!Global.quiet) // feedback
				std::cout << term::get_log(!Global.no_color) << "Successfully read commands from \"" << file << "\"\n";

			commands.reserve(commands.size() + script_commands.size());

			for (auto& command : script_commands)
				commands.emplace_back(command);
		}
		else std::cerr << term::get_warn(!Global.no_color) << "Failed to read any commands from \"" << file << "\"\n";
	}
	commands.shrink_to_fit();
	return commands;
}

int main(const int argc, char** argv)
{
	try {
		std::cout << term::EnableANSI; // enable ANSI escape sequences on windows
		const opt::ParamsAPI2 args{ argc, argv, 'H', "host", 'P', "port", 'p', "pass", 'd', "delay", 'f', "file", "save-host", "remove-host" }; // parse arguments

		// check for disable colors argument:
		if (const auto arg{ args.typegetv_any<opt::Option, opt::Flag>('n', "no-color") }; arg.has_value())
			Global.palette.setActive(Global.no_color = false);

		// Initialize the PATH variable & locate the program using argv[0]
		env::PATH PATH{ argv[0] };
		const auto& [myDir, myName] { PATH.resolve_split(argv[0]) };
		Global.EnvVar_CONFIG_DIR = str::toupper(std::filesystem::path(myName).replace_extension().generic_string()) + "_CONFIG_DIR";

		// help:
		if (args.check_any<opt::Flag, opt::Option>('h', "help")) {
			std::cout << Help(myName) << std::endl;
			return 0;
		}
		// version: (mutually exclusive with help as it shows the version number as well)
		if (args.check_any<opt::Flag, opt::Option>('v', "version")) {
			if (!args.check_any<opt::Flag, opt::Option>('q', "quiet"))
				std::cout << DEFAULT_PROGRAM_NAME << ' ';
			std::cout << ARRCON_VERSION << std::endl;
			return 0;
		}

		// get the config file path.
		const auto cfg_dir{ config::getDirPath(myDir, Global.EnvVar_CONFIG_DIR) };

		// Get the INI file's path
		std::filesystem::path ini_path{ (cfg_dir / myName).replace_extension(".ini") };

		// Read the INI if it exists
		if (file::exists(ini_path))
			config::load_ini(ini_path);

		if (args.empty() && !Global.allow_no_args) {
			std::cerr << Help(myName) << std::endl << std::endl;
			throw make_exception("No arguments were specified!\n\t\t(You can disable this error by setting \"bAllowNoArgs = true\" in the INI)");
		}

		// Initialize the hostlist
		config::HostList hosts;

		const auto hostfile_path{ (cfg_dir / myName).replace_extension(".hosts") };
		if (file::exists(hostfile_path)) // load the hostfile if it exists
			hosts = config::load_hostfile(hostfile_path);

		// get the target server's connection information
		const auto& [host, port, pass] { get_target_info(args, hosts) };
		handle_args(args, hosts, { host, port, pass }, ini_path, hostfile_path);

		// get the commands to execute on the server
		const auto commands{ get_commands(args, PATH) };

		// If no custom prompt is set, use the default one
		if (Global.custom_prompt.empty())
			Global.custom_prompt = (Global.no_prompt ? "" : str::stringify(Global.palette.set(UIElem::TERM_PROMPT_NAME), "RCON@", host, Global.palette.reset(UIElem::TERM_PROMPT_ARROW), '>', Global.palette.reset(), ' '));


		// Register the cleanup function before connecting the socket
		std::atexit(&net::cleanup);

		// Connect the socket
		Global.socket = net::connect(host, port);

		// set & check if the socket was connected successfully
		if (!(Global.connected = (Global.socket != SOCKET_ERROR)))
			throw make_exception("Socket \'", Global.socket, "\' is invalid, but no exceptions were thrown!\tLast socket error: (", LAST_SOCKET_ERROR_CODE(), ") ", net::getLastSocketErrorMessage());

		// authenticate with the server, run queued commands, and open an interactive session if necessary.
		if (rcon::authenticate(Global.socket, pass)) {
			// authentication succeeded, run commands
			if (mode::commandline(commands) == 0ull || Global.force_interactive)
				mode::interactive(Global.socket); // if no commands were executed from the commandline or if the force interactive flag was set
		}
		else throw make_exception("Authentication failure:  Incorrect password for ", host, ':', port);

		return 0;
	} catch (const std::exception& ex) { ///< catch exceptions
		std::cerr << term::get_error(!Global.no_color) << ex.what() << std::endl;
		return -1;
	} catch (...) { ///< catch all other exceptions
		std::cerr << term::get_crit(!Global.no_color) << "An unknown exception occurred!" << std::endl;
		return -1;
	}
}