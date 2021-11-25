#include "mode.hpp"


#include <ParamsAPI2.hpp>
#include <fileio.hpp>
#include <env.hpp>

#include <signal.h>
#include <unistd.h>
#undef read		///< fileio.hpp compat
#undef write	///< fileio.hpp compat

inline constexpr const auto
VERSION{ "1.0.0" },
DEFAULT_PROGRAM_NAME{ "RCON.exe" },
DEFAULT_HOST{ "localhost" },
DEFAULT_PORT{ "27015" };

#ifdef MULTITHREADING
#include <future>

static std::optional<std::exception> listener_ex{ std::nullopt };

/**
 * @brief		Listens for packets on the global socket, receives them, and pushes them to the global queue.
 * @param mtx	Mutex Reference
 */
inline void listener(std::mutex& mtx)
{
	try {
		fd_set set{ 1u, g_socket };
		const TIMEVAL sel_t{ 2L, 0L }; // 2s
		while (g_connected) { // while the socket is connected
			if (select(NULL, &set, NULL, NULL, &sel_t)) { // check if the socket has data to read (timeout 2s)
				const auto p{ net::recv_packet(g_socket) };
				std::scoped_lock<std::mutex> lock{ mtx };
				packet::Queue.push(p);
				g_wait_for_listener = false; // unset wait for listener
			}
		}
	} catch (const std::exception& ex) {
		listener_ex = ex;
		g_connected = false;
	}
}
#endif

/**
 * @brief	Emergency stop handler, should be passed to the std::atexit() function to allow a controlled shutdown of the socket.
 */
inline void safeExit(void)
{
	if (g_socket != SOCKET_ERROR)
		net::close_socket(g_socket);
	WSACleanup();
}

/**
 * @struct	Help
 * @brief	Functor that prints out the help display with auto-formatting.
 */
