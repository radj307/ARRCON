#include "net/mode.hpp"		///< RCON client modes
#include "utils.hpp"

#include <make_exception.hpp>
#include <opt3.hpp>
#include <fileio.hpp>			///< file I/O functions
#include <TermAPI.hpp>			///< file I/O functions

#include <signal.h>				///< signal handling
#include <unistd.h>

using namespace net;

int main(const int argc, char** argv)
{
	try {
		// Fixes a "bug" with the Wezterm terminal emulator where characters have opaque black backgrounds when using transparent backgrounds.
		Global.palette.setDefaultResetSequence(color::reset_f);

		std::cout << term::EnableANSI; // enable ANSI escape sequences on windows
		const opt3::ArgManager args{ argc, argv,
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'H', "host"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'S', "saved"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'P', "port"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'p', "pass"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'w', "wait"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'f', "file"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, "save-host"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, "remove-host"),
		}; // parse arguments

		// Argument:  [-n|--no-color]
		if (args.check_any<opt3::Option, opt3::Flag>('n', "no-color")) {
			Global.no_color = true;
			Global.enable_bukkit_color_support = false;
			Global.palette.setActive(false);
		}

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

		Global.env.load_all(myNameNoExt);

		const config::Locator cfg_path(myDir, myNameNoExt);

		// Argument:  [-q|--quiet]
		Global.quiet = args.check_any<opt3::Option, opt3::Flag>('q', 's', "quiet");
		// Argument:  [-h|--help]
		if (args.check_any<opt3::Flag, opt3::Option>('h', "help")) {
			std::cout << Help(myName) << std::endl;
			return 0;
		}
		// Argument:  [-v|--version] (mutually exclusive with help as it shows the version number as well)
		else if (args.check_any<opt3::Flag, opt3::Option>('v', "version")) {
			if (!Global.quiet) std::cout << DEFAULT_PROGRAM_NAME << " v";
			std::cout << ARRCON_VERSION;
			if (!Global.quiet) std::cout << " (" << ARRCON_COPYRIGHT << ')';
			std::cout << std::endl;
			return 0;
		}
		// Argument:  [--print-env]
		else if (args.check<opt3::Option>("print-env")) {
			(std::cout << Global.env).flush();
			return 0;
		}

		// Get the INI file's path
		std::filesystem::path ini_path{ cfg_path.from_extension(".ini") };

		// Read the INI if it exists
		if (file::exists(ini_path))
			config::load_ini(ini_path);

		// Override with environment variables if specified
		Global.target.hostname = Global.env.Values.hostname.value_or(Global.target.hostname);
		Global.target.port = Global.env.Values.port.value_or(Global.target.port);
		Global.target.password = Global.env.Values.password.value_or(Global.target.password);

		if (args.empty() && !Global.allow_no_args) {
			std::cerr << Help(myName) << std::endl << std::endl;
			throw make_exception(
				"No arguments were specified!\n",
				indent(10), "Function Name:        main()\n",
				indent(10), "Suggested Solutions:\n",
				indent(10), "1.  Specify a target to connect to with the [-H|--host], [-P|--port], & [-p|--pass] options.\n",
				indent(10), "2.  Set ", Global.palette(Color::YELLOW), "bAllowNoArgs = true", Global.palette(), " in the INI config file.\n",
				indent(10), "3.  Read the help display above for command assistance."
			);
		}

		// Initialize the hostlist
		net::HostList hosts;

		const auto hostfile_path{ cfg_path.from_extension(".hosts") };
		if (file::exists(hostfile_path)) // load the hostfile if it exists
			hosts = file::INI{ hostfile_path };

		// get the target server's connection information
		Global.target = resolveTargetInfo(args, hosts);

		handle_arguments(args, ini_path);

		handle_hostfile_arguments(args, hosts, hostfile_path);

		// get the commands to execute on the server
		const auto commands{ get_commands(args, PATH) };

		// If no custom prompt is set, use the default one
		if (Global.custom_prompt.empty())
			Global.custom_prompt = (Global.no_prompt ? "" : str::stringify(Global.palette.set(Color::GREEN), "RCON@", Global.target.hostname, Global.palette.reset(Color::GREEN), '>', Global.palette.reset(), ' '));

		// Register the cleanup function before connecting the socket
		std::atexit(&net::cleanup);

		// Connect the socket
		Global.socket = net::connect(Global.target.hostname, Global.target.port);

		// set & check if the socket was connected successfully
		if (!(Global.connected = (Global.socket != SOCKET_ERROR)))
			throw connection_exception("main()", "Socket descriptor was set to (" + std::to_string(Global.socket) + ") after successfully initializing the connection.", Global.target.hostname, Global.target.port, LAST_SOCKET_ERROR_CODE(), net::getLastSocketErrorMessage());

		if (Global.target.password.empty())
			throw make_exception("Password cannot be blank!");

		// authenticate with the server, run queued commands, and open an interactive session if necessary.
		if (rcon::authenticate(Global.socket, Global.target.password)) {
			// authentication succeeded, run commands
			if (mode::commandline(commands) == 0ull || Global.force_interactive)
				mode::interactive(Global.socket); // if no commands were executed from the commandline or if the force interactive flag was set
		}
		else throw badpass_exception(Global.target.hostname, Global.target.port, LAST_SOCKET_ERROR_CODE(), net::getLastSocketErrorMessage());

		return 0;
	} catch (const ex::except& ex) { // custom exception type
		std::cerr << Global.palette.get_fatal() << ex.what() << std::endl;
		return 1;
	} catch (const std::exception& ex) { // standard exceptions
		std::cerr << Global.palette.get_fatal() << ex.what() << std::endl;
		std::cerr << Global.palette.get_placeholder() << "Please report this exception here: " << ISSUE_REPORT_URL << std::endl;
		return 1;
	} catch (...) { // undefined exceptions
		std::cerr << Global.palette.get_crit() << "An unknown exception occurred!" << std::endl;
		return 1;
	}
}
