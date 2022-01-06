"""
Setup script to install the FastFEC python package
"""

from glob import glob
import os
import sys
import subprocess

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

# get current directory
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PARENT_DIR = os.path.dirname(CURRENT_DIR)

def compile_library():
    print("COMPILING LIBRARY")
    subprocess.call([sys.executable, "-m", "ziglang", "build", "-Dlib-only=true"], cwd=PARENT_DIR)

compile_library()
library_dir = os.path.join(PARENT_DIR, 'zig-out/lib')
# Collect compiled library files (extension .dylib|.so|.dll)
library_files = glob(os.path.join(library_dir, '*.dylib')) + glob(os.path.join(library_dir, '*.so')) + glob(os.path.join(library_dir, '*.dll'))

setup(
    name="FastFEC",
    version="0.0.5",
    description="An extremely fast FEC filing parser written in C",
    author="Washington Post News Engineering",
    license="MIT",
    url="https://github.com/washingtonpost/FastFEC",
    packages=["fastfec"],
    data_files=[('.', library_files)],
    package_dir={"": "src"},
)
