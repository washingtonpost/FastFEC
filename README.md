# fastfec

[working pretty much fully; needs some more documentation]

A C program to stream and parse FEC filings, writing output to CSV.

To run, open `src/main.c` and set the url on line ~19 to the desired filing (todo: CLI).
Also set the output filing ID on line ~40 to correspond.
Then run: `make buildrun` to run the program. See installation steps below.

## Dependencies

libcurl

#### Time benchmarks

Using massive `1533121.fec` (5.8gb)

* 148.24 seconds

#### Installation steps

* `brew install pcre`

#### Testing

Currently, there's only C tests for specific CSV/ascii28 parsing functionality, but ideally once a Python wrapper is completed, we can have Python unit tests.

To run the current tests: `make test`

#### Scripts

`scripts/generate_mappings.py`: A Python command to auto-generate C header files containing column header and type mappings
