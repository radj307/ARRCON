<p align="center">
  <img
       src="https://i.imgur.com/yARb4DV.png"
       alt="ARRCON Banner"
  />
</p>  
<p align="center">
<p align="center">
  <a href="https://github.com/radj307/ARRCON/releases"><img alt="GitHub release (latest by date)" src="https://img.shields.io/github/v/release/radj307/ARRCON?color=ffffff&label=Version&logo=github&style=for-the-badge"></a>
  <nobr></nobr>
  <a href="https://github.com/radj307/ARRCON/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/radj307/ARRCON/total?label=Downloads&color=ffffff&logo=github&style=for-the-badge"></a>
</p>
<p align="center">
  <a href="https://github.com/radj307/ARRCON/actions/workflows/Windows.yml"><img alt="Windows Build Status" src="https://img.shields.io/github/workflow/status/radj307/ARRCON/Windows%20Smoketest?label=Windows%20Smoketest&logo=github&style=for-the-badge"></a>
  <a href="https://github.com/radj307/ARRCON/actions/workflows/Linux.yml"><img alt="Linux Build Status" src="https://img.shields.io/github/workflow/status/radj307/ARRCON/Linux%20Smoketest?label=Linux%20Smoketest&logo=github&style=for-the-badge"></a>
  <a href="https://github.com/radj307/ARRCON/actions/workflows/Windows.yml"><img alt="macOS Build Status" src="https://img.shields.io/github/workflow/status/radj307/ARRCON/macOS%20Smoketest?label=macOS%20Smoketest&logo=github&style=for-the-badge"></a>
</p>
<p align="center">
  <a href="https://github.com/radj307/ARRCON/releases">Releases</a>&nbsp&nbsp|&nbsp&nbsp<a href="https://github.com/radj307/ARRCON/wiki">Wiki</a>&nbsp&nbsp|&nbsp&nbsp<a href="https://github.com/radj307/ARRCON/issues">Issues</a>
</p>

A lightweight terminal-based remote console client compatible with any game using the Source RCON Protocol.  

ARRCON was inspired by [mcrcon by Tiiffi](https://github.com/Tiiffi/mcrcon), and will be familiar to anyone who has used mcrcon previously.  

# Features
  - Cross-Platform:
    - Windows
    - Linux
    - macOS
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
      - Line comments can be written using semicolons `;` or pound signs '#'.
  - Save & recall a server's IP/Hostname, port, and password directly from the commandline without editing files.  
    _Passwords are __never__ printed to the terminal._
    

# Download & Install
Get the latest version for your OS from the [releases](https://github.com/radj307/ARRCON/releases) page.

There is no installation process, simply extract the archive & it's ready to go.  
To use ARRCON without specifying the absolute filepath to it, you have to [add the location to your Path variable](https://github.com/radj307/ARRCON/wiki/Adding-To-Path).

## Building from Source
See [here](https://github.com/radj307/ARRCON/wiki/Building-from-Source) for a brief guide on building ARRCON from source.


# Usage
ARRCON is a CLI _(Command-Line Interface)_ program, which means you need to run it through a terminal.  
On Windows, you can use `cmd.exe` or `powershell.exe` by R+Clicking on the start menu and selecting "Command Prompt" or "PowerShell".  
For more detailed usage instructions, see [Getting Started](https://github.com/radj307/ARRCON/wiki)

To see a list of commands, use `ARRCON -h` or `ARRCON --help`  
To see a list of environment variables, their current values, and a description of each, use `ARRCON --print-env`


## Modes
The operation mode is selected based on context, but can be influenced by some options.  
There are 2 modes:
- ___Interactive Shell___
  - Used by default when there are no command arguments.
  - Opens an interactive console session. You can send commands and view the responses in real-time.
  - Connection remains open until you disconnect or kill the process, or if the server closes.
- ___Commandline / Scripting___  
  - Automatically used when additional input is given on the commandline, or when piping input from STDIN.  
    ![ARRCON Scripting Support](https://i.imgur.com/oPX47RD.png)  
  - Executes a list of commands in order with a configurable delay between sending each packet.
  - You can also use the `-f <filepath>` or `--file <filepath>` options to specify a scriptfile, which is executed line-by-line after any commands passed on the commandline.

# Contributing

## Feedback & Requests
Feel free to leave feedback on the issues tab!  
There are a number of premade templates for the following situations:
- [Questions](https://github.com/radj307/ARRCON/issues/new?assignees=radj307&labels=question&template=question.md&title=%5BQUESTION%5D+)
- [Bug Reports](https://github.com/radj307/ARRCON/issues/new?assignees=radj307&labels=bug&template=bug-report.md&title=%5BBUG%5D+%E2%80%A6)
- [Protocol Support Requests](https://github.com/radj307/ARRCON/issues/new?assignees=radj307&labels=bug%2C+enhancement%2C+support&template=support-request.md&title=Unsupported+Title%3A+%3Ctitle%3E)  
  - __A note on Battleye's RCON protocol:__  
    Battleye's RCON protocol requires sending "keep-alive" packets at least every 45 seconds to maintain the connection, which is better suited by a multithreaded GUI application, and as such will not be implemented in ARRCON.  
    Other protocols or game-specific implementations however, will be considered.
- [Feature Requests](https://github.com/radj307/ARRCON/issues/new?assignees=&labels=enhancement%2C+new+feature+request&template=request-a-new-feature.md&title=%5BNEW%5D)
- [Suggestions](https://github.com/radj307/ARRCON/issues/new?assignees=&labels=&template=change-an-existing-feature.md&title=%5BCHANGE%5D+)
- [Documentation Suggestions or Additions](https://github.com/radj307/ARRCON/issues/new?assignees=&labels=documentation&template=documentation-request.md&title=%5BDOC%5D+)

## Pull Requests
Feel free to submit a pull request if you've added a feature or fixed a bug with the project!  
Contributions are always welcomed, I'll review it as soon as I can.
