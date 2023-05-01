#pragma once
#include "riscv.h"

class BranchPredictor {
public:
	virtual bool predictJump(int currentPC, int destination) = 0;

	virtual void reportResult(bool correct, int instructionAddress) = 0;
};

class SimpleBranchPredictor final: public BranchPredictor {
public:
	enum class Mode {
		Always, Never, AlwaysForwards, AlwaysBackwards
	};

	void reportResult(bool, int)final override {}

	bool predictJump(int currentPC, int destination)final override {
		switch (mode) {
		case Mode::Always:
			return true;
		case Mode::Never:
			return false;
		case Mode::AlwaysForwards:
			return destination > currentPC;
		case Mode::AlwaysBackwards:
			return destination < currentPC;
		default:
			throw(0);
		}
	}

	SimpleBranchPredictor(Mode mode):
		mode(mode)
	{}

private:
	const Mode mode;
};


class OneBitBranchPredictor final: public BranchPredictor {
public:
	bool predictJump(int currentPC, int)final override {
		return maskedPrediction[currentPC & mask];
	}
	void reportResult(bool correct, int instructionAddress)final override {
		maskedPrediction[instructionAddress & mask] = correct;
	}

	OneBitBranchPredictor(int bitsForAddress):
		mask(pow(2, bitsForAddress) - 1)
	{}
private:
	const int mask;
	std::unordered_map<int, bool> maskedPrediction;
};

class TwoBitBranchPredictor final: public BranchPredictor {
public:
	bool predictJump(int currentPC, int)final override {
		return maskedPrediction[currentPC & mask] > 1;
	}

	void reportResult(bool correct, int instructionAddress)final override {
		char& confidence = maskedPrediction[instructionAddress & mask];
		bool predictBranch = maskedPrediction[instructionAddress & mask] > 1;

		if ((predictBranch && correct) || (!predictBranch && !correct)) {
			if (confidence < 3)
				confidence += 1;
		}
		else {
			if(confidence > 0)
				confidence -= 1;
		}
	}

	TwoBitBranchPredictor(int bitsForAddress) :
		mask(pow(2, bitsForAddress) - 1)
	{}
private:
	const int mask;
	std::unordered_map<int, char> maskedPrediction;
};