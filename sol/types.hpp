// The MIT License (MIT)

// Copyright (c) 2013-2017 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SOL_TYPES_HPP
#define SOL_TYPES_HPP

#include "error.hpp"
#include "optional.hpp"
#include "compatibility.hpp"
#include "forward.hpp"
#include "forward_detail.hpp"
#include "traits.hpp"
#include "string_view.hpp"
#include "raii.hpp"
#include "filters.hpp"

#include <array>
#include <initializer_list>
#include <string>
#ifdef SOL_CXX17_FEATURES
#include <string_view>
#include <variant>
#endif // C++17

namespace sol {
	namespace detail {
#ifdef SOL_NOEXCEPT_FUNCTION_TYPE
		typedef int (*lua_CFunction_noexcept)(lua_State* L) noexcept;
#endif // noexcept function type for lua_CFunction

#ifdef SOL_NO_EXCEPTIONS
		template <lua_CFunction f>
		int static_trampoline(lua_State* L) noexcept {
			return f(L);
		}

#ifdef SOL_NOEXCEPT_FUNCTION_TYPE
		template <lua_CFunction_noexcept f>
		int static_trampoline_noexcept(lua_State* L) noexcept {
			return f(L);
		}
#endif

		template <typename Fx, typename... Args>
		int trampoline(lua_State* L, Fx&& f, Args&&... args) noexcept {
			return f(L, std::forward<Args>(args)...);
		}

		inline int c_trampoline(lua_State* L, lua_CFunction f) noexcept {
			return trampoline(L, f);
		}
#else
		template <lua_CFunction f>
		int static_trampoline(lua_State* L) {
#if defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) && !defined(SOL_LUAJIT)
			return f(L);

#else
			try {
				return f(L);
			}
			catch (const char* cs) {
				lua_pushstring(L, cs);
			}
			catch (const std::string& s) {
				lua_pushlstring(L, s.c_str(), s.size());
			}
			catch (const std::exception& e) {
				lua_pushstring(L, e.what());
			}
#if !defined(SOL_EXCEPTIONS_SAFE_PROPAGATION)
			// LuaJIT cannot have the catchall when the safe propagation is on
			// but LuaJIT will swallow all C++ errors 
			// if we don't at least catch std::exception ones
			catch (...) {
				lua_pushstring(L, "caught (...) exception");
			}
#endif // LuaJIT cannot have the catchall, but we must catch std::exceps for it
			return lua_error(L);
#endif // Safe exceptions
		}

#ifdef SOL_NOEXCEPT_FUNCTION_TYPE
#if 0 
		// impossible: g++/clang++ choke as they think this function is ambiguous:
		// to fix, wait for template <auto X> and then switch on no-exceptness of the function
		template <lua_CFunction_noexcept f>
		int static_trampoline(lua_State* L) noexcept {
			return f(L);
		}
#else
		template <lua_CFunction_noexcept f>
		int static_trampoline_noexcept(lua_State* L) noexcept {
			return f(L);
		}
#endif // impossible

#else
		template <lua_CFunction f>
		int static_trampoline_noexcept(lua_State* L) noexcept {
			return f(L);
		}
#endif // noexcept lua_CFunction type

		template <typename Fx, typename... Args>
		int trampoline(lua_State* L, Fx&& f, Args&&... args) {
			if (meta::bind_traits<meta::unqualified_t<Fx>>::is_noexcept) {
				return f(L, std::forward<Args>(args)...);
			}
#if defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) && !defined(SOL_LUAJIT)
			return f(L, std::forward<Args>(args)...);
#else
			try {
				return f(L, std::forward<Args>(args)...);
			}
			catch (const char* s) {
				lua_pushstring(L, s);
			}
			catch (const std::exception& e) {
				lua_pushstring(L, e.what());
			}
#if !defined(SOL_EXCEPTIONS_SAFE_PROPAGATION)
			// LuaJIT cannot have the catchall when the safe propagation is on
			// but LuaJIT will swallow all C++ errors 
			// if we don't at least catch std::exception ones
			catch (...) {
				lua_pushstring(L, "caught (...) exception");
			}
#endif
			return lua_error(L);
#endif
		}

		inline int c_trampoline(lua_State* L, lua_CFunction f) {
			return trampoline(L, f);
		}
#endif // Exceptions vs. No Exceptions

