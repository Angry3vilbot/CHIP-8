#include "Chip8.h"
#include <fstream>
#include <chrono>
#include <iostream>

const unsigned int FONTSET_SIZE = 80;
uint8_t fontset[FONTSET_SIZE] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x30, 0x50, 0x10, 0x10, 0x10, // 1
	0x60, 0x90, 0x10, 0x20, 0xF0, // 2
	0xF0, 0x10, 0x20, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xE0, 0x10, 0xE0, // 5
	0xE0, 0x80, 0xE0, 0x90, 0xE0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x80, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0x70, 0x90, 0x70, 0x10, 0x70, // 9
	0x60, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8() : generator(std::chrono::system_clock::now().time_since_epoch().count())
{
	// Init the program counter
	PC = STARTING_ADDRESS;
	// Load the fontset into memory
	for (int i = 0; i < FONTSET_SIZE; i++) {
		memory[FONTSET_START_ADDRESS + i] = fontset[i];
	}
	// Init the RNG values
	randomBytes = std::uniform_int_distribution<unsigned int>(0, 255);
	// Set up the function pointer table
	masterTable[0x0] = &Chip8::Table0;
	masterTable[0x1] = &Chip8::JMP;
	//masterTable[0x2] = &Chip8::CALL;
	//masterTable[0x3] = &Chip8::SE;
	//masterTable[0x4] = &Chip8::SNE;
	//masterTable[0x5] = &Chip8::SE_Vy;
	masterTable[0x6] = &Chip8::LD_Vx;
	masterTable[0x7] = &Chip8::ADD_Vx;
	masterTable[0x8] = &Chip8::Table8;
	//masterTable[0x9] = &Chip8::SNE_Vy;
	masterTable[0xA] = &Chip8::LD_I;
	//masterTable[0xB] = &Chip8::JMP_V0;
	//masterTable[0xC] = &Chip8::RND;
	masterTable[0xD] = &Chip8::DRW;
	masterTable[0xE] = &Chip8::TableE;
	masterTable[0xF] = &Chip8::TableF;
	// Set up the auxiliary tables
	for (size_t i = 0; i <= 0xE; i++)
	{
		table0[i] = &Chip8::No_Op;
		table8[i] = &Chip8::No_Op;
		tableE[i] = &Chip8::No_Op;
	}

	table0[0x0] = &Chip8::CLS;
	//table0[0xE] = &Chip8::RET;

	//table8[0x0] = &Chip8::LD_Vx_Vy;
	//table8[0x1] = &Chip8::OR_Vx_Vy;
	//table8[0x2] = &Chip8::AND_Vx_Vy;
	//table8[0x3] = &Chip8::XOR_Vx_Vy;
	//table8[0x4] = &Chip8::ADD_Vx_Vy;
	//table8[0x5] = &Chip8::SUB_Vx_Vy;
	//table8[0x6] = &Chip8::SHR_Vx;
	//table8[0x7] = &Chip8::SUBN_Vx_Vy;
	//table8[0xE] = &Chip8::SHL_Vx;

	//tableE[0x1] = &Chip8::SKNP
	//tableE[0xE] = &Chip8::SKP

	for (size_t i = 0; i <= 0x65; i++)
	{
		tableF[i] = &Chip8::No_Op;
	}

	//tableF[0x07] = &Chip8::LD_Vx_DT;
	//tableF[0x0A] = &Chip8::LD_Vx_K;
	//tableF[0x15] = &Chip8::LD_DT_Vx;
	//tableF[0x18] = &Chip8::LD_ST_Vx;
	//tableF[0x1E] = &Chip8::ADD_I_Vx;
	//tableF[0x29] = &Chip8::LD_F_Vx;
	//tableF[0x33] = &Chip8::LD_B_Vx;
	//tableF[0x55] = &Chip8::LD_I_V0_Vx;
	//tableF[0x65] = &Chip8::LD_V0_Vx_I;
}

