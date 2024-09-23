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

- [x] Initialize SPI master for reading data from the SD card
- [ ] Figure out the structure of an SD card
- [ ] Will have to read on FAT32
- [ ] Get a file list from the SD card
- [ ] Read a file from the SD card
- [ ] Initialize I2S
- [ ] Figure out how I2S works and what we need to feed it audio wise
- [ ] Read audio data from SD and play it

- What are the formats of wav? pcm_s16le,pcm_s24le,pcm_s32le. (Pulse Code Modulation) (Little Endian?)

### SD

Data is transfered in blocks of 512 bytes (or X bytes).

In SPI mode communication is done be sending commands and recieving responses. Most responses are 8 bit, with two exceptions when they are 40.

A command is 48 bits.

After a command is send, we need to toggle the clock 8 times.

CLK should be in the range of 100-400kHz.

The card must be set into SPI mode - MOSI/CS held high for atleast 74 cycles. Afterwards we send CMD0.

- [Flow PDF](https://www.dejazzer.com/ee379/lecture_notes/lec12_sd_card.pdf)
- [Extras from PDF](http://elm-chan.org/docs/mmc/mmc_e.html)
- [Overflow](https://electronics.stackexchange.com/questions/602105/how-can-i-initialize-use-sd-cards-with-spi)
- [FAT32](https://www.pjrc.com/tech/8051/ide/fat32.html)
- [Tutorial series](http://www.rjhcoding.com/avrc-sd-interface-1.php)

## Upload

To upload to the clone - power the board on while holding down boot & then flash the device. If that fails check device manager if the device is visible.

Don't be afraid of re-plugging the hub!

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
