# fastFEC

An experimental C program to stream and parse FEC filings, writing output to CSV. This project is in early stages and will benefit from rigorous testing.

## Usage

Ensure you have dependencies listed below installed, and then compile with
* `make build`

This will output a binary in `bin/fastfec`. The usage of that binary is as follows:

```
Usage: ./bin/fastfec [flags] <id, file, or url> [output directory=output] [override id]
```

* `[flags]`: optional flags which must come before other args; see below
* `<id, file, or url>` is either
  * a numeric ID, in which case the filing is streamed from the FEC website
  * a file, in which case the filing is read from disk at the specified local path
  * a url, in which case the filing is streamed from the specified remote URL
* `[output directory]` is the folder in which CSV files will be written. By default, it is `output/`.
* `[override id]` is an ID to use as the filing ID. If not specified, this ID is pulled out of the first parameter as a numeric component that can be found at the end of the path/URL.

The CLI will download or read from disk the specified filing and then write output CSVs for each form type in the output directory. The paths of the outputted files are:
* `{output directory}/{filing id}/{form type}.csv`

You can also pipe the output of another command in by following this usage:

```
[some command] | ./bin/fastfec [flags] <id> [output directory=output]
```

### Flags

The CLI supports the following flags:

* `--include-filing-id` / `-i`: if this flag is passed, then the generated output will include a column at the beginning of every generated file called `filing_id` that gets passed the filing ID. This can be useful for bulk uploading CSVs into a database
* `--silent` / `-s` : suppress all non-error output messages
* `--suppress` / `-w` : suppress all warning messages

The short form of flags can be combined, e.g. `-is` would include filing IDs and suppress output.

### Examples

`./bin/fastfec -s 13360 fastfec_output/`
* This will run FastFEC in silent mode, download and parse filing ID 13360, and store the output in CSV files at `fastfec_output/13360/`.

## Dependencies

* libcurl (should be installed already)
* pcre (`brew install pcre`)

#### Time benchmarks

Using massive `1533121.fec` (5.8gb)

* 2m 11s

#### Testing

Currently, there's only C tests for specific CSV/ascii28 parsing functionality, but ideally once a Python wrapper is completed, we can have Python unit tests.

To run the current tests: `make test`

#### Scripts

`scripts/generate_mappings.py`: A Python command to auto-generate C header files containing column header and type mappings