		template <typename F, F fx>
		inline int typed_static_trampoline_raw(std::true_type, lua_State* L) {
			return static_trampoline_noexcept<fx>(L);
		}

		template <typename F, F fx>
		inline int typed_static_trampoline_raw(std::false_type, lua_State* L) {
			return static_trampoline<fx>(L);
		}

		template <typename F, F fx>
		inline int typed_static_trampoline(lua_State* L) {
			return typed_static_trampoline_raw<F, fx>(std::integral_constant<bool, meta::bind_traits<F>::is_noexcept>(), L);
		}

		template <typename T>
		struct unique_usertype {};

		template <typename T>
		struct implicit_wrapper {
			T& item;
			implicit_wrapper(T* item)
			: item(*item) {
			}
			implicit_wrapper(T& item)
			: item(item) {
			}
			operator T&() {
				return item;
			}
			operator T*() {
				return std::addressof(item);
			}
		};

		struct unchecked_t {};
		const unchecked_t unchecked = unchecked_t{};
	} // namespace detail

	struct lua_nil_t {};
	const lua_nil_t lua_nil{};
	inline bool operator==(lua_nil_t, lua_nil_t) {
		return true;
	}
	inline bool operator!=(lua_nil_t, lua_nil_t) {
		return false;
	}
	typedef lua_nil_t nil_t;
#if !defined(SOL_NO_NIL)
	const nil_t nil{};
#endif

	struct metatable_t {};
	const metatable_t metatable_key = {};

	struct env_t {};
	const env_t env_key = {};

	struct no_metatable_t {};
	const no_metatable_t no_metatable = {};

	typedef std::remove_pointer_t<lua_CFunction> lua_CFunction_ref;

	template <typename T>
	struct unique_usertype_traits {
		typedef T type;
		typedef T actual_type;
		static const bool value = false;

		template <typename U>
		static bool is_null(U&&) {
			return false;
		}

		template <typename U>
		static auto get(U&& value) {
			return std::addressof(detail::deref(value));
		}
	};

	template <typename T>
	struct unique_usertype_traits<std::shared_ptr<T>> {
		typedef T type;
		typedef std::shared_ptr<T> actual_type;
		static const bool value = true;

		static bool is_null(const actual_type& p) {
			return p == nullptr;
		}

		static type* get(const actual_type& p) {
			return p.get();
		}
	};

	template <typename T, typename D>
	struct unique_usertype_traits<std::unique_ptr<T, D>> {
		typedef T type;
		typedef std::unique_ptr<T, D> actual_type;
		static const bool value = true;

		static bool is_null(const actual_type& p) {
			return p == nullptr;
		}

		static type* get(const actual_type& p) {
			return p.get();
		}
	};

	template <typename T>
	struct non_null {};

	template <typename... Args>
	struct function_sig {};

	struct upvalue_index {
		int index;
		upvalue_index(int idx)
		: index(lua_upvalueindex(idx)) {
		}

		operator int() const {
			return index;
		}
	};

	struct raw_index {
		int index;
		raw_index(int i)
		: index(i) {
		}

		operator int() const {
			return index;
		}
	};

	struct absolute_index {
		int index;
		absolute_index(lua_State* L, int idx)
		: index(lua_absindex(L, idx)) {
		}

		operator int() const {
			return index;
		}
	};

	struct ref_index {
		int index;
		ref_index(int idx)
		: index(idx) {
		}

		operator int() const {
			return index;
		}
	};

	struct stack_count {
		int count;

		stack_count(int cnt)
		: count(cnt) {
		}
	};

	struct lightuserdata_value {
		void* value;
		lightuserdata_value(void* data)
		: value(data) {
		}
		operator void*() const {
			return value;
		}
	};

	struct userdata_value {
		void* value;
		userdata_value(void* data)
		: value(data) {
		}
		operator void*() const {
			return value;
		}
	};

	template <typename L>
	struct light {
		L* value;

		light(L& x)
		: value(std::addressof(x)) {
		}
		light(L* x)
		: value(x) {
		}
		light(void* x)
		: value(static_cast<L*>(x)) {
		}
		operator L*() const {
			return value;
		}
		operator L&() const {
			return *value;
		}
	};

	template <typename T>
	auto make_light(T& l) {
		typedef meta::unwrapped_t<std::remove_pointer_t<std::remove_pointer_t<T>>> L;
		return light<L>(l);
	}

	template <typename U>
	struct user {
		U value;

