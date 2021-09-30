#pragma once
#pragma once
#include <coroutine>
#include <concepts>
namespace pisces
{
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

	struct co_tag_t{};

	//-------------------------------------------------------------
	struct initial_suspend_always_tag:co_tag_t{};
	struct initial_suspend_never_tag :co_tag_t{};
	struct final_suspend_never_tag   :co_tag_t{};
	struct final_suspend_always_tag  :co_tag_t{};
	struct yield_value_always_tag    :co_tag_t{};
	struct yield_value_never_tag     :co_tag_t{};
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
		||(derived_from<tag, yield_value_always_tag>
			&& derived_from<tag, yield_value_never_tag>);


	template<typename tag>
	concept RightTag = !mutex_tag_v<tag>;
}

