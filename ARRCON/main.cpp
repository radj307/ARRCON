#include "mode.hpp"				///< RCON client modes
#include "config.hpp"			///< INI functions
#include "arg-handlers.hpp"		///< CLI argument handler

#include <fileio.hpp>			///< file I/O functions

#include <signal.h>				///< signal handling
#include <unistd.h>

// undefine unistd macros for fileio.hpp compat
#undef read
#undef write

/// @brief	Emergency stop handler, should be passed to the std::atexit() function to allow a controlled shutdown of the socket.
inline void safeExit(void)
{
	if (Global.socket != SOCKET_ERROR)
		net::close_socket(Global.socket);
	WSACleanup();
	std::cout << Global.palette.reset();
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
		std::atexit(&safeExit);

	#ifdef OS_WIN
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
			throw make_exception("Socket error: ");
			throw std::exception(("Failed to connect to "s + host + ":"s + port + "\n\t\tLast socket error code: "s + std::to_string(net::lastError())).c_str());
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