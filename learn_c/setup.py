from setuptools import setup, Extension
setup(name="custom2", version="1.0",
      ext_modules=[Extension("custom2", ["custom2.c"])])