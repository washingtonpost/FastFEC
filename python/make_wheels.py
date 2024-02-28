"""
Build wheels for each platform possible via Zig cross-compilation.
Why not use cibuildwheel? Using a custom Zig build extension is not
easy to support within this system, and platform-specific bugs are
hard to debug. This system should provide the same benefits of the
release workflow, but for Python packaging.

Inspired and heavily copied from:
https://github.com/ziglang/zig-pypi/blob/main/make_wheels.py
"""

import os
import platform
import shutil
import subprocess
import sys
import time
from email.message import EmailMessage
from glob import glob
from zipfile import ZIP_DEFLATED, ZipInfo

from wheel.wheelfile import WheelFile

matrix = [
    # (platform, Zig target, wheel)
    ("Linux", "x86_64-linux-gnu", "manylinux1_x86_64"),
    ("Linux", "aarch64-linux-gnu", "manylinux2014_aarch64"),
    ("Darwin", "x86_64-macos", "macosx_10_9_x86_64"),
    ("Darwin", "aarch64-macos", "macosx_11_0_arm64"),
    ("Windows", "x86_64-windows", "win_amd64"),
    ("Windows", "aarch64-windows", "win_arm64"),
]

if len(sys.argv) == 2:
    # If an arg is passed, filter the matrix for only the specified target
    matrix = [row for row in matrix if row[1] == sys.argv[1].strip()]

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PARENT_DIR = os.path.dirname(CURRENT_DIR)
SRC_DIR = os.path.join(CURRENT_DIR, "src", "fastfec")
LIBRARY_DIR = os.path.join(PARENT_DIR, "zig-out", "lib")
OUTPUT_DIR = "wheelhouse"
PACKAGE_NAME = "fastfec"
with open(os.path.join(PARENT_DIR, "README.md"), "r") as f:
    readme = f.read()
with open(os.path.join(PARENT_DIR, "VERSION"), "r") as f:
    version = f.read().strip()


def make_message(headers, payload=None):
    # Copied from Zig make_wheels.py
    msg = EmailMessage()
    for name, value in headers.items():
        if isinstance(value, list):
            for value_part in value:
                msg[name] = value_part
        else:
            msg[name] = value
    if payload:
        msg.set_payload(payload)
    return msg


def write_wheel_file(filename, contents):
    with WheelFile(filename, "w") as wheel:
        for member_info, member_source in contents.items():
            if not isinstance(member_info, ZipInfo):
                member_info = ZipInfo(member_info)
                member_info.external_attr = 0o644 << 16
            member_info.file_size = len(member_source)
            member_info.compress_type = ZIP_DEFLATED
            wheel.writestr(member_info, bytes(member_source))
    return filename


def write_wheel(out_dir, *, name, version, tag, metadata, description, contents):
    # Copied from Zig make_wheels.py
    wheel_name = f"{name}-{version}-{tag}.whl"
    dist_info = f"{name}-{version}.dist-info"
    return write_wheel_file(
        os.path.join(out_dir, wheel_name),
        {
            **contents,
            f"{dist_info}/METADATA": make_message(
                {
                    "Metadata-Version": "2.1",
                    "Name": name,
                    "Version": version,
                    **metadata,
                },
                description,
            ),
            f"{dist_info}/WHEEL": make_message(
                {
                    "Wheel-Version": "1.0",
                    "Generator": "fastfec make_wheels.py",
                    "Root-Is-Purelib": "false",
                    "Tag": tag,
                }
            ),
        },
    )


# Gather contents
base_contents = {}
for path in glob(os.path.join(SRC_DIR, "*.py"), recursive=True):
    with open(path, "rb") as f:
        file_contents = f.read()
    base_contents[os.path.join(PACKAGE_NAME, os.path.relpath(path, SRC_DIR))] = (
        file_contents
    )

current_platform = platform.system()
for target_platform, zig_target, wheel_platform in matrix:
    # Compile the executable for the target platform
    contents = base_contents.copy()
    # First clear the target directory of any stray files
    if os.path.exists(LIBRARY_DIR):
        shutil.rmtree(LIBRARY_DIR)
    # Compile! Requires ziglang==0.11.0 to be installed
    subprocess.call(
        [
            sys.executable,
            "-m",
            "ziglang",
            "build",
            "-Dlib-only=true",
            f"-Dtarget={zig_target}",
        ],
        cwd=PARENT_DIR,
    )
    # Collect compiled library files (extension .dylib|.so|.dll)
    library_files = (
        glob(os.path.join(LIBRARY_DIR, "*.dylib"))
        + glob(os.path.join(LIBRARY_DIR, "*.so"))
        + glob(os.path.join(LIBRARY_DIR, "*.dll"))
    )
    # Write the library file to the archive contents
    for library_file in library_files:
        with open(library_file, "rb") as f:
            contents[
                os.path.join(PACKAGE_NAME, os.path.relpath(library_file, LIBRARY_DIR))
            ] = f.read()

    # Create output directory if needed
    if not os.path.exists(OUTPUT_DIR):
        os.mkdir(OUTPUT_DIR)
    # Write the wheel
    write_wheel(
        OUTPUT_DIR,
        name="fastfec",
        version=version,
        tag=f"py3-none-{wheel_platform}",
        metadata={
            "Summary": "An extremely fast FEC filing parser written in C",
            "Author": "Washington Post News Engineering",
            "License": "MIT",
            "Project-URL": [
                "Homepage, https://github.com/washingtonpost/FastFEC",
                "Source Code, https://github.com/washingtonpost/FastFEC",
                "Bug Tracker, https://github.com/washingtonpost/FastFEC/issues",
            ],
            "Classifier": [
                "License :: OSI Approved :: MIT License",
            ],
            "Requires-Python": "~=3.5",
            "Description-Content-Type": "text/markdown",
        },
        description=readme,
        contents=contents,
    )
