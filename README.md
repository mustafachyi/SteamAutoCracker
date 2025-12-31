# SteamAutoCracker

A fast and simple tool to play Steam games without Steam. It automatically detects games, finds the right settings, and applies the necessary files to make them run. This tool is written in C++ to ensure it is lightweight and quick.

## Overview

SteamAutoCracker helps you "crack" your legally owned Steam games so they can run independently. It handles the complicated parts for you, like finding the Game ID, getting the list of DLCs, and unpacking protected files. It comes with a simple windowed interface that supports dark mode.

## Features

*   **Easy Detection**: Automatically finds the Game ID from your game folder.
*   **Game Info**: Uses a custom API to get the correct game name and DLC list.
*   **Steamless Support**: Can automatically use Steamless to unpack protected game files (`.exe`).
*   **Multiple Options**: Includes different emulators like Ali213, Goldberg, and CreamAPI.
*   **Safe**: Creates backups of your files so you can easily undo changes.
*   **Fast**: Built with C++ for high performance on any computer.

## Requirements

*   **OS**: Windows 10 or 11 (64-bit)
*   **Build Tools**: CMake 3.20+ and a C++20 compiler (like MSVC)

## How to Build

1.  Download the code:
    ```bash
    git clone https://github.com/mustafachyi/SteamAutoCracker.git
    cd SteamAutoCracker
    ```

2.  Prepare the build:
    ```bash
    mkdir build
    cd build
    cmake ..
    ```

3.  Compile:
    ```bash
    cmake --build . --config Release
    ```

## Setup

For the tool to work, it needs a `tools` folder next to the executable. This folder should contain the emulators and Steamless.

Structure:
```text
SteamAutoCracker.exe
tools/
├── Steamless/
│   └── Steamless.CLI.exe
└── sac_emu/
    ├── game_ali213/
    ├── game_goldberg/
    └── dlc_creamapi/
```

## How to Use

1.  Open **SteamAutoCracker**.
2.  Click **Change** to select your game folder.
3.  The tool will try to find the Game ID. If it fails, type the game name in the search box and click **Search**.
4.  Select an emulator from the list (e.g., Goldberg).
5.  Click **Crack**. The tool will:
    *   Backup your original files.
    *   Unpack the game file if needed.
    *   Copy the emulator files.
    *   Create the necessary config files.
6.  If you want to go back to the original files, just click **Restore**.

## Credits

*   **Inspiration & Emulators**: This project was inspired by the Python version created by [BigBoiCJ](https://github.com/BigBoiCJ/SteamAutoCracker). The emulator files included in this tool were taken from that repository.
*   **API**: Game data is fetched using a custom API built specifically for this tool: [mustafachyi/steam](https://github.com/mustafachyi/steam).
*   **Libraries**: 
    *   [Steamless](https://github.com/atom0s/Steamless) for unpacking files.

## Disclaimer

This tool is for educational purposes and for preserving software you already own. Please do not use it for piracy. Support developers by buying the games you play.
