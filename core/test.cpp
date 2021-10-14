
#include "co.hpp"
#include <iostream>
#include "fectory.hpp"
#include <thread>

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
	void co_init() {
		cout << "init()" << endl;
	}
	class Iter 
	{
		Generate* pG;
	public:
		void operator++() {
			pG->resume();
		}
		const A operator*() const {
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
		co_yield co_await Add(beg++);
	
}
task<void> add100(int a)
{
	cout << co_await Add(a);
}

class _A
{

};
class B :public _A
{};
class C :public B
{
	
};



#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>
int main() {
	//for (auto i : range(1, 10))
	//	cout << i.val << " ";
	//cout << endl;
	auto fun = [](int beg,int end)->task<int,emptyDerived,initial_suspend_never_tag> {
		while (beg < end)
			co_yield beg++;
	};
	auto co = fun(0, 20);
	std::mutex mx;
	for (int i = 0; i < 20; ++i)
	{
		thread t([&]() {
			while (true)
			{
				std::lock_guard lck(mx);
				if (!co.done())
				{
					cout << "threadID: "<< this_thread::get_id() << " get: " << co.get() << endl;
					co.resume();
				}
				else{
					if(co)
						co.destory();
					break;
				}
			}
			}); 
		t.detach();
	}
	std::this_thread::sleep_for(1s);
	return 0;
}
