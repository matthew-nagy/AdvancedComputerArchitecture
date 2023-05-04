#pragma once

#include "riscv.h"
#include <iostream>

class GlobalData {
public:
	struct EUData {
		int numberOfUnits;
		int sizeOfReservations;
		int cyclesNeeded;

		void print() {
			std::cout << "\t\t" << numberOfUnits << " units\n\t\t" << sizeOfReservations << " reservation spaces\n\t\t" << cyclesNeeded << " cycles to execute\n";
		}

		EUData(int numberOfUnits, int sizeOfReservations, int cyclesNeeded):
			numberOfUnits(numberOfUnits),
			sizeOfReservations(sizeOfReservations),
			cyclesNeeded(cyclesNeeded)
		{}
	};
	static EUData simpleInteger;
	static EUData complexInteger;
	static EUData branchUnits;
	static EUData loadStoreUnits;
	static int memorySize;
	static int reorderBufferSize;
	static int width;

	static const std::unordered_map<std::string, EUData*> euNameMap;

	static void loadFrom(const std::string& filename) {
		std::string line = "";
		std::ifstream file(filename);
		while (std::getline(file, line)) {
			if (line.size() == 0)continue;
			if (line[0] == '#')continue;
			auto splits = assembler::splitLine(line);
			if (splits[0] == "memory")
				memorySize = std::atoi(splits[1].c_str());
			else if (splits[0] == "robSize")
				reorderBufferSize = std::atoi(splits[1].c_str());
			else if (splits[0] == "width")
				width = std::atoi(splits[1].c_str());
			else {
				EUData* target = euNameMap.at(splits[0]);
				target->numberOfUnits = std::atoi(splits[1].c_str());
				target->sizeOfReservations = std::atoi(splits[2].c_str());
				target->cyclesNeeded = std::atoi(splits[3].c_str());
			}
		}
	}

	static void print() {
		std::cout << "Hardware is\n\tWidth " << width << " pipeline\n";
		std::cout << "\tMemory " << memorySize << " Bytes\n\tROB size " << reorderBufferSize << "\n";
		std::cout << "\tALU properties:\n";
		simpleInteger.print();
		std::cout << "\tCALU properties:\n";
		complexInteger.print();
		std::cout << "\tBU properties:\n";
		branchUnits.print();
		std::cout << "\tLSU properties:\n";
		loadStoreUnits.print();
	}
};

GlobalData::EUData GlobalData::simpleInteger = GlobalData::EUData(1, 2, 1);
GlobalData::EUData GlobalData::complexInteger = GlobalData::EUData(1, 2, 4);
GlobalData::EUData GlobalData::branchUnits = GlobalData::EUData(1, 2, 2);
GlobalData::EUData GlobalData::loadStoreUnits = GlobalData::EUData(1, 2, 3);
int GlobalData::memorySize = 2048;
int GlobalData::reorderBufferSize = 32;
int GlobalData::width = 1;
const std::unordered_map<std::string, GlobalData::EUData*> GlobalData::euNameMap = assembler::MapBuilder<std::string, EUData*>()
("alu", &simpleInteger)("calu", &complexInteger)("bu", &branchUnits)("lsu", &loadStoreUnits)
;