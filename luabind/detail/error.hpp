// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef LUABIND_ERROR_HPP_INCLUDED
#define LUABIND_ERROR_HPP_INCLUDED

#include <exception>
#include <luabind/config.hpp>

namespace luabind
{

#ifndef LUABIND_NO_EXCEPTIONS

	// this exception usually means that the lua function you called
	// from C++ failed with an error code. You will have to
	// read the error code from the top of the lua stack
	// the reason why this exception class doesn't contain
	// the message itself is that std::string's copy constructor
	// may throw, if the copy constructor of an exception that is
	// being thrown throws another exception, terminate will be called
	// and the entire application is killed.
	struct error : public std::exception
	{
		virtual const char* what() const throw()
		{
			return "lua runtime error";
		}
	};

	// this exception is thrown when you call a lua function
	// from C++, and the return value on the lua stack cannot
	// be converted to the type you specified in the C++ call
	// (as a template parameter)
	class cant_convert_return_value: public std::exception
	{
	public:

		virtual const char* what() const throw()
		{
			return "The return value from the called function cannot be converted by the given return value policy";
		}

	};

	// if a object_cast<>() fails, this is thrown
	class cast_failed : public std::exception
	{
	public:
		virtual const char* what() const throw() { return "unable to make cast"; }
	};

#else

	typedef void(*error_callback_fun)(lua_State*);
	typedef void(*cast_failed_callback_fun)(lua_State*);

	namespace detail
	{
		// we use a singleton instead of a global variable
		// because we want luabind to be headers only

		struct error_callback
		{
			error_callback_fun err;
			cast_failed_callback_fun cast;			
			
			error_callback(): err(0), cast(0) {}

			static error_callback& get()
			{
				static error_callback instance;
				return instance;
			}
		};
	}

	inline void set_error_callback(error_callback_fun e)
	{
		detail::error_callback::get().err = e;
	}

	inline void set_cast_failed_callback(cast_failed_callback_fun c)
	{
		detail::error_callback::get().cast = c;
	}

#endif

}

#endif // LUABIND_ERROR_HPP_INCLUDED
