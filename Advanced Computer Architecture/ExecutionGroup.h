#pragma once
#include "ReservationStation.h"

class ExecutionGroup {
public:

	bool canTakeInstruction() {
		return station.hasRoom();
	}
	void pushInstruction(PipelineEntry& pipelineEntry) {
		station.push(pipelineEntry);
	}



private:
	ReservationStation station;
	std::vector<ExecutionUnit> eus;
};