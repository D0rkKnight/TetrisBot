from setuptools import setup, Extension

module1 = Extension('tetrisCore',
                    sources = ['tetrisCore.c', 'tetrisCore_Search.c'])

setup (name = 'TetrisCore',
       version = '1.0',
       description = 'This is a test',
       ext_modules = [module1])