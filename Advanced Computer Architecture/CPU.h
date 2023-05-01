#pragma once

#include "ReservationStation.h"
#include "globalValues.h"
#include "rob.h"
#include "ExecutionGroup.h"
#include "operations.h"
#include <queue>

std::vector<PipelineEntry> operator+(std::vector<PipelineEntry> left, const std::vector<PipelineEntry>& right) {
	left.insert(left.end(), right.begin(), right.end());
	return left;
}

#include "MattQueue.h"

class CPU {
public:

	void update() {
		commit();
		execute();
		issue();
		decode();
		fetch();
	}

	void regPrint(const char* prior) {
		for (int i = 0; i < 32; i++) {
			printf("%sr%d:\t%d\n", prior, i, registers[i]);
		}
	}

	void printMemory(const char* prior, int from, int upTo) {
		for (int i = from; i < upTo; i++)
			printf("%s%d:\t%d\n", prior, i, memory[i]);
	}

	int operator[](const std::string& index) {
		return labels.at(index);
	}
	//Get memory value
	int operator[](int index) {
		return memory[index];
	}
	//Get reguister value
	int operator()(int index) {
		if (index == 0)return 0;
		return registers[index];
	}

	CPU(int width, std::string filename, BranchPredictor* branchPredictor) :
		width(width),
		pc(0),
		rob(GlobalData::reorderBufferSize),
		eu_simpleArthmatic(GlobalData::simpleInteger, false),
		eu_complexArithmatic(GlobalData::complexInteger, false),
		eu_branches(GlobalData::branchUnits, false),
		eu_loadStore(GlobalData::loadStoreUnits, true),
		branchPredictor(branchPredictor)
	{

		for (int i = 0; i < 32; i++)registers.emplace_back(0);

		auto result = assembler::compile(filename, GlobalData::memorySize);
		instructions = std::move(result.instructions);
		memory = result.memory;
		labels = result.labels;
	}

private:
	struct BranchHistory {
		int pcIfPoorlyPredicted;
		bool branchPredicted;
		BranchHistory(int pcIfPoorlyPredicted, bool branchPredicted):
			pcIfPoorlyPredicted(pcIfPoorlyPredicted),
			branchPredicted(branchPredicted)
		{}
	};

	std::vector<Instruction> instructions;
	std::unordered_map<std::string, int> labels;
	int pc;
	std::queue<BranchHistory> branchHistory;
	word* memory;
	std::vector<word> registers;
	ReOrderBuffer rob;
	BranchPredictor* branchPredictor;

	ExecutionGroup eu_simpleArthmatic;
	ExecutionGroup eu_complexArithmatic;
	ExecutionGroup eu_branches;
	ExecutionGroup eu_loadStore;

	int width;
	MattQueue<PipelineEntry> fetchedInstructions;
	MattQueue<PipelineEntry> decodedInstructions;

	InstructionType getRobType(Opcode op) {
		if (groups::stores.count(op) > 0)
			return InstructionType::Store;
		if (groups::conditionalBranches.count(op) > 0 || groups::jump.count(op) > 0)
			return InstructionType::Branch;
		return InstructionType::RegisterOp;
	}

	void getRobIndexOrRegisterValue(word& robIndex, word& source, word requestedReg) {
		if (requestedReg == 0) {
			source = 0;
		}
		word neccessaryRobIndex = rob.getRobOrMinus(
			[](RobEntry& entry, word requestedReg) {
				return entry.type == InstructionType::RegisterOp && entry.desination == requestedReg && entry.active == true;
			}, requestedReg);
		if (neccessaryRobIndex == -1)
			source = registers[requestedReg];
		else {
			if (rob[neccessaryRobIndex].ready)
				source = rob[neccessaryRobIndex].valueField;
			else robIndex = neccessaryRobIndex;
		}
	}

