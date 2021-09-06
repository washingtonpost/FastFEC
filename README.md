# fastfec

[in-progress / not working yet fully]

A C program to stream and parse FEC filings, writing output to CSV.

## Dependencies

libcurl

#### Time benchmarks

Using massive `1533121.fec`

* putting line directly in: 50.81 seconds
* lowercasing every line: 63.11 seconds
* unescaping from ascii28 format and rewriting as CSV: 117.20
