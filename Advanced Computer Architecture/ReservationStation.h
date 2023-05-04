#pragma once

#include "ExecutionUnit.h"
#include "MattQueue.h"

class GenericReservationStation {
public:
	virtual void commonDataBus(word robIndex, word value) = 0;

	virtual bool hasRoom(Opcode) = 0;

	virtual bool readyToExecute() = 0;

	virtual void executeOn(ExecutionUnit* eu) = 0;

	virtual void push(PipelineEntry entry) = 0;

	virtual void flushEverything() = 0;

	virtual std::optional<word> getReturnAddress() = 0;
};

class ReservationStation final : public GenericReservationStation{
public:
	void commonDataBus(word robIndex, word value)final override {
		for (auto& currentEntry : entries) {
			currentEntry.commonDataBus(robIndex, value);
		}
	}

	bool hasRoom(Opcode)final override {
		return entries.size() != capacity;
	}

	bool readyToExecute()final override {
		for (auto& currentEntry : entries)
			if (currentEntry.readyToExecute())
				return true;
		return false;
	}

	void executeOn(ExecutionUnit* eu)final override {
		for (auto currentEntry = entries.begin(); currentEntry != entries.end(); ++currentEntry) {
			if (currentEntry->readyToExecute()) {
				eu->place(*currentEntry);
				entries.erase(currentEntry);
				return;
			}
		}
	}

	void push(PipelineEntry entry)final override {
		entries.emplace_back(entry);
	}

	void flushEverything()final override {
		entries.clear();
	}

	std::optional<word> getReturnAddress()final override {
		for (auto& e : entries)
			if (e.opcode == Jlr)
				return e.instructionAddress;
		return std::nullopt;
	}

	ReservationStation(int capacity) :
		capacity(capacity)
	{}
private:
	std::vector<PipelineEntry> entries;
	const int capacity;
};

class LoadStoreQueue : public GenericReservationStation{
public:
	void commonDataBus(word robIndex, word value)final override {
		for (auto& s : stores)
			s.first.commonDataBus(robIndex, value);
		for (auto& l : loads)
			l.first.commonDataBus(robIndex, value);
	}

	bool hasRoom(Opcode op)final override {
		if (groups::stores.count(op) > 0 && stores.size() < capacity)
			return true;
		else if (groups::loads.count(op) > 0 && loads.size() < capacity)
			return true;
		return false;
	}

	bool readyToExecute()final override {
		auto instruction = getExecutableInstruction();
		return instruction.first != nullptr;
	}

	void executeOn(ExecutionUnit* eu)final override {
		auto executable = getExecutableInstruction();
		eu->place((*executable.first)[executable.second].first);
		executable.first->erase(executable.first->begin() + executable.second);
	}

	void push(PipelineEntry entry)final override {
		if (groups::loads.count(entry.opcode) > 0)
			loads.push(std::pair(entry, nextIndex));
		else
			stores.push(std::pair(entry, nextIndex));

		nextIndex += 1;
		//This probably won't happen often
		if (nextIndex == INT_MAX) {
			int min = INT_MAX;
			for (auto& s : stores)if (s.second < min) min = s.second;
			for (auto& l : loads)if (l.second < min) min = l.second;
			int newMax = 0;


			for (auto& s : stores) {
				s.second -= min;
				if (s.second > newMax)
					newMax = s.second;
			}
			for (auto& l : loads) {
				l.second -= min;
				if (l.second > newMax)
					newMax = l.second;
			}

			nextIndex = newMax + 1;
		}
	}

	void flushEverything()final override {
		loads.clear();
		stores.clear();
	}

	std::optional<word> getReturnAddress() {
		return std::nullopt;
	}

	LoadStoreQueue(int capacity):
		nextIndex(0),
		capacity(capacity)
	{}
private:
	using lsQueue = MattQueue<std::pair<PipelineEntry, int>>;
	lsQueue loads;
	lsQueue stores;
	int nextIndex;
	const int capacity;

	std::pair<lsQueue*, int> getExecutableInstruction() {
		std::pair<lsQueue*, int> response(nullptr, -1);
		int index = 0;
		for (auto& s : stores) {
			if (s.first.readyToExecute()) {
				response = std::pair(&stores, index);
				break;
			}
			index += 1;
		}

		int earlisetStore = INT_MAX;
		if (stores.size() > 0)
			earlisetStore = stores.front().second;
		index = 0;
		for (auto& l : loads) {
			if (l.second > earlisetStore)break;
			if (l.first.readyToExecute()) {
				response = std::pair(&loads, index);
				break;
			}
			index += 1;
		}

		return response;
	}
};
