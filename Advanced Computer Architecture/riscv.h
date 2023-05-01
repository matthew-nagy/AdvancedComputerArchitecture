#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include<stdint.h>
#include <optional>

enum Opcode {
	IAdd, IAnd, IOr, IXor, ISlt,
	ILsl, ILsr,
	Add, Sub, And, Or, Xor, Slt,
	Lsl, Lsr,
	Jmp, Jlr, Rtl,
	Beq, Bne, Blt, Bge,
	Lda, Sta,
	Mul, Div, Rem
};

namespace assembler {
	template<class A, class B>
	class MapBuilder {
	public:

		MapBuilder<A, B>& operator()(A a, B b) {
			m[a] = b;
			return *this;
		}

		operator std::unordered_map<A, B>() { return m; }
	private:
		std::unordered_map<A, B> m;
	};
	const std::unordered_map<std::string, Opcode> opMappings = MapBuilder<std::string, Opcode>()
		("addi", IAdd)("andi", IAnd)("ori", IOr)("xori", IXor)("stli", ISlt)("lsli", ILsl)("lsri", ILsr)
		("add", Add)("sub", Sub)("and", And)("or", Or)("xor", Xor)("stl", Slt)("lsl", Lsl)("lsr", Lsr)
		("jmp", Jmp)("jlr", Jlr)("beq", Beq)("bne", Bne)("blt", Blt)("bge", Bge)("lda", Lda)("sta", Sta)
		("mul", Mul)("div", Div)("rem", Rem)("rtl", Rtl)
		;
}

using word = int32_t;

struct Instruction {
	Opcode operation;
	word source1, source2, destination;

	Instruction(Opcode op) :
		operation(op)
	{
		source1 = source2 = destination = 0;
	}

	std::string toString() {
		std::string s = "";

		for (auto& [a, b] : assembler::opMappings) {
			if (b == operation)
				s += a;
		}

		s += "\tdest: " + std::to_string(destination) + "\tsrc1: " + std::to_string(source1) + "\tsrc2: " + std::to_string(source2);

		return s;
	}
};

namespace groups {
	const std::unordered_set<Opcode> immediates = { IAdd, IAnd, IOr, IXor, ISlt, ILsl, ILsr };
	const std::unordered_set<Opcode> simpleArithmetic = { IAdd, IAnd, IOr, IXor, ISlt, ILsl, ILsr, Add, Sub, And, Or, Xor, Slt, Lsl, Lsr};
	const std::unordered_set<Opcode> conditionalBranches = { Beq, Bne, Blt, Bge };
	const std::unordered_set<Opcode> jump = { Jmp, Jlr, Rtl};
	const std::unordered_set<Opcode> loads = { Lda };
	const std::unordered_set<Opcode> stores = { Sta };
	const std::unordered_set<Opcode> complexArithmetic = { Mul, Div, Rem };
	const std::unordered_set<Opcode> sourceAdders = { Jmp, Jlr };

	const std::unordered_map<std::string, std::string> originalMacros = assembler::MapBuilder<std::string, std::string>()
		("zero", "r0")("ra", "r1")("returnAddress", "r1")("sp", "r2")
		("stackPointer", "r2")("gp", "r3")("globalPointer", "r3")
		("tp", "r4")("threadPointer", "r4")("t0", "r5")("t1", "r6")
		("t2", "r7")("s0", "r8")("s1", "r8")("a0", "r9")("a1", "r10")
		("a2", "r11")("a3", "r12")("a4", "r13")("a5", "r14")("a6", "r15")
		("a7", "r16")("s2", "r18")("s3", "r19")("s4", "r20")("s5", "r21")
		("s6", "r22")("s7", "r23")("s8", "r24")("s9", "r25")("s10", "r26")
		("s11", "r27")("t3", "r28")("t4", "r29")("t5", "r30")("t6", "r31")
		;
}

namespace assembler {

