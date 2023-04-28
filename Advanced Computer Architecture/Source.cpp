#include "CPU.h"
int main() {

	auto res = assembler::compile("lilTestMain.txt", 1024);

	for (auto& i : res.instructions)
		printf("%s\n", i.toString().c_str());

	return 0;
}