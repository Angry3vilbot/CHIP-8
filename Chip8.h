#pragma once
#include <cstdint>
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

		Chip8();
		void LoadROM(char const* filename);
};