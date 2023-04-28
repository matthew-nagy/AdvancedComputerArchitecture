#pragma once
#include "BranchPredictor.h"
#include "ExecutionUnit.h"

word getSimpleArithmetic(BranchPredictor*, PipelineEntry& e) {
	switch (e.opcode) {
	case IAdd:
	case Add:
		return e.sourceValue1 + e.sourceValue2;
	case IAnd:
	case And:
		return e.sourceValue1 & e.sourceValue2;
	case IOr:
	case Or:
		return e.sourceValue1 | e.sourceValue2;
	case IXor:
	case Xor:
		return e.sourceValue1 | e.sourceValue2;
	case ISlt:
	case Slt:
		return e.sourceValue1 < e.sourceValue2 ? 1 : 0;
	case ILsl:
	case Lsl:
		return e.sourceValue1 << e.sourceValue2;
	case ILsr:
	case Lsr:
		return e.sourceValue1 >> e.sourceValue2;
	default:
		//This isn't possible
		throw(0);
	}
}

bool checkIfBranchTaken(PipelineEntry& e) {
	switch (e.opcode) {
	case Beq:
		return e.sourceValue1 == e.sourceValue2;
	case Bne:
		return e.sourceValue1 != e.sourceValue2;
	case Blt:
		return e.sourceValue1 < e.sourceValue2;
	case Bge:
		return e.sourceValue1 >= e.sourceValue2;
	default:
		throw(0);
	}
}

word getConditionalBranch(BranchPredictor* b, PipelineEntry& e) {
	bool taken = checkIfBranchTaken;
	bool prediction = b->predictJump(e.instructionAddress, e.destination);
	b->reportResult(prediction == taken, e.instructionAddress);
	return taken ? 1 : 0;
}

word getResultOfOperation(BranchPredictor* b, PipelineEntry& e) {
	if (groups::simpleArithmetic.count(e.opcode) > 0)
		return getSimpleArithmetic(b, e);
	if (groups::conditionalBranches.count(e.opcode) > 0)
		return getConditionalBranch(b, e);
}