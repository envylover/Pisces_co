#include "co.hpp"
#include <iostream>
#include "fectory.hpp"

using namespace pisces;
class Add
	:public awaitalbe<Add>
{
public:
	int resume() {
		return _a;
	}
	void suspend(auto h) {
		_a += 100;
		h.resume();
	}
public:
	Add(int a):_a(a){
	}
	int _a = 0;
	int add() {
		return 0;
	}
} ;
class Generate
	:public task<int, Generate>
{
public:
	using base = task<int, Generate>;
	Generate(std::coroutine_handle<base::promise_type> h) :base(h) {}
	Generate(){}
	class Iter 
	{
		Generate* pG;
	public:
		void operator++() {
			pG->resume();
		}
		const int& operator*() const {
			auto i = pG->get();
			return i;
		}
		bool operator==(std::default_sentinel_t) const {
			return !pG->operator bool() ||pG->done();
		}

		explicit Iter(Generate* p) :
			pG{ p }
		{}
	};

	Iter begin() {
		if (operator bool()) {
			resume();
		}
		return Iter{ this };
	}
	std::default_sentinel_t end() {
		return {};
	}
};

Generate range(int first, int last)
{
	while (first < last)
		co_yield first++;
}


#include <algorithm>
int main() {
	
	for (auto i : range(1, 10))
		cout << i << " ";
	cout << endl;
	return 0;
}
