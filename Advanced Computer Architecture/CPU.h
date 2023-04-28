#pragma once

#include "ReservationStation.h"
#include "globalValues.h"
#include <queue>

class CPU {
public:


private:
	struct BranchHistory {
		int pcIfPoorlyPredicted;
		bool branchPredicted;
	};

	std::vector<Instruction> instructions;
	int pc;
	std::queue<BranchHistory> branchHistory;
	word* memory;
	word* registers;


};