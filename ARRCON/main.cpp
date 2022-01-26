#include "globals.h"
#include "mode.hpp"				///< RCON client modes
#include "config.hpp"			///< INI functions
#include "Help.hpp"				///< CLI usage instructions
#include "exceptions.hpp"

#include <make_exception.hpp>
#include <ParamsAPI2.hpp>
#include <fileio.hpp>			///< file I/O functions
#include <TermAPI.hpp>			///< file I/O functions
#include <hasPendingDataSTDIN.h>

#include <signal.h>				///< signal handling
#include <unistd.h>

#undef read
#undef write

inline HostInfo get_environment_target(std::string programNameNoExt)
{
	programNameNoExt = str::toupper(programNameNoExt);
	return{
		env::getvar(programNameNoExt + "_HOST").value_or(""),
		env::getvar(programNameNoExt + "_PORT").value_or(""),
		env::getvar(programNameNoExt + "_PASS").value_or("")
	};
}

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
		host{ args.typegetv_any<opt::Flag, opt::Option>('H', "host") }, //< Argument:  [-H|--host]
		port{ args.typegetv_any<opt::Flag, opt::Option>('P', "port") }, //< Argument:  [-P|--port]
		pass{ args.typegetv_any<opt::Flag, opt::Option>('p', "pass") }; //< Argument:  [-p|--pass]

	// Argument:  [-S|--saved]
	if (const auto saved{ args.typegetv_any<opt::Flag, opt::Option>('S', "saved") }; saved.has_value()) {
		if (const auto target{ hostlist.find(saved.value()) }; target != hostlist.end()) {
			const auto info{ to_hostinfo(target->second) };
			return HostInfo{ host.value_or(info.hostname), port.value_or(info.port), pass.value_or(info.password) };
		}
		throw make_exception("Couldn't find a saved host named \"", saved.value(), "\"!");
	}

	return{
		host.value_or(Global.DEFAULT_TARGET.hostname),
		port.value_or(Global.DEFAULT_TARGET.port), // port
		pass.value_or(Global.DEFAULT_TARGET.password)  // password
	};
}

inline void handle_hostfile_arguments(const opt::ParamsAPI2& args, config::HostList& hosts, const HostInfo& target, const std::filesystem::path& hostfile_path)
{

	const auto do_list_hosts{ args.check<opt::Option>("list-hosts") };
	// remove-host
	if (const auto remove_hosts{ args.typegetv_all<opt::Option>("remove-host") }; !remove_hosts.empty()) {
		std::stringstream message_buffer; // save the messages in a buffer to prevent misleading messages in the event of a file writing error
		for (const auto& name : remove_hosts) {
			if (hosts.erase(name))
				message_buffer << term::get_msg(!Global.no_color) << "Removed " << Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT) << name << Global.palette.reset() << '\n';
			else
				message_buffer << term::get_error(!Global.no_color) << "Hostname \"" << Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT) << name << Global.palette.reset() << " doesn't exist!" << '\n';
		}

		// if the hosts file is empty, delete it
		if (hosts.empty()) {
			if (std::filesystem::remove(hostfile_path))
				std::cout << message_buffer.rdbuf() << term::get_msg(!Global.no_color) << "Deleted the hostfile as there are no remaining entries." << std::endl;
			else
				throw permission_exception("handle_hostfile_arguments()", hostfile_path, "Failed to delete empty Hostfile!");
			std::exit(EXIT_SUCCESS); // host list is empty, ignore do_list_hosts as nothing will happen
		} // otherwise, save the modified hosts file.
		else {
			if (config::save_hostfile(hosts, hostfile_path)) // print a success message or throw failure exception
				std::cout << message_buffer.rdbuf() << term::get_msg(!Global.no_color) << "Saved modified hostlist to " << hostfile_path << std::endl;
			else
				throw permission_exception("handle_hostfile_arguments()", hostfile_path, "Failed to write modified Hostfile to disk!");
			if (!do_list_hosts) std::exit(EXIT_SUCCESS);
		}

	}
	// save-host
	else if (const auto save_host{ args.typegetv<opt::Option>("save-host") }; save_host.has_value()) {
		std::stringstream message_buffer; // save the messages in a buffer to prevent misleading messages in the event of a file writing error


		const auto target_info{ from_hostinfo(target) };
		const auto& [existing, added] {
			hosts.insert(std::make_pair(save_host.value(), target_info))
		};

		if (added)
			message_buffer << term::get_msg(!Global.no_color) << "Added host: " << Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT) << save_host.value() << Global.palette.reset() << " " << target.hostname << ':' << target.port << '\n';
		else if ([](const file::INI::SectionContent& left, const file::INI::SectionContent& right) -> bool {
			if (const auto left_host{ left.find("sHost") }, right_host{ right.find("sHost") }; left_host == left.end() || right_host == right.end() || left_host->second != right_host->second) {
				return false;
			}
			if (const auto left_port{ left.find("sPort") }, right_port{ right.find("sPort") }; left_port == left.end() || right_port == right.end() || left_port->second != right_port->second) {
				return false;
			}
			if (const auto left_pass{ left.find("sPass") }, right_pass{ right.find("sPass") }; left_pass == left.end() || right_pass == right.end() || left_pass->second != right_pass->second) {
				return false;
			}
			return true;
			}(existing->second, target_info))
			throw make_exception("Host ", Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT), save_host.value(), Global.palette.reset(), " is already set to ", target.hostname, ':', target.port, '\n');
		else
			message_buffer << term::get_msg(!Global.no_color) << "Updated " << Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT) << save_host.value() << Global.palette.reset() << ": " << target.hostname << ':' << target.port << '\n';


		if (config::save_hostfile(hosts, hostfile_path)) // print a success message or throw failure exception
			std::cout << message_buffer.rdbuf() << term::get_msg(!Global.no_color) << "Saved modified hostlist to " << hostfile_path << std::endl;
		else
			throw permission_exception("handle_hostfile_arguments()", hostfile_path, "Failed to write modified Hostfile to disk!");

		if (!do_list_hosts) std::exit(EXIT_SUCCESS);
	}
	// list all hosts
	if (do_list_hosts) {
		if (hosts.empty()) {
			std::cerr << "There are no saved hosts in the list." << std::endl;
			std::exit(EXIT_SUCCESS);
		}

		const auto indentation_max{ [&hosts]() {
			if (Global.quiet)
				return 0ull; // don't process the list if this won't be used
			size_t longest{0ull};
			for (const auto& [name, _] : hosts)
				if (const auto sz{ name.size() }; sz > longest)
					longest = sz;
			return longest + 2ull;
		}() };

		for (const auto& [name, info] : hosts) {
			const auto& hostinfo{ to_hostinfo(info) };
			if (!Global.quiet) {
				std::cout
					<< Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT) << name << Global.palette.reset() << '\n'
					<< "    Host:  " << hostinfo.hostname << '\n'
					<< "    Port:  " << hostinfo.port << '\n';
			}
			else {
				std::cout << Global.palette.set(UIElem::HOST_NAME_HIGHLIGHT) << name << Global.palette.reset()
					<< str::VIndent(indentation_max, name.size()) << "( " << hostinfo.hostname << ':' << hostinfo.port << " )\n";
			}
		}
		std::exit(EXIT_SUCCESS);
	}

}

