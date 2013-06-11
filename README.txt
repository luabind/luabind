Luabind
=======

Luabind is a library that helps you create bindings between C++ and Lua. It has the ability to expose functions and classes, written in C++, to Lua. It will also supply the functionality to define classes in lua and let them derive from other lua classes or C++ classes. Lua classes can override virtual functions from their C++ baseclasses. It is written towards Lua 5.x, and does not work with Lua 4.

It is implemented utilizing template meta programming. That means that you don't need an extra preprocess pass to compile your project (it is done by the compiler). It also means you don't (usually) have to know the exact signature of each function you register, since the library will generate code depending on the compile-time type of the function (which includes the signature). The main drawback of this approach is that the compilation time will increase for the file that does the registration, it is therefore recommended that you register everything in the same cpp-file.

Luabind is released under the terms of the [MIT license][1].

> Copyright Daniel Wallin, Arvid Norberg 2003.
> Extracted from <http://www.rasterbar.com/products/luabind/docs.html>

[1]: http://www.opensource.org/licenses/mit-license.php


This fork
---------

I forked the project since it seems abandoned (latest commit from January 2012
on the 0.9 branch) and I ran into certain bugs which needed fixing (see
commits).

If you know another fork which fixes this bugs or in which this fork should be
merged for other reasons, please let me know.