		user(U x)
		: value(std::forward<U>(x)) {
		}
		operator std::add_pointer_t<std::remove_reference_t<U>>() {
			return std::addressof(value);
		}
		operator std::add_lvalue_reference_t<U>() {
			return value;
		}
		operator std::add_const_t<std::add_lvalue_reference_t<U>>&() const {
			return value;
		}
	};

	template <typename T>
	auto make_user(T&& u) {
		typedef meta::unwrapped_t<meta::unqualified_t<T>> U;
		return user<U>(std::forward<T>(u));
	}

	template <typename T>
	struct metatable_registry_key {
		T key;

		metatable_registry_key(T key)
		: key(std::forward<T>(key)) {
		}
	};

	template <typename T>
	auto meta_registry_key(T&& key) {
		typedef meta::unqualified_t<T> K;
		return metatable_registry_key<K>(std::forward<T>(key));
	}

	template <typename... Upvalues>
	struct closure {
		lua_CFunction c_function;
		std::tuple<Upvalues...> upvalues;
		closure(lua_CFunction f, Upvalues... targetupvalues)
		: c_function(f), upvalues(std::forward<Upvalues>(targetupvalues)...) {
		}
	};

	template <>
	struct closure<> {
		lua_CFunction c_function;
		int upvalues;
		closure(lua_CFunction f, int upvalue_count = 0)
		: c_function(f), upvalues(upvalue_count) {
		}
	};

	typedef closure<> c_closure;

	template <typename... Args>
	closure<Args...> make_closure(lua_CFunction f, Args&&... args) {
		return closure<Args...>(f, std::forward<Args>(args)...);
	}

	template <typename Sig, typename... Ps>
	struct function_arguments {
		std::tuple<Ps...> arguments;
		template <typename Arg, typename... Args, meta::disable<std::is_same<meta::unqualified_t<Arg>, function_arguments>> = meta::enabler>
		function_arguments(Arg&& arg, Args&&... args)
		: arguments(std::forward<Arg>(arg), std::forward<Args>(args)...) {
		}
	};

	template <typename Sig = function_sig<>, typename... Args>
	auto as_function(Args&&... args) {
		return function_arguments<Sig, std::decay_t<Args>...>(std::forward<Args>(args)...);
	}

	template <typename Sig = function_sig<>, typename... Args>
	auto as_function_reference(Args&&... args) {
		return function_arguments<Sig, Args...>(std::forward<Args>(args)...);
	}

	template <typename T>
	struct as_table_t {
		T source;

		as_table_t() = default;
		as_table_t(const as_table_t&) = default;
		as_table_t(as_table_t&&) = default;
		as_table_t& operator=(const as_table_t&) = default;
		as_table_t& operator=(as_table_t&&) = default;
		template <typename Arg, meta::enable<meta::neg<std::is_same<meta::unqualified_t<Arg>, as_table_t>>, meta::neg<std::is_base_of<proxy_base_tag, meta::unqualified_t<Arg>>>> = meta::enabler>
		as_table_t(Arg&& arg)
		: source(std::forward<Arg>(arg)) {
		}
		template <typename Arg0, typename Arg1, typename... Args>
		as_table_t(Arg0&& arg0, Arg1&& arg1, Args&&... args)
		: source(std::forward<Arg0>(arg0), std::forward<Arg1>(arg1), std::forward<Args>(args)...) {
		}

		operator std::add_lvalue_reference_t<T>() {
			return source;
		}
	};

	template <typename T>
	struct nested {
		T source;

		nested() = default;
		nested(const nested&) = default;
		nested(nested&&) = default;
		nested& operator=(const nested&) = default;
		nested& operator=(nested&&) = default;
		template <typename Arg, meta::enable<meta::neg<std::is_same<meta::unqualified_t<Arg>, nested>>, meta::neg<std::is_base_of<proxy_base_tag, meta::unqualified_t<Arg>>>> = meta::enabler>
		nested(Arg&& arg)
		: source(std::forward<Arg>(arg)) {
		}
		template <typename Arg0, typename Arg1, typename... Args>
		nested(Arg0&& arg0, Arg1&& arg1, Args&&... args)
		: source(std::forward<Arg0>(arg0), std::forward<Arg1>(arg1), std::forward<Args>(args)...) {
		}

		operator std::add_lvalue_reference_t<T>() {
			return source;
		}
	};

