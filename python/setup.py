"""
Setup script to install the FastFEC python package
"""

from distutils.core import Extension
from setuptools import find_packages, setup

fastfec_library = Extension(
    "fastfec_lib",
    [
        "../src/buffer.c",
        "../src/memory.c",
        "../src/encoding.c",
        "../src/csv.c",
        "../src/writer.c",
        "../src/fec.c",
        # Pcre files
        "../deps/pcre/pcre_chartables.c",
        "../deps/pcre/pcre_byte_order.c",
        "../deps/pcre/pcre_compile.c",
        "../deps/pcre/pcre_config.c",
        "../deps/pcre/pcre_dfa_exec.c",
        "../deps/pcre/pcre_exec.c",
        "../deps/pcre/pcre_fullinfo.c",
        "../deps/pcre/pcre_get.c",
        "../deps/pcre/pcre_globals.c",
        "../deps/pcre/pcre_jit_compile.c",
        "../deps/pcre/pcre_maketables.c",
        "../deps/pcre/pcre_newline.c",
        "../deps/pcre/pcre_ord2utf8.c",
        "../deps/pcre/pcre_refcount.c",
        "../deps/pcre/pcre_string_utils.c",
        "../deps/pcre/pcre_study.c",
        "../deps/pcre/pcre_tables.c",
        "../deps/pcre/pcre_ucd.c",
        "../deps/pcre/pcre_valid_utf8.c",
        "../deps/pcre/pcre_version.c",
        "../deps/pcre/pcre_xclass.c",
    ],
    include_dirs=['../deps/pcre'],
    runtime_library_dirs=["../deps/pcre"],
)

setup(
    name='FastFEC',
    version='0.0.4',
    description='An extremely fast FEC filing parser written in C',
    author='Washington Post News Engineering',
    license='MIT',
    url='https://github.com/washingtonpost/FastFEC',
    packages=['fastfec'],
    package_dir={'': 'src'},
    ext_modules=[fastfec_library],
)
