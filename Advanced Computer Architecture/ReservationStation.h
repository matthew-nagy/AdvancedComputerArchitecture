#pragma once

#include "ExecutionUnit.h"

class ReservationStation {
public:
	void commonDataBus(word robIndex, word value) {
		for (auto& currentEntry : entries) {
			if (currentEntry.inputRobIndex1 == robIndex) {
				currentEntry.inputRobIndex1 = -1;
				currentEntry.sourceValue1 = value;
			}
			else if (currentEntry.inputRobIndex2 == robIndex) {
				currentEntry.inputRobIndex2 = -1;
				currentEntry.sourceValue2 = value;
			}
		}
	}

	bool hasRoom() {
		return entries.size() != capacity;
	}

	bool readyToExecute() {
		for (auto& currentEntry : entries)
			if (currentEntry.readyToExecute())
				return true;
		return false;
	}

	void executeOn(ExecutionUnit* eu) {
		for (auto currentEntry = entries.begin(); currentEntry != entries.end(); ++currentEntry) {
			if (currentEntry->readyToExecute()) {
				eu->place(*currentEntry);
				entries.erase(currentEntry);
				return;
			}
		}
	}

	void push(PipelineEntry entry) {
		entries.emplace_back(entry);
	}

	void flushEverything() {
		entries.clear();
	}
private:
	std::vector<PipelineEntry> entries;
	const int capacity;
};

