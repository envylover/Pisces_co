#pragma once

#include <coroutine>
#include <string>
#include <functional>
#include <tuple>
#include <concepts>
#include <memory>
#include "p_basis.h"
namespace pisces
{
	using namespace std;
	struct emptyDerived {};
	template<
	    typename RTY,
		typename Derived = emptyDerived,
	    typename Co_tag = co_tag<>
	>
	requires RightTag<Co_tag> && default_initializable<emptyDerived>
	class task
	{
		Derived* pDeri = nullptr;
	public:
		struct promise_type;
		explicit task(std::coroutine_handle<promise_type> h):_handler(h) {
			pDeri = static_cast<Derived*>(this);
		}
		task() {}
	    //----------------------------------------------------
		


	public:
		struct promise_type
		{
			using co_handle = std::coroutine_handle<promise_type>;
			auto get_return_object() {
				return Derived{co_handle::from_promise(*this)};}
			auto initial_suspend() { 

				if constexpr(default_initialize<Derived>)
					pDeri->init();

				if constexpr (derived_from<Co_tag, initial_suspend_never_tag>)
					return suspend_never{};
				else
					return suspend_always{};
			}
			auto final_suspend() noexcept {

				if constexpr (default_final<Derived>)
					pDeri->co_final();

				if constexpr (derived_from<Co_tag, final_suspend_always_tag>)
					return suspend_always{};
				else
					return suspend_never{};
			}
			auto yield_value(RTY&& val) {
				_res = val;
				if constexpr (derived_from<Co_tag, yield_value_never_tag>)
					return suspend_never{};
				else
					return suspend_always();
			}
			//void return_void() {}
			void unhandled_exception() {
				if constexpr (default_except<Derived>)
					pDeri->co_except();
				else
					throw;
			}
			void return_value(RTY&& res)
			{
				_res = res;
			}
			decay_t<RTY>                            _res;
		};
		
		RTY get()
		{
			return _handler.promise()._res;
		}
		void resume()
		{
			if (_handler)
				_handler.resume();
		}
		operator bool() {
			return _handler.operator bool();
		}
		bool done() {
			if (_handler)
				return _handler.done();
		}
		void destory()
		{
			if (_handler)
				return _handler.destroy();
		}
		std::coroutine_handle<promise_type> _handler;


	};
	template<>
	class task<void>
	{
		template<typename FUNC, typename ...Arg>
		task(FUNC func, Arg&& ...x) requires std::invocable<FUNC, Arg&&...>
		{
			_func = std::bind(func, std::forward<Arg>(x)...);
		}
		task() {}
		//----------------------------------------------------

	public:
		struct promise_type
		{
			auto get_return_object() { return task{}; }
			std::suspend_never initial_suspend() { return {}; }
			std::suspend_never final_suspend() noexcept { return {}; }
			std::suspend_always yield_value() {
				return {};
			}
			void return_void() {}
			void unhandled_exception() {
				throw;
			}
		};


		function<void(void)>                _func;
		std::coroutine_handle<promise_type> _handler;
	};
	


	template<typename Derived,typename promise_type = void>
	class awaitalbe
	{
		Derived*            pS;
	public:
		awaitalbe() {
			pS = static_cast<Derived*>(this);
		}
		bool await_ready() { 
			if constexpr (exist_await_ready<Derived>)
				return pS->ready();
			 return false;
		}
		void await_suspend(std::coroutine_handle<promise_type> h) {
			if constexpr (exist_await_suspend<Derived>)
				pS->suspend(h);
			else
				h.resume();
		}
		auto await_resume() {
			if constexpr (exist_await_resume<Derived>)
				return pS->resume();
			else
				return;
		}
	};


}
