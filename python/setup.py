"""
Setup script to install the FastFEC python package
"""

import os
import shutil
import subprocess
import sys
from glob import glob

from setuptools import setup

# get current directory
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PARENT_DIR = os.path.dirname(CURRENT_DIR)
with open(os.path.join(PARENT_DIR, "README.md"), "r") as f:
    readme = f.read()
with open(os.path.join(PARENT_DIR, "VERSION"), "r") as f:
    version = f.read().strip()


def compile_library():
    subprocess.call(
        [sys.executable, "-m", "ziglang", "build", "-Dlib-only=true"], cwd=PARENT_DIR
    )


compile_library()
library_dir = os.path.join(PARENT_DIR, "zig-out/lib")
# Collect compiled library files (extension .dylib|.so|.dll)
raw_library_files = (
    glob(os.path.join(library_dir, "*.dylib"))
    + glob(os.path.join(library_dir, "*.so"))
    + glob(os.path.join(library_dir, "*.dll"))
)
# Copy them into the Python project src directory and get their relative paths
library_files = [
    os.path.basename(shutil.copy(library_file, os.path.join(CURRENT_DIR, "src")))
    for library_file in raw_library_files
]

# Force building a non-pure lib wheel
# From https://stackoverflow.com/a/45150383
try:
    from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

    class bdist_wheel(_bdist_wheel):
        def finalize_options(self):
            _bdist_wheel.finalize_options(self)
            self.root_is_pure = False

except ImportError:
    bdist_wheel = None

setup(
    name="fastfec",
    version=version,
    description="An extremely fast FEC filing parser written in C",
    long_description=readme,
    long_description_content_type="text/markdown",
    author="Washington Post News Engineering",
    license="MIT",
    url="https://github.com/washingtonpost/FastFEC",
    packages=["fastfec"],
    package_data={"fastfec": library_files},
    package_dir={"": "src"},
    cmdclass={"bdist_wheel": bdist_wheel},
)
