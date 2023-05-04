#pragma once
#include "ReservationStation.h"

class ExecutionGroup {
public:

	bool canTakeInstruction(Opcode op) {
		return station->hasRoom(op);
	}
	void pushInstruction(PipelineEntry& pipelineEntry) {
		station->push(pipelineEntry);
	}

	void commonDataBus(word robIndex, word value) {
		station->commonDataBus(robIndex, value);
	}

	void flushEverything() {
		station->flushEverything();
		for (auto& eu : eus)
			eu.flushEverything();
	}

	std::vector<PipelineEntry> update(BranchPredictor* branchPredictor, std::vector<word>& registers, std::vector<word>& memory) {
		updateReservationStations();
		return updateEUs(branchPredictor, registers, memory);
	}

	std::optional<word> getReturnAddress() {
		auto ra = station->getReturnAddress();
		if (ra.has_value())
			return ra;
		int leastCycles = INT_MAX;
		for (auto& eu : eus) {
			auto nra = eu.fetchReturnAddress();
			if (nra.has_value() && eu.currrentCompleteCycles() < leastCycles) {
				leastCycles = eu.currrentCompleteCycles();//Make sure you have the least executed
				ra = nra;
			}
		}
		return ra;
	}

	ExecutionGroup(int reservationCapacity, int numberOfEus, int cyclesToComplete, bool loadStore){
		for (int i = 0; i < numberOfEus; i++)
			eus.emplace_back(cyclesToComplete);

		if (loadStore)
			station = new LoadStoreQueue(reservationCapacity);
		else station = new ReservationStation(reservationCapacity);
	}

	ExecutionGroup(GlobalData::EUData data, bool loadStore):
		ExecutionGroup(data.sizeOfReservations, data.numberOfUnits, data.cyclesNeeded, loadStore)
	{}

private:
	GenericReservationStation* station;
	std::vector<ExecutionUnit> eus;

	void updateReservationStations() {
		for (auto& eu : eus) {
			if (eu.hasSpace()) {
				if (station->readyToExecute()) {
					station->executeOn(&eu);
				}
			}
		}
	}
	std::vector<PipelineEntry> updateEUs(BranchPredictor* branchPredictor, std::vector<word>& registers, std::vector<word>& memory) {
		std::vector<PipelineEntry> finished;
		for (auto& eu : eus) {
			eu.update();
			if (eu.hasFinishedExecuting()) {
				finished.emplace_back(eu.getCompletedEntry(branchPredictor, registers, memory));
			}
		}

		return finished;
	}
};