/**
 * @brief		Handle commandline arguments.
 * @param args	Arguments from main()
 */
inline void handle_arguments(const opt::ParamsAPI2& args, const HostInfo& target, const std::filesystem::path& ini_path)
{
	// write-ini:
	if (args.check_any<opt::Option>("write-ini")) {
		if (!ini_path.empty() && config::save_ini(ini_path)) {
			std::cout << term::get_msg(!Global.no_color) << "Successfully wrote config: " << ini_path << std::endl;
			std::exit(EXIT_SUCCESS);
		}
		else
			throw permission_exception("handle_arguments()", ini_path, "Failed to open INI for writing!");
	}
	// force interactive:
	if (args.check_any<opt::Option, opt::Flag>('i', "interactive"))
		Global.force_interactive = true;
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
	for (const auto& scriptfile : args.typegetv_all<opt::Option, opt::Flag>('f', "file"))
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
		for (std::string ln{}; str::getline(std::cin, ln, '\n'); ) {
			ln = str::strip_line(ln); // remove preceeding & trailing whitespace
			if (!ln.empty())
				commands.emplace_back(ln); // read all available lines from STDIN into the commands list
		}
	}

	// read all user-specified files
	for (const auto& file : Global.scriptfiles) {
		if (const auto script_commands{ read_script_file(file, pathvar) }; !script_commands.empty()) {
			if (!Global.quiet) // feedback
				std::cout << term::get_log(!Global.no_color) << "Successfully read commands from \"" << file << "\"\n";

			commands.reserve(commands.size() + script_commands.size());

			for (const auto& command : script_commands)
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
		// This fixes a potential bug on terminal emulators with transparent backgrounds; so long as the palette doesn't contain background colors or formatting
		Global.palette.setDefaultResetSequence(color::reset_f);

		std::cout << term::EnableANSI; // enable ANSI escape sequences on windows
		const opt::ParamsAPI2 args{ argc, argv, 'H', "host", 'S', "saved", 'P', "port", 'p', "pass", 'd', "delay", 'f', "file", "save-host", "remove-host" }; // parse arguments

		// check for disable colors argument:
		if (const auto arg{ args.typegetv_any<opt::Option, opt::Flag>('n', "no-color") }; arg.has_value())
			Global.palette.setActive(Global.no_color = false);

		// Initialize the PATH variable & locate the program using argv[0]
		env::PATH PATH{ argv[0] };
		const auto& [myDir, myName] { PATH.resolve_split(argv[0]) };

		const std::string myNameNoExt{
			[](auto&& p) -> std::string {
				const std::string s{ p.generic_string() };
				if (const auto pos{ s.find('.') }; pos < s.size())
					return str::toupper(s.substr(0ull, pos));
				return str::toupper(s);
		}(myName) };

		const config::Locator cfg_path(myDir, myNameNoExt);
		Global.EnvVar_CONFIG_DIR = cfg_path.getEnvironmentVariableName();

		// Argument:  [-q|--quiet]
		Global.quiet = args.check_any<opt::Option, opt::Flag>('q', "quiet");
		// Argument:  [-h|--help]
		if (args.check_any<opt::Flag, opt::Option>('h', "help")) {
			std::cout << Help(myName) << std::endl;
			return 0;
		}
		// Argument:  [-v|--version] (mutually exclusive with help as it shows the version number as well)
		if (args.check_any<opt::Flag, opt::Option>('v', "version")) {
			if (!Global.quiet)
				std::cout << DEFAULT_PROGRAM_NAME << ' ';
			std::cout << ARRCON_VERSION << std::endl;
			return 0;
		}
		// Argument:  [--print-env]
		if (args.check<opt::Option>("print-env")) {
			std::cout << EnvHelp(myNameNoExt) << std::endl;
			return 0;
		}

		// Get the INI file's path
		std::filesystem::path ini_path{ cfg_path.from_extension(".ini") };

		// Read the INI if it exists
		if (file::exists(ini_path))
			config::load_ini(ini_path);

		// load target-specifier environment variables
		config::load_environment(myNameNoExt);

		if (args.empty() && !Global.allow_no_args) {
			std::cerr << Help(myName) << std::endl << std::endl;
			throw make_exception(
				"No arguments were specified!\n",
				"        Function Name:        main()\n",
				"        Suggested Solutions:\n",
				"        1.  Set ", Global.palette.set(UIElem::INI_KEY_HIGHLIGHT), "bAllowNoArgs = true", Global.palette.reset(), " in the INI config file.\n",
				"        2.  Specify a target to connect to with the [-H|--host], [-P|--port], & [-p|--pass] options.\n",
				"        3.  Read the help display above for command assistance.\n"
			);
		}

		// Initialize the hostlist
		config::HostList hosts;

		const auto hostfile_path{ cfg_path.from_extension(".hosts") };
		if (file::exists(hostfile_path)) // load the hostfile if it exists
			hosts = config::load_hostfile(hostfile_path);

		// get the target server's connection information
		const auto& hostinfo{ get_target_info(args, hosts) };
		handle_arguments(args, hostinfo, ini_path);
		handle_hostfile_arguments(args, hosts, hostinfo, hostfile_path);

		// get the commands to execute on the server
		const auto commands{ get_commands(args, PATH) };

		// If no custom prompt is set, use the default one
		if (Global.custom_prompt.empty())
			Global.custom_prompt = (Global.no_prompt ? "" : str::stringify(Global.palette.set(UIElem::TERM_PROMPT_NAME), "RCON@", hostinfo.hostname, Global.palette.reset(UIElem::TERM_PROMPT_ARROW), '>', Global.palette.reset(), ' '));


		// Register the cleanup function before connecting the socket
		std::atexit(&net::cleanup);

		// Connect the socket
		Global.socket = net::connect(hostinfo.hostname, hostinfo.port);

		// set & check if the socket was connected successfully
		if (!(Global.connected = (Global.socket != SOCKET_ERROR)))
			throw connection_exception("main()", "Socket descriptor was set to (" + std::to_string(Global.socket) + ") after successfully initializing the connection.", hostinfo.hostname, hostinfo.port, LAST_SOCKET_ERROR_CODE(), net::getLastSocketErrorMessage());

		// authenticate with the server, run queued commands, and open an interactive session if necessary.
		if (rcon::authenticate(Global.socket, hostinfo.password)) {
			// authentication succeeded, run commands
			if (mode::commandline(commands) == 0ull || Global.force_interactive)
				mode::interactive(Global.socket); // if no commands were executed from the commandline or if the force interactive flag was set
		}
		else {
			throw make_exception("Authentication failure:  Incorrect Password!\n",
				"        Target Hostname/IP:  ", hostinfo.hostname, '\n',
				"        Target Port:         ", hostinfo.port, '\n',
				"        Suggested Solutions:\n",
				"        1.  Verify that you're using the right password.\n",
				"        2.  Verify that this is the correct target."
			);
		}

		return 0;
	} catch (const std::exception& ex) { ///< catch exceptions
		std::cerr << term::get_error(!Global.no_color) << ex.what() << std::endl;
		std::cerr << term::get_info(!Global.no_color) << "You can report bugs here: " << ISSUE_REPORT_URL << std::endl;
		return -1;
	} catch (...) { ///< catch all other exceptions
		std::cerr << term::get_crit(!Global.no_color) << "An unknown exception occurred!" << std::endl;
		std::cerr << term::get_info(!Global.no_color) << "Please report this exception here: " << ISSUE_REPORT_URL << std::endl;
		return -1;
	}
}