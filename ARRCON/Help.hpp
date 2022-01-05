/**
 * @file	Help.hpp
 * @author	radj307
 * @brief	Contains the Help object, which contains the help display menu & hard-coded options.
 */
#pragma once
#include "globals.h"

 /**
  * @struct	Help
  * @brief	Functor that prints out the help display with auto-formatting.
  */
struct Help {
private:
	const std::string _program_name;
	const std::vector<std::pair<std::string, std::string>> _options{
		{ "-H <Host>  --host <Host>"s, "RCON Server Address or saved hostname.	(Default: \""s + Global.DEFAULT_TARGET.hostname + "\")"s },
		{ "-P <Port>  --port <Port>"s, "RCON Server Port.						(Default: \""s + Global.DEFAULT_TARGET.port + "\")"s },
		{ "-p <Password>  --pass <Pass>"s, "RCON Server Password." },
		{ "-f <file>  --file <file>"s, "Load the specified file and run each line as a command."s },
		{ "-h  --help"s, "Show the help display."s },
		{ "-v  --version"s, "Print the current version number."s },
		{ "-i  --interactive"s, "Start interactive mode after sending all commands specified on the commandline."s},
		{ "-q  --quiet"s, "Don't print server response packets."s },
		{ "-d <ms>  --delay <ms>"s, "Time in milliseconds to wait between each command in commandline mode."s },
		{ "-n  --no-color"s, "Disable colorized console output."s },
		{ "-Q  --no-prompt"s,"Hides the prompt. In commandline mode, this also disables command echo."s},
		{ "--write-ini"s, "(Over)write the configuration file with the default values & exit."s},
		{ "--save-host <name>"s, "Save the current target to the .hosts config as <name> & exit."s },
		{ "--list-hosts"s, "Display all detected hosts from the .hosts config & exit."s },
	};
	const size_t _longest_optname, _max_line_length;

public:
	Help(const std::string& program_name, const size_t& max_line_sz = 120ull) : _program_name{ program_name }, _longest_optname{ [this]() { size_t longest{0ull}; for (auto& [optname, desc] : _options) if (const auto sz{optname.size()}; sz > longest) longest = sz; return longest; }() }, _max_line_length{ max_line_sz } {}
	friend std::ostream& operator<<(std::ostream& os, const Help& help)
	{
		const auto tabSize{ help._longest_optname + 2ull };
		os << help._program_name << ((help._program_name != DEFAULT_PROGRAM_NAME) ? "("s + DEFAULT_PROGRAM_NAME + ") "s : " ") << "v" << ARRCON_VERSION << '\n'
			<< "CLI Application that allows communicating with servers using the Source RCON Protocol.\n"
			<< '\n'
			<< "USAGE:\n"
			<< "  " << help._program_name << " [OPTIONS] [CONFIG HOSTNAME] [COMMANDS]\n"
			<< '\n'
			<< "OPTIONS:\n";
		for (auto& [optname, desc] : help._options)
			os << "  " << optname << str::VIndent(tabSize + 2ull, optname.size()) << desc << '\n';
		os << '\n'
			<< "MODES:\n"
			<< "  [1]\tInteractive\tInteractive shell prompt. This is the default mode.\n"
			<< "  [2]\tCommandline\tExecutes commands passed on the commandline.\n"
			<< "\t\t\tThis mode is automatically used when commandline input is detected (Excluding OPTIONS).\n"
			<< "\t\t\tYou can also specify files using \"-f <file>\" or \"--file <file>\".\n"
			<< "\t\t\tEach line will be executed as a command in commandline mode.\n"
			<< "\t\t\tScript commands are executed in - order, after any commands passed as arguments.\n"
			<< "\t\t\tYou can specify multiple files per command.\n"
			;
		return os.flush();
	}
};
