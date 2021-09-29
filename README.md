# fastfec

An experimental C program to stream and parse FEC filings, writing output to CSV. This project is in early stages and will benefit from rigorous testing.

## Usage

Ensure you have dependencies listed below installed, and then compile with
* `make build`

This will output a binary in `bin/fastfec`. The usage of that binary is as follows:

`Usage: ./bin/fastfec <id, file, or url> [output directory=output] [override id]`

* `<id, file, or url>` is either
  * a numeric ID, in which case the filing is streamed from the FEC website
  * a file, in which case the filing is read from disk at the specified local path
  * a url, in which case the filing is streamed from the specified remote URL
* `[output directory]` is the folder in which CSV files will be written. By default, it is `output/`.
* `[override id]` is an ID to use as the filing ID. If not specified, this ID is pulled out of the first parameter as a numeric component that can be found at the end of the path/URL.

The CLI will download or read from disk the specified filing and then write output CSVs for each form type in the output directory. The paths of the outputted files are:
* `{output directory}/{filing id}/{form type}.csv`

To run, open `src/main.c` and set the url on line ~19 to the desired filing (todo: CLI).
Also set the output filing ID on line ~40 to correspond.
Then run: `make buildrun` to run the program. See installation steps below.

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
