// Copyright (c) 2012 Christopher Gassib

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

#ifndef LUABIND_STD_SHARED_PTR_120603_HPP
# define LUABIND_STD_SHARED_PTR_120603_HPP

//
// This is a work around for luabind which allows use of std:: shared
// pointers.  This code needs to be parsed before boost's
// <get_pointer.hpp> header. Because it depends on standard <memory>,
// it's probably desirable to make this an optional user include.
//

# include <memory>

namespace luabind { namespace detail { namespace has_get_pointer_ {

    template<typename T> inline T* get_pointer(const std::shared_ptr<T>& p)
    {
        return p.get();
    }

    template<typename T> inline T* get_pointer(std::shared_ptr<T>& p)
    {
        return p.get();
    }

} } } // namespace luabind::detail::has_get_pointer_

namespace boost {

    template<typename T> inline T* get_pointer(const std::shared_ptr<T>& p)
    {
        return p.get();
    }

    template<typename T> inline T* get_pointer(std::shared_ptr<T>& p)
    {
        return p.get();
    }

} // namespace boost

#endif // LUABIND_STD_SHARED_PTR_120603_HPP
