#include <iostream>

#include "cpu.h"

int main(int argc, char **argv) {
	Cpu cpu;
	if(argc < 2) {
		if(!cpu.LoadRom("test.ch8")) {
			std::cerr << "Error al abrir test.ch8" << std::endl;
			return 0;
		}
	} else {
		if(!cpu.LoadRom(argv[1])) {
			std::cerr << "Error al abrir " << argv[1] << std::endl;
			return 0;
		}
	}
	cpu.Start();

	return 0;
}
