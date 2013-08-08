+++++++++++++++++++
 luabind |version|
+++++++++++++++++++

Introduction
============

Luabind is a library that helps you create bindings between C++ and Lua. It has
the ability to expose functions and classes, written in C++, to Lua. It will
also supply the functionality to define classes in Lua and let them derive from
other Lua classes or C++ classes. Lua classes can override virtual functions
from their C++ base classes. It is written towards Lua 5.0, and does not work
with Lua 4.

It is implemented utilizing template meta programming. That means that you
don't need an extra preprocess pass to compile your project (it is done by the
compiler). It also means you don't (usually) have to know the exact signature 
of each function you register, since the library will generate code depending 
on the compile-time type of the function (which includes the signature). The 
main drawback of this approach is that the compilation time will increase for 
the file that does the registration, it is therefore recommended that you 
register everything in the same cpp-file.

Luabind is released under the terms of the `MIT license`_.

We are very interested in hearing about projects that use luabind, please let
us know about your project.

The main channel for help and feedback is the `luabind mailing list`_.
There's also an IRC channel ``#luabind`` on irc.freenode.net.

.. _`luabind mailing list`: https://lists.sourceforge.net/lists/listinfo/luabind-user
.. _MIT license: http://www.opensource.org/licenses/mit-license.php
.. _Boost: http://www.boost.org


Features
========

Luabind supports:

 - Overloaded free functions 
 - C++ classes in Lua 
 - Overloaded member functions 
 - Operators 
 - Properties 
 - Enums 
 - Lua functions in C++ 
 - Lua classes in C++ 
 - Lua classes (single inheritance) 
 - Derives from Lua or C++ classes 
 - Override virtual functions from C++ classes 
 - Implicit casts between registered types 
 - Best match signature matching 
 - Return value policies and parameter policies 


Portability
===========

Luabind has been tested to work on the following compilers:

 - Visual Studio 7.1 
 - Intel C++ 6.0 (Windows) 
 - GCC 2.95.3 (cygwin) 
 - GCC 3.0.4 (Debian/Linux) 
 - GCC 3.1 (SunOS 5.8) 
 - GCC 3.2 (cygwin) 
 - GCC 3.3.1 (cygwin)
 - GCC 3.3 (Apple, MacOS X)
 - GCC 4.0 (Apple, MacOS X)

It has been confirmed not to work with:

 - GCC 2.95.2 (SunOS 5.8) 

Metrowerks 8.3 (Windows) compiles but fails the const-test. This 
means that const member functions are treated as non-const member 
functions.

If you have tried luabind with a compiler not listed here, let us know 
your result with it.
