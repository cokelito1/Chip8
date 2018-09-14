#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cstdarg>

#include <imgui.h>
#include <imgui-SFML.h>

#include "cpu.h"

static void dbg_printf(const char *msg, ...) {
#ifdef DEBUG
	va_list vl;
	va_start(vl, msg);
	vprintf(msg, vl);
	va_end(vl);
#endif
}

Cpu::Cpu() {
	for (unsigned int i = 0; i < 16; i++) {
		Stack[i] = 0x00;
		Keyboard[i] = 0x00;
		RegisterBank.V[i] = 0x00;
	}

	for (unsigned int i = 0; i < (32 * 64); i++) {
		Framebuffer[i] = 0;
	}

	for (unsigned int i = 0; i < Memory.size(); i++) {
		Memory[i] = 0x00;
	}

	for (unsigned int i = 0; i < 80; i++) {
		Memory[i] = Font[i];
	}

	RegisterBank.I = 0x00;
	RegisterBank.PC = 0x200;
	RegisterBank.SP = 0x00;

	ImguiGUI = false;
	Paused = false;
	Step = false;

	window = new sf::RenderWindow(sf::VideoMode(800, 600), "Chip-8");
	sf::View view = window->getView();
	view.setCenter(width / 2, height / 2);

	ImGui::SFML::Init(*window);

	view.setSize(sf::Vector2f(width, height));
	window->setView(view);
	window->setFramerateLimit(250);


	frameTexture.create(width, height);
	frame.setTexture(frameTexture);

	sf::Int16 raw[44100];
	if (!ToneBuffer.loadFromSamples(raw, 44100, 1, 44100)) {
		std::cerr << "Error al cargar samples" << std::endl;
		exit(0);
	}
	Tone.setBuffer(ToneBuffer);
	Tone.setLoop(true);

	std::cout << "Window " << window->getSize().x << " " << window->getSize().y << std::endl;
	std::cout << "View " << view.getSize().x << " " << view.getSize().y << std::endl;
	std::cout << "Window View " << window->getView().getSize().x << " " << window->getView().getSize().y << std::endl;

	OpTable[0x00] = &Cpu::Op0xxx;
	OpTable[0x01] = &Cpu::Op1xxx;
	OpTable[0x02] = &Cpu::Op2xxx;
	OpTable[0x03] = &Cpu::Op3xxx;
	OpTable[0x04] = &Cpu::Op4xxx;
	OpTable[0x05] = &Cpu::Op5xxx;
	OpTable[0x06] = &Cpu::Op6xxx;
	OpTable[0x07] = &Cpu::Op7xxx;
	OpTable[0x08] = &Cpu::Op8xxx;
	OpTable[0x09] = &Cpu::Op9xxx;
	OpTable[0x0A] = &Cpu::OpAxxx;
	OpTable[0x0B] = &Cpu::OpBxxx;
	OpTable[0x0C] = &Cpu::OpCxxx;
	OpTable[0x0D] = &Cpu::OpDxxx;
	OpTable[0x0E] = &Cpu::OpExxx;
	OpTable[0x0F] = &Cpu::OpFxxx;

	RegisterBank.SoundTimer = 0;
	RegisterBank.DelayTimer = 0;
}

Cpu::Cpu(const std::string& RomfilePath) : Cpu() {
	if (!LoadRom(RomfilePath)) {
		std::cerr << "Error al abrir " << RomfilePath << std::endl;
		exit(0);
	}
}

Cpu::~Cpu() {
	ImGui::SFML::Shutdown();
	delete window;
}

