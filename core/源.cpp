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
struct A
{
	int val = 0;
};
class Generate
	:public task<A, Generate>
{
public:
	using base = task<A, Generate>;
	Generate(std::coroutine_handle<base::promise_type> h) :base(h) {}
	Generate(){}
	~Generate() {}
	class Iter 
	{
		Generate* pG;
	public:
		void operator++() {
			pG->resume();
		}
		const A& operator*() const {
			auto i = pG->get();
			return i;
		}
		bool operator==(std::default_sentinel_t) const {
			return !(*pG) || pG->done();
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
		co_yield A(first++);
}


task<int> getValue(int beg,int end)
{
	while (beg < end)
		co_yield beg++;
}


#include <algorithm>
#include <thread>
int main() {
	for (auto i : range(1, 10))
		cout << i.val << " ";
	cout << endl;
	auto a = getValue(1, 15);
	/*a.resume();
	std::thread t([&](){

		while (a && !a.done())
		{
			cout << a.get() << " ";
			a.resume();
		}
		a.destory();
	});
	t.join();*/
	auto b = a;
	auto d = std::move(a);
	a.destory();
	bool c = b;
	b.destory();
	bool cd = d;
	return 0;
}
