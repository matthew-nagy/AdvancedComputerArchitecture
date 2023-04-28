#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include<stdint.h>

enum Opcode {
	IAdd, IAnd, IOr, IXor, ISlt,
	ILsl, ILsr,
	Add, Sub, And, Or, Xor, Slt,
	Lsl, Lsr,
	Jmp, Jlr,
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
		("mul", Mul)("div", Div)("rem", Rem)
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
	const std::unordered_set<Opcode> jump = { Jmp, Jlr };
	const std::unordered_set<Opcode> loads = { Lda };
	const std::unordered_set<Opcode> stores = { Sta };
	const std::unordered_set<Opcode> complexArithmetic = { Mul, Div, Rem };
}

namespace assembler {

	int parseOp(const std::unordered_map<std::string, int>& labels, const std::string& op) {
		if (labels.count(op) > 0)
			return labels.at(op);
		if (op[0] == 'r')
			return std::atoi(op.substr(1).c_str());
		return std::atoi(op.c_str());
	}

	
	Instruction parseRegularOp(std::unordered_map<std::string, int>& labels, const std::vector<std::string>& splits, int lineNumber, const std::string& filename) {
		Instruction i(opMappings.at(splits[0]));

		if (splits.size() > 1) {
			i.destination = parseOp(labels, splits[1]);
			if (splits.size() > 2) {
				i.source1 = parseOp(labels, splits[2]);
				if (splits.size() > 3) {
					i.source2 = parseOp(labels, splits[3]);
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
		CompileResult result;
		result.memory = new word[memorySize];
		ParseMemory mem;
		mem.memory = result.memory;
		mem.currentIndex = 0;

		parseFile(parsedInstructions, filename, result.labels, mem);

		for (auto& pi : parsedInstructions)
			result.instructions.emplace_back(parseRegularOp(result.labels, pi.splits, pi.lineNumber, pi.filename));

		return result;
	}
}