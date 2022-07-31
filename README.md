# FastFEC

A C program to stream and parse [Federal Election Commission](https://www.fec.gov/) (FEC) filings, writing output to CSV.

## Installation

Download the [latest release](https://github.com/WPMedia/FastFEC/releases/latest) and place it on your path, or if you have [Homebrew](https://brew.sh/) and are on Mac or Linux, you can install via:

```
brew install fastfec
```

You can also build a binary yourself following the development instructions below.

## Usage

Once FastFEC has been installed, you can run the program by calling `fastfec` in your terminal:

```
Usage: fastfec [flags] <id or file> [output directory=output] [override id]
```

- `[flags]`: optional flags which must come before other args; see below
- `<id, file, or url>` is either
  - a numeric ID, in which case the filing is streamed from the FEC website
  - a file, in which case the filing is read from disk at the specified local path
  - (URL support is blocked pending https://github.com/washingtonpost/FastFEC/issues/16)
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

**Parsing a local filing**

`fastfec -s 13360.fec fastfec_output/`

- This will run FastFEC in silent mode, parse the local filing 13360.fec, and store the output in CSV files at `fastfec_output/13360/`.

**Downloading and parsing a filing**

`curl https://docquery.fec.gov/dcdev/posted/13360.fec | fastfec 13360`

- This will download the filing with ID 13360 from the FEC's servers and stream/parse it, storing the output in CSV files at `output/13360/`
- Ensure you have [`curl`](https://curl.se/download.html) installed before running this command

## Local development

### Build system

[Zig](https://ziglang.org/) is used to build and compile the project. Download and install the latest version of Zig (>=0.9.1) by following the instructions on the website (you can verify it's working by typing `zig` in the terminal and seeing help commands).

### Dependencies

FastFEC has no dependencies.

### Building

From the root directory of the repo, run:

```sh
zig build
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
