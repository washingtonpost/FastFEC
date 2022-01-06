"""
Setup script to install the FastFEC python package
"""

import os
import sys

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

# get current directory
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PARENT_DIR = os.path.dirname(CURRENT_DIR)

deps = [
    "src/buffer.c",
    "src/memory.c",
    "src/encoding.c",
    "src/csv.c",
    "src/writer.c",
    "src/fec.c",
    # Pcre files
    "deps/pcre/pcre_chartables.c",
    "deps/pcre/pcre_byte_order.c",
    "deps/pcre/pcre_compile.c",
    "deps/pcre/pcre_config.c",
    "deps/pcre/pcre_dfa_exec.c",
    "deps/pcre/pcre_exec.c",
    "deps/pcre/pcre_fullinfo.c",
    "deps/pcre/pcre_get.c",
    "deps/pcre/pcre_globals.c",
    "deps/pcre/pcre_jit_compile.c",
    "deps/pcre/pcre_maketables.c",
    "deps/pcre/pcre_newline.c",
    "deps/pcre/pcre_ord2utf8.c",
    "deps/pcre/pcre_refcount.c",
    "deps/pcre/pcre_string_utils.c",
    "deps/pcre/pcre_study.c",
    "deps/pcre/pcre_tables.c",
    "deps/pcre/pcre_ucd.c",
    "deps/pcre/pcre_valid_utf8.c",
    "deps/pcre/pcre_version.c",
    "deps/pcre/pcre_xclass.c",
]


def print_pass(p):
    print("PP", p)
    return p


fastfec_library = Extension(
    "fastfec_lib",
    print_pass([os.path.join(PARENT_DIR, i) for i in deps]),
    include_dirs=[os.path.join(PARENT_DIR, "deps/pcre")],
    runtime_library_dirs=[os.path.join(PARENT_DIR, "deps/pcre")],
)


class zig_build_ext(build_ext):
    def build_extensions(self):
        # Override the compiler executable to use zig
        zig_compiler = f"{sys.executable} -m ziglang cc"
        self.compiler.set_executable("preprocessor", zig_compiler)
        self.compiler.set_executable("compiler", zig_compiler)
        self.compiler.set_executable("compiler_so", zig_compiler)
        self.compiler.set_executable("compiler_cxx", zig_compiler)
        self.compiler.set_executable("compiler_cxx_so", zig_compiler)
        build_ext.build_extensions(self)


setup(
    name="FastFEC",
    version="0.0.5",
    description="An extremely fast FEC filing parser written in C",
    author="Washington Post News Engineering",
    license="MIT",
    url="https://github.com/washingtonpost/FastFEC",
    packages=["fastfec"],
    package_dir={"": "src"},
    ext_modules=[fastfec_library],
    cmdclass={"build_ext": zig_build_ext},
)
