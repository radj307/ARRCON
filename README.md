# ARRCON - Another RCON Client
Commandline client application using the [Source RCON Protocol](https://developer.valvesoftware.com/wiki/Source_RCON_Protocol) written in C++.

Capable of handling packets up to 10kB in size, as well as multi-packet responses.  
Supports running scripts & commands directly from the commandline, and/or an interactive terminal session with ANSI color sequence support.

# Installation
No installation is required, simply download the executable and put it somewhere.  

[_(Optional)_ Add location to your PATH](https://github.com/radj307/ARRCON/wiki/Adding-To-Path)

# Usage

Open a terminal in the location where you placed the executable, and run `arrcon --help` for usage instructions.  
Additional documentation is available below, but it may be out of date.  

## Modes
The operation mode is selected based on context, but can be influenced by some options.  
There are 2 modes:
- ___Interactive___
  - Used by default when there are no command arguments.
  - Opens an interactive console session. You can send commands and view the responses.
- ___Commandline___
  - Executes a list of commands in order with a configurable delay between sending each packet.
  - You can also use the `-f <filepath>` or `--file <filepath>` options to specify a scriptfile, which is executed line-by-line after any commands passed on the commandline.

## Options
Options can be specified anywhere on the commandline, and must use a dash `-` delimiter.  
| Commandline Option / Flag   | Description                                              |
|-----------------------------|:---                                                      |
|`-H <Address>`               | Specify the RCON server's IP address or hostname.
|`-P <Port>`                  | Specify the RCON server's port number.
|`-p <Password>`              | Specify the RCON server's authentication password.
|`-f <File>` `--file <file>`  | Load the specified file and run each line as a command. You can specify this argument multiple times to include multiple files.
|`-h` `--help`                | Shows the help display, including a brief description of the program & option documentation, then exits.
|`-v` `--version`             | Prints the current version number, then exits.
|`-i` `--interactive`         | Starts an interactive session after executing any commands specified on the commandline.
|`-d <ms>` `--delay <ms>`     | Set the delay (in milliseconds) between sending each command packet when using batch mode.
|`-q` `--quiet`               | Prevents server response packets from being displayed, but does not silence errors or exception messages.
|`-n` `--no-color`            | Disables colorized terminal output.
| `--no-prompt`               | Hides the prompt (Default prompt is "RCON@HOST> ").
| `--write-ini`               | (Over)Writes the default `.ini` configuration file.

For example, to connect to _MyHostname:27015_ using _myPassword_ as the password, send the _help_ command, then start an interactive session:  
`arrcon -i -H MyHostname -P 27015 -p myPassword help`  


## INI Configuration
INI files must have the same name as the executable (excluding extension) to be detected.  
Keep this in mind if you rename the executable!  

You can create a default INI file by using the `arrcon --write-ini` command.  
```ini
[target]
sHost =                           ; Defines the default hostname/IP, unless a hostname/IP was specified on the commandline.
sPort =                           ; Defines the default port, unless a port was specified on the commandline.
sPass =                           ; Defines the default password, unless a password was specified on the commandline.

[appearance]
bDisablePrompt = false            ; When true, this is the equivalent of always specifying the --no-prompt option.
bDisableColors = false            ; When true, this is the equivalent of always specifying the -n/--no-color option.
sCustomPrompt =                   ; Accepts a string to use in place of the default prompt, excluding the end, which is always "> ".

[timing]
iCommandDelay = 0                 ; The delay in milliseconds between sending each command when using commandline/scriptfile mode.
iReceiveDelay = 10                ; The time in milliseconds to wait after receiving packets. Raise this if multi-packet responses aren't fully received. 
iSelectTimeout = 500              ; The max amount of time to wait for packets before timing out. Raise this if your network is slow.
```