	int parseOp(const std::unordered_map<std::string, int>& labels, const std::string& op, const std::unordered_map<std::string, std::string>& macros) {
		if (macros.count(op) > 0)
			return parseOp(labels, macros.at(op), macros);
		if (labels.count(op) > 0)
			return labels.at(op);
		if (op[0] == 'r')
			return std::atoi(op.substr(1).c_str());
		return std::atoi(op.c_str());
	}

	
	Instruction parseRegularOp(
		std::unordered_map<std::string, int>& labels,
		std::unordered_map<std::string, std::string>& macros,
		const std::vector<std::string>& splits, 
		int lineNumber, 
		const std::string& filename) {
		Instruction i(opMappings.at(splits[0]));

		if (splits.size() > 1) {
			i.destination = parseOp(labels, splits[1], macros);
			if (splits.size() > 2) {
				i.source1 = parseOp(labels, splits[2], macros);
				if (splits.size() > 3) {
					i.source2 = parseOp(labels, splits[3], macros);
					if (splits.size() > 4) {
						printf("Cannot parse file %s line %d; too many things (from %s)\n", filename.c_str(), lineNumber, splits[4].c_str());
						throw(0);
					}
				}
			}
		}

		return i;
	}

	struct ParsedInstruction {
		std::vector<std::string> splits;
		std::string filename;
		int lineNumber;

		ParsedInstruction(std::vector<std::string>& splits, std::string& filename, int lineNumber):
			splits(splits),
			filename(filename),
			lineNumber(lineNumber)
		{}
	};

	std::vector<std::string> splitLine(const std::string& line) {
		std::vector<std::string> splits;
		if (line.size() == 0)
			return splits;
		int lastIndex = 0;
		for (size_t i = 0; i < line.size(); i++) {
			if (line[i] == ' ') {
				splits.emplace_back(line.substr(lastIndex, i - lastIndex));
				lastIndex = i + 1;
			}
		}
		splits.emplace_back(line.substr(lastIndex));

		return splits;
	}

	struct ParseMemory {
		word* memory;
		int currentIndex = 0;

		void write(word value) {
			memory[currentIndex] = value;
			currentIndex += 1;
		}
	};

	bool c_prettyPrint = false;

	void parseFile(std::vector<ParsedInstruction>& parsedInstructions, std::string filename, std::unordered_map<std::string, int>& labels, ParseMemory& memory) {
		std::ifstream file(filename);
		std::string line;
		int lineNum = -1;
		printf(":)\n");
		if (!file.is_open()) {
			printf("File %s is bad\n", filename.c_str());
		}
		while (std::getline(file, line)) {
			if(c_prettyPrint)
				printf("> %s\n", line.c_str());
			lineNum += 1;
			auto splits = splitLine(line);
			if (splits.size() == 0)continue;
			if (opMappings.count(splits[0]) > 0) {
				parsedInstructions.emplace_back(splits, filename, lineNum);
			}
			//Macro
			else if (splits[0][0] == '.') {
				if (splits[0] == ".label") {
					labels[splits[1]] = parsedInstructions.size();
				}
				else if (splits[0] == ".data") {
					labels[splits[1]] = memory.currentIndex;
					for (size_t i = 2; i < splits.size(); i++)
						memory.write(std::atoi(splits[i].c_str()));
				}
				else if (splits[0] == ".include") {
					parseFile(parsedInstructions, splits[1], labels, memory);
				}
				else {
					printf("Filename %s, line %d, Unknown macro '%s'\n", filename.c_str(), lineNum, line.c_str());
				}
			}
			else if (splits[0][0] == '#') { continue; }
			else {
				printf("Filename %s, line %d, Unknown operator '%s'\n", filename.c_str(), lineNum, line.c_str());
				throw(0);
			}
		}

	}


	struct CompileResult {
		std::vector<Instruction> instructions;
		word* memory;
		std::unordered_map<std::string, int> labels;
	};

	CompileResult compile(std::string filename, int memorySize) {
		std::vector<ParsedInstruction> parsedInstructions;
		std::unordered_map<std::string, std::string> macros = groups::originalMacros;
		CompileResult result;
		result.memory = new word[memorySize];
		ParseMemory mem;
		mem.memory = result.memory;
		mem.currentIndex = 0;

		parseFile(parsedInstructions, filename, result.labels, mem);

		for (auto& pi : parsedInstructions)
			result.instructions.emplace_back(parseRegularOp(result.labels, macros, pi.splits, pi.lineNumber, pi.filename));

		return result;
	}
}