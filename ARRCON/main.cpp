#include "net/mode.hpp"		///< RCON client modes
#include "utils.hpp"

#include <make_exception.hpp>
#include <ParamsAPI2.hpp>
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
		const opt::ParamsAPI2 args{ argc, argv, 'H', "host", 'S', "saved", 'P', "port", 'p', "pass", 'w', "wait", 'f', "file", "save-host", "remove-host" }; // parse arguments

		// Argument:  [-n|--no-color]
		if (args.check_any<opt::Option, opt::Flag>('n', "no-color")) {
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
		Global.quiet = args.check_any<opt::Option, opt::Flag>('q', 's', "quiet");
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
				TABSPACE"Function Name:        main()\n",
				TABSPACE"Suggested Solutions:\n",
				TABSPACE"1.  Set ", Global.palette.set(Color::YELLOW), "bAllowNoArgs = true", Global.palette.reset(), " in the INI config file.\n",
				TABSPACE"2.  Specify a target to connect to with the [-H|--host], [-P|--port], & [-p|--pass] options.\n",
				TABSPACE"3.  Read the help display above for command assistance.\n"
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


		bool do_exit{ false };
		// remove-host
		if (const auto remove_hosts{ args.typegetv_all<opt::Option>("remove-host") }; !remove_hosts.empty()) {
			do_exit = true;
			std::stringstream message_buffer; // save the messages in a buffer to prevent misleading messages in the event of a file writing error
			for (const auto& name : remove_hosts) {
				if (hosts.erase(name))
					message_buffer << Global.palette.get_msg() << "Removed " << Global.palette(Color::YELLOW, '\"') << name << Global.palette('\"') << '\n';
				else
					message_buffer << term::get_error(!Global.no_color) << "Hostname \"" << Global.palette(Color::YELLOW, '\"') << name << Global.palette('\"') << " doesn't exist!" << '\n';
			}

			// if the hosts file is empty, delete it
			if (hosts.empty()) {
				if (std::filesystem::remove(hostfile_path))
					std::cout << message_buffer.rdbuf() << Global.palette.get_msg() << "Deleted the hostfile as there are no remaining entries." << std::endl;
				else
					throw permission_exception("handle_hostfile_arguments()", hostfile_path, "Failed to delete empty Hostfile!");
				std::exit(EXIT_SUCCESS); // host list is empty, ignore do_list_hosts as nothing will happen
			} // otherwise, save the modified hosts file.
			else {
				if (config::save_hostfile(hosts, hostfile_path)) // print a success message or throw failure exception
					std::cout << message_buffer.rdbuf() << Global.palette.get_msg() << "Successfully saved modified hostfile " << hostfile_path << std::endl;
				else
					throw permission_exception("handle_hostfile_arguments()", hostfile_path, "Failed to write modified Hostfile to disk!");
			}
		}
		// save-host
		if (const auto save_host{ args.typegetv<opt::Option>("save-host") }; save_host.has_value()) {
			do_exit = true;
			std::stringstream message_buffer; // save the messages in a buffer to prevent misleading messages in the event of a file writing error

			const file::INI::SectionContent target_info{ Global.target };
			const auto& [existing, added] {
				hosts.insert(std::make_pair(save_host.value(), target_info))
			};

			if (added)
				message_buffer << Global.palette.get_msg() << "Added host: " << Global.palette(Color::YELLOW, '\"') << save_host.value() << Global.palette.reset_or('\"') << " " << Global.target.hostname << ':' << Global.target.port << '\n';
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
				throw make_exception("Host ", Global.palette(Color::YELLOW, '\"'), save_host.value(), Global.palette('\"'), " is already set to ", Global.target.hostname, ':', Global.target.port, '\n');
			else
				message_buffer << Global.palette.get_msg() << "Updated " << Global.palette(Color::YELLOW, '\"') << save_host.value() << Global.palette('\"') << ": " << Global.target.hostname << ':' << Global.target.port << '\n';


			if (config::save_hostfile(hosts, hostfile_path)) // print a success message or throw failure exception
				std::cout << message_buffer.rdbuf() << Global.palette.get_msg() << "Successfully saved modified hostlist to " << hostfile_path << std::endl;
			else
				throw permission_exception("handle_hostfile_arguments()", hostfile_path, "Failed to write modified Hostfile to disk!");
		}
		// list all hosts
		if (args.check<opt::Option>("list-hosts")) {
			do_exit = true;
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
				const net::HostInfo& hostinfo{ info };
				if (!Global.quiet) {
					std::cout
						<< Global.palette(Color::YELLOW, '\"') << name << Global.palette('\"') << '\n'
						<< "    Host:  " << hostinfo.hostname << '\n'
						<< "    Port:  " << hostinfo.port << '\n';
				}
				else {
					std::cout << Global.palette(Color::YELLOW, '\"') << name << Global.palette('\"')
						<< str::VIndent(indentation_max, name.size()) << "( " << hostinfo.hostname << ':' << hostinfo.port << " )\n";
				}
			}
		}
		if (do_exit)
			std::exit(EXIT_SUCCESS);

//		handle_hostfile_arguments(args, hosts, hostfile_path);

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
		std::cerr << Global.palette.get_error() << ' ' << ex.what() << std::endl;
		return 1;
	} catch (const std::exception& ex) { // standard exceptions
		std::cerr << Global.palette.get_error() << ' ' << ex.what() << std::endl;
		std::cerr << Global.palette.get_placeholder() << "  " << "Please report this exception here: " << ISSUE_REPORT_URL << std::endl;
		return 1;
	} catch (...) { // undefined exceptions
		std::cerr << Global.palette.get_crit() << "  " << "An unknown exception occurred!" << std::endl;
		return 1;
	}
}