void Cpu::Reset() {
	for (unsigned int i = 0; i < 16; i++) {
		Stack[i] = 0x00;
		Keyboard[i] = 0x00;
		RegisterBank.V[i] = 0x00;
	}

	for (unsigned int i = 0; i < (32 * 64); i++) {
		Framebuffer[i] = 0;
	}

	for (unsigned int i = 0; i < Memory.size(); i++) {
		Memory[i] = 0x00;
	}

	for (unsigned int i = 0; i < 80; i++) {
		Memory[i] = Font[i];
	}

	RegisterBank.I = 0x00;
	RegisterBank.PC = 0x200;
	RegisterBank.SP = 0x00;

	if (!LoadRom(RomPath)) {
		std::cerr << "Error al abrir " << RomPath << std::endl;
		exit(0);
	}

	RegisterBank.SoundTimer = 0;
	RegisterBank.DelayTimer = 0;
}

bool Cpu::LoadRom(const std::string& RomfilePath) {
	RomPath = RomfilePath;
	std::ifstream Rom(RomfilePath, std::ios::in | std::ios::binary | std::ios::ate);
	if (!Rom.is_open()) {
		return false;
	}

	size_t FileSize = Rom.tellg();
	if (FileSize > (0x1000 - 0x200)) {
		std::cerr << "Rom muy grande" << std::endl;
		return false;
	}
	Rom.seekg(0);

	uint8_t *RomBuffer = new uint8_t[FileSize];
	Rom.read((char *) RomBuffer, FileSize);
	Rom.close();

	for (unsigned int i = 0; i < FileSize; i++) {
		Memory[0x200 + i] = RomBuffer[i];
	}

	delete RomBuffer;
	return true;
}

