#pragma once

#include "riscv.h"

enum class InstructionType {
	Branch, Store, RegisterOp
};

enum class CommitResult {
	Complete, BranchCorrect, FlushEverything
};

struct RobEntry {
	InstructionType type;
	word desination;
	//Either the value to write, or a boolean of the branch being predicted correct
	word valueField;
	bool ready = false;
	bool active = false;

	Instruction instruction = Instruction(Add);
	int instructionIndex = -1;//Used for return address bodging

	RobEntry() = default;

	RobEntry(InstructionType type) :
		type(type),
		instruction(Add)
	{
		desination = valueField = ready = 0;
	}

	CommitResult commit(word* memory, std::vector<word>& registers) {
		switch (type)
		{
		case InstructionType::Branch:
			if (valueField > 0) {
				if (instruction.operation == Jlr)
					registers[1] = instructionIndex;
				return CommitResult::BranchCorrect;
			}
			return CommitResult::FlushEverything;
		case InstructionType::Store:
			memory[desination] = valueField;
			return CommitResult::Complete;
		case InstructionType::RegisterOp:
			registers[desination] = valueField;
			return CommitResult::Complete;
		default:
			printf("Unknown instruction type in rob\n");
			throw(0);
		}
	}
};

class ReOrderBuffer {
public:
	bool hasRoom() {
		return size < capacity;
	}
	RobEntry& operator[](int index) {
		return entries[index];
	}
	RobEntry& head() {
		return entries[headIndex];
	}
	RobEntry pop() {
		head().active = false;
		RobEntry copy = head();
		incrimentIndex(headIndex);
		size -= 1;
		return copy;
	}
	int push(RobEntry entry) {
		entries[nextIndex] = entry;
		entries[nextIndex].active = true;
		int returningIndex = nextIndex;
		incrimentIndex(nextIndex);
		size += 1;
		return returningIndex;
	}
	int length() {
		return size;
	}

	void flushEverything() {
		size = 0;
		nextIndex = 0;
		headIndex = 0;
	}

	word getRobOrMinus(bool(*predicate)(RobEntry&, word), word secondryArg) {
		word finalResult = -1;
		int lookedAt = 0;
		while (lookedAt < size) {
			int checkIndex = headIndex + lookedAt;
			if (checkIndex >= capacity)checkIndex -= capacity;

			if (predicate(entries[checkIndex], secondryArg)) {
				finalResult = checkIndex;
			}
			lookedAt += 1;
		}
		return finalResult;
	}

	ReOrderBuffer(int capacity):
		capacity(capacity),
		size(0),
		nextIndex(0),
		headIndex(0)
	{
		for (size_t i = 0; i < capacity; i++)
			entries.emplace_back();
	}
private:
	std::vector<RobEntry> entries;
	int capacity, size, nextIndex, headIndex;
	void incrimentIndex(int& index) {
		index += 1;
		if (index == capacity)
			index = 0;
	}
};