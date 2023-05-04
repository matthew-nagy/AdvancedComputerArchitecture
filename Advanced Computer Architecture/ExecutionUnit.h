#pragma once
#include "riscv.h"

struct PipelineEntry {
	Opcode opcode;
	int instructionAddress;
	word inputRobIndex1, inputRobIndex2;
	word sourceValue1, sourceValue2;
	word outputRobIndex;

	//Only used for branches
	int destination;
	//Set when the instruction executes
	word result;

	PipelineEntry() = default;
	PipelineEntry(int instructionAddress, int destination = -1):
		instructionAddress(instructionAddress),
		destination(destination)
	{
		inputRobIndex1 = sourceValue1 = inputRobIndex2 = sourceValue2 = outputRobIndex = -1;
	}

	bool readyToExecute() {
		return inputRobIndex1 == -1 && inputRobIndex2 == -1;
	}

	void commonDataBus(word robIndex, word value) {
		if (inputRobIndex1 == robIndex) {
			inputRobIndex1 = -1;
			sourceValue1 = value;
		}
		if (inputRobIndex2 == robIndex) {
			inputRobIndex2 = -1;
			sourceValue2 = value;
		}
	}
};

#include "BranchPredictor.h"

word getResultOfOperation(BranchPredictor*, PipelineEntry&, std::vector<word>&, std::vector<word>&);

class ExecutionUnit {
public:
	bool hasSpace() {
		return waiting;
	}
	void place(PipelineEntry task) {
		currentTask = task;
		waiting = false;
		currentCycles = 0;
	}
	void update() {
		if(!waiting && currentCycles < cyclesToComplete)
			currentCycles += 1;
	}
	bool hasFinishedExecuting() {
		return currentCycles == cyclesToComplete && !waiting;
	}

	std::optional<word> fetchReturnAddress() {
		if (waiting == false && currentTask.opcode == Jlr)
			return currentTask.instructionAddress;
		return std::nullopt;
	}

	PipelineEntry getCompletedEntry(BranchPredictor* branchPredictor, std::vector<word>& registers, std::vector<word>& memory) {
		currentTask.result = getResultOfOperation(branchPredictor, currentTask, registers, memory);
		//printf("Finished task %d %d %d %d\n", (int)currentTask.opcode, currentTask.destination, currentTask.sourceValue1, currentTask.sourceValue2);
		waiting = true;
		return currentTask;
	}

	void flushEverything() {
		waiting = true;
	}

	int currrentCompleteCycles() {
		return currentCycles;
	}

	ExecutionUnit(int cyclesToCompleteTask):
		cyclesToComplete(cyclesToCompleteTask),
		currentTask(-1)
	{}

private:
	int cyclesToComplete;
	PipelineEntry currentTask;
	int currentCycles = 0;
	bool waiting = true;
};
