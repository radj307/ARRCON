# ARRCON - Another RCON Client
Commandline client application using the [Source RCON Protocol](https://developer.valvesoftware.com/wiki/Source_RCON_Protocol) written in C++.

Capable of handling packets up to 10kB in size, as well as multi-packet responses.  
Supports running scripts & commands directly from the commandline, and/or an interactive terminal session with ANSI color sequence support.

# Installation
No installation is required, simply download the executable and put it somewhere.  

## _(Optional)_ Add location to your PATH
This allows you to call it from anywhere you open a terminal, or from scripts, without having to specify the absolute filepath.  
_Note: If you already had a terminal open, you will have to close & re-open it to update its environment._

### __Windows__
1. Press __Win+R__ to open the run dialog, then paste the following and press enter:  
   `rundll32.exe sysdm.cpl,EditEnvironmentVariables`
2. Under _User variables for \<USERNAME\>_, select _Path_, then click _Edit..._ -> _New_ & paste the path to the directory containing `RCON.exe`

   For example, if you placed the executable here: `X:\bin\RCON.exe`, you would put `X:\bin` on your path.  

### __Linux__
I'd recommend just placing the executable somewhere that is already on your path, such as `/usr/local/bin` as modifying the path permanently requires modifying the shell configuration files.  
_If you want to anyway:_
1. Choose whether you want all users to be able to use the program, or just your current user.
  - Current User:  
    Use `~/.bashrc` for ___\<SHELL_CFG_FILE\>___ in the command below.
  - All Users:  
    Use `/etc/environment` or `/etc/profile` for ___\<SHELL_CFG_FILE\>___ in the command below.
2. Use `echo "export PATH="<RCON_PATH>:$PATH"" >> <SHELL_CFG_FILE>`  
   _Make sure to swap __\<RCON_PATH\>__ with the directory where the RCON program is located._

# Usage

Open a terminal in the location where you placed the executable

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
| `--no-prompt`               | Hides the prompt when using interactive mode.
