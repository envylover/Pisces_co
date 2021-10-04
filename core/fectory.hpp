//#pragma once
//
//#pragma once
//
//#include <coroutine>
//#include <string>
//#include <iostream>
//namespace pisces
//{
//	using namespace std;
//	class Object {
//	public:
//		virtual string to_string() = 0;
//	};
//
//	class PiscesFectory
//	{
//	public:
//		virtual bool createObject() = 0;
//	};
//
//
//	template<typename value_type>
//	class yeild
//	{
//		value_type  copy;
//	public:
//		yeild(value_type&& val) :copy(val) {}
//		template<typename FUNC>
//		void to(FUNC func) requires std::invocable<FUNC, value_type&&>
//		{
//			func(copy);
//		}
//	};
//	template<typename return_type>
//	class wait
//	{
//		function<auto(void)->return_type>           _func;
//	public:
//		template<typename FUNC, typename ...Arg>
//		wait(FUNC func, Arg&& ...x) 
//		requires std::invocable<FUNC, Arg&&...>
//		{
//			_func = std::bind(func, std::forward<Arg>(x)...);
//		}
//		return_type operator()()
//		{
//			return _func();
//		}
//	};
//
//};
//
//
//
