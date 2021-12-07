#include "mode.hpp"				///< RCON client modes
#include "config.hpp"			///< INI functions
#include "Help.hpp"				///< CLI usage instructions

#include <ParamsAPI2.hpp>		///< CLI option handler/wrapper
#include <fileio.hpp>			///< file I/O functions
#include <env.hpp>

#include <signal.h>				///< signal handling
#include <unistd.h>

// undefine unistd macros for fileio.hpp compat
#undef read
#undef write


/**
 * @brief		Retrieve the user's specified connection target.
 * @param args	Arguments from main().
 * @returns		std::tuple<std::string, std::string, std::string>
 *\n			0	RCON Hostname
 *\n			1	RCON Port
 *\n			2	RCON Password
 */
inline std::tuple<std::string, std::string, std::string> get_server_info(const opt::ParamsAPI2& args, const config::HostList& hostlist)
{
	if (const auto param{ args.typeget_any<opt::Parameter>() }; param.has_value()) {
		if (const auto entry{ hostlist.find(param.value()) }; entry != hostlist.end()) {
			Global.using_hostname = entry->first;
			const auto [host, port, pass] { entry->second };
			return{ args.typegetv<opt::Flag>('H').value_or(host), args.typegetv<opt::Flag>('P').value_or(port), args.typegetv<opt::Flag>('p').value_or(pass) };
		}
	}
	return{
		args.typegetv<opt::Flag>('H').value_or(Global.DEFAULT_HOST), // host
		args.typegetv<opt::Flag>('P').value_or(Global.DEFAULT_PORT), // port
		args.typegetv<opt::Flag>('p').value_or(Global.DEFAULT_PASS)  // password
	};
}

/**
 * @brief		Handle commandline arguments.
 * @param args	Arguments from main()
 */
inline void handle_args(const opt::ParamsAPI2& args, config::HostList& hosts, const config::HostInfo& target, const std::string& program_name, const std::string& ini_path, const std::string& hostfile_path)
{
	// help:
	if (args.check_any<opt::Flag, opt::Option>('h', "help")) {
		std::cout << Help(program_name);
		std::exit(EXIT_SUCCESS);
	}
	// version: (mutually exclusive with help as it shows the version number as well)
	else if (args.check_any<opt::Flag, opt::Option>('v', "version")) {
		std::cout << DEFAULT_PROGRAM_NAME << " v" << VERSION << std::endl;
		std::exit(EXIT_SUCCESS);
	}
	// write-ini:
	if (args.check_any<opt::Option>("write-ini")) {
		if (!ini_path.empty() && config::write_default_config(ini_path)) {
			std::cout << "Successfully wrote to config: \"" << ini_path << '\"' << std::endl;
			std::exit(EXIT_SUCCESS);
		}
		else throw make_exception("I/O operation failed: \""s, ini_path, "\" couldn't be written to."s);
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
			else throw make_exception("Cannot set a delay value longer than ", std::to_string(MAX_DELAY.count()), " hours!");
		}
		else throw make_exception("Invalid delay value given: \"", arg.value(), "\", expected an integer.");
	}
	// disable colors:
	if (const auto arg{ args.typegetv_any<opt::Option, opt::Flag>('n', "no-color") }; arg.has_value())
		Global.palette.setActive(false);
	// scriptfiles:
	for (auto& scriptfile : args.typegetv_all<opt::Option, opt::Flag>('f', "file"))
		Global.scriptfiles.emplace_back(scriptfile);
	// save-host
	if (const auto arg{ args.typegetv<opt::Option>("save-host") }; arg.has_value()) {
		if (config::save_hostinfo(hosts, arg.value(), target).second)
			std::cout << "Saved \"" << target.hostname << ':' << target.port << "\" as \"" << arg.value() << "\"\n";
		else std::cout << "Updated \"" << arg.value() << "\" with target \"" << target.hostname << ':' << target.port << "\"\n";
		config::write_hostfile(hosts, hostfile_path);
	}
}



/**
 * @brief		Handler function for OS signals. Passed to sigaction/signal to intercept interrupts and shut down the socket correctly.
 * @param sig	The signal thrown to prompt the calling of this function.
 */
