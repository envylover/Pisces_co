#pragma once

#include <coroutine>

#include <tuple>
#include <concepts>


namespace pisces
{

	struct emptyDerived {};

	using namespace std;
	template<typename T>
	concept exist_await_ready = requires (T && t) {
		t.ready();
		requires same_as<decltype(t.ready()), bool>;
	};

	template<typename T, typename promise_type = void>
	concept exist_await_suspend = requires (T && t, std::coroutine_handle<promise_type> h) {
		t.suspend(h);
		requires same_as<decltype(t.suspend(h)), void>;
	};

	template<typename T>
	concept exist_await_resume = requires (T && t) {
		t.resume();
		//requires same_as<decltype(t.resume()), void>;
	};

	template<class Ty>
	concept default_initialize = requires (Ty && t) {
		t.co_init();
	};
	template<class Ty>
	concept default_final = requires (Ty && t) {
		t.co_final();
	};

	template<class Ty>
	concept default_except = requires (Ty && t) {
		t.co_except();
	};

	struct co_tag_t {};

	//-------------------------------------------------------------
	struct initial_suspend_always_tag :co_tag_t {};
	struct initial_suspend_never_tag :co_tag_t {};
	struct final_suspend_never_tag :co_tag_t {};
	struct final_suspend_always_tag :co_tag_t {};
	struct yield_value_always_tag :co_tag_t {};
	struct yield_value_never_tag :co_tag_t {};
	//-------------------------------------------------------------


	template<typename...tags>
	struct co_tag :tags...{};

	template<>
	struct co_tag<> :initial_suspend_always_tag, final_suspend_always_tag, yield_value_always_tag {};

	template<typename tag>
	constexpr bool mutex_tag_v = (derived_from<tag, initial_suspend_always_tag>
		&& derived_from<tag, initial_suspend_never_tag>)
		|| (derived_from<tag, final_suspend_never_tag>
			&& derived_from<tag, final_suspend_always_tag>)
		|| (derived_from<tag, yield_value_always_tag>
			&& derived_from<tag, yield_value_never_tag>);


	template<typename tag>
	concept RightTag = !mutex_tag_v<tag>;
}


namespace pisces
{
	using namespace std;

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
		explicit task(std::coroutine_handle<promise_type>& h):_handler(h) {
			if constexpr (same_as<Derived, emptyDerived>);
			else
				pDeri = static_cast<Derived*>(this);
		}

		explicit task(std::coroutine_handle<promise_type>&& h) :_handler(h) {
			if constexpr (same_as<Derived, emptyDerived>);
			else
				pDeri = static_cast<Derived*>(this);
		}

		task(task& other)
		{
			task(other._handler);
		}
		task(task&& other)
		{
			task(std::move(other._handler));
		}
		task() {}
	    //----------------------------------------------------
		
		~task() {
			pDeri = nullptr;
			if (_handler)
				_handler.destroy();
			_handler = nullptr;
		}

	public:
		struct promise_type
		{
			using co_handle = std::coroutine_handle<promise_type>;
			auto get_return_object() {
				if constexpr (same_as<Derived, emptyDerived>)
					return task{ co_handle::from_promise(*this) };
				else
					return Derived{ co_handle::from_promise(*this) };
			}
			auto initial_suspend() { 

				if constexpr (default_initialize<Derived>)
				{
					if (pDeri)
						pDeri->init();
				}

				if constexpr (derived_from<Co_tag, initial_suspend_never_tag>)
					return suspend_never{};
				else
					return suspend_always{};
			}
			auto final_suspend() noexcept {

				if constexpr (default_final<Derived>)
				{
					if (pDeri)
						pDeri->co_final();
				}
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
			auto yield_value(RTY& val) {
				_res = val;
				if constexpr (derived_from<Co_tag, yield_value_never_tag>)
					return suspend_never{};
				else
					return suspend_always();
			}
			//void return_void() {}
			void unhandled_exception() {
				if constexpr (default_except<Derived>)
				{
					if (pDeri)
						pDeri->co_except();
				}
				else
					throw;
			}
			void return_value(RTY&& res)
			{
				_res = res;
			}
			decay_t<RTY>                            _res;
		};
		void* address()
		{
			if (_handler)
				return _handler.address();
			return nullptr;
		}

		void from_address(void* addr)
		{
			if (addr)
			{
				if (_handler)
					_handler.destroy();
				_handler = _handler.from_address(addr);
			}

		}

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
			return true;
		}
		void destory()
		{
			if (_handler)
				 _handler.destroy();
			_handler = nullptr;
			return;
		}


	private:
		std::coroutine_handle<promise_type> _handler;


	};
	template<>
	class task<void>
	{
		
	public:
		task() {}
		//----------------------------------------------------
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
