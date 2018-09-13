#pragma once

#include <array>
#include <string>
#include <random>
#include <functional>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

struct chip_register_bank {
	uint8_t V[16];
	uint8_t SoundTimer;
	uint8_t DelayTimer;

	uint16_t I;
	uint16_t PC;
	uint16_t SP;
};

class Cpu {
public:
	Cpu();
	Cpu(const std::string& RomfilePath);

	~Cpu();

	void Start();
	bool LoadRom(const std::string& RomfilePath);
private:
	typedef void (Cpu::*OpcodeTable)();

	std::array<uint8_t, 0x1000> Memory;
	std::array<uint8_t, 16> Keyboard;
	std::array<uint8_t, 32 * 64> Framebuffer;
	
	std::array<uint16_t, 16> Stack;

	std::random_device rd;
	std::uniform_int_distribution<int> distribution{ 0, 255 };

	std::string RomPath;

	chip_register_bank RegisterBank;

	const uint8_t Font[80] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, //0
		0x20, 0x60, 0x20, 0x20, 0x70, //1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
		0x90, 0x90, 0xF0, 0x10, 0x10, //4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
		0xF0, 0x10, 0x20, 0x40, 0x40, //7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
		0xF0, 0x90, 0xF0, 0x90, 0x90, //A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
		0xF0, 0x80, 0x80, 0x80, 0xF0, //C
		0xE0, 0x90, 0x90, 0x90, 0xE0, //D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
		0xF0, 0x80, 0xF0, 0x80, 0x80  //F
	};

	uint16_t Opcode;

	const int height = 32;
	const int width = 64;

	OpcodeTable OpTable[0x10];

	void Exec();
	void Reset();

	void Op0xxx();
	void Op1xxx();
	void Op2xxx();
	void Op3xxx();
	void Op4xxx();
	void Op5xxx();
	void Op6xxx();
	void Op7xxx();
	void Op8xxx();
	void Op9xxx();
	void OpAxxx();
	void OpBxxx();
	void OpCxxx();
	void OpDxxx();
	void OpExxx();
	void OpFxxx();

	sf::RenderWindow *window;

	sf::Sprite frame;
	sf::Texture frameTexture;

	sf::Sound Tone;
	sf::SoundBuffer ToneBuffer;
};