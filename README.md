<p align="center">
  <img
       src="https://i.imgur.com/yARb4DV.png"
       alt="ARRCON Banner"
  />
</p>  
<p align="center">
A lightweight cross-platform RCON client compatible with <b>any game using the Source RCON Protocol</b>.<br/>
<p align="center">
  <a href="https://github.com/radj307/ARRCON/releases"><img alt="GitHub release (latest by date)" src="https://img.shields.io/github/v/release/radj307/ARRCON?label=Latest+Version&style=flat"></a>
  <nobr/>
  <a href="https://github.com/awesome-selfhosted/awesome-selfhosted#games---administrative-utilities--control-panels"><img alt="Mentioned in Awesome-Selfhosted" src="https://awesome.re/mentioned-badge.svg"></a>
  <nobr/>
  <a href="https://github.com/radj307/ARRCON/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/radj307/ARRCON/total?label=Downloads&style=flat"></a>
</p>
<p align="center">
  <a href="https://github.com/radj307/ARRCON/actions/workflows/Windows.yml"><img alt="Windows Status" src="https://img.shields.io/github/workflow/status/radj307/ARRCON/Windows%20Smoketest?label=Windows&logo=github&style=flat"></a>
  <a href="https://github.com/radj307/ARRCON/actions/workflows/Linux.yml"><img alt="Linux Status" src="https://img.shields.io/github/workflow/status/radj307/ARRCON/Linux%20Smoketest?label=Linux&logo=github&style=flat"></a>
  <a href="https://github.com/radj307/ARRCON/actions/workflows/Windows.yml"><img alt="macOS Status" src="https://img.shields.io/github/workflow/status/radj307/ARRCON/macOS%20Smoketest?label=macOS&logo=github&style=flat"></a>
</p>
<p align="center">
  <a href="https://github.com/radj307/ARRCON/releases">Releases</a>&nbsp&nbsp|&nbsp&nbsp<a href="https://github.com/radj307/ARRCON/wiki">Wiki</a>&nbsp&nbsp|&nbsp&nbsp<a href="https://github.com/radj307/ARRCON/issues">Issues</a>
</p>


# Features
  - Highly configurable
  - **Cross-Platform:**
    - Windows
    - Linux
    - macOS
  - **Works for any game using the [Source RCON Protocol](https://developer.valvesoftware.com/wiki/Source_RCON_Protocol)**
  - **Handles large packets without issue**
  - **Handles multi-packet responses without issue**
  - **Supports Minecraft Bukkit's colorized text**
  - You can set delays in the INI file or directly on the commandline
  - Supports saving a server's connection info so you can connect to it with 1 word  
    If you've ever used `ssh`'s `config` file, this will be very familiar. *(albeit with more sensible syntax)*
    - This can be done in a text editor **or entirely from the commandline**
  - Can be used as a one-off from the commandline, or in an interactive console
    - Supports piped input using shell operators.  
      For example; `echo "help" | ARRCON -S myServer` would send the `help` command to the `myServer` host
      - Piped commands are sent _after_ any commands explicitly specified on the commandline
  - You can write scripts and manually execute them with the `-f`/`--file` options in addition to shell scripts
    - Commands are separated by newlines
    - Commands from script files are sent _after_ any piped commands
    - Line comments can be written using semicolons `;` or pound signs '#'
  - Shows an indicator when the server didn't respond to your command
    

# Installation
Get the latest version for your OS from the [releases](https://github.com/radj307/ARRCON/releases) page.

There is no installation process, simply extract the archive to a location of your choice, then run it using a terminal emulator.  
If you want to be able to run ARRCON from any working directory without specifying its location, you must [add the location to your environment's PATH variable](https://github.com/radj307/ARRCON/wiki/Adding-To-Path).

## Microsoft's Infinite Wisdom
On newer versions of Windows, you may be required to "unblock" the executable before Windows will let you use it.  
This is because the executable isn't signed with a Microsoft-approved signing certificate, which costs upwards of [$300/year](https://docs.microsoft.com/en-us/windows-hardware/drivers/dashboard/get-a-code-signing-certificate#step-2-buy-a-new-code-signing-certificate).  
To unblock it, ___Right-Click___ on `ARRCON.exe` in the file explorer and click ___Properties___ at the bottom of the right-click menu.  
![](https://i.imgur.com/LKLZPVX.png)  
Check the ___unblock___ box, then click ___Apply___.  


## Building from Source
See [here](https://github.com/radj307/ARRCON/wiki/Building-from-Source) for a brief guide on building ARRCON from source.


# Usage
ARRCON is a CLI _(Command-Line Interface)_ program, which means you need to run it through a terminal.  

__On Windows, you can use `cmd.exe` or `powershell.exe` by R+Clicking on the start menu and selecting "Command Prompt" or "PowerShell".__  

For more detailed usage instructions, see the [Getting Started](https://github.com/radj307/ARRCON/wiki) page on the wiki.

To see a list of commands, use `ARRCON -h` or `ARRCON --help`  
To see a list of environment variables, their current values, and a description of each, use `ARRCON --print-env`


## Modes
- ___Interactive Shell___  
  ![](https://i.imgur.com/4d4Epkb.png)  
  Opens an interactive console session. You can send commands and view the responses in real-time.
  - Used by default when there are no command arguments.
  - Connection remains open until you disconnect or kill the process, or if the server closes.
- ___One-Shot___  
  ![ARRCON Scripting Support](https://i.imgur.com/oPX47RD.png)  
  This mode is designed for scripting, it sends commands directly from the commandline in sequential order before exiting.  
  _(You can also open an interactive shell at the same time with the `-i` / `--interactive` options.)_
  
  Supported input methods:
    - Commandline Parameters  
      _These are any arguments that are __not__ short/long-opts and __not captured by__ short/long-opts._
    - Shell Scripts
    - Redirected input from STDIN
    - Script Files  
      Splits commands by line, and allows comments using a semicolon `;` or pound sign `#`.   
      Comments are always considered line comments.  
      _Use the '`-f`' or '`--file`' options to specify a scriptfile to load._

# Contributing

If you want to add a new feature, fix a bug, or just improve something that annoys you, feel free to submit pull requests and/or issues.

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
Contributions are always welcomed, I'll review it as soon as I see the notification.
