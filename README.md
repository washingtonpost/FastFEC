# fastfec

A C program to stream and parse FEC filings, writing output to CSV.

## Dependencies

libcurl

#### Time benchmarks

Using massive `1533121.fec`

* putting line directly in: 50.81 seconds
* lowercasing every line: 63.11 seconds

#### Scripts

`scripts/generate_mappings.py`: A Python command to auto-generate C header files containing column header and type mappings