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

public:
	Help(const std::filesystem::path& program_name) : _program_name{ program_name.generic_string() } {}
	friend std::ostream& operator<<(std::ostream& os, const Help& help)
	{
		return os << help._program_name << ' ' << ((help._program_name != std::string_view(DEFAULT_PROGRAM_NAME)) ? "("s + DEFAULT_PROGRAM_NAME + ") "s : "") << "v" << ARRCON_VERSION << '\n'
			<< "Another RCON Client, compatible with any game using the Source RCON Protocol.\n"
			<< '\n'
			<< "USAGE:" << '\n'
			<< "  " << help._program_name << " [OPTIONS] [COMMANDS]\n"
			<< '\n'
			<< "Arguments with spaces must be enclosed by single (\') or double (\") quotation marks." << '\n'
			<< '\n'
			<< "OPTIONS:" << '\n'
			<< "  -H <Host>  --host <Host>       RCON Server IP/Hostname.  (Default: \"" << Global.DEFAULT_TARGET.hostname << "\")" << '\n'
			<< "  -P <Port>  --port <Port>       RCON Server Port.         (Default: \"" << Global.DEFAULT_TARGET.port + "\")" << '\n'
			<< "  -p <Password>  --pass <Pass>   RCON Server Password." << '\n'
			<< "  -S <host>  --saved <host>      Use a saved host's IP, Port, & Password if the [-H|-P|-p] options are not specified." << '\n'
			<< "  --list-hosts                   Show a list of all currently saved hosts, then exit." << '\n'
			<< "  --save-host <name>             Save the target specified with the [-H|-P|-p] options as \"<name>\"" << '\n'
			<< "  --remove-host <name>           Remove a saved host named \"<name>\" from the list, then exit." << '\n'
			<< "  -f <file>  --file <file>       Load the specified file and run each line as a command." << '\n'
			<< "  -h  --help                     Show the help display, then exit." << '\n'
			<< "  -v  --version                  Print the current version number, then exit." << '\n'
			<< "  -t  -i  --interactive          Always use interactive terminal mode. Any one-off commands are executed first." << '\n'
			<< "  -q  -s  --quiet                Silent mode, prevents most console output from being printed." << '\n'
			<< "  -w <ms>  --wait <ms>           Time in milliseconds to wait between each command in commandline mode." << '\n'
			<< "  -n  --no-color                 Disable colorized console output." << '\n'
			<< "  -Q  --no-prompt                Disables the prompt in interactive mode, and command echo in commandline mode." << '\n'
			<< "  --print-env                    Prints all recognized environment variables, their values, and descriptions." << '\n'
			<< "  --write-ini                    (Over)write the configuration file with the default values & exit." << '\n'
			<< '\n'
			<< "ENVIRONMENT:\n"
			<< ""
			<< '\n'
			<< "MODES:\n"
			<< "  [1]  Interactive    Interactive terminal mode. This is the default mode when no commands are specified" << '\n'
			<< "                      directly on the commandline. You can always force interactive mode even when there" << '\n'
			<< "                      are commands with the [-i|--interactive] option; commands are always executed first." << '\n'
			<< "  [2]  Commandline    Executes commands that were directly passed on the commandline, or from STDIN." << '\n'
			<< "                      This mode is automatically used when non-option arguments are detected. This behaviour" << '\n'
			<< "                      can be overridden with the [-i|--interactive] option." << '\n'
			<< "                      You can also specify files using \"-f <file>\" or \"--file <file>\"." << '\n'
			<< "                      Each line will be executed as a command in commandline mode after any arguments." << '\n'
			<< "                      Input received from STDIN follows the same rules as script files."
			;
	}
};
