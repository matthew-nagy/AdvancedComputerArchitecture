#pragma once
#include<vector>

template<class T>
class MattQueue : public std::vector<T> {
public:
	void pop() {
		this->erase(this->begin());
	}
	void push(T value) {
		this->emplace_back(value);
	}
	void emplace(T value) {
		this->emplace_back(value);
	}
};