struct Help {
private:
	const std::string _program_name;
	const std::vector<std::pair<std::string, std::string>> _options{
		{ "-H <Host>"s, "RCON Address.   (Default: \""s + DEFAULT_HOST + "\")"s },
		{ "-P <Port>"s, "RCON Port.      (Default: \""s + DEFAULT_PORT + "\")"s },
		{ "-p <Pass>"s, "RCON Password." },
		{ "-f <file>  --file <file>"s, "Load the specified file and run each line as a command."s },
		{ "-h  --help"s, "Show the help display."s },
		{ "-v  --version"s, "Print the current version number."s },
		{ "-i  --interactive"s, "Start interactive mode after sending all commands specified on the commandline."s},
		{ "-q  --quiet"s, "Don't print server response packets."s },
		{ "-d <ms>  --delay <ms>"s, "Time in milliseconds to wait between each command in commandline mode."s },
		{ "-n  --no-color"s, "Disable colorized console output."s },
		{ "--no-prompt"s,"Hides the prompt in interactive mode."s},
	};
	const size_t _longest_optname, _max_line_length;

public:
	Help(const std::string& program_name, const size_t& max_line_sz = 120ull) : _program_name{ program_name }, _longest_optname{ [this]() { size_t longest{0ull}; for (auto& [optname, desc] : _options) if (const auto sz{optname.size()}; sz > longest) longest = sz; return longest; }() }, _max_line_length{ max_line_sz } {}
	friend std::ostream& operator<<(std::ostream& os, const Help& help)
	{
		const auto margin_width{ help._longest_optname + 2ull };
		os << help._program_name << ' ';
		if (help._program_name != DEFAULT_PROGRAM_NAME)
			os << '(' << DEFAULT_PROGRAM_NAME << ") ";
		os << "v" << VERSION << "\n"
			<< "CLI Application that allows communicating with servers using the Source RCON Protocol.\n"
			<< "\nUSAGE:\n  "
			<< help._program_name << " [OPTIONS] [COMMANDS]\n"
			<< "\nOPTIONS:\n";

		for (auto& [optname, desc] : help._options)
			os << "  " << optname << str::VIndent(margin_width + 2ull, optname.size()) << desc << '\n';

		os << "\nMODES:\n"
			<< "  [1]\tInteractive\tInteractive shell prompt. This is the default mode.\n"
			<< "  [2]\tCommandline\tExecutes commands passed on the commandline.\n"
			<< "\t\t\tThis mode is automatically used when commandline input is detected (Excluding OPTIONS).\n"
			<< "\t\t\tYou can also specify files using \"-f <file>\" or \"--file <file>\".\n\t\t\tEach line will be executed as a command in commandline mode.\n\t\t\tScript commands are executed in-order, after any commands passed as arguments.\n\t\t\tYou can specify multiple files per command.\n"
			;
		return os.flush();
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
inline std::tuple<std::string, std::string, std::string> get_target(const opt::ParamsAPI2& args)
{
	return{
		args.typegetv<opt::Flag>('H').value_or(DEFAULT_HOST), // hostname
		args.typegetv<opt::Flag>('P').value_or(DEFAULT_PORT), // port
		args.typegetv<opt::Flag>('p').value_or("")            // password
	};
}

static struct {
	/// @brief When true, interactive mode is started after running any commands specified on the commandline.
	bool force_interactive{ false };
	std::vector<std::string> scriptfiles{};
} MainGlobal;

/**
 * @brief		Handle commandline arguments.
 * @param args	Arguments from main()
 */
inline void handle_args(const opt::ParamsAPI2& args)
{
	// help:
	if (args.check_any<opt::Flag, opt::Option>('h', "help")) {
		const auto& [prog_path, prog_name] { env::PATH().resolve_split(args.arg0().value()) };
		std::cout << Help(prog_name);
		std::exit(EXIT_SUCCESS);
	}
	if (args.check_any<opt::Flag, opt::Option>('v', "version")) {
		std::cout << DEFAULT_PROGRAM_NAME << " v" << VERSION << std::endl;
		std::exit(EXIT_SUCCESS);
	}
	// quiet:
	if (args.check_any<opt::Option, opt::Flag>('q', "quiet"))
		g_quiet = true;
	// no-prompt
	if (args.check_any<opt::Option>("no-prompt"))
		g_no_prompt = true;
	// force interactive:
	if (args.check_any<opt::Option, opt::Flag>('i', "interactive"))
		MainGlobal.force_interactive = true;
	// command delay:
	if (const auto arg{ args.typegetv_any<opt::Flag, opt::Option>('d', "delay") }; arg.has_value())
		g_command_delay = str::stoi(arg.value());
	// disable colors:
	if (const auto arg{ args.typegetv_any<opt::Option, opt::Flag>('n', "no-color") }; arg.has_value())
		g_palette.setActive(false);
	// scriptfiles:
	for (auto& scriptfile : args.typegetv_all<opt::Option, opt::Flag>('f', "file"))
		MainGlobal.scriptfiles.emplace_back(scriptfile);
}

inline std::vector<std::string> read_script_file(std::string filename)
{
	if (!file::exists(filename))
		filename = env::PATH{}.resolve(filename, { "", ".ini", ".txt", ".bat", ".scr" });
	if (!file::exists(filename))
		return{};
	if (auto file{ file::read(filename) }; !file.fail()) {
		std::vector<std::string> commands;
		commands.reserve(file::count(file, '\n'));

		for (std::string line{}; str::getline(file, line, '\n'); )
			commands.emplace_back(str::strip_line(line, "#;"));

		commands.shrink_to_fit();
		return commands;
	}
	return {};
}

#ifdef OS_WIN
#include <ControlEventHandler.hpp>
inline void handler(void) { g_connected = false; throw std::exception("SIGINT"); }
#endif

int main(int argc, char** argv, char** envp)
{
	try {
	#ifdef OS_WIN
		sys::registerEventHandler();
		sys::setEventHandlerFunc(sys::Event::CTRL_C, handler);
	#endif
		std::cout << sys::term::EnableANSI; // enable ANSI escape sequences on windows

		const opt::ParamsAPI2 args{ argc, argv, 'H', 'P', 'p', 'd', "delay" }; // parse arguments
		const auto& [host, port, pass] { get_target(args) }; // get connection target info

		handle_args(args);
		std::atexit(&safeExit);		// register the cleanup function

		// open socket
		g_socket = net::connect(host, port);
		g_connected = g_socket != -1;

		if (!g_connected) throw std::exception(("Failed to connect to "s + host + ":"s + port).c_str()); // connection failed

		// auth & commands
		if (rcon::authenticate(g_socket, pass)) {
		#ifdef MULTITHREADING
			auto thread_listener{ std::async(std::launch::async, listener, std::ref(g_mutex)) };
		#endif
			std::vector<std::string> commands{ args.typegetv_all<opt::Parameter>() };
			// read script files:
			if (!MainGlobal.scriptfiles.empty()) {
				for (auto& filename : MainGlobal.scriptfiles) {
					const auto file{ read_script_file(filename) };
					commands.reserve(commands.size() + file.size());
					for (auto& it : file)
						commands.emplace_back(it);
					commands.shrink_to_fit();
				}
			}
			// check if commandline/scriptfile commands were found
			const auto no_commands{ commands.empty() };
			if (!no_commands)
				mode::commandline(commands);
			if (no_commands || MainGlobal.force_interactive)
				mode::interactive(g_socket, "RCON@"s + host);
			g_connected = false; ///< kill listener thread

		#ifdef MULTITHREADING
			if (thread_listener.valid() && listener_ex.has_value())
				throw listener_ex.value();
		#endif
		}
		else throw std::exception(("Authentication Failed! ("s + host + ":"s + port + ")"s).c_str());

		return EXIT_SUCCESS;
	} catch (const std::exception& ex) { ///< catch std::exception
		std::cerr << sys::term::error << ex.what() << std::endl;
	} catch (...) { ///< catch all other exceptions
		std::cerr << sys::term::error << "An unknown exception occurred!" << std::endl;
	}
	return EXIT_FAILURE;
}