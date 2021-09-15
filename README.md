# fastfec

[working pretty much fully; needs some documentation]

A C program to stream and parse FEC filings, writing output to CSV.

## Dependencies

libcurl

#### Time benchmarks

Using massive `1533121.fec` (5.8gb)

* putting line directly in: 50.81 seconds
* lowercasing every line: 63.11 seconds

#### Installation steps

* `brew install pcre`

#### Scripts

`scripts/generate_mappings.py`: A Python command to auto-generate C header files containing column header and type mappings