	int fetchLastReturnAddress() {
		for (auto& f : fetchedInstructions)
			if (f.opcode == Jlr)
				return f.instructionAddress;
		for (auto& f : decodedInstructions)
			if (f.opcode == Jlr)
				return f.instructionAddress;
		auto possibleRA = eu_branches.getReturnAddress();
		if (possibleRA.has_value())
			return *possibleRA;
		return registers[1];
	}

	void getRobIndexOrMemoryValue(word& robIndex, word& source, word memoryAddress) {
		word neccessaryRobIndex = rob.getRobOrMinus(
			[](RobEntry& entry, word memoryAddress) {
				return entry.type == InstructionType::Store && entry.desination == memoryAddress;
			}, memoryAddress);
		if (neccessaryRobIndex == -1)
			source = memory[memoryAddress];
		else
			robIndex = neccessaryRobIndex;
	}

	PipelineEntry fetchInstruction() {
		Instruction& fetchedInstruction = instructions[pc];
		pc += 1;
		PipelineEntry pipelinedInstruction(pc, fetchedInstruction.destination);
		pipelinedInstruction.opcode = fetchedInstruction.operation;
		RobEntry newEntry(getRobType(fetchedInstruction.operation));
		newEntry.desination = fetchedInstruction.destination;
		newEntry.instruction = fetchedInstruction;

		if (groups::jump.count(fetchedInstruction.operation) > 0) {
			pipelinedInstruction.destination = pipelinedInstruction.instructionAddress;
			pc = fetchedInstruction.destination;
			newEntry.valueField = 1;//Always taken, so it was correct
		}
		else if (groups::conditionalBranches.count(fetchedInstruction.operation) > 0) {
			getRobIndexOrRegisterValue(pipelinedInstruction.inputRobIndex1, pipelinedInstruction.sourceValue1, fetchedInstruction.source1);
			getRobIndexOrRegisterValue(pipelinedInstruction.inputRobIndex2, pipelinedInstruction.sourceValue2, fetchedInstruction.source2);

			if (branchPredictor->predictJump(pc - 1, fetchedInstruction.destination)) {
				branchHistory.emplace(pc, true);
				pc = fetchedInstruction.destination;
			}
			else {
				branchHistory.emplace(fetchedInstruction.destination, false);
			}
		}
		else if (groups::immediates.count(fetchedInstruction.operation) > 0) {
			pipelinedInstruction.sourceValue2 = fetchedInstruction.source2;
			getRobIndexOrRegisterValue(pipelinedInstruction.inputRobIndex1, pipelinedInstruction.sourceValue1, fetchedInstruction.source1);
		}
		//We already checked immediates, so simple is now the register to register (this is horrible)
		else if (groups::complexArithmetic.count(fetchedInstruction.operation) > 0 ||
			groups::simpleArithmetic.count(fetchedInstruction.operation) > 0) {
			getRobIndexOrRegisterValue(pipelinedInstruction.inputRobIndex1, pipelinedInstruction.sourceValue1, fetchedInstruction.source1);
			getRobIndexOrRegisterValue(pipelinedInstruction.inputRobIndex2, pipelinedInstruction.sourceValue2, fetchedInstruction.source2);
		}
		//Load or store :) :) :) :)
		else if (groups::loads.count(fetchedInstruction.operation) > 0) {
			getRobIndexOrRegisterValue(pipelinedInstruction.inputRobIndex1, pipelinedInstruction.sourceValue1, fetchedInstruction.source1);
			pipelinedInstruction.sourceValue2 = fetchedInstruction.source2;
		}
		else if(groups::stores.count(fetchedInstruction.operation) > 0){
			pipelinedInstruction.sourceValue1 = fetchedInstruction.source1;
			getRobIndexOrRegisterValue(pipelinedInstruction.inputRobIndex2, pipelinedInstruction.sourceValue2, fetchedInstruction.source2);
		}

		pipelinedInstruction.outputRobIndex = rob.push(newEntry);
		return pipelinedInstruction;
	}

	void fetch() {
		for (size_t i = 0; i < width; i++) {
			if (!rob.hasRoom())return;
			if (pc >= instructions.size() || pc < 0)throw(0);
			if (fetchedInstructions.size() < width) {
				fetchedInstructions.emplace(fetchInstruction());
			}
		}
	}

