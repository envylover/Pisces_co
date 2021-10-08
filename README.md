# Pisces_co  
只是对cpp20协程的简单封装，可以不需要重写某些函数即可实现功能

## 核心类
### 1.task  
```C++
template<typename RTY,typename Derived,typename Co_tag>
class task;
```
**RTY**为co_yield与co_return受支持的操作类型,暂时需要支持可复制，可移动，可构造操作<BR>
<br>
**Derived**当需要拓展task功能时<br>
```C++
template<typename T>
class Derived :public task<T,Derived>;
```
默认为emptyDerived  
<br>
对**Derived**约束：
<br>
<br>
- 需要实现构造函数<br>
```C++
Derived(std::corountine<task<RTY,Derived>::promise_type>& h):task<RTY,Derived>(h){};
```
使task顺利获取协程句柄。<br>
<br>
task在挂起，结束以及产生异常时，会分别调用Derived类的co_init,co_final,co_except方法，Derived按需实现.<br>  
声明形式如下：
```C++
void co_init(void);
void co_final(void);
void co_except(void);
```
<br>

## example
```C++
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
```
协程对象为Generate，规定当出现co_yield,co_return,co_await任意关键字时，该函数被识别为协程，返回类型为协程对象<br>
具体使用:<br>
```C++
Generate range(int first, int last)
{
	while (first < last)
		co_yield A(first++);
}
int main()
{
    for(auto i : range(1,10))
       cout << i.val << " ";
}
```
运行结果
```
1 2 3 4 5 6 7 8 9
```
**注意**
当协程对象复制多个时候，协程句柄只会有一个，请注意内存问题。
<br>    
### 模板参数Co_tag 
**作用**<br>
+ 控制协程行为      

协程在初始化，yield_value,结束时，都存在两种路径<br>
+ 始终挂起
+ 从不挂起  

决定其行为请使用模板co_tag<...>，参数为控制参数，共六种
```C++
struct initial_suspend_always_tag；//初始化始终挂起
struct initial_suspend_never_tag ；//初始化从不挂起
struct final_suspend_never_tag   ；//结束从不挂起
struct final_suspend_always_tag  ；//结束始终挂起
struct yield_value_always_tag    ；//yield_value始终挂起
struct yield_value_never_tag     ；//yield_value从不挂起
```
注意互斥条件  
Co_tag默认为
```C++
co_tag<initial_suspend_always_tag, final_suspend_always_tag, yield_value_always_tag>
```
### example
```C++
int main()
{
    auto fun = [](int beg,int end)->task<int,emptyDerived,initial_suspend_never_tag> {
		while (beg < end)
			co_yield beg++;
	};
	
	auto co = fun(0, 3);
	while (!co.done())
	{
		cout << co.get() << " ";
		co.resume();
	}
	co.destory();
}

```
输出为:
```C++
0 1 2
```
# 注意！！！  
协程对象复制多个时候，只会有一个协程句柄！！！co_final等三个方法只调用最后一个复制的对象的方法！  
### awaitable

用来支持co_await关键字，
直接看例子, 剩下的看代码吧
```C++
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
};

task<void> add(int a)
{
    int a = co_await Add(10);
    cout << a;
}

int main()
{
    add(10).resume();
}

```
输出：
```C++
110
```
