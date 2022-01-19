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

### `fastfec.parse(file_handle, include_filing_id=None, should_parse_date=True)`

Parses a .fec filing in `file_handle` line by line, returning a generator that can view parsed results.

Each iteration yields the tuple `(form_type, line)`, where `form_type` is a string describing the form type of the parsed line and `line` is a Python dictionary mapping header keys to parsed line values. The type of each value in the `line` dictionary can be a string, float, datetime dates, or None.

If `include_filing_id` is set to a string, each output .csv file will have an initial column inserted before all the other columns containing the specified filing id. If `should_parse_date` is false, dates will be returned as strings (rather than datetime date objects).

Example usage:

```python
from fastfec import FastFEC
with open('12345.fec', 'rb') as f:
    with FastFEC() as fastfec:
        for form, line in fastfec.parse(f):
            print("GOT", form, line)
```

### `fastfec.parse_as_files(file_handle, output_directory, include_filing_id=None)`

Parses a .fec filing in `file_handle`, writing output parsed .csv files in the specified `output_directory` (creating parent directories as needed).

If `include_filing_id` is set to a string, each output .csv file will have an initial column inserted before all the other columns containing the specified filing id.

This method returns a status code: 1 indicates a successful parse, 0 indicates an unsuccessful one.

Example usage:

```python
from fastfec import FastFEC
with open('12345.fec', 'rb') as f:
    with FastFEC() as fastfec:
        fastfec.parse_as_files(f, 'output/')
```

### `fastfec.parse_as_files_custom(file_handle, open_output_file, include_filing_id=None)`

Parses a .fec filing in `file_handle`, writing output parsed .csv files using the custom provided `open_output_file` method (which should emulate the system `open` method).

This is an advanced method intended to give more control over writing to unconventional output streams, e.g. streaming output directory to s3 using the `smart-open` package or changing the output directory structure more broadly.

If `include_filing_id` is set to a string, each output .csv file will have an initial column inserted before all the other columns containing the specified filing id.

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
        fastfec.parse_as_files_custom(f, open_output_file)
```

## Development

### Setup

1. If you have not done so already follow the instructions in the root readme to build FastFEC locally.

1. Set your working directory to the python subdirectory `cd python`.

2. Install the dev requirements in a virtual environment with `pip install -r requirements-dev.txt`.

3. Install the pre-commit hooks with `pre-commit install`.

4. Verify things worked by running the unit tests with `tox`.

### Running tests

You can run the default test and linting setup on the repo by running `tox`. Settings for this are handled in the [tox.ini](tox.ini) file.

You can run the pre-commit hooks, including flake8, using `pre-commit run --all-files`.

**Linting with pylint**

If you just want to lint the repo, you can run `pylint --rcfile=setup.cfg [FILES OR FOLDERS]`, e.g. `pylint --rcfile=setup.cfg src`.

### Releases

To build and install a release for your local computer, you can use `setup.py` in the traditional way.

To coordinate multi-platform releases, there is a GitHub actions flow that uses the `make_wheels.py` script to build releases for multiple architectures at once using Zig. This script will manually invoke the Zig build process for a matrix of architectures and Python release names and create the resulting Python wheels manually.

This approach is used rather than a more traditional setup to make use of Zig and not create unnecessary builds. The `make_wheels.py` script is inspired from [Zig's Python package](https://github.com/ziglang/zig-pypi) which has a similar approach.
