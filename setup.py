from setuptools import setup, Extension
import sysconfig
import os

# _DEBUG = False

# compile_args = []
# def_args = sysconfig.get_config_var('CFLAGS')
# if def_args is not None:
#        compile_args = compile_args = sysconfig.get_config_var('CFLAGS').split()

# if _DEBUG:
#        print("Compiling in debug mode")
#        # This somehow blocks compilation?
#        compile_args += ["/Z7"]
# else:
#        print("Compiling in optimized mode")
#        compile_args += ["/O2"]

# print("Extra compile args are: "+str(compile_args))

# module1 = Extension('tetrisCore',
#                     sources = ['tetrisCore.c', 'tetrisCore_Search.c'], extra_compile_args=compile_args)

module1 = Extension('tetrisCore',
                    sources = ['tetrisCore.c', 'tetrisCore_Search.c'])

setup (name = 'TetrisCore',
       version = '1.0',
       description = 'This is a test',
       ext_modules = [module1])