void Cpu::Start() {

	int counter = 0;

	sf::Clock deltaClock;
	while (RegisterBank.PC < Memory.size() && window->isOpen()) {
		sf::Uint8 FrameBufferSFML[(32 * 64) * 4];
		
		int PixelOffsetCounter = 0;
		for (int i = 0; i < (32 * 64); i++) {
			if (Framebuffer[i] == 1) {
				FrameBufferSFML[PixelOffsetCounter] = 0xFF;
				FrameBufferSFML[PixelOffsetCounter + 1] = 0xFF;
				FrameBufferSFML[PixelOffsetCounter + 2] = 0xFF;
				FrameBufferSFML[PixelOffsetCounter + 3] = 0xFF;
			}
			else {
				FrameBufferSFML[PixelOffsetCounter] = 0x00;
				FrameBufferSFML[PixelOffsetCounter + 1] = 0x00;
				FrameBufferSFML[PixelOffsetCounter + 2] = 0x00;
				FrameBufferSFML[PixelOffsetCounter + 3] = 0x00;
			}
			PixelOffsetCounter += 4;
		}
		frameTexture.update(FrameBufferSFML, width, height, 0, 0);
		
		sf::Event evt;
		while (window->pollEvent(evt)) {
			ImGui::SFML::ProcessEvent(evt);

			switch (evt.type) {
			case sf::Event::Closed:
				window->close();
				break;
			case sf::Event::KeyPressed:
				switch (evt.key.code)
				{
				case sf::Keyboard::Num1:
					Keyboard[0] = 1;
					break;
				case sf::Keyboard::Num2:
					Keyboard[1] = 1;
					break;
				case sf::Keyboard::Num3:
					Keyboard[2] = 1;
					break;
				case sf::Keyboard::Num4:
					Keyboard[3] = 1;
					break;
				case sf::Keyboard::Q:
					Keyboard[4] = 1;
					break;
				case sf::Keyboard::W:
					Keyboard[5] = 1;
					break;
				case sf::Keyboard::E:
					Keyboard[6] = 1;
					break;
				case sf::Keyboard::R:
					Keyboard[7] = 1;
					break;
				case sf::Keyboard::A:
					Keyboard[8] = 1;
					break;
				case sf::Keyboard::S:
					Keyboard[9] = 1;
					break;
				case sf::Keyboard::D:
					Keyboard[10] = 1;
					break;
				case sf::Keyboard::F:
					Keyboard[11] = 1;
					break;
				case sf::Keyboard::Z:
					Keyboard[12] = 1;
					break;
				case sf::Keyboard::X:
					Keyboard[13] = 1;
					break;
				case sf::Keyboard::C:
					Keyboard[14] = 1;
					break;
				case sf::Keyboard::V:
					Keyboard[15] = 1;
					break;
				case sf::Keyboard::P:
					Reset();
					break;
				case sf::Keyboard::G:
					ImguiGUI = !ImguiGUI;
					break;
				}
				break;
			case sf::Event::KeyReleased:
				switch (evt.key.code)
				{
				case sf::Keyboard::Num1:
					Keyboard[0] = 0;
					break;
				case sf::Keyboard::Num2:
					Keyboard[1] = 0;
					break;
				case sf::Keyboard::Num3:
					Keyboard[2] = 0;
					break;
				case sf::Keyboard::Num4:
					Keyboard[3] = 0;
					break;
				case sf::Keyboard::Q:
					Keyboard[4] = 0;
					break;
				case sf::Keyboard::W:
					Keyboard[5] = 0;
					break;
				case sf::Keyboard::E:
					Keyboard[6] = 0;
					break;
				case sf::Keyboard::R:
					Keyboard[7] = 0;
					break;
				case sf::Keyboard::A:
					Keyboard[8] = 0;
					break;
				case sf::Keyboard::S:
					Keyboard[9] = 0;
					break;
				case sf::Keyboard::D:
					Keyboard[10] = 0;
					break;
				case sf::Keyboard::F:
					Keyboard[11] = 0;
					break;
				case sf::Keyboard::Z:
					Keyboard[12] = 0;
					break;
				case sf::Keyboard::X:
					Keyboard[13] = 0;
					break;
				case sf::Keyboard::C:
					Keyboard[14] = 0;
					break;
				case sf::Keyboard::V:
					Keyboard[15] = 0;
					break;
				case sf::Keyboard::M:
					std::cout << "Ingrese path a rom: ";
					std::cin >> RomPath;
					Reset();
				}
			default:
				break;
			}
			
		}
		if (!Paused) {
			Exec();
		}
		if (Step) {
			Paused = true;
			Step = false;
		}

		if (RegisterBank.DelayTimer > 0) {
			RegisterBank.DelayTimer--;
		}
		if (RegisterBank.SoundTimer > 0) {
			Tone.play();
			RegisterBank.SoundTimer--;
		}
		else {
			Tone.stop();
		}
	
		if (ImguiGUI) {
			std::string PauseButtonStr;
			if (Paused) {
				PauseButtonStr = "Unpause";
			}
			else {
				PauseButtonStr = "Pause";
			}
			
			ImGui::SFML::Update(*window, deltaClock.restart());
			ImGui::Begin("Registers");
			for (int i = 0; i < 16; i += 4) {
				std::ostringstream TmpStream;
				TmpStream << std::showbase << std::internal << std::setfill('0') << std::hex << "V[" << i << "] = " << std::setw(4) << (short)RegisterBank.V[i] << " ";
				TmpStream << "V[" << i + 1 << "] = " << (short)RegisterBank.V[i + 1] << " ";
				TmpStream << "V[" << i + 2 << "] = " << (short)RegisterBank.V[i + 2] << " ";
				TmpStream << "V[" << i + 3 << "] = "  << (short)RegisterBank.V[i + 3] << std::endl;
				ImGui::Text(TmpStream.str().c_str());
			}
			std::ostringstream TmpStream;
			TmpStream << "SP = " << std::showbase << std::internal << std::setfill('0') << std::hex << std::setw(6) << (short)RegisterBank.SP << std::endl;
			TmpStream << "PC = " << std::showbase << std::internal << std::setfill('0') << std::hex << std::setw(6) << (short)RegisterBank.PC;
			ImGui::Text(TmpStream.str().c_str());
			TmpStream.clear();

			if (ImGui::Button(PauseButtonStr.c_str())) {
				Paused = !Paused;
			}
			if (Paused) {
				if (ImGui::Button("Step")) {
					Step = true;
					Paused = false;
				}
			}

			ImGui::End();
		}

		window->clear();
		window->draw(frame);
		if (ImguiGUI) {
			ImGui::SFML::Render(*window);
		}
		window->display();
	}
}

