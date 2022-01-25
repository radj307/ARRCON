# ARRCON - Another __RCON__ Client
![Windows](https://github.com/radj307/ARRCON/actions/workflows/Windows.yml/badge.svg)
![Linux Build](https://github.com/radj307/ARRCON/actions/workflows/Linux.yml/badge.svg)
![macOS Build](https://github.com/radj307/ARRCON/actions/workflows/macOS.yml/badge.svg)  
Remote console client that is compatible with any game using the Source Rcon Protocol.  
While originally based on [mcrcon](https://github.com/Tiiffi/mcrcon) by [Tiiffi](https://github.com/Tiiffi), it has since completely eclipsed it in terms of features and compatibility with games other than minecraft.  

### Features
  - Works for any game using the [Source RCON Protocol](https://developer.valvesoftware.com/wiki/Source_RCON_Protocol).
  - Handles large packets without issue.
  - Handles multi-packet responses without issue, and has configurable delays in the INI file.
  - Supports Minecraft Bukkit terminal colors using the section sign `§`.
  - Can be used as a one-off from the commandline, or in an interactive shell.
    - Supports piping input from STDIN using shell operators.  
      For example; `echo "help" | ARRCON -S myServer` would send the `help` command to the `myServer` host.
        - Commands from STDIN are sent _after_ any commands explicitly specified on the commandline.
    - Supports running external "scripts" using the `-f`\|`--file` option.
      - Commands are separated by newlines.
      - Commands from script files are sent _after_ any commands from STDIN.
  - Save & recall a server's IP/Hostname, port, and password directly from the commandline without editing a file.
    - _Passwords are under no circumstances printed to the screen at any point; however, the RCON protocol does not allow encrypted messages so keep this in mind when using any RCON client._
  - Cross-Platform:
    - Windows
    - Linux
    - macOS  
      _Pre-built macOS binaries are available as of version 3.2.12_


# Download & Install
There are 2 places you can get release archives:
 1.  The [Releases Pages](https://github.com/radj307/ARRCON/releases)  
 2.  The [Actions Tab](https://github.com/radj307/ARRCON/actions)  
     To download releases from the Actions tab:
     - Click on the most recent build for your OS. _(Nearest to the Top)_
     - Scroll down to ___Artifacts___ & click `latest-<OS>` to download it.

No installation is required, simply download the executable and put it somewhere on your [path](https://github.com/radj307/ARRCON/wiki/Adding-To-Path).  

Alternatively, see [Build From Source](https://github.com/radj307/ARRCON/wiki/Building-from-Source) to build it yourself.


# Usage
Open a terminal in the location where you placed the executable, and run `./ARRCON -h` or `./ARRCON --help` for usage instructions.  

## Modes
The operation mode is selected based on context, but can be influenced by some options.  
There are 2 modes:
- ___Interactive___
  - Used by default when there are no command arguments.
  - Opens an interactive console session. You can send commands and view the responses.
- ___Commandline___
  - Executes a list of commands in order with a configurable delay between sending each packet.
  - You can also use the `-f <filepath>` or `--file <filepath>` options to specify a scriptfile, which is executed line-by-line after any commands passed on the commandline.
