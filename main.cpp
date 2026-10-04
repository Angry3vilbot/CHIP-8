#include <chrono>
#include <iostream>
#include <string>
#include "SDLRenderer.cpp"
#include "Chip8.h"

int main(int argc, char** argv) {
	// All 4 arguments are required
	if (argc != 4) {
		std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
		std::exit(EXIT_FAILURE);
	}
	// Factor to which to scale the video output, since the size of the video buffer of a CHIP-8 is only 64x32 pixels
	int videoScale = std::stoi(argv[1]);
	// Delay between each CPU cycle, to make games actually playable
	int cycleDelay = std::stoi(argv[2]);
	// The file name of the ROM
	char const* rom = argv[3];
	// SDL Context that handles creating a window, rendering the graphics to the screen and reading keyboard input
	SDLRenderer sdl("CHIP-8", 64 * videoScale, 32 * videoScale, 64, 32);
	// Load the ROM into the emulator
	Chip8 chip8;
	chip8.LoadROM(rom);
	// The number of bytes in a row of pixels
	int videoPitch = sizeof(chip8.display_memory[0]) * 64;
	// The timestamp of the last CPU cycle
	auto lastCycleTime = std::chrono::high_resolution_clock::now();
	bool quit = false;

	while (!quit) {
		quit = sdl.ProcessInput(chip8.keypad);

		auto currentTime = std::chrono::high_resolution_clock::now();
		// Time difference between the current time and the time of the last CPU cycle
		float timeDifference = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();

		if (timeDifference > cycleDelay) {
			lastCycleTime = currentTime;
			chip8.DoCycle();
			sdl.Update(chip8.display_memory, videoPitch);
		}
	}

	return 0;
}