void Cpu::Exec() {
	if (RegisterBank.PC >= Memory.size()) {
		std::cerr << "Error PC sobrepaso a la size de memory" << std::endl;
		exit(0);
	}

	Opcode = (Memory[RegisterBank.PC] << 8);
	Opcode |= Memory[RegisterBank.PC + 1];

	(this->*OpTable[((Opcode & 0xF000) >> 12)])();

	if (RegisterBank.SoundTimer > 0) {
		RegisterBank.SoundTimer--;
	}
	if (RegisterBank.DelayTimer > 0) {
		RegisterBank.DelayTimer--;
	}
}

void Cpu::Op0xxx() {
	switch (Opcode & 0x000F) {
	case 0x0000:
		dbg_printf("CLS\n");
		for (int i = 0; i < 32 * 64; i++) {
			Framebuffer[i] = 0x00;
		}
		RegisterBank.PC += 2;
		break;
	case 0x000E:
		dbg_printf("RET\n");
		RegisterBank.PC = Stack[RegisterBank.SP];
		RegisterBank.SP--;
		RegisterBank.PC += 2;
		break;
	}
}

void Cpu::Op1xxx() {
	dbg_printf("JP 0x%04X\n", (Opcode & 0x0FFF));
	RegisterBank.PC = (Opcode & 0x0FFF);
}

void Cpu::Op2xxx() {
	dbg_printf("CALL 0x%04X\n", (Opcode & 0x0FFF));
	RegisterBank.SP++;
	Stack[RegisterBank.SP] = RegisterBank.PC;
	RegisterBank.PC = (Opcode & 0x0FFF);
}


void Cpu::Op3xxx() {
	dbg_printf("SE V[0x%02X], 0x%02X\n", ((Opcode & 0x0F00) >> 8), (Opcode & 0x00FF));
	if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] == (Opcode & 0x00FF)) {
		RegisterBank.PC += 2;
	}
	RegisterBank.PC += 2;
}

void Cpu::Op4xxx() {
	dbg_printf("SNE V[0x%02X], 0x%02X\n", ((Opcode & 0x0F00) >> 8), (Opcode & 0x00FF));
	if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] != (Opcode & 0x00FF)) {
		RegisterBank.PC += 2;
	}
	RegisterBank.PC += 2;
}

void Cpu::Op5xxx() {
	dbg_printf("SE V[0x%02X], V[0x%02X]\n", ((Opcode & 0x0F00) >> 8), ((Opcode & 0x00F0) >> 4));
	if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] == RegisterBank.V[((Opcode & 0x00F0) >> 4)]) {
		RegisterBank.PC += 2;
	}
	RegisterBank.PC += 2;
}

void Cpu::Op6xxx() {
	dbg_printf("LD V[0x%02X], 0x%02X\n", ((Opcode & 0x0F00) >> 8), (Opcode & 0x00FF));
	RegisterBank.V[((Opcode & 0x0F00) >> 8)] = (Opcode & 0x00FF);
	RegisterBank.PC += 2;
}

void Cpu::Op7xxx() {
	dbg_printf("ADD V[0x%02X], 0x%02X\n", ((Opcode & 0x0F00) >> 8), (Opcode & 0x00FF));
	RegisterBank.V[((Opcode & 0x0F00) >> 8)] += (Opcode & 0x00FF);
	RegisterBank.PC += 2;
}

void Cpu::Op8xxx() {
	switch (Opcode & 0x000F) {
	case 0x0000:
		dbg_printf("LD V[0x%02X], V[0x%02X]\n", ((Opcode & 0x0F00) >> 8), ((Opcode & 0x00F0) >> 4));
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] = RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x0001:
		dbg_printf("OR V[0x%02X], V[0x%02X]\n", ((Opcode & 0x0F00) >> 8), ((Opcode & 0x00F0) >> 4));
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] |= RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x0002:
#ifdef DEBUG
		std::cout << "AND V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] &= RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x0003:
