#pragma once
#include <cstdint>
#include <random>
class Chip8 {
	public:
		uint8_t registers[16]{};
		uint8_t memory[4096]{};
		uint16_t index_register{};
		uint16_t PC{};
		uint16_t stack[16]{};
		uint8_t SP{};
		uint8_t delay_timer{};
		uint8_t sound_timer{};
		uint8_t keypad[16]{};
		uint32_t display_memory[64 * 32]{};
		uint16_t opcode;

		const unsigned int STARTING_ADDRESS = 0x200;
		const unsigned int FONTSET_START_ADDRESS = 0x50;

		// Random Number Generator
		std::default_random_engine generator;
		std::uniform_int_distribution<unsigned int> randomBytes;
		// Function table for instructions indexed by opcode. 1 is added to the highest hex value required by the group of opcodes
		// when creating the table to be able to index the group by said hex value.
		typedef void (Chip8::*Chip8Func)();
		// The master table contains function pointers to opcodes based on the first hex number of the opcode.
		// If the hex number has multiple opcodes that start with it, it instead indexes to a function that calls the opcode from its own table.
		Chip8Func masterTable[0xF + 1];
		// Table 0 contains functions for opcodes that start with 0
		Chip8Func table0[0xE + 1];
		// Table 8 contains functions for opcodes that start with 8
		Chip8Func table8[0xE + 1];
		// Table E contains functions for opcodes that start with E
		Chip8Func tableE[0xE + 1];
		// Table F contains functions for opcodes that start with F
		Chip8Func tableF[0x65 + 1];

		// Table indexing functions
		void Table0();
		void Table8();
		void TableE();
		void TableF();
		// Used when the opcode is invalid / not needed
		void No_Op();

		Chip8();
		void LoadROM(char const* filename);
		// CPU Cycle (Fetch-Decode-Execute)
		void DoCycle();
		void Fetch();
		void DecodeAndExecute();
		// Instructions
		void CLS();
		void JMP();
		void LD_Vx();
		void ADD_Vx();
		void LD_I();
		void DRW();
		void CALL();
		void RET();
		void SKP();
		void SKNP();
		void SE();
		void SNE();
		void SE_Vy();
		void SNE_Vy();
		void JMP_V0();
		void RND();
};