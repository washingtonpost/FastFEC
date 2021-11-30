# FastFEC

A C program to stream and parse FEC filings, writing output to CSV. This project is in early stages but works on a wide variety of filings and will benefit from additional rigorous testing.

## Installation

Download the [latest release](https://github.com/WPMedia/FastFEC/releases/latest) and place it on your path, or if you have [Homebrew](https://brew.sh/) and are on Mac or Linux, you can install via:

```
brew install fastfec
```

You can also build a binary yourself following the development instructions below.

## Usage

Once FastFEC has been installed, you can run the program by calling `fastfec` in your terminal:

```
Usage: fastfec [flags] <id, file, or url> [output directory=output] [override id]
```

- `[flags]`: optional flags which must come before other args; see below
- `<id, file, or url>` is either
  - a numeric ID, in which case the filing is streamed from the FEC website
  - a file, in which case the filing is read from disk at the specified local path
  - a url, in which case the filing is streamed from the specified remote URL
- `[output directory]` is the folder in which CSV files will be written. By default, it is `output/`.
- `[override id]` is an ID to use as the filing ID. If not specified, this ID is pulled out of the first parameter as a numeric component that can be found at the end of the path/URL.

The CLI will download or read from disk the specified filing and then write output CSVs for each form type in the output directory. The paths of the outputted files are:

- `{output directory}/{filing id}/{form type}.csv`

You can also pipe the output of another command in by following this usage:

```
[some command] | fastfec [flags] <id> [output directory=output]
```

### Flags

The CLI supports the following flags:

- `--include-filing-id` / `-i`: if this flag is passed, then the generated output will include a column at the beginning of every generated file called `filing_id` that gets passed the filing ID. This can be useful for bulk uploading CSVs into a database
- `--silent` / `-s` : suppress all non-error output messages
- `--warn` / `-w` : show warning messages (e.g. for rows with unexpected numbers of fields or field types that don't match exactly)
- `--no-stdin` / `-x`: disable receiving piped input from other programs (stdin)

The short form of flags can be combined, e.g. `-is` would include filing IDs and suppress output.

### Examples

`fastfec -s 13360 fastfec_output/`

- This will run FastFEC in silent mode, download and parse filing ID 13360, and store the output in CSV files at `fastfec_output/13360/`.

#### Example via Python wrapper

Log into the AWS Eletions account in your browser via Okta, and then go to https://s3.console.aws.amazon.com/s3/buckets/elex-fec-test?region=us-east-1&prefix=test-architecture/test-filings/&showversions=false, find a filing that is more than 0B of data AND for which there isn't a corresponding output folder here https://s3.console.aws.amazon.com/s3/buckets/elex-fec-test?region=us-east-1&prefix=test-architecture/test-fastfec-output/&showversions=false and then use that filing number in the command below, which you should run from the root of the FastFEC repo:
`python /python/fastfec.py -f "[FILING NUMBER GOES HERE]" -i "s3://elex-fec-test/test-architecture/test-filings" -o "s3://elex-fec-test/test-architecture/test-fastfec-output"`

After running the command, make sure you get something like this in the console:

```
➜  fastFEC git:(python-ctypes) ✗ python /python/fastfec.py -f "1375137" -i "s3://elex-fec-test/test-architecture/test-filings" -o "s3://elex-fec-test/test-architecture/test-fastfec-output"
Filing ID is 1375137
Input file is s3://elex-fec-test/test-architecture/test-filings/1375137
Output file is s3://elex-fec-test/test-architecture/test-fastfec-output/1375137
Parsing (py)
Parsed; status 1
1.2386590242385864e-07
4.439614713191986e-06
7.579103112220764e-06
```

This will run FastFEC in silent mode, download and parse filing ID 1375137, and store the output in CSV files at `fastfec_output/1375137/`.

Make sure the output for that filing appears here: https://s3.console.aws.amazon.com/s3/buckets/elex-fec-test?region=us-east-1&prefix=test-architecture/test-fastfec-output/&showversions=false

## Local development

### Build system

[Zig](https://ziglang.org/) is used to build and compile the project. Download and install the latest version of Zig (>=9.0.0) by following the instructions on the website (you can verify it's working by typing `zig` in the terminal and seeing help commands).

### Dependencies

The following libraries are used:

- curl (needed for the CLI, not the library)
- pcre (only needed on Windows)

Installing these libraries varies by OS:

#### Mac OS X

Ensure [Homebrew](https://brew.sh/) is installed and run the following `brew` command to install the libraries:

```sh
brew install pkg-config curl
```

#### Ubuntu

```sh
sudo apt install -y libcurl4-openssl-dev
```

#### Windows

Install [vcpkg](https://vcpkg.io) and run the following:

```sh
vcpkg integrate install
vcpkg install pcre curl --triplet x64-windows-static
```

### Building

From the root directory of the repo, run:

```sh
zig build
```

On Windows, you may have to supply additional arguments to locate vcpkg dependencies and ensure the msvc toolchain is used:

```sh
zig build --search-prefix C:/vcpkg/packages/pcre_x64-windows-static --search-prefix C:/vcpkg/packages/curl_x64-windows-static --search-prefix C:/vcpkg/packages/zlib_x64-windows-static -Dtarget=x86_64-windows-msvc
```

The above commands will output a binary at `zig-out/bin/fastfec` and a shared library file in the `zig-out/lib/` directory. If you want to only build the library, you can pass `-Dlib-only=true` as a build option following `zig build`.

#### Time benchmarks

Using massive `1464847.fec` (8.4gb) on an M1 MacBook Air

- 1m 42s

#### Testing

Currently, there's only C tests for specific parsing/buffer/write functionality, but we hope to expand unit testing soon.

To run the current tests: `zig build test`

#### Scripts

`python scripts/generate_mappings.py`: A Python script to auto-generate C header files containing column header and type mappings
