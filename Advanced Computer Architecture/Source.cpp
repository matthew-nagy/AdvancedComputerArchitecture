#include "CPU.h"

const char* tab = "\t";
const char* nothing = "";

void genericAckermann() {
	CPU myCpu(GlobalData::width, "Ackermann.txt", new TwoBitBranchPredictor(8));// new SimpleBranchPredictor(SimpleBranchPredictor::Mode::AlwaysForwards));


	int cycle = 0;
	while (myCpu(3) != 1) {
		cycle += 1;
		myCpu.update();
		printf("\n\nCycle %d:\n", cycle);

		//if ((cycle & 0b11111) == 0b11111)
		//	myCpu.printMemory(tab, 0, 10);

		//myCpu.regPrint(tab);
		//myCpu.printMemory(tab, 38, 42);
	}

	std::cout << "Commits: " << myCpu.commited << std::endl;
	std::cout << "Instructions per clock: " << float(myCpu.commited) / float(cycle) << std::endl;

	//myCpu.regPrint(nothing);
}

void runProgram(const std::string& filename, const std::vector<std::string>& arguments) {
	BranchPredictor* bp = new SimpleBranchPredictor(SimpleBranchPredictor::Mode::Always);
	std::unordered_map<std::string, BranchPredictor*> bpMap = assembler::MapBuilder<std::string, BranchPredictor*>()
		("Always", bp)("Never", new SimpleBranchPredictor(SimpleBranchPredictor::Mode::Never))("Forwards", new SimpleBranchPredictor(SimpleBranchPredictor::Mode::AlwaysForwards))
		("Backwards", new SimpleBranchPredictor(SimpleBranchPredictor::Mode::AlwaysBackwards))
		("1bit", new OneBitBranchPredictor(8))
		("2bit", new TwoBitBranchPredictor(8))
		;

	bool printIPC = false;
	bool instrumentFlushes = false;
	bool insrumentClogs = false;
	bool debugPrint = false;

	for (size_t i = 2; i < arguments.size(); i++) {
		if (arguments[i] == "-bp") {
			i++;
			bp = bpMap.at(arguments[i]);
		}
		else if (arguments[i] == "-ipc")
			printIPC = true;
		else if (arguments[i] == "-flushes")
			instrumentFlushes = true;
		else if (arguments[i] == "-clogs")
			insrumentClogs = true;
		else if (arguments[i] == "-d")
			debugPrint = true;
	}
	CPU myCPU(GlobalData::width, filename, bp);
	int cycle = 0;
	while (myCPU(3) != 1) {
		cycle += 1;
		myCPU.update();
		if (debugPrint)
			std::cout << "Cycle " << cycle << std::endl;
	}

	std::cout << myCPU.commited << " instructions finished in " << cycle << " cycles\n";
	if (printIPC)
		std::cout << "\tIPC was " << float(myCPU.commited) / float(cycle) << std::endl;
	if (instrumentFlushes)
		std::cout << "\tPipeline was flushed " << myCPU.flushes << " times\n";
}

int main() {
	bool running = true;
	while (running) {
		std::string userInput;
		std::cout << ">>: ";
		std::getline(std::cin, userInput);
		if (userInput == "quit")
			running = false;
		else {
			auto splits = assembler::splitLine(userInput);
			if (splits[0] == "config") {
				GlobalData::loadFrom(splits[1]);
				GlobalData::print();
			}
			else if (splits[0] == "run") {
				runProgram(splits[1], splits);
			}
			else if (splits[0] == "hardware")
				GlobalData::print();
		}
	}

	return 0;
}