#ifdef DEBUG
		std::cout << "XOR V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] ^= RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x0004: {
#ifdef DEBUG
		std::cout << "ADD V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] += RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		if (RegisterBank.V[((Opcode & 0x00F0) >> 4)] > (0xFF - RegisterBank.V[((Opcode & 0x0F00) >> 8)])) {
			RegisterBank.V[0x0F] = 1;
		}
		else {
			RegisterBank.V[0x0F] = 0;
		}

		RegisterBank.PC += 2;
	}
				 break;
	case 0x0005:
#ifdef DEBUG
		std::cout << "SUB V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] > RegisterBank.V[((Opcode & 0x00F0) >> 4)]) {
			RegisterBank.V[0xF] = 1;
		}
		else {
			RegisterBank.V[0xF] = 0;
		}
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] -= RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x0006:
#ifdef DEBUG
		std::cout << "SHR V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] & 0x01 == 1) {
			RegisterBank.V[0xF] = 1;
		}
		else {
			RegisterBank.V[0xF] = 0;
		}
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] /= 2;
		RegisterBank.PC += 2;
		break;
	case 0x0007:
#ifdef DEBUG
		std::cout << "SUBN V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] < RegisterBank.V[((Opcode & 0x00F0) >> 4)]) {
			RegisterBank.V[0xF] = 1;
		}
		else {
			RegisterBank.V[0x0F] = 0;
		}
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] -= RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x000E:
#ifdef DEBUG
		std::cout << "SHL V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] & 0x80 == 1) {
			RegisterBank.V[0xF] = 1;
		}
		else {
			RegisterBank.V[0xF] = 0;
		}
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] *= 2;
		RegisterBank.PC += 2;
		break;
	default:
		std::cout << "Opcode Unrecognized " << std::hex << Opcode << std::endl;
		break;
	}
}

void Cpu::Op9xxx() {
#ifdef DEBUG
	std::cout << "SNE VX VY" << std::endl;
#endif
	if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] != RegisterBank.V[((Opcode & 0x00F0) >> 4)]) {
		RegisterBank.PC += 2;
	}
	RegisterBank.PC += 2;
}

void Cpu::OpAxxx() {
#ifdef DEBUG
	std::cout << "LD I, " << std::setw(2) << std::setfill('0') << std::hex << (Opcode & 0x0FFF) << std::endl;
#endif
	RegisterBank.I = (Opcode & 0x0FFF);
	RegisterBank.PC += 2;
}

void Cpu::OpBxxx() {
#ifdef DEBUG
	std::cout << "JMP V[0], " << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0FFF)) << std::endl;
#endif
	RegisterBank.PC = (RegisterBank.V[0] + (Opcode & 0x0FFF));
}

void Cpu::OpCxxx() {
#ifdef DEBUG
	std::cout << "RND V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], " << (Opcode & 0x00FF) << std::endl;
#endif
	RegisterBank.V[((Opcode & 0x0F00) >> 8)] = distribution(rd);
	RegisterBank.V[((Opcode & 0x0F00) >> 8)] &= (Opcode & 0x00FF);
	RegisterBank.PC += 2;
}

void Cpu::OpDxxx() {
	int Vx = RegisterBank.V[((Opcode & 0x0F00) >> 8)];
	int Vy = RegisterBank.V[((Opcode & 0x00F0) >> 4)];

#ifdef DEBUG
	std::cout << "DWR " << std::setw(4) << std::setfill('0') << std::hex << RegisterBank.I << ", " << Vx << ", " << Vy << ", " << (Opcode & 0x000F) << std::endl;
#endif

	RegisterBank.V[0xF] = 0;
	for (int y = 0; (y < (Opcode & 0x000F)); y++) {
		uint8_t pixel = Memory[RegisterBank.I + y];
		for (int x = 0; x < 8; x++) {
			if ((pixel & (0x80 >> x)) != 0 && (x + Vx + ((y + Vy) * 64)) < (32 * 64)) {
				if (Framebuffer[(x + Vx + ((y + Vy) * 64))] == 1) {
					RegisterBank.V[0x0F] = 1;
				}
				Framebuffer[(x + Vx + ((y + Vy) * 64))] ^= 1;
			}
		}
	}

	RegisterBank.PC += 2;
}

