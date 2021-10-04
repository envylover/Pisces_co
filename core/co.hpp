#pragma once

#include <coroutine>

#include <tuple>
#include <concepts>
#include <memory>

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
		using co_handle = std::coroutine_handle<promise_type>;
		explicit task(std::coroutine_handle<promise_type>& h):_handler(h),
			_pInfo(new info(1,false), [](info* p) {
			delete p;
				}	
			)
		{
			if constexpr (same_as<Derived, emptyDerived>);
			else
				pDeri = static_cast<Derived*>(this);

		}

		explicit task(std::coroutine_handle<promise_type>&& h) :_handler(h),
			_pInfo(new info(1,false), [](info* p) {
			delete p;
				}
			)
		{
			if constexpr (same_as<Derived, emptyDerived>);
			else
				pDeri = static_cast<Derived*>(this);
		}

		task(task& other):_handler(other._handler),_pInfo(other._pInfo)
		{
			if constexpr (same_as<Derived, emptyDerived>);
			else
				pDeri = static_cast<Derived*>(this);
			_pInfo->ref_cnt++;
		}
		task(task&& other):_handler(std::move(other._handler)), _pInfo(std::move(other._pInfo))
		{
			other._handler = nullptr;
			if constexpr (same_as<Derived, emptyDerived>);
			else
				pDeri = static_cast<Derived*>(this);
		}
		task& operator = (task& other)
		{
			dec_cnt();
			_handler = other._handler();
			_pInfo = other._pInfo;
		}
		task& operator = (task&& other)
		{
			dec_cnt();
			_handler = std::move(other._handler());
			_pInfo = std::move(other._pInfo);
		};
		task() {

		}
	    //----------------------------------------------------
		
		~task() {
			pDeri = nullptr;
			dec_cnt();
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
			promise_type() = default;
			promise_type(promise_type&) = default;
			promise_type(promise_type&&) = default;
			promise_type& operator = (promise_type&) = default;
			promise_type& operator = (promise_type&&) = default;
			decay_t<RTY>                            _res;
		};
		void* address()
		{
			if (_pInfo)
				if (!_pInfo->expire)
					return _handler.address();
			return nullptr;
		}

		void from_address(void* addr)
		{
			if (addr)
			{
				dec_cnt();
				_handler = _handler.from_address(addr);
				
				_pInfo.reset(new info(1, false));
			}
		}

		RTY get()
		{
			if (!_pInfo->expire)
				return _handler.promise()._res;
			throw;
		}
		void resume()noexcept
		{
			if (!_pInfo->expire)
				_handler.resume();
		}
		operator bool()noexcept {
			if (_pInfo)
				if (!_pInfo->expire)
					return _handler.operator bool();
			return false;
		}
		bool done()noexcept {
			if (_pInfo)
				if (!_pInfo->expire)
					return _handler.done();
			return true;
		}
		void destory()
		{
			if (_pInfo)
			{
				if (!_pInfo->expire)
					_handler.destroy();
				_pInfo->expire = true;
				_pInfo->ref_cnt--;
				_handler = nullptr;
			}
			return;
		}


	private:
		void dec_cnt()
		{
			if (_pInfo)
			{
				_pInfo->ref_cnt--;
				if (_pInfo->ref_cnt <= 0 || _pInfo->expire)
				{
					destory();
				}
			}
		}
		struct info
		{
			int ref_cnt = 0;
			bool expire = false;
		};
		std::coroutine_handle<promise_type> _handler = nullptr;
		std::shared_ptr<info>               _pInfo = nullptr;

	};



	template<
		typename Derived,
		typename Co_tag
	>
	requires RightTag<Co_tag>&& default_initializable<emptyDerived>
	class task<void, Derived, Co_tag>
	{
		Derived* pDeri = nullptr;
	public:
		struct promise_type;
		using co_handle = std::coroutine_handle<promise_type>;
		explicit task(std::coroutine_handle<promise_type>& h) :_handler(h),
			_pInfo(new info(1, false), [](info* p) {
			delete p;
				}
			)
		{
			if constexpr (same_as<Derived, emptyDerived>);
			else
				pDeri = static_cast<Derived*>(this);

		}

		explicit task(std::coroutine_handle<promise_type>&& h) :_handler(h),
			_pInfo(new info(1, false), [](info* p) {
			delete p;
				}
			)
		{
			if constexpr (same_as<Derived, emptyDerived>);
			else
				pDeri = static_cast<Derived*>(this);
		}

		task(task& other) :_handler(other._handler), _pInfo(other._pInfo)
		{
			if constexpr (same_as<Derived, emptyDerived>);
			else
				pDeri = static_cast<Derived*>(this);
			_pInfo->ref_cnt++;
		}
		task(task&& other) :_handler(std::move(other._handler)), _pInfo(std::move(other._pInfo))
		{
			other._handler = nullptr;
			if constexpr (same_as<Derived, emptyDerived>);
			else
				pDeri = static_cast<Derived*>(this);
		}
		task& operator = (task& other)
		{
			dec_cnt();
			_handler = other._handler();
			_pInfo = other._pInfo;
		}
		task& operator = (task&& other)
		{
			dec_cnt();
			_handler = std::move(other._handler());
			_pInfo = std::move(other._pInfo);
		};
		task() {

		}
		//----------------------------------------------------

		~task() {
			pDeri = nullptr;
			dec_cnt();
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

			
			void return_void() {}
			void unhandled_exception() {
				if constexpr (default_except<Derived>)
				{
					if (pDeri)
						pDeri->co_except();
				}
				else
					throw;
			}
			promise_type() = default;
			promise_type(promise_type&) = default;
			promise_type(promise_type&&) = default;
			promise_type& operator = (promise_type&) = default;
			promise_type& operator = (promise_type&&) = default;
		};
		void* address()
		{
			if (_pInfo)
				if (!_pInfo->expire)
					return _handler.address();
			return nullptr;
		}

		void from_address(void* addr)
		{
			if (addr)
			{
				dec_cnt();
				_handler = _handler.from_address(addr);

				_pInfo.reset(new info(1, false));
			}
		}
		void resume()noexcept
		{
			if (!_pInfo->expire)
				_handler.resume();
		}
		operator bool()noexcept {
			if (_pInfo)
				if (!_pInfo->expire)
					return _handler.operator bool();
			return false;
		}
		bool done()noexcept {
			if (_pInfo)
				if (!_pInfo->expire)
					return _handler.done();
			return true;
		}
		void destory()
		{
			if (_pInfo)
			{
				if (!_pInfo->expire)
					_handler.destroy();
				_pInfo->expire = true;
				_pInfo->ref_cnt--;
				_handler = nullptr;
			}
			return;
		}


	private:
		void dec_cnt()
		{
			if (_pInfo)
			{
				_pInfo->ref_cnt--;
				if (_pInfo->ref_cnt <= 0 || _pInfo->expire)
				{
					destory();
				}
			}
		}
		struct info
		{
			int ref_cnt = 0;
			bool expire = false;
		};
		std::coroutine_handle<promise_type> _handler = nullptr;
		std::shared_ptr<info>               _pInfo = nullptr;

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
