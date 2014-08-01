#!/usr/bin/env python

import os

top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_cxx waf_unit_test')
    gr = opt.get_option_group('configure options')
    gr.add_option('--with-lua', action='store', help='path to lua', dest='LUA_PATH')
    gr.add_option('--with-boost', action='store', help='path to boost', dest='BOOST_PATH')

def configure(ctx):
    ctx.load('compiler_cxx waf_unit_test')
    ctx.env.LIB_LUA = ['lua']
    ctx.env.LIBPATH_LUA = os.path.join(ctx.options.LUA_PATH, 'lib')
    ctx.env.INCLUDES_LUA = os.path.join(ctx.options.LUA_PATH, 'include')
    ctx.env.LIB_BOOST = []
    ctx.env.LIBPATH_BOOST = os.path.join(ctx.options.BOOST_PATH, 'lib')
    ctx.env.INCLUDES_BOOST = os.path.join(ctx.options.BOOST_PATH, 'include')

def build(bld):
    ant_glob = bld.path.ant_glob

    bld.shlib(source=ant_glob(['src/*.cpp']),
              use=['LUA', 'BOOST'],
              includes=['.'],
              target='luabind')

    bld.objects(source='test/test_has_get_pointer.cpp', 
                use=['BOOST'],
                includes=['.'],
                target='test_has_get_pointer')

    for f in ant_glob(['test/test_*.cpp'],
                      excl=['test/test_has_get_pointer.cpp',
                            'test/test_value_wrapper.cpp']):
        bld.program(features='test',
                    source=[f, "test/main.cpp"],
                    use=['luabind', 'BOOST', 'test_has_get_pointer'],
                    lib=['dl'],
                    includes=['.'],
                    target=os.path.splitext(str(f))[0])

    bld.program(features='test',
                source=['test/test_value_wrapper.cpp'],
                use=['luabind', 'BOOST'],
                lib=['dl', 'boost_unit_test_framework'],
                includes=['.'],
                target='test_value_wrapper')

    from waflib.Tools import waf_unit_test
    bld.add_post_fun(waf_unit_test.summary)