	template <typename T>
	as_table_t<T> as_table(T&& container) {
		return as_table_t<T>(std::forward<T>(container));
	}

	template <typename T>
	nested<T> as_nested(T&& container) {
		return nested<T>(std::forward<T>(container));
	}

	struct this_state {
		lua_State* L;

		this_state(lua_State* Ls)
		: L(Ls) {
		}

		operator lua_State*() const noexcept {
			return lua_state();
		}

		lua_State* operator->() const noexcept {
			return lua_state();
		}

		lua_State* lua_state() const noexcept {
			return L;
		}
	};

	struct this_main_state {
		lua_State* L;

		this_main_state(lua_State* Ls)
		: L(Ls) {
		}

		operator lua_State*() const noexcept {
			return lua_state();
		}

		lua_State* operator->() const noexcept {
			return lua_state();
		}

		lua_State* lua_state() const noexcept {
			return L;
		}
	};

	struct new_table {
		int sequence_hint = 0;
		int map_hint = 0;

		new_table() = default;
		new_table(const new_table&) = default;
		new_table(new_table&&) = default;
		new_table& operator=(const new_table&) = default;
		new_table& operator=(new_table&&) = default;

		new_table(int sequence_hint, int map_hint = 0)
		: sequence_hint(sequence_hint), map_hint(map_hint) {
		}
	};

	enum class call_syntax {
		dot = 0,
		colon = 1
	};

	enum class load_mode {
		any = 0,
		text = 1,
		binary = 2,
	};

	enum class call_status : int {
		ok = LUA_OK,
		yielded = LUA_YIELD,
		runtime = LUA_ERRRUN,
		memory = LUA_ERRMEM,
		handler = LUA_ERRERR,
		gc = LUA_ERRGCMM,
		syntax = LUA_ERRSYNTAX,
		file = LUA_ERRFILE,
	};

	enum class thread_status : int {
		ok = LUA_OK,
		yielded = LUA_YIELD,
		runtime = LUA_ERRRUN,
		memory = LUA_ERRMEM,
		gc = LUA_ERRGCMM,
		handler = LUA_ERRERR,
		dead = -1,
	};

	enum class load_status : int {
		ok = LUA_OK,
		syntax = LUA_ERRSYNTAX,
		memory = LUA_ERRMEM,
		gc = LUA_ERRGCMM,
		file = LUA_ERRFILE,
	};

	enum class type : int {
		none = LUA_TNONE,
		lua_nil = LUA_TNIL,
#if !defined(SOL_NO_NIL)
		nil = lua_nil,
#endif // Objective C/C++ Keyword that's found in OSX SDK and OBJC -- check for all forms to protect
		string = LUA_TSTRING,
		number = LUA_TNUMBER,
		thread = LUA_TTHREAD,
		boolean = LUA_TBOOLEAN,
		function = LUA_TFUNCTION,
		userdata = LUA_TUSERDATA,
		lightuserdata = LUA_TLIGHTUSERDATA,
		table = LUA_TTABLE,
		poly = none | lua_nil | string | number | thread | table | boolean | function | userdata | lightuserdata
	};

	inline const std::string& to_string(call_status c) {
		static const std::array<std::string, 8> names{ {
			"ok",
			"yielded",
			"runtime",
			"memory",
			"handler",
			"gc",
			"syntax",
			"file",
		} };
		switch (c) {
		case call_status::ok:
			return names[0];
		case call_status::yielded:
			return names[1];
		case call_status::runtime:
			return names[2];
		case call_status::memory:
			return names[3];
		case call_status::handler:
			return names[4];
		case call_status::gc:
			return names[5];
		case call_status::syntax:
			return names[6];
		case call_status::file:
			return names[7];
		}
		return names[0];
	}

	inline const std::string& to_string(load_status c) {
		static const std::array<std::string, 8> names{ {
			"ok",
			"memory",
			"gc",
			"syntax",
			"file",
		} };
		switch (c) {
		case load_status::ok:
			return names[0];
		case load_status::memory:
			return names[1];
		case load_status::gc:
			return names[2];
		case load_status::syntax:
			return names[3];
		case load_status::file:
			return names[4];
		}
		return names[0];
	}

	inline const std::string& to_string(load_mode c) {
		static const std::array<std::string, 3> names{ {
			"bt",
			"t",
			"b",
		} };
		return names[static_cast<std::size_t>(c)];
	}

