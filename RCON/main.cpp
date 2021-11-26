#include "mode.hpp"
#include "config.hpp"
#include "arg-handlers.hpp"

#include <fileio.hpp>

#include <signal.h>
#include <unistd.h>
#undef read		///< fileio.hpp compat
#undef write	///< fileio.hpp compat

/// @brief	Emergency stop handler, should be passed to the std::atexit() function to allow a controlled shutdown of the socket.
inline void safeExit(void)
{
	if (Global.socket != SOCKET_ERROR)
		net::close_socket(Global.socket);
	WSACleanup();
}

/**
 * @brief			Reads the target file and returns a vector of command strings for each valid line.
 * @param filename	Target Filename
 * @returns			std::vector<std::string>
 */
inline std::vector<std::string> read_script_file(std::string filename)
{
	if (!file::exists(filename)) // if the filename doesn't exist, try to resolve it from the PATH
		filename = env::PATH{}.resolve(filename, { "", ".ini", ".txt", ".bat", ".scr" });
	if (!file::exists(filename)) // if the resolved filename still doesn't exist, throw
		throw std::exception(("Failed to locate file: \""s + filename + "\""s).c_str());
	// read the file, parse it if the stream didn't fail
	if (auto file{ file::read(filename) }; !file.fail()) {
		std::vector<std::string> commands;
		commands.reserve(file::count(file, '\n'));

		for (std::string line{}; str::getline(file, line, '\n'); )
			if (line = str::strip_line(line, "#;"); !line.empty())
				commands.emplace_back(line);

		commands.shrink_to_fit();
		return commands;
	}
	throw std::exception(("IO Error Reading File: \""s + filename + "\""s).c_str());
}

#ifdef OS_WIN
#include <ControlEventHandler.hpp>
/// @brief Convert Windows Control Events into C++ Exceptions
inline void handler(void) { Global.connected = false; throw std::exception("SIGINT"); }
#endif

int main(int argc, char** argv, char** envp)
{
	try {
	#ifdef OS_WIN
		sys::registerEventHandler();
		sys::setEventHandlerFunc(sys::Event::CTRL_C, handler);
	#endif

		std::cout << sys::term::EnableANSI; // enable ANSI escape sequences on windows

		const opt::ParamsAPI2 args{ argc, argv, 'H', 'P', 'p', 'd', "delay", 'f', "file" }; // parse arguments
		const auto& [host, port, pass] { get_server_info(args) };
		const auto& [prog_path, prog_name] { env::PATH{}.resolve_split(args.arg0().value()) };

		if (Global.ini_path = prog_path + std::filesystem::path(prog_name).replace_extension(".ini").generic_string(); file::exists(Global.ini_path))
			config::apply_config(Global.ini_path);

		handle_args(args, prog_name);

		// Register the cleanup function before connecting the socket
		std::atexit(&safeExit);
		// Connect the socket
		Global.socket = net::connect(host, port);

		if (Global.connected = Global.socket != SOCKET_ERROR; !Global.connected)
			throw std::exception(("Failed to connect to "s + host + ":"s + port).c_str()); // connection failed

		// auth & commands
		if (rcon::authenticate(Global.socket, pass)) {
			std::vector<std::string> commands{ args.typegetv_all<opt::Parameter>() };
			// read script files:
			for (auto& filename : Global.scriptfiles) {
				const auto file{ read_script_file(filename) };
				commands.reserve(commands.size() + file.size());
				for (auto& it : file)
					commands.emplace_back(it);
				commands.shrink_to_fit();
			}
			// check if commandline/scriptfile commands were found
			const auto no_commands{ commands.empty() };
			if (!no_commands)
				mode::commandline(commands);
			if (Global.force_interactive || no_commands)
				mode::interactive(Global.socket, (Global.custom_prompt.empty() ? ("RCON@"s + host) : Global.custom_prompt));
		}
		else
			throw std::exception(("Authentication Failed! ("s + host + ":"s + port + ")"s).c_str());

		return EXIT_SUCCESS;
	} catch (const std::exception& ex) { ///< catch std::exception
		std::cerr << sys::term::error << ex.what() << std::endl;
	} catch (...) { ///< catch all other exceptions
		std::cerr << sys::term::error << "An unknown exception occurred!" << std::endl;
	}
	return EXIT_FAILURE;
}