void Cpu::OpExxx() {
	switch ((Opcode & 0x000F)) {
	case 0x0001:
#ifdef DEBUG
		std::cout << "SKNP V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		if (Keyboard[RegisterBank.V[((Opcode & 0x0F00) >> 8)]] == 0) {
			RegisterBank.PC += 2;
		}
		RegisterBank.PC += 2;
		break;
	case 0x000E:
#ifdef DEBUG
		std::cout << "SKP V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		if (Keyboard[RegisterBank.V[((Opcode & 0x0F00) >> 8)]] == 1) {
			RegisterBank.PC += 2;
		}
		RegisterBank.PC += 2;
		break;
	}
}

void Cpu::OpFxxx() {
	switch (Opcode & 0x00FF) {
	case 0x0007:
#ifdef DEBUG
		std::cout << "LD V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], DT" << std::endl;
#endif
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] = RegisterBank.DelayTimer;
		RegisterBank.PC += 2;
		break;
	case 0x000A:
#ifdef DEBUG
		std::cout << "LD V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], K" << std::endl;
#endif
		for (int i = 0; i < 16; i++) {
			if (Keyboard[i] == 1) {
				RegisterBank.V[((Opcode & 0x0F00) >> 8)] = i;
				RegisterBank.PC += 2;
				break;
			}
		}
		break;
	case 0x001E:
#ifdef DEBUG
		std::cout << "ADD I, V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], " << (Opcode & 0x00FF) << std::endl;
#endif
		RegisterBank.I += RegisterBank.V[((Opcode & 0x0F00) >> 8)];
		RegisterBank.PC += 2;
		break;
	case 0x0015:
#ifdef DEBUG
		std::cout << "LD DT, V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		RegisterBank.DelayTimer = RegisterBank.V[((Opcode & 0x0F00) >> 8)];
		RegisterBank.PC += 2;
		break;
	case 0x0018:
#ifdef DEBUG
		std::cout << "LD ST, V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		RegisterBank.SoundTimer = RegisterBank.V[((Opcode & 0x0F00) >> 8)];
		RegisterBank.PC += 2;
		break;
	case 0x0029:
#ifdef DEBUG
		std::cout << "LD F, V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		RegisterBank.I = RegisterBank.V[((Opcode & 0x0F00) >> 8)] * 5;
		RegisterBank.PC += 2;
		break;
	case 0x0033: {
#ifdef DEBUG
		std::cout << "LD B, V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		int tmp = RegisterBank.V[((Opcode & 0x0F00) >> 8)];
		Memory[RegisterBank.I] = tmp / 100;
		Memory[RegisterBank.I + 1] = ((tmp / 10) % 10);
		Memory[RegisterBank.I + 2] = ((tmp % 100) % 10);

		RegisterBank.PC += 2;
	}
				 break;

	case 0x0055:
#ifdef DEBUG
		std::cout << "LD [I], V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		for (int i = 0; i < ((Opcode & 0x0F00) >> 8); i++) {
			Memory[RegisterBank.I + i] = RegisterBank.V[i];
		}
		RegisterBank.PC += 2;
		break;
	case 0x0065:
#ifdef DEBUG
		std::cout << "LD V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], [I]" << std::endl;
#endif
		for (int i = 0; i < ((Opcode & 0x0F00) >> 8); i++) {
			RegisterBank.V[i] = Memory[RegisterBank.I + i];
		}

		//RegisterBank.I += (((Opcode & 0x0F00) >> 8) + 1);
		RegisterBank.PC += 2;
		break;
	default:
		std::cout << "Opcode Unrecognized " << std::hex << Opcode << std::endl;
		break;
	}
}