	void decode() {
		for (size_t i = 0; i < width; i++) {
			if (fetchedInstructions.size() > 0 && decodedInstructions.size() < width) {
				decodedInstructions.emplace(fetchedInstructions.front());
				fetchedInstructions.pop();
			}
		}
	}

	bool tryIssue(PipelineEntry& pipeEntry, ExecutionGroup& eGroup) {
		if (eGroup.canTakeInstruction(pipeEntry.opcode)) {
			eGroup.pushInstruction(pipeEntry);
			return true;
		}
		return false;
	}

	void issue() {
		//Used to itterate through the decodedInstructions
		for (size_t i = 0; i < width; i++) {
			if (decodedInstructions.size() > 0) {//We have an instruction to send!!
				PipelineEntry& pipeEntry = decodedInstructions.front();
				bool issued = false;
				if (groups::simpleArithmetic.count(pipeEntry.opcode) > 0)
					issued = tryIssue(pipeEntry, eu_simpleArthmatic);
				else if (groups::complexArithmetic.count(pipeEntry.opcode) > 0)
					issued = tryIssue(pipeEntry, eu_complexArithmatic);
				else if (groups::loads.count(pipeEntry.opcode) > 0 ||
					groups::stores.count(pipeEntry.opcode) > 0)
					issued = tryIssue(pipeEntry, eu_loadStore);
				else//Branch or jump
					issued = tryIssue(pipeEntry, eu_branches);
				
				if(issued)
					decodedInstructions.pop();
			}
		}
	}

	void execute() {
		auto s1 = eu_simpleArthmatic.update(branchPredictor, registers, memory);
		auto s2 = eu_complexArithmatic.update(branchPredictor, registers, memory);
		auto s3 = eu_branches.update(branchPredictor, registers, memory);
		auto s4 = eu_loadStore.update(branchPredictor, registers, memory);
		auto finishedValues = s1 + s2 + s3 + s4;
			
		if (finishedValues.size() == 0)
			return;

		for (auto& fVal : finishedValues) {
			eu_simpleArthmatic.commonDataBus(fVal.outputRobIndex, fVal.result);
			eu_complexArithmatic.commonDataBus(fVal.outputRobIndex, fVal.result);
			eu_branches.commonDataBus(fVal.outputRobIndex, fVal.result);
			eu_loadStore.commonDataBus(fVal.outputRobIndex, fVal.result);

			for (auto& d : decodedInstructions)
				d.commonDataBus(fVal.outputRobIndex, fVal.result);
			for (auto& f : fetchedInstructions)
				f.commonDataBus(fVal.outputRobIndex, fVal.result);

			rob[fVal.outputRobIndex].ready = true;
			rob[fVal.outputRobIndex].valueField = fVal.result;
			if (groups::stores.count(fVal.opcode) > 0) {
				rob[fVal.outputRobIndex].desination = fVal.destination + registers[fVal.sourceValue1];
				rob[fVal.outputRobIndex].valueField = fVal.sourceValue2;
			}
		}
	}

	void commit() {
		for (size_t i = 0; i < width; i++) {
			if (rob.length() == 0)return;
			if (rob.head().ready) {
				auto result = rob.head().commit(memory, registers);
				auto popped = rob.pop();
				if (result == CommitResult::FlushEverything) {
					flushEverything();
					return;
				}
			}
			else return;
		}
	}

	void flushEverything() {
		rob.flushEverything();
		eu_simpleArthmatic.flushEverything();
		eu_complexArithmatic.flushEverything();
		eu_loadStore.flushEverything();
		eu_branches.flushEverything();
		while (fetchedInstructions.size() > 0)fetchedInstructions.pop();
		while (decodedInstructions.size() > 0)decodedInstructions.pop();
		auto fix = branchHistory.front();
		pc = fix.pcIfPoorlyPredicted;
		while (branchHistory.size() > 0)branchHistory.pop();
	}
};