	enum class meta_function {
		construct,
		index,
		new_index,
		mode,
		call,
		call_function = call,
		metatable,
		to_string,
		length,
		unary_minus,
		addition,
		subtraction,
		multiplication,
		division,
		modulus,
		power_of,
		involution = power_of,
		concatenation,
		equal_to,
		less_than,
		less_than_or_equal_to,
		garbage_collect,
		floor_division,
		bitwise_left_shift,
		bitwise_right_shift,
		bitwise_not,
		bitwise_and,
		bitwise_or,
		bitwise_xor,
		pairs,
		ipairs,
		next,
		type,
		type_info,
	};

	typedef meta_function meta_method;

	inline const std::array<std::string, 32>& meta_function_names() {
		static const std::array<std::string, 32> names = { { "new",
			"__index",
			"__newindex",
			"__mode",
			"__call",
			"__mt",
			"__tostring",
			"__len",
			"__unm",
			"__add",
			"__sub",
			"__mul",
			"__div",
			"__mod",
			"__pow",
			"__concat",
			"__eq",
			"__lt",
			"__le",
			"__gc",

			"__idiv",
			"__shl",
			"__shr",
			"__bnot",
			"__band",
			"__bor",
			"__bxor",

			"__pairs",
			"__ipairs",
			"__next",
			"__type",
			"__typeinfo" } };
		return names;
	}

	inline const std::string& to_string(meta_function mf) {
		return meta_function_names()[static_cast<int>(mf)];
	}

	inline type type_of(lua_State* L, int index) {
		return static_cast<type>(lua_type(L, index));
	}

	inline std::string type_name(lua_State* L, type t) {
		return lua_typename(L, static_cast<int>(t));
	}

	namespace detail {
		template <typename T>
		struct is_initializer_list : std::false_type {};

		template <typename T>
		struct is_initializer_list<std::initializer_list<T>> : std::true_type {};

		template <typename T, typename C = void>
		struct is_container : std::false_type {};

		template <typename T>
		struct is_container<std::initializer_list<T>> : std::false_type {};

		template <>
		struct is_container<std::string> : std::false_type {};

		template <>
		struct is_container<std::wstring> : std::false_type {};

		template <>
		struct is_container<std::u16string> : std::false_type {};

		template <>
		struct is_container<std::u32string> : std::false_type {};

#ifdef SOL_CXX17_FEATURES
		template <>
		struct is_container<std::string_view> : std::false_type {};

		template <>
		struct is_container<std::wstring_view> : std::false_type {};

		template <>
		struct is_container<std::u16string_view> : std::false_type {};

		template <>
		struct is_container<std::u32string_view> : std::false_type {};
#endif // C++ 17

		template <typename T>
		struct is_container<T,
			std::enable_if_t<meta::has_begin_end<meta::unqualified_t<T>>::value && !is_initializer_list<meta::unqualified_t<T>>::value>> : std::true_type {};

		template <typename T>
		struct is_container<T, std::enable_if_t<std::is_array<meta::unqualified_t<T>>::value && !meta::any_same<std::remove_all_extents_t<meta::unqualified_t<T>>, char, wchar_t, char16_t, char32_t>::value>> : std::true_type {};
	} // namespace detail

	template <typename T>
	struct is_container : detail::is_container<T> {};

	template <typename T>
	struct is_to_stringable : meta::any<meta::supports_to_string_member<meta::unqualified_t<T>>, meta::supports_adl_to_string<meta::unqualified_t<T>>, meta::supports_ostream_op<meta::unqualified_t<T>>> {};

	namespace detail {
		template <typename T, typename = void>
		struct lua_type_of : std::integral_constant<type, type::userdata> {};

		template <>
		struct lua_type_of<std::string> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<std::wstring> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<std::u16string> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<std::u32string> : std::integral_constant<type, type::string> {};

		template <std::size_t N>
		struct lua_type_of<char[N]> : std::integral_constant<type, type::string> {};

		template <std::size_t N>
		struct lua_type_of<wchar_t[N]> : std::integral_constant<type, type::string> {};

		template <std::size_t N>
		struct lua_type_of<char16_t[N]> : std::integral_constant<type, type::string> {};

		template <std::size_t N>
		struct lua_type_of<char32_t[N]> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<char> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<wchar_t> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<char16_t> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<char32_t> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<const char*> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<const char16_t*> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<const char32_t*> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<bool> : std::integral_constant<type, type::boolean> {};