inline void sighandler(int sig)
{
	Global.connected = false;
	std::cout << Global.palette.reset();
	switch (sig) {
	case SIGINT:
		throw make_exception("Received SIGINT");
	case SIGTERM:
		throw make_exception("Received SIGTERM");
	#ifdef OS_WIN
	case SIGABRT_COMPAT: [[fallthrough]];
	#endif
	case SIGABRT:
		throw make_exception("Received SIGABRT");
	default:break;
	}
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
		std::cerr << sys::term::warn << "Couldn't find file: \"" << filename << "\"\n";
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
	if (Global.using_hostname.has_value()) {
		const auto pos{ std::find(commands.begin(), commands.end(), Global.using_hostname.value()) };
		commands.erase(pos);
	}
	// iterate through all user-specified files
	for (auto& file : Global.scriptfiles) {
		if (const auto script_commands{ read_script_file(file, pathvar) }; !script_commands.empty()) {
			if (!Global.quiet) // feedback
				std::cout << sys::term::log << "Successfully read commands from \"" << file << "\"\n";

			commands.reserve(commands.size() + script_commands.size());

			for (auto& command : script_commands)
				commands.emplace_back(command);
		}
		else std::cerr << sys::term::warn << "Failed to read any commands from \"" << file << "\"\n";
	}
	commands.shrink_to_fit();
	return commands;
}

int main(int argc, char** argv)
{
	try {
		std::cout << sys::term::EnableANSI; // enable ANSI escape sequences on windows
		const opt::ParamsAPI2 args{ argc, argv, 'H', "host", 'P', "port", 'p', "password", 'd', "delay", 'f', "file", "save-host" }; // parse arguments

		env::PATH PATH{ args.arg0() };
		const auto& [prog_path, prog_name] { PATH.resolve_split(args.arg0()) };

		config::HostList hosts;

		const auto ini_path{ (prog_path / prog_name).replace_extension(".ini").generic_string() };
		if (file::exists(ini_path))
			config::apply_config(ini_path);

		const auto hostfile_path{ (prog_path / prog_name).replace_extension(".hosts").generic_string() };
		if (file::exists(hostfile_path))
			hosts = config::read_hostfile(hostfile_path);

		const auto& [host, port, pass] { get_server_info(args, hosts) };
		handle_args(args, hosts, { host, port, pass }, prog_name.generic_string(), ini_path, hostfile_path);

		const auto commands{ get_commands(args, PATH) };

		if (Global.custom_prompt.empty())
			Global.custom_prompt = (Global.no_prompt ? "" : str::stringify(Global.palette.set(UIElem::TERM_PROMPT_NAME), "RCON@", host, Global.palette.reset(UIElem::TERM_PROMPT_ARROW), '>', Global.palette.reset(), ' '));

		signal(SIGINT, &sighandler);
		signal(SIGTERM, &sighandler);
		signal(SIGABRT, &sighandler);

		// Register the cleanup function before connecting the socket
		std::atexit(&net::cleanup);

		// Connect the socket
		Global.socket = net::connect(host, port);

		if (Global.connected = Global.socket != SOCKET_ERROR; !Global.connected) {
			std::cerr << "Failed to connect to " << host << ':' << port << ", is the target reachable?\n";
			throw make_exception("Socket error: ", net::lastError());
		}

		// auth & commands
		if (rcon::authenticate(Global.socket, pass)) {
			// authentication succeeded, run commands
			if (mode::commandline(commands) == 0ull || Global.force_interactive)
				mode::interactive(Global.socket); // if no commands were executed from the commandline or if the force interactive flag was set
		}
		else throw make_exception("Authentication with ", host, ':', port, ") failed because of an incorrect password.");

		return EXIT_SUCCESS;
	} catch (const std::exception& ex) { ///< catch exceptions
		std::cerr << sys::term::error << ex.what() << std::endl;
	} catch (...) { ///< catch all other exceptions
		std::cerr << sys::term::error << "An unknown exception occurred!" << std::endl;
	}
	return EXIT_FAILURE;
}