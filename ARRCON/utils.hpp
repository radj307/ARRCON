#pragma once
#include "globals.h"

#include <filei.hpp>
#include <fileutil.hpp>
#include <hasPendingDataSTDIN.h>
#include <str.hpp>
#include <ParamsAPI2.hpp>
#include <term.hpp>
#include <envpath.hpp>

#include <iostream>
#include <vector>
#include <string>

#undef read
#undef write

/**
 * @brief			Resolve the target server's connection information with the User's inputs.
 * @param args		Commandline argument container. The following options are checked:
 *\n				[-S|--saved]
 *\n				[-H|--host]
 *\n				[-P|--port]
 *\n				[-p|--pass]
 * #param saved		The user's list of saved connection targets, if one exists.
 * @returns			HostInfo
 *\n				This contains the resolved connection information of the target server.
 */
inline HostInfo resolveTargetInfo(const opt::ParamsAPI2& args, const config::HostList& saved = {})
{
	// Argument:  [-S|--saved]
	if (const auto savedArg{ args.typegetv_any<opt::Flag, opt::Option>('S', "saved") }; savedArg.has_value()) {
		if (const auto it{ saved.find(savedArg.value()) }; it != saved.end()) {
			return std::move(HostInfo{ it->second, Global.DEFAULT_TARGET }.moveWithOverrides(
				args.typegetv_any<opt::Flag, opt::Option>('H', "host"),
				args.typegetv_any<opt::Flag, opt::Option>('P', "port"),
				args.typegetv_any<opt::Flag, opt::Option>('p', "pass")
			));
		}
		else throw make_exception("There is no saved target named ", (Global.no_color ? "\"" : ""), Global.palette.set(UIElem::INI_KEY_HIGHLIGHT), savedArg.value(), Global.palette.reset(), (Global.no_color ? "\"" : ""), " in the hosts file!");
	}
	else return{
		args.typegetv_any<opt::Flag, opt::Option>('H', "host").value_or(Global.DEFAULT_TARGET.hostname),
		args.typegetv_any<opt::Flag, opt::Option>('P', "port").value_or(Global.DEFAULT_TARGET.port),
		args.typegetv_any<opt::Flag, opt::Option>('p', "pass").value_or(Global.DEFAULT_TARGET.password)
	};
}

/**
 * @brief			Reads the target file and returns a vector of command strings for each valid line.
 * @param filename	Target Filename
 * @param pathvar	The value of the PATH environment variable as a PATH utility object.
 * @returns			std::vector<std::string>
 *\n				Each index contains a command in the file.
 */
inline std::vector<std::string> read_script_file(std::string filename, const env::PATH& pathvar)
{
	if (!file::exists(filename)) // if the filename doesn't exist, try to resolve it from the PATH
		filename = pathvar.resolve(filename, { ".txt" }).generic_string();
	if (!file::exists(filename)) // if the resolved filename still doesn't exist, print warning
		std::cerr << term::get_warn() << "Couldn't find file: \"" << filename << "\"\n";
	// read the file, parse it if the stream didn't fail
	else if (auto fss{ file::read(filename) }; !fss.fail()) {
		std::vector<std::string> commands;
		commands.reserve(file::count(fss, '\n') + 1ull);

		for (std::string lnbuf{}; std::getline(fss, lnbuf, '\n'); )
			if (lnbuf = str::strip_line(lnbuf, "#;"); !lnbuf.empty())
				commands.emplace_back(lnbuf);

		commands.shrink_to_fit();
		return commands;
	}
	return{};
}
/**
 * @brief			Retrieves a list of all user-specified commands to be sent to the RCON server, in order.
 * @param args		All commandline arguments.
 * @param pathvar	The value of the PATH environment variable as a PATH utility object.
 * @returns			std::vector<std::string>
 */
inline std::vector<std::string> get_commands(const opt::ParamsAPI2& args, const env::PATH& pathvar)
{
	std::vector<std::string> commands{ args.typegetv_all<opt::Parameter>() }; // Arg<std::string> is implicitly convertable to std::string

	// Check for piped data on STDIN
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

#pragma region ArgumentHandlers
inline void handle_hostfile_arguments(const opt::ParamsAPI2& args, config::HostList& hosts, const HostInfo& target, const std::filesystem::path& hostfile_path)
{
	bool do_exit{ false };
	// remove-host
	if (const auto remove_hosts{ args.typegetv_all<opt::Option>("remove-host") }; !remove_hosts.empty()) {
		do_exit = true;
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
				std::cout << message_buffer.rdbuf() << term::get_msg(!Global.no_color) << "Successfully saved modified hostfile " << hostfile_path << std::endl;
			else
				throw permission_exception("handle_hostfile_arguments()", hostfile_path, "Failed to write modified Hostfile to disk!");
		}
	}
	// save-host
	if (const auto save_host{ args.typegetv<opt::Option>("save-host") }; save_host.has_value()) {
		do_exit = true;
		std::stringstream message_buffer; // save the messages in a buffer to prevent misleading messages in the event of a file writing error

		const file::INI::SectionContent target_info{ target };
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
			std::cout << message_buffer.rdbuf() << term::get_msg(!Global.no_color) << "Successfully saved modified hostlist to " << hostfile_path << std::endl;
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
			const HostInfo& hostinfo{ info };
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
	}
	if (do_exit)
		std::exit(EXIT_SUCCESS);
}

/**
 * @brief		Handle commandline arguments.
 * @param args	Arguments from main()
 */
inline void handle_arguments(const opt::ParamsAPI2& args, const std::filesystem::path& ini_path)
{
	// write-ini:
	if (args.check<opt::Option>("write-ini")) {
		if (!ini_path.empty() && config::save_ini(ini_path)) {
			std::cout << term::get_msg(!Global.no_color) << "Successfully wrote config: " << ini_path << std::endl;
			std::exit(EXIT_SUCCESS);
		}
		else
			throw permission_exception("handle_arguments()", ini_path, "Failed to open INI for writing!");
	}
	// update-ini
	if (args.check<opt::Option>("update-ini")) {
		if (!ini_path.empty() && config::save_ini(ini_path, false)) {
			std::cout << term::get_msg(!Global.no_color) << "Successfully updated config: " << ini_path << std::endl;
			std::exit(EXIT_SUCCESS);
		}
		else
			throw permission_exception("handle_arguments()", ini_path, "Failed to open INI for writing!");
	}
	// force interactive:
	Global.force_interactive = args.check_any<opt::Option, opt::Flag>('t', 'i', "interactive");
	// no-prompt
	Global.no_prompt = args.check_any<opt::Flag, opt::Option>('Q', "no-prompt");
	// command delay:
	if (const auto arg{ args.typegetv_any<opt::Flag, opt::Option>('w', "wait") }; arg.has_value()) {
		if (std::all_of(arg.value().begin(), arg.value().end(), isdigit))
			Global.command_delay = std::chrono::milliseconds(std::abs(str::stoll(arg.value())));
		else throw make_exception("Invalid delay value given: \"", arg.value(), "\", expected an integer.");
	}
	// scriptfiles:
	for (const auto& scriptfile : args.typegetv_all<opt::Option, opt::Flag>('f', "file"))
		Global.scriptfiles.emplace_back(scriptfile);
}
#pragma endregion ArgumentHandlers
