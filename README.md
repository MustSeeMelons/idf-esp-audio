# ESP Audio

## High-level overview

We could re-implement it all, but I think a good middle ground would be:
- Scan SD card for songs
- Play a song, uncomprossed wav
- We can pause songs
- We can skip songs

If that goes swimingly and we have the time: implement the game as well, except yeet the startup do nothing state.

We could read-up on mp3 decompression, but that seems a bit much, but lets see.

## TODO

A list of small steps to achieve for the larget goal! Feel free to add new items as we go.

- Initialize SPI master for reading data from the SD card
- Figure out the structure of an SD card
- Get a file list from the SD card
- Read a file from the SD card
- Initialize I2S
- Figure out how I2S works and what we need to feed it audio wise
- Read audio data from SD and play it

## Build

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
