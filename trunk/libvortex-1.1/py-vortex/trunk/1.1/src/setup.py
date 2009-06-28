from distutils.core import setup, Extension
setup(
    name="vortex", version="1.1", 
    ext_modules=[Extension("vortex", ["py_vortex_ctx.c", "py_vortex.c"])]
)
