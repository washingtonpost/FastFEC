# AGENTS.md

## What this repo is

FastFEC is a C parser for raw `.fec` filings. It has:

- a CLI binary (`fastfec`) that writes parsed CSV files
- a shared C library (`libfastfec`)
- a Python wrapper package in `python/` that loads the shared library

## Repo structure

- `src/`: C source code for parser, CLI, writer, and bundled PCRE
- `build.zig`: Zig build script for CLI, shared lib, and C tests
- `scripts/`: mapping generation scripts (`generate_mappings.py`)
- `python/src/fastfec/`: Python wrapper (`ctypes` client + utils)
- `python/tests/`: Python tests + fixture `.fec` files

## macOS setup

1. Install Zig (recommended on current macOS):

```sh
brew install zig@0.14
```

2. Use this Zig in shell for the session:

```sh
export PATH="/opt/homebrew/opt/zig@0.14/bin:$PATH"
```

3. Verify:

```sh
zig version
```

## Build (C CLI + shared lib)

From repo root:

```sh
zig build
```

Outputs:

- `zig-out/bin/fastfec`
- `zig-out/lib/libfastfec.dylib`

## Run CLI

Parse a local filing:

```sh
./zig-out/bin/fastfec -x -s python/tests/fixtures/13360.fec /private/tmp/fastfec_out 13360
```

Notes:

- `-x` disables stdin and forces file input mode
- output files are written under `{output_dir}/{filing_id}/`

## Run C tests

```sh
zig build test
```

## Makefile shortcuts

From repo root:

```sh
make help
```

Main targets:

- `make build`: build CLI + shared library
- `make c-test`: run C tests
- `make cli-smoke`: parse fixture filing and write CSV output to `/private/tmp/fastfec_make_smoke`
- `make py-setup`: create `python/.venv`
- `make py-install`: install Python dev dependencies
- `make py-test`: run Python tests (`tox -e py`)
- `make all`: run full verification (`build`, `c-test`, `cli-smoke`, `py-test`)

## Python interface setup

From repo root:

```sh
python3 -m venv python/.venv
source python/.venv/bin/activate
cd python
pip install -r requirements-dev.txt
```

## Run Python tests

```sh
source python/.venv/bin/activate
cd python
tox -e py
```

## Dependency notes

- The Python package depends on `ziglang==0.11.0` for wheel/build workflows.
- On newer macOS, using system `zig 0.11.0` directly may fail; building the repo with `zig@0.14` is the practical path.
