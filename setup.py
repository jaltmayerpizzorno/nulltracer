from distutils.core import setup, Extension

setup(name = 'nulltracer',
      version = '1.0',
      description = 'Test null tracer',
      packages=['nulltracer'],
      ext_modules = [
          Extension('nulltracer.nulltracer', sources = ['nulltracer.cxx'],
                    extra_compile_args=['-std=c++17'])
      ])