		template <>
		struct lua_type_of<lua_nil_t> : std::integral_constant<type, type::lua_nil> {};

		template <>
		struct lua_type_of<nullopt_t> : std::integral_constant<type, type::lua_nil> {};

		template <>
		struct lua_type_of<std::nullptr_t> : std::integral_constant<type, type::lua_nil> {};

		template <>
		struct lua_type_of<error> : std::integral_constant<type, type::string> {};

		template <bool b, typename Base>
		struct lua_type_of<basic_table_core<b, Base>> : std::integral_constant<type, type::table> {};

		template <>
		struct lua_type_of<metatable_t> : std::integral_constant<type, type::table> {};

		template <typename B>
		struct lua_type_of<basic_environment<B>> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<env_t> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<new_table> : std::integral_constant<type, type::table> {};

		template <typename T>
		struct lua_type_of<as_table_t<T>> : std::integral_constant<type, type::table> {};

		template <typename T>
		struct lua_type_of<std::initializer_list<T>> : std::integral_constant<type, type::table> {};

		template <bool b>
		struct lua_type_of<basic_reference<b>> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<stack_reference> : std::integral_constant<type, type::poly> {};

		template <typename Base>
		struct lua_type_of<basic_object<Base>> : std::integral_constant<type, type::poly> {};

		template <typename... Args>
		struct lua_type_of<std::tuple<Args...>> : std::integral_constant<type, type::poly> {};

		template <typename A, typename B>
		struct lua_type_of<std::pair<A, B>> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<void*> : std::integral_constant<type, type::lightuserdata> {};

		template <>
		struct lua_type_of<lightuserdata_value> : std::integral_constant<type, type::lightuserdata> {};

		template <>
		struct lua_type_of<userdata_value> : std::integral_constant<type, type::userdata> {};

		template <typename T>
		struct lua_type_of<light<T>> : std::integral_constant<type, type::lightuserdata> {};

		template <typename T>
		struct lua_type_of<user<T>> : std::integral_constant<type, type::userdata> {};

		template <typename Base>
		struct lua_type_of<basic_lightuserdata<Base>> : std::integral_constant<type, type::lightuserdata> {};

		template <typename Base>
		struct lua_type_of<basic_userdata<Base>> : std::integral_constant<type, type::userdata> {};

		template <>
		struct lua_type_of<lua_CFunction> : std::integral_constant<type, type::function> {};

		template <>
		struct lua_type_of<std::remove_pointer_t<lua_CFunction>> : std::integral_constant<type, type::function> {};

		template <typename Base, bool aligned>
		struct lua_type_of<basic_function<Base, aligned>> : std::integral_constant<type, type::function> {};

		template <typename Base, bool aligned, typename Handler>
		struct lua_type_of<basic_protected_function<Base, aligned, Handler>> : std::integral_constant<type, type::function> {};

		template <typename Base>
		struct lua_type_of<basic_coroutine<Base>> : std::integral_constant<type, type::function> {};

		template <typename Base>
		struct lua_type_of<basic_thread<Base>> : std::integral_constant<type, type::thread> {};

		template <typename Signature>
		struct lua_type_of<std::function<Signature>> : std::integral_constant<type, type::function> {};

		template <typename T>
		struct lua_type_of<optional<T>> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<variadic_args> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<variadic_results> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<stack_count> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<this_state> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<this_main_state> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<this_environment> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<type> : std::integral_constant<type, type::poly> {};

		template <typename T>
		struct lua_type_of<T*> : std::integral_constant<type, type::userdata> {};

		template <typename T>
		struct lua_type_of<T, std::enable_if_t<std::is_arithmetic<T>::value>> : std::integral_constant<type, type::number> {};

		template <typename T>
		struct lua_type_of<T, std::enable_if_t<std::is_enum<T>::value>> : std::integral_constant<type, type::number> {};

		template <>
		struct lua_type_of<meta_function> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<string_view> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<wstring_view> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<u16string_view> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<u32string_view> : std::integral_constant<type, type::string> {};

#ifdef SOL_CXX17_FEATURES
		template <typename... Tn>
		struct lua_type_of<std::variant<Tn...>> : std::integral_constant<type, type::poly> {};
#endif // C++ 17 (or not) features

