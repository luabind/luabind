// Copyright (c) 2005 Daniel Wallin

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

#define LUABIND_SOURCE

#include <luabind/config.hpp>
#include <luabind/exception_handler.hpp>
#include <stdexcept>

namespace luabind { namespace detail {

namespace
{
  exception_handler_base* handler_chain = 0;

  void push_exception_string(lua_State* L, char const* exception, char const* what)
  {
      lua_pushstring(L, exception);
      lua_pushstring(L, ": `");
      lua_pushstring(L, what);
      lua_pushstring(L, "'");
      lua_concat(L, 4);
  }
}

void exception_handler_base::try_next(lua_State* L) const
{
    if (next)
        next->handle(L);
    else
        throw;
}

LUABIND_DECL void handle_exceptions_aux(lua_State* L)
{
    try
    {
        if (handler_chain)
            handler_chain->handle(L);
        else
            throw;
    }
    catch (std::logic_error const& e)
    {
        push_exception_string(L, "std::logic_error", e.what());
    }
    catch (std::runtime_error const& e)
    {
        push_exception_string(L, "std::runtime_error", e.what());
    }
    catch (std::exception const& e)
    {
        push_exception_string(L, "std::exception", e.what());
    }
    catch (char const* str)
    {
        push_exception_string(L, "c-string", str);
    }
    catch (...)
    {
        lua_pushstring(L, "Unknown C++ exception");
    }
}

LUABIND_DECL void register_exception_handler(exception_handler_base* handler)
{
    if (!handler_chain) handler_chain = handler;
    else
    {
        exception_handler_base* p = handler_chain;

        for (; p->next; p = p->next);

        handler->next = 0;
        p->next = handler;
    }
}

}} // namespace luabind::detail

