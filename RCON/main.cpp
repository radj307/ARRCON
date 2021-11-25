#include "mode.hpp"

#include <ParamsAPI2.hpp>
#include <env.hpp>
#include <future>

#include <signal.h>
#include <unistd.h>

inline constexpr const auto VERSION{ "1.0.0" };

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
 * @brief	Functor that prints out the help display with formatting.
 */
struct Help {
private:
	const std::string _program_name;
	const std::vector<std::pair<std::string, std::string>> _options{
		{ "-H"s, "RCON Address.   (Default: \""s + DEFAULT_HOST + "\")"s },
		{ "-P"s, "RCON Port.      (Default: \""s + DEFAULT_PORT + "\")"s },
		{ "-p"s, "RCON Password." },
		{ "-h  --help"s, "Show the help display."s },
		{ "-q  --quiet"s, "Don't print server response packets."s },
		{ "-d  --delay"s, "(Batch Mode) Time in milliseconds to wait between each command."s },
		{ "-n  --no-colors"s, "Disable colorized console output."s },
	};
	const size_t _longest_optname;
public:
	Help(const std::string& program_name) : _program_name{ program_name }, _longest_optname{ [this]() { size_t longest{0ull}; for (auto& [optname, desc] : _options) if (const auto sz{ optname.size() }; sz > longest) longest = sz; return longest; }() } {}
	friend std::ostream& operator<<(std::ostream& os, const Help& help)
	{
		const auto margin_width{ help._longest_optname + 2ull };
		os << help._program_name << " v" << VERSION << "\n"
			<< "CLI Application that allows communicating with servers using the Source RCON Protocol.\n"
			<< "\nUSAGE:\n  "
			<< help._program_name << " [OPTIONS] [COMMANDS]\n"
			<< "\nOPTIONS:\n";
		for (auto& [optname, desc] : help._options)
			os << "  " << optname << str::VIndent(margin_width + 2ull, optname.size()) << desc << '\n';
		os << "\nMODES:\n"
			<< "  [1]\tBatch\t\tExecutes commands passed on the commandline.\n\t\t\tThis mode is automatically used when commandline input is detected (Excluding OPTIONS).\n"
			<< "  [2]\tInteractive\tInteractive shell prompt. This is the default mode.";
		return os;
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

/**
 * @brief		Handle commandline arguments.
 * @param args	Arguments from main()
 */
inline void handle_args(const opt::ParamsAPI2& args)
{
	// help:
	if (args.check_any<opt::Flag, opt::Option>('h', "help")) {
		const auto& [prog_path, prog_name] { env::PATH().resolve_split(args.arg0().value()) };
		std::cout << Help(prog_name) << std::endl;
		std::exit(EXIT_SUCCESS);
	}
	// quiet:
	if (args.check_any<opt::Option, opt::Flag>('q', "quiet"))
		g_quiet = true;
	// command delay:
	if (const auto arg{ args.typegetv_any<opt::Flag, opt::Option>('d', "delay") }; arg.has_value())
		g_command_delay = str::stoi(arg.value());
	// disable colors:
	if (const auto arg{ args.typegetv_any<opt::Option, opt::Flag>('n', "no-color") }; arg.has_value())
		g_palette.setActive(false);
}

#ifdef OS_WIN
#include <ControlEventHandler.hpp>
inline void handler(void) { g_connected = false; throw std::exception("SIGINT"); }
#endif

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
			auto thread_listener{ std::async(std::launch::async, listener, std::ref(g_mutex)) };
			if (const std::vector<std::string> parameters{ args.typegetv_all<opt::Parameter>() }; !parameters.empty())
				mode::batch(parameters);
			else
				mode::interactive(g_socket, "RCON@"s + host);
			g_connected = false; ///< kill listener thread

			if (thread_listener.valid() && listener_ex.has_value())
				throw listener_ex.value();
		}
		else throw std::exception("Authentication Failed!");

		return EXIT_SUCCESS;
	} catch (const std::exception& ex) { ///< catch std::exception
		std::cerr << sys::term::error << ex.what() << std::endl;
	} catch (...) { ///< catch all other exceptions
		std::cerr << sys::term::error << "An unknown exception occurred!" << std::endl;
	}
	return EXIT_FAILURE;
}