# fastfec

A Python library to interface with FastFEC.

---

## Basic usage

Import the library helper via:

```python
from fastfec import FastFEC
```

To receive an instance of the LibFastFEC class that automatically handles freeing up used memory:

```python
with FastFEC() as fastfec:
    ...
```

## API

### `fastfec.parse(file_handle)`

Parses a .fec filing in `file_handle` line by line, returning a generator that can view parsed results.

Each iteration yields the tuple `(form_type, line)`, where `form_type` is a string describing the form type of the parsed line and `line` is a Python dictionary mapping header keys to parsed line values. The type of each value in the `line` dictionary can be a string, float, datetime, or None.

Example usage:

```python
from fastfec import FastFEC
with open('12345.fec', 'rb') as f:
    with FastFEC() as fastfec:
        for form, line in fastfec.parse(f):
            print("GOT", form, line)
```

### `fastfec.parse_as_files(file_handle, output_directory)`

Parses a .fec filing in `file_handle`, writing output parsed .csv files in the specified `output_directory` (creating parent directories as needed).

This method returns a status code: 1 indicates a successful parse, 0 indicates an unsuccessful one.

Example usage:

```python
from fastfec import FastFEC
with open('12345.fec', 'rb') as f:
    with FastFEC() as fastfec:
        fastfec.parse_as_files(f, 'output/')
```

### `fastfec.parse_as_files_custom(file_handle, open_output_file)`

Parses a .fec filing in `file_handle`, writing output parsed .csv files using the custom provided `open_output_file` method (which should emulate the system `open` method).

This is an advanced method intended to give more control over writing to unconventional output streams, e.g. streaming output directory to s3 using the `smart-open` package.

This method returns a status code: 1 indicates a successful parse, 0 indicates an unsuccessful one.

Example usage:

```python
import os
from fastfec import FastFEC
from pathlib import Path

# Custom open method that places files in an output directory,
# and creates the directory if it does not exist
def open_output_file(filename, *args, **kwargs):
    filename = os.path.join('custom_python_output', filename)
    output_file = Path(filename)
    output_file.parent.mkdir(exist_ok=True, parents=True)
    return open(filename, *args, **kwargs)

with open('12345.fec', 'rb') as f:
    with FastFEC() as fastfec:
        fastfec.parse_as_files(f, open_output_file)
```