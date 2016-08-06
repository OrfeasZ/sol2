// The MIT License (MIT) 

// Copyright (c) 2013-2016 Rapptz, ThePhD and contributors

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

#ifndef SOL_USERTYPE_HPP
#define SOL_USERTYPE_HPP

#include "stack.hpp"
#include "usertype_metatable.hpp"
#include "simple_usertype_metatable.hpp"
#include <memory>

namespace sol {

	class usertype_base {
	// TODO: Make this less awkward.
	public:
		std::shared_ptr<usertype_detail::registrar> metatableregister;

	public:
		virtual void push_functions(std::vector<luaL_Reg>& l, int& index) {}
	};

	template<typename T>
	class usertype : public usertype_base {
	private:
		template<typename... Args>
		usertype(usertype_detail::verified_tag, Args&&... args) { metatableregister = detail::make_shared<usertype_metatable<T, std::make_index_sequence<sizeof...(Args) / 2>, Args...>>(std::forward<Args>(args)...); }

		template<typename... Args>
		usertype(usertype_detail::add_destructor_tag, Args&&... args) : usertype(usertype_detail::verified, std::forward<Args>(args)..., "__gc", default_destructor) {}

		template<typename... Args>
		usertype(usertype_detail::check_destructor_tag, Args&&... args) : usertype(meta::condition<meta::all<std::is_destructible<T>, meta::neg<usertype_detail::has_destructor<Args...>>>, usertype_detail::add_destructor_tag, usertype_detail::verified_tag>(), std::forward<Args>(args)...) {}

	public:
		template<typename... Args>
		usertype(Args&&... args) : usertype(meta::condition<meta::all<std::is_default_constructible<T>, meta::neg<usertype_detail::has_constructor<Args...>>>, decltype(default_constructor), usertype_detail::check_destructor_tag>(), std::forward<Args>(args)...) {}

		template<typename... Args, typename... CArgs>
		usertype(constructors<CArgs...> constructorlist, Args&&... args) : usertype(usertype_detail::check_destructor_tag(), std::forward<Args>(args)..., "new", constructorlist) {}

		template<typename... Args, typename... Fxs>
		usertype(constructor_wrapper<Fxs...> constructorlist, Args&&... args) : usertype(usertype_detail::check_destructor_tag(), std::forward<Args>(args)..., "new", constructorlist) {}

		template<typename... Args>
		usertype(simple_tag, lua_State* L, Args&&... args) { metatableregister = detail::make_shared<simple_usertype_metatable<T>>(L, std::forward<Args>(args)...); }

		~usertype()
		{
			printf("Destroying usertype\n");
		}

		int push(lua_State* L) {
			return metatableregister->push_um(L);
		}

		void set_reference(reference* r) {
			metatableregister->set_reference(r);
		}

		virtual void push_functions(std::vector<luaL_Reg>& l, int& index) override {
			metatableregister->push_functions(l, index);
		}
	};

	namespace stack {
		template<typename T>
		struct pusher<usertype<T>> {
			static int push(lua_State* L, usertype<T>& user) {
				return user.push(L);
			}
		};
	} // stack
} // sol

#endif // SOL_USERTYPE_HPP
