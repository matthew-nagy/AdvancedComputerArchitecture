#pragma once

class GlobalData {
	struct EUData {
		int numberOfUnits;
		int sizeOfReservations;
		int cyclesNeeded;

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
};

GlobalData::EUData GlobalData::simpleInteger = GlobalData::EUData(1, 2, 2);
GlobalData::EUData GlobalData::complexInteger = GlobalData::EUData(1, 2, 8);
GlobalData::EUData GlobalData::branchUnits = GlobalData::EUData(1, 2, 3);
GlobalData::EUData GlobalData::loadStoreUnits = GlobalData::EUData(1, 2, 4);
int GlobalData::memorySize = 1024;
int GlobalData::reorderBufferSize = 8;