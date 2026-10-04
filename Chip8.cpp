#include "Chip8.h"
#include <fstream>
Chip8::Chip8()
{
	// Init the program counter
	PC = STARTING_ADDRESS;
}

void Chip8::LoadROM(char const* filename) {
	// Open the file as a binary stream and move the file pointer to the end
	std::ifstream file(filename, std::ios::binary, std::ios::ate);

	if (file.is_open()) {
		// Get the size of the file
		std::streampos size = file.tellg();
		// Allocate a buffer to hold the contents of the file
		char* buffer = new char[size];

		// Go back to the beginning of the file and fill the buffer with the contents
		file.seekg(0, std::ios::beg);
		file.read(buffer, size);
		file.close();

		// Load the ROM contents into the Chip8's memory, starting at 0x200
		for (long i = 0; i < size; i++)
		{
			#pragma warning(suppress: 6385)
			memory[STARTING_ADDRESS + i] = buffer[i];
		}

		// Free the buffer
		delete[] buffer;
	}
}