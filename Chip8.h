#pragma once
#include <cstdint>
class Chip8 {
	uint8_t registers[16]{};
	uint8_t memory[4096]{};
	uint16_t index_register;
	uint16_t PC;
	uint16_t stack[16]{};
	uint8_t SP;
	uint8_t delay_timer;
	uint8_t sound_timer;
};