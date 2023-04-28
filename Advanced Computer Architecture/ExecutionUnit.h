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

	PipelineEntry(int instructionAddress, int destination = -1):
		instructionAddress(instructionAddress),
		destination(destination)
	{
		inputRobIndex1 = sourceValue1 = inputRobIndex2 = sourceValue2 = outputRobIndex = -1;
	}

	bool readyToExecute() {
		return inputRobIndex1 != -1 && inputRobIndex2 != -1;
	}
};

#include "BranchPredictor.h"

word getResultOfOperation(BranchPredictor*, PipelineEntry&);

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
		return currentCycles == cyclesToComplete;
	}

	PipelineEntry getCompletedEntry(BranchPredictor* branchPredictor) {
		currentTask.result = getResultOfOperation(branchPredictor, currentTask);
		waiting = true;
		return currentTask;
	}

	bool flushEverything() {
		waiting = true;
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
