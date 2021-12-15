"""
Setup script to install the FastFEC python package
"""

import os
from distutils.core import Extension

from setuptools import setup

# get current directory
CURRENT_DIR = os.getcwd()
PARENT_DIR = os.path.dirname(CURRENT_DIR)

fastfec_library = Extension(
    "fastfec_lib",
    [
        os.path.join(PARENT_DIR, "src/buffer.c"),
        os.path.join(PARENT_DIR, "src/memory.c"),
        os.path.join(PARENT_DIR, "src/encoding.c"),
        os.path.join(PARENT_DIR, "src/csv.c"),
        os.path.join(PARENT_DIR, "src/writer.c"),
        os.path.join(PARENT_DIR, "src/fec.c"),
        # Pcre files
        os.path.join(PARENT_DIR, "deps/pcre/pcre_chartables.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_byte_order.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_compile.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_config.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_dfa_exec.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_exec.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_fullinfo.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_get.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_globals.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_jit_compile.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_maketables.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_newline.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_ord2utf8.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_refcount.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_string_utils.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_study.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_tables.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_ucd.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_valid_utf8.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_version.c"),
        os.path.join(PARENT_DIR, "deps/pcre/pcre_xclass.c"),
    ],
    include_dirs=[os.path.join(PARENT_DIR, "deps/pcre")],
    runtime_library_dirs=[os.path.join(PARENT_DIR, "deps/pcre")],
)

setup(
    name="FastFEC",
    version="0.0.4",
    description="An extremely fast FEC filing parser written in C",
    author="Washington Post News Engineering",
    license="MIT",
    url="https://github.com/washingtonpost/FastFEC",
    packages=["fastfec"],
    package_dir={"": "src"},
    ext_modules=[fastfec_library],
)
