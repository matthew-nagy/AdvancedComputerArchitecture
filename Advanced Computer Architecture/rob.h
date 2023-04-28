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
	bool ready;

	RobEntry(InstructionType type) :
		type(type)
	{
		desination = valueField = ready = 0;
	}

	CommitResult commit(word* memory, word* registers) {
		switch (type)
		{
		case InstructionType::Branch:
			if (valueField > 0) {
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
		return size != capacity;
	}
	RobEntry& operator[](int index) {
		return entries[index];
	}
	RobEntry& head() {
		return entries[headIndex];
	}
	RobEntry pop() {
		RobEntry copy = head();
		incrimentIndex(headIndex);
		size -= 1;
		return copy;
	}
	int push(RobEntry entry) {
		entries[nextIndex] = entry;
		int returningIndex = nextIndex;
		incrimentIndex(nextIndex);
		return returningIndex;
	}

	void flushEverything() {
		size = 0;
		nextIndex = 0;
		headIndex = 0;
	}
private:
	RobEntry* entries;
	int capacity, size, nextIndex, headIndex;
	void incrimentIndex(int& index) {
		index += 1;
		if (index == capacity)
			index = 0;
	}
};