		template <typename T>
		struct lua_type_of<nested<T>, std::enable_if_t<::sol::is_container<T>::value>> : std::integral_constant<type, type::table> {};

		template <typename T>
		struct lua_type_of<nested<T>, std::enable_if_t<!::sol::is_container<T>::value>> : lua_type_of<T> {};

		template <typename C, C v, template <typename...> class V, typename... Args>
		struct accumulate : std::integral_constant<C, v> {};

		template <typename C, C v, template <typename...> class V, typename T, typename... Args>
		struct accumulate<C, v, V, T, Args...> : accumulate<C, v + V<T>::value, V, Args...> {};
	} // namespace detail

	template <typename T>
	struct is_unique_usertype : std::integral_constant<bool, unique_usertype_traits<T>::value> {};

	template <typename T>
	struct lua_type_of : detail::lua_type_of<T> {
		typedef int SOL_INTERNAL_UNSPECIALIZED_MARKER_;
	};

	template <typename T>
	struct lua_size : std::integral_constant<int, 1> {
		typedef int SOL_INTERNAL_UNSPECIALIZED_MARKER_;
	};

	template <typename A, typename B>
	struct lua_size<std::pair<A, B>> : std::integral_constant<int, lua_size<A>::value + lua_size<B>::value> {};

	template <typename... Args>
	struct lua_size<std::tuple<Args...>> : std::integral_constant<int, detail::accumulate<int, 0, lua_size, Args...>::value> {};

	namespace detail {
		template <typename...>
		struct void_ { typedef void type; };
		template <typename T, typename = void>
		struct has_internal_marker_impl : std::false_type {};
		template <typename T>
		struct has_internal_marker_impl<T, typename void_<typename T::SOL_INTERNAL_UNSPECIALIZED_MARKER_>::type> : std::true_type {};

		template <typename T>
		struct has_internal_marker : has_internal_marker_impl<T> {};
	} // namespace detail

	template <typename T>
	struct is_lua_primitive : std::integral_constant<bool,
							 type::userdata != lua_type_of<meta::unqualified_t<T>>::value
								 || ((type::userdata == lua_type_of<meta::unqualified_t<T>>::value)
									    && detail::has_internal_marker<lua_type_of<meta::unqualified_t<T>>>::value
									    && !detail::has_internal_marker<lua_size<meta::unqualified_t<T>>>::value)
								 || std::is_base_of<reference, meta::unqualified_t<T>>::value
								 || std::is_base_of<main_reference, meta::unqualified_t<T>>::value
								 || std::is_base_of<stack_reference, meta::unqualified_t<T>>::value
								 || meta::is_specialization_of<std::tuple, meta::unqualified_t<T>>::value
								 || meta::is_specialization_of<std::pair, meta::unqualified_t<T>>::value> {};

	template <typename T>
	struct is_lua_reference : std::integral_constant<bool,
							 std::is_base_of<reference, meta::unqualified_t<T>>::value
								 || std::is_base_of<main_reference, meta::unqualified_t<T>>::value
								 || std::is_base_of<stack_reference, meta::unqualified_t<T>>::value> {};

	template <typename T>
	struct is_lua_reference_or_proxy : std::integral_constant<bool,
									is_lua_reference<meta::unqualified_t<T>>::value
										|| meta::is_specialization_of<proxy, meta::unqualified_t<T>>::value> {};

	template <typename T>
	struct is_main_threaded : std::is_base_of<main_reference, T> {};

	template <typename T>
	struct is_stack_based : std::is_base_of<stack_reference, T> {};
	template <>
	struct is_stack_based<variadic_args> : std::true_type {};
	template <>
	struct is_stack_based<unsafe_function_result> : std::true_type {};
	template <>
	struct is_stack_based<protected_function_result> : std::true_type {};
	template <>
	struct is_stack_based<stack_proxy> : std::true_type {};
	template <>
	struct is_stack_based<stack_proxy_base> : std::true_type {};

	template <typename T>
	struct is_lua_primitive<T*> : std::true_type {};
	template <>
	struct is_lua_primitive<unsafe_function_result> : std::true_type {};
	template <>
	struct is_lua_primitive<protected_function_result> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<std::reference_wrapper<T>> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<user<T>> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<light<T>> : is_lua_primitive<T*> {};
	template <typename T>
	struct is_lua_primitive<optional<T>> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<as_table_t<T>> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<nested<T>> : std::true_type {};
	template <>
	struct is_lua_primitive<userdata_value> : std::true_type {};
	template <>
	struct is_lua_primitive<lightuserdata_value> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<non_null<T>> : is_lua_primitive<T*> {};

