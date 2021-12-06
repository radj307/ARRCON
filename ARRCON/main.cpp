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
	// version: (mutually exclusive with help as it shows the version number as well)
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
		else throw make_exception("I/O operation failed: \""s, Global.ini_path, "\" couldn't be written to."s);
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
		throw std::exception("Received SIGINT");
	case SIGTERM:
		throw std::exception("Received SIGTERM");
	case SIGABRT_COMPAT: [[fallthrough]];
	case SIGABRT:
		throw std::exception("Received SIGABRT");
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
	for (auto& file : Global.scriptfiles) {// iterate through all user-specified files
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
		const opt::ParamsAPI2 args{ argc, argv, 'H', "host", 'P', "port", 'p', "password", 'd', "delay", 'f', "file" }; // parse arguments
		env::PATH PATH{ args.arg0().value_or("") };
		const auto& [prog_path, prog_name] { PATH.resolve_split(args.arg0().value()) };

		if (Global.ini_path = (prog_path / prog_name).replace_extension(".ini").generic_string(); file::exists(Global.ini_path))
			config::apply_config(Global.ini_path);

		const auto& [host, port, pass] { get_server_info(args) };
		handle_args(args, prog_name.generic_string());

		const auto commands{ get_commands(args, PATH) };

		if (Global.custom_prompt.empty())
			Global.custom_prompt = (Global.no_prompt ? "" : str::stringify(Global.palette.set(UIElem::TERM_PROMPT_NAME), "RCON@", host, Global.palette.reset(UIElem::TERM_PROMPT_ARROW), '>', Global.palette.reset(), ' '));

		// Register the cleanup function before connecting the socket
		std::atexit(&net::cleanup);

	#ifdef OS_WIN // use signal function as sigaction doesn't work correctly
		signal(SIGINT, &sighandler);
		signal(SIGTERM, &sighandler);
		signal(SIGABRT, &sighandler);
	#else
		struct sigaction sa { &sighandler, nullptr, {}, SA_RESTART, nullptr };
		sigaction(SIGINT, &sa, nullptr);
		sigaction(SIGTERM, &sa, nullptr);
		sigaction(SIGABRT, &sa, nullptr);
	#endif

		// Connect the socket
		Global.socket = net::connect(host, port);

		if (Global.connected = Global.socket != SOCKET_ERROR; !Global.connected) {
			std::cerr << "Failed to connect to " << host << ':' << port << ", is the target reachable?\n";
			throw make_exception("Socket error: ", net::lastError());
		}

		// auth & commands
		if (rcon::authenticate(Global.socket, pass)) {
			if (mode::commandline(commands) == 0ull || Global.force_interactive)
				mode::interactive(Global.socket);
		}
		else throw make_exception("Authentication Failed! (", host, ':', port, ')');

		return EXIT_SUCCESS;
	} catch (const std::exception& ex) { ///< catch std::exception
		std::cerr << sys::term::error << ex.what() << std::endl;
	} catch (...) { ///< catch all other exceptions
		std::cerr << sys::term::error << "An unknown exception occurred!" << std::endl;
	}
	return EXIT_FAILURE;
}