void Chip8::LoadROM(char const* filename) {
	// Open the file as a binary stream and move the file pointer to the end
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

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
//#region CPU Cycle

// Run one CPU Cycle
void Chip8::DoCycle() {
	Fetch();
	DecodeAndExecute();
}
// Fetch the opcode from memory
void Chip8::Fetch() {
	// Read two bytes from memory at the address stored in the Program Counter
	uint8_t byteOne = memory[PC];
	uint8_t byteTwo = memory[PC + 1];
	opcode = (byteOne << 8) | byteTwo;
	// Increment the PC before execution
	PC += 2;
}
// Decode and execute the instruction based on the fetched opcode
void Chip8::DecodeAndExecute() {
	// Decode the instruction using the function table
	// Index the function in the table by masking all but the first hexadecimal number in the opcode and shifting said number to the right
	// by 12 to remove the resulting trailing zeroes from the masking operation.
	((*this).*(masterTable[(opcode & 0xF000u) >> 12u]))();
	// If there is a delay timer set, decrement it
	if (delay_timer > 0) {
		delay_timer--;
	}
	// If there is a sound timer set, decrement it
	if (sound_timer > 0) {
		sound_timer--;
	}
}

//#endregion
//#region Instructions (Opcodes)

// Clear the screen
void Chip8::CLS() {
	// Fill the display memory with 0s
	std::fill(std::begin(display_memory), std::end(display_memory), 0u);
}
// Jump to address NNN
void Chip8::JMP() {
	// Set the PC to the value of NNN by extracting the lower 12 bits of the opcode
	PC = opcode & 0x0FFFu;
}
// Set Vx to value NN
void Chip8::LD_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndex = opcode & 0x0F00u;
	// Set the value of the register at the index to NN
	registers[registerIndex] = opcode & 0x00FFu;
}
// Add value NN to register Vx
void Chip8::ADD_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndex = opcode & 0x0F00u;
	// Add the value NN to the value of the register at the index
	registers[registerIndex] += opcode & 0x00FFu;
}
// Set index register I to value NNN
void Chip8::LD_I() {
	// Set the value of the index register to NNN
	index_register = opcode & 0x0FFFu;
}
// Draw sprite from memory at location I to the screen at (Vx, Vy)
void Chip8::DRW() {
	// Get the X and Y coordinates of the sprite from Vx and Vy
	// Modulo 64 in order to allow the starting position's value to wrap around the screen
	uint8_t xCoord = registers[opcode & 0x0F00u] % 64;
	uint8_t yCoord = registers[opcode & 0x00F0u] % 32;
	// Reset the VF register to 0
	registers[0xF] = 0;
	// Starting at the address from the index register, draw N bytes (rows) of the sprite
	uint8_t byteCount = opcode & 0x000Fu;
	for (uint8_t row = 0; row < byteCount; row++) {
		uint8_t data = memory[index_register + row];
		// Set the pixel in the display memory for each of the 8 bits (columns) in the data
		for (uint8_t col = 0; col < 8; col++) {
			// Get the sprite's pixel by masking the data with the binary 10000000 shifted to the index of the column
			// Bit n = Column n
			uint8_t spritePixel = data & (0x80 >> col);
			// Get the screen's pixel at X,Y
			uint32_t* screenPixel = &display_memory[(row + yCoord) * 64 + (col + xCoord)];
			
			if (spritePixel) {
				if (*screenPixel == 0xFFFFFFFF) {
					// If both are on, set the VF flag register to 1
					registers[0xF] = 1u;
				}
				// If the sprite pixel is turned on, XOR the screen pixel with the high value
				*screenPixel = *screenPixel ^ 0xFFFFFFFF;
			}
			// If the edge of the screen is reached, stop drawing (sprites do not wrap around the edge, only the coordinates do)
			if (col + xCoord == 63u) break;
		}
		// If the edge of the screen is reached, stop drawing (sprites do not wrap around the edge, only the coordinates do)
		if (row + yCoord == 31u) break;
	}
}

//#endregion
//#region Table indexing functions

void Chip8::Table0() {
	((*this).*(table0[opcode & 0x000Fu]))();
}
void Chip8::Table8() {
	((*this).*(table8[opcode & 0x000Fu]))();
}
void Chip8::TableE() {
	((*this).*(tableE[opcode & 0x000Fu]))();
}
void Chip8::TableF() {
	((*this).*(tableF[opcode & 0x00FFu]))();
}
void Chip8::No_Op() {
	std::cerr << "Warning. Invalid/Unused Opcode: " + opcode << std::endl;
}

//#endregion