	template <typename T>
	struct is_proxy_primitive : is_lua_primitive<T> {};

	template <typename T>
	struct is_transparent_argument : std::false_type {};
	template <>
	struct is_transparent_argument<this_state> : std::true_type {};
	template <>
	struct is_transparent_argument<this_main_state> : std::true_type {};
	template <>
	struct is_transparent_argument<this_environment> : std::true_type {};
	template <>
	struct is_transparent_argument<variadic_args> : std::true_type {};
	template <typename T>
	struct is_variadic_arguments : std::is_same<meta::unqualified_t<T>, variadic_args> {};

	template <typename T>
	struct is_lua_index : std::is_integral<T> {};
	template <>
	struct is_lua_index<raw_index> : std::true_type {};
	template <>
	struct is_lua_index<absolute_index> : std::true_type {};
	template <>
	struct is_lua_index<ref_index> : std::true_type {};
	template <>
	struct is_lua_index<upvalue_index> : std::true_type {};

	template <typename Signature>
	struct lua_bind_traits : meta::bind_traits<Signature> {
	private:
		typedef meta::bind_traits<Signature> base_t;

	public:
		typedef std::integral_constant<bool, meta::count_for<is_variadic_arguments, typename base_t::args_list>::value != 0> runtime_variadics_t;
		static const std::size_t true_arity = base_t::arity;
		static const std::size_t arity = base_t::arity - meta::count_for<is_transparent_argument, typename base_t::args_list>::value;
		static const std::size_t true_free_arity = base_t::free_arity;
		static const std::size_t free_arity = base_t::free_arity - meta::count_for<is_transparent_argument, typename base_t::args_list>::value;
	};

	template <typename T>
	struct is_table : std::false_type {};
	template <bool x, typename T>
	struct is_table<basic_table_core<x, T>> : std::true_type {};

	template <typename T>
	struct is_function : std::false_type {};
	template <typename T, bool aligned>
	struct is_function<basic_function<T, aligned>> : std::true_type {};
	template <typename T, bool aligned, typename Handler>
	struct is_function<basic_protected_function<T, aligned, Handler>> : std::true_type {};

	template <typename T>
	struct is_lightuserdata : std::false_type {};
	template <typename T>
	struct is_lightuserdata<basic_lightuserdata<T>> : std::true_type {};

	template <typename T>
	struct is_userdata : std::false_type {};
	template <typename T>
	struct is_userdata<basic_userdata<T>> : std::true_type {};

	template <typename T>
	struct is_environment : std::integral_constant<bool, is_userdata<T>::value || is_table<T>::value> {};

	template <typename T>
	inline type type_of() {
		return lua_type_of<meta::unqualified_t<T>>::value;
	}

	namespace detail {
		template <typename T>
		struct is_non_factory_constructor : std::false_type {};

		template <typename... Args>
		struct is_non_factory_constructor<constructors<Args...>> : std::true_type {};

		template <typename... Args>
		struct is_non_factory_constructor<constructor_wrapper<Args...>> : std::true_type {};

		template <>
		struct is_non_factory_constructor<no_construction> : std::true_type {};

		template <typename T>
		struct is_constructor : is_non_factory_constructor<T> {};

		template <typename... Args>
		struct is_constructor<factory_wrapper<Args...>> : std::true_type {};

		template <typename T>
		struct is_constructor<protect_t<T>> : is_constructor<meta::unqualified_t<T>> {};

		template <typename F, typename... Filters>
		struct is_constructor<filter_wrapper<F, Filters...>> : is_constructor<meta::unqualified_t<F>> {};

		template <typename... Args>
		using has_constructor = meta::any<is_constructor<meta::unqualified_t<Args>>...>;

		template <typename T>
		struct is_destructor : std::false_type {};

		template <typename Fx>
		struct is_destructor<destructor_wrapper<Fx>> : std::true_type {};

		template <typename... Args>
		using has_destructor = meta::any<is_destructor<meta::unqualified_t<Args>>...>;

		struct add_destructor_tag {};
		struct check_destructor_tag {};
		struct verified_tag {
		} const verified{};
	} // namespace detail
} // namespace sol

#endif // SOL_TYPES_HPP
