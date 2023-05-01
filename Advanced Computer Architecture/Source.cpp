#include "CPU.h"

const char* tab = "\t";
int main() {

	CPU myCpu(GlobalData::width, "fibonnaci.txt", new SimpleBranchPredictor(SimpleBranchPredictor::Mode::Always));


	int cycle = 0;
	while(myCpu(3) != 1){
		cycle += 1;
		myCpu.update();
		printf("\n\nCycle %d:\n", cycle);
		//myCpu.regPrint(tab);
		myCpu.printMemory(tab, 38, 42);
	}


	return 0;
}