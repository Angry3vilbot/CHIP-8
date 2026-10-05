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
	masterTable[0x2] = &Chip8::CALL;
	masterTable[0x3] = &Chip8::SE;
	masterTable[0x4] = &Chip8::SNE;
	masterTable[0x5] = &Chip8::SE_Vy;
	masterTable[0x6] = &Chip8::LD_Vx;
	masterTable[0x7] = &Chip8::ADD_Vx;
	masterTable[0x8] = &Chip8::Table8;
	masterTable[0x9] = &Chip8::SNE_Vy;
	masterTable[0xA] = &Chip8::LD_I;
	masterTable[0xB] = &Chip8::JMP_V0;
	masterTable[0xC] = &Chip8::RND;
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
	table0[0xE] = &Chip8::RET;

	table8[0x0] = &Chip8::LD_Vx_Vy;
	table8[0x1] = &Chip8::OR_Vx_Vy;
	table8[0x2] = &Chip8::AND_Vx_Vy;
	table8[0x3] = &Chip8::XOR_Vx_Vy;
	table8[0x4] = &Chip8::ADD_Vx_Vy;
	table8[0x5] = &Chip8::SUB_Vx_Vy;
	table8[0x6] = &Chip8::SHR_Vx;
	table8[0x7] = &Chip8::SUBN_Vx_Vy;
	table8[0xE] = &Chip8::SHL_Vx;

	tableE[0x1] = &Chip8::SKNP;
	tableE[0xE] = &Chip8::SKP;

	for (size_t i = 0; i <= 0x65; i++)
	{
		tableF[i] = &Chip8::No_Op;
	}

	tableF[0x07] = &Chip8::LD_Vx_DT;
	tableF[0x0A] = &Chip8::LD_Vx_K;
	tableF[0x15] = &Chip8::LD_DT_Vx;
	tableF[0x18] = &Chip8::LD_ST_Vx;
	tableF[0x1E] = &Chip8::ADD_I_Vx;
	tableF[0x29] = &Chip8::LD_F_Vx;
	tableF[0x33] = &Chip8::LD_B_Vx;
	tableF[0x55] = &Chip8::LD_I_V0_Vx;
	tableF[0x65] = &Chip8::LD_V0_Vx_I;
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
	uint8_t registerIndex = (opcode & 0x0F00u) >> 8u;
	// Set the value of the register at the index to NN
	registers[registerIndex] = opcode & 0x00FFu;
}
// Add value NN to register Vx
void Chip8::ADD_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndex = (opcode & 0x0F00u) >> 8u;
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
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the index of the register (y)
	uint8_t registerIndexY = (opcode & 0x00F0u) >> 4u;
	// Get the X and Y coordinates of the sprite from Vx and Vy
	// Modulo 64 in order to allow the starting position's value to wrap around the screen
	uint8_t xCoord = registers[registerIndexX] % 64;
	uint8_t yCoord = registers[registerIndexY] % 32;
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
// Call subroutine at NNN
void Chip8::CALL() {
	// Increment the stack pointer
	SP++;
	// Put the current program counter onto the stack
	stack[SP] = PC;
	// Set the program counter to the subroutine's address (NNN)
	PC = opcode & 0x0FFFu;
}
// Return to the last call site on the stack
void Chip8::RET() {
	// Set the program counter to the address in the stack at the stack pointer
	PC = stack[SP];
	// Decrement the stack pointer
	SP--;
}
// Skip the next instruction if the key with the value of Vx is pressed
void Chip8::SKP() {
	// Extract the index of the register (x)
	uint8_t registerIndex = (opcode & 0x0F00u) >> 8u;
	// Extract the stored value of the register
	uint8_t keyValue = registers[registerIndex];
	// Check if the key is pressed
	if (keypad[keyValue] == 1) {
		// Skip the next instruction by incrementing the PC by 2
		PC += 2;
	}
}
// Skip the next instruction if the key with the value of Vx is NOT pressed
void Chip8::SKNP() {
	// Extract the index of the register (x)
	uint8_t registerIndex = (opcode & 0x0F00u) >> 8u;
	// Extract the stored value of the register
	uint8_t keyValue = registers[registerIndex];
	// Check if the key stored in Vx is NOT pressed
	if (keypad[keyValue] != 1) {
		// Skip the next instruction by incrementing the PC by 2
		PC += 2;
	}
}
// Skip the next instruction if the value of Vx equals NN
void Chip8::SE() {
	// Extract the index of the register (x)
	uint8_t registerIndex = (opcode & 0x0F00u) >> 8u;
	// Extract the stored value of the register
	uint8_t value = registers[registerIndex];
	// Extract NN
	uint8_t checkVal = opcode & 0x00FFu;
	// Check if the value stored in Vx is equal to NN
	if (value == checkVal) {
		// Skip the next instruction by incrementing the PC by 2
		PC += 2;
	}
}
// Skip the next instruction if the value of Vx does NOT equal NN
void Chip8::SNE() {
	// Extract the index of the register (x)
	uint8_t registerIndex = (opcode & 0x0F00u) >> 8u;
	// Extract the stored value of the register
	uint8_t value = registers[registerIndex];
	// Extract NN
	uint8_t checkVal = opcode & 0x00FFu;
	// Check if the value stored in Vx does NOT equal NN
	if (value != checkVal) {
		// Skip the next instruction by incrementing the PC by 2
		PC += 2;
	}
}
// Skip the next instruction if the value of Vx equals the value of Vy
void Chip8::SE_Vy() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the index of the register (y)
	uint8_t registerIndexY = (opcode & 0x00F0u) >> 4u;
	// Extract the stored value of the register (x)
	uint8_t valueX = registers[registerIndexX];
	// Extract the stored value of the register (y)
	uint8_t valueY = registers[registerIndexY];
	// Check if the value stored in Vx is equal to the value of Vy
	if (valueX == valueY) {
		// Skip the next instruction by incrementing the PC by 2
		PC += 2;
	}
}
// Skip the next instruction if the value of Vx does NOT equal the value of Vy
void Chip8::SNE_Vy() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the index of the register (y)
	uint8_t registerIndexY = (opcode & 0x00F0u) >> 4u;
	// Extract the stored value of the register (x)
	uint8_t valueX = registers[registerIndexX];
	// Extract the stored value of the register (y)
	uint8_t valueY = registers[registerIndexY];
	// Check if the value stored in Vx does NOT equal the value of Vy
	if (valueX != valueY) {
		// Skip the next instruction by incrementing the PC by 2
		PC += 2;
	}
}
// Jump to the memory address NNN plus the offset in V0
void Chip8::JMP_V0() {
	// Extract the address NNN
	uint16_t baseAddress = opcode & 0x0FFFu;
	// Extract the offset from V0
	uint8_t offset = registers[0x0];
	// Calculate the final memory address
	uint16_t address = baseAddress + offset;
	// Set the program counter to the address
	PC = address;
}
// Get a random byte, AND it with NN and put the result in Vx
void Chip8::RND() {
	// Extract the index of the register (x)
	uint8_t registerIndex = (opcode & 0x0F00u) >> 8u;
	// Extract NN
	uint8_t byte = opcode & 0x00FFu;
	// Calculate and store the result in the register (x)
	registers[registerIndex] = randomBytes(generator) & byte;
}
// Stores the value of the register Vy in the register Vx
void Chip8::LD_Vx_Vy() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the index of the register (y)
	uint8_t registerIndexY = (opcode & 0x00F0u) >> 4u;
	// Extract the stored value of the register (y)
	uint8_t valueY = registers[registerIndexY];
	// Store the value of the register (y) in the register (x)
	registers[registerIndexX] = valueY;
}
// ORs the values of Vx and Vy and stores the result in Vx
void Chip8::OR_Vx_Vy() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the index of the register (y)
	uint8_t registerIndexY = (opcode & 0x00F0u) >> 4u;
	// Extract the stored value of the register (x)
	uint8_t valueX = registers[registerIndexX];
	// Extract the stored value of the register (y)
	uint8_t valueY = registers[registerIndexY];
	// Store the result of the OR operation in the register (x)
	registers[registerIndexX] = valueX | valueY;
}
// ANDs the values of Vx and Vy and stores the result in Vx
void Chip8::AND_Vx_Vy() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the index of the register (y)
	uint8_t registerIndexY = (opcode & 0x00F0u) >> 4u;
	// Extract the stored value of the register (x)
	uint8_t valueX = registers[registerIndexX];
	// Extract the stored value of the register (y)
	uint8_t valueY = registers[registerIndexY];
	// Store the result of the OR operation in the register (x)
	registers[registerIndexX] = valueX & valueY;
}
// XORs the values of Vx and Vy and stores the result in Vx
void Chip8::XOR_Vx_Vy() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the index of the register (y)
	uint8_t registerIndexY = (opcode & 0x00F0u) >> 4u;
	// Extract the stored value of the register (x)
	uint8_t valueX = registers[registerIndexX];
	// Extract the stored value of the register (y)
	uint8_t valueY = registers[registerIndexY];
	// Store the result of the OR operation in the register (x)
	registers[registerIndexX] = valueX ^ valueY;
}
// Adds the values of Vx and Vy together and stores the result in Vx. 
// If the result is larger than 8 bits, only the lowest 8 bits are kept and VF is set to 1
void Chip8::ADD_Vx_Vy() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the index of the register (y)
	uint8_t registerIndexY = (opcode & 0x00F0u) >> 4u;
	// Extract the stored value of the register (x)
	uint8_t valueX = registers[registerIndexX];
	// Extract the stored value of the register (y)
	uint8_t valueY = registers[registerIndexY];
	// Calculate the sum
	uint16_t sum = valueX + valueY;
	// If the sum is greater than 255, the value overflowed
	registers[0xF] = (sum > 255) ? 1 : 0;
	// Store the lowest 8 bits of the sum in the register (x)
	registers[registerIndexX] = sum & 0xFFu;
}
// Subtracts the value of Vy from Vx and stores the result in Vx. Set VF to 1 if we didn't have to borrow (i.e. if Vx >= Vy)
void Chip8::SUB_Vx_Vy() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the index of the register (y)
	uint8_t registerIndexY = (opcode & 0x00F0u) >> 4u;
	// Extract the stored value of the register (x)
	uint8_t valueX = registers[registerIndexX];
	// Extract the stored value of the register (y)
	uint8_t valueY = registers[registerIndexY];
	// Calculate the result
	uint8_t result = valueX - valueY;
	// Store the result in the register (x)
	registers[registerIndexX] = result;
	// If we did NOT have to borrow from the next higher order bit (i.e. we did NOT underflow), set the VF flag to 1
	registers[0xF] = (valueX >= valueY) ? 1 : 0;
}
// Shift the value in Vx to the right by 1, set VF to 1 if the LSB of Vx is 1, otherwise 0
void Chip8::SHR_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the stored value of the register (x)
	uint8_t valueX = registers[registerIndexX];
	// If the LSB of the value was 1, set VF to 1
	registers[0xF] = valueX & 0x01u;
	// Store the result in register (x)
	registers[registerIndexX] = valueX / 2;
}
// Same as SUB_Vx_Vy, but subtracts Vx from Vy
void Chip8::SUBN_Vx_Vy() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the index of the register (y)
	uint8_t registerIndexY = (opcode & 0x00F0u) >> 4u;
	// Extract the stored value of the register (x)
	uint8_t valueX = registers[registerIndexX];
	// Extract the stored value of the register (y)
	uint8_t valueY = registers[registerIndexY];
	// Calculate the result
	uint8_t result = valueY - valueX;
	// Store the result in the register (x)
	registers[registerIndexX] = result;
	// If we did NOT have to borrow from the next higher order bit (i.e. we did NOT underflow), set the VF flag to 1
	registers[0xF] = (valueY >= valueX) ? 1 : 0;
}
// Shift the value of Vx to the left by 1. If the MSB is 1, set the VF to 1, otherwise set it to 0
void Chip8::SHL_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the stored value of the register (x)
	uint8_t valueX = registers[registerIndexX];
	// If the MSB of the value was 1, set VF to 1
	registers[0xF] = (valueX & 0x80u) ? 1 : 0;
	// Store the result in register (x)
	registers[registerIndexX] = valueX * 2;
}
// Set Vx to the value of the Delay Timer
void Chip8::LD_Vx_DT() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Set the value of the register (x) to the value of the Delay Timer
	registers[registerIndexX] = delay_timer;
}
// Stop all execution until a key is pressed, then store the value of the key in Vx 
void Chip8::LD_Vx_K() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Set the value of the register (x) to the value of the pressed key (if there is one pressed)
	if (keypad[0]) registers[registerIndexX] = 0;
	else if (keypad[1]) registers[registerIndexX] = 1;
	else if (keypad[2]) registers[registerIndexX] = 2;
	else if (keypad[3]) registers[registerIndexX] = 3;
	else if (keypad[4]) registers[registerIndexX] = 4;
	else if (keypad[5]) registers[registerIndexX] = 5;
	else if (keypad[6]) registers[registerIndexX] = 6;
	else if (keypad[7]) registers[registerIndexX] = 7;
	else if (keypad[8]) registers[registerIndexX] = 8;
	else if (keypad[9]) registers[registerIndexX] = 9;
	else if (keypad[10]) registers[registerIndexX] = 10;
	else if (keypad[11]) registers[registerIndexX] = 11;
	else if (keypad[12]) registers[registerIndexX] = 12;
	else if (keypad[13]) registers[registerIndexX] = 13;
	else if (keypad[14]) registers[registerIndexX] = 14;
	else if (keypad[15]) registers[registerIndexX] = 15;
	// If no key is pressed, decrement the PC to repeat the instruction (equivalent to waiting for input)
	else PC -= 2;
}
// Set the Delay Timer to the value of Vx
void Chip8::LD_DT_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Set the value of the Delay Timer to the value of the register (x)
	delay_timer = registers[registerIndexX];
}
// Set the Sound Timer to the value of Vx
void Chip8::LD_ST_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Set the value of the Sound Timer to the value of the register (x)
	sound_timer = registers[registerIndexX];
}
// Add the value of Vx to the value of the Index Register
void Chip8::ADD_I_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the value of the register (x)
	uint8_t value = registers[registerIndexX];
	// Add the value of the register (x) to the value of the Index Register
	index_register += value;
}
// Set the value of the Index Register to the location of the font letter sprite for the digit Vx
void Chip8::LD_F_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the value of the register (x)
	uint8_t value = registers[registerIndexX];
	// Set the value of the Index Register
	index_register = FONTSET_START_ADDRESS + (value * 5);
}
// Store the Binary Coded Decimal representation of Vx in memory locations I, I+1 and I+2
void Chip8::LD_B_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndexX = (opcode & 0x0F00u) >> 8u;
	// Extract the value of the register (x)
	uint8_t value = registers[registerIndexX];
	// Place the number in. Hundreds first, ones last in memory.
	// Ones
	memory[index_register + 2] = value % 10;
	value /= 10;
	// Tens
	memory[index_register + 1] = value % 10;
	value /= 10;
	// Hundreds
	memory[index_register] = value % 10;
}
// Store the values of registers V0 through Vx in memory starting at location I.
void Chip8::LD_I_V0_Vx() {
	// Extract the index of the register (x)
	uint8_t registerIndex = (opcode & 0x0F00u) >> 8u;
	// Loop through the registers from V0 to Vx
	for (unsigned int i = 0; i <= registerIndex; i++) {
		// Extract the value of the register (i)
		uint8_t value = registers[i];
		// Store the value at I+i
		memory[index_register + i] = value;
	}
}
// Load values into registers V0 through Vx from memory starting at location I
void Chip8::LD_V0_Vx_I() {
	// Extract the index of the register (x)
	uint8_t registerIndex = (opcode & 0x0F00u) >> 8u;
	// Loop through the registers from V0 to Vx
	for (unsigned int i = 0; i <= registerIndex; i++) {
		// Extract the value in memory at I+i
		uint8_t value = memory[index_register + i];
		// Store the value in the register (i)
		registers[i] = value;
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
	std::cerr << "Warning. Invalid/Unused Opcode: " << std::hex << opcode << std::endl;
}

//#endregion