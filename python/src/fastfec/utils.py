"""
A Python library to interface with FastFEC.

This library provides methods to
  * parse a .fec file line by line, yieling a parsed result
  * parse a .fec file into parsed output .csv files
"""

import csv
import datetime
import logging
from ctypes import (
    CFUNCTYPE,
    POINTER,
    c_char,
    c_char_p,
    c_int,
    c_size_t,
    c_void_p,
    memmove,
)
from pathlib import Path

logger = logging.getLogger("fastfec")

# Directories used for locating the shared library
SCRIPT_DIR = Path(__file__).parent.absolute()
REPO_DIR = SCRIPT_DIR.parent.parent.parent  # REPO_DIR/python/src/fastfec/utils.py
assert REPO_DIR.name == "FastFEC"

# Buffer constants
BUFFER_SIZE = 1024 * 1024

# Callback function ctypes
BUFFER_READ = CFUNCTYPE(c_size_t, POINTER(c_char), c_int, c_void_p)
CUSTOM_WRITE = CFUNCTYPE(None, c_char_p, c_char_p, POINTER(c_char), c_int)
CUSTOM_LINE = CFUNCTYPE(None, c_char_p, c_char_p, c_char_p)


def make_read_buffer(file_input):
    """
    Creates a read buffer callback given an open input stream

    Arguments:
        file_input -- An input stream for the callback to consume

    Returns:
        A callback function that C code can wrap to consume the input stream
    """

    def read_buffer(buffer, want, _):
        contents_raw = file_input.read(want)
        received = len(contents_raw)
        contents = c_char_p(contents_raw)
        memmove(buffer, contents, received)
        return received

    return read_buffer


def find_fastfec_lib():
    """
    Scans for the fastfec shared library and returns the path of the found library

    This method tries searching in package directories, with a priority to the local
    zig build directory for development work.
    """
    prefixes = ["fastfec", "libfastfec"]
    suffixes = ["so", "dylib", "dll"]
    patterns = [f"{prefix}*.{suffix}" for prefix in prefixes for suffix in suffixes]
    directories = [
        REPO_DIR / "zig-out/lib",  # prioritize local dev
        SCRIPT_DIR,
        SCRIPT_DIR.parent,
    ]

    for root_dir in directories:
        files = []
        for pattern in patterns:
            files.extend(root_dir.glob(pattern))
        if files:
            if len(files) > 1:
                logger.warning("Expected just one library file")
            return files[0]

    raise LookupError("Unable to find libfastfec")


def as_bytes(text):
    """
    Converts text to bytes, or leaves intact if already bytes or none
    """
    if isinstance(text, str):
        # Convert to bytes if in string form
        return text.encode("utf8")
    return text


class WriterCache:
    def __init__(self, open_function):
        self._open_function = open_function
        self._writers = {}

    def get_writer(self, filename: bytes, extension: bytes):
        path = filename + extension
        if path not in self._writers:
            self._writers[path] = self._open_function(path.decode("utf8"), mode="wb")
        return self._writers[path]

    def close(self) -> None:
        for writers in self._writers.values():
            writers.close()


def parse_csv_line(line):
    """
    Parses a string holding a CSV line into a Python list
    """
    return list(csv.reader([line]))[0]


def array_get(array, idx, fallback):
    """
    Retrieves the item at position idx in array, or fallback if out of bounds
    """
    try:
        return array[idx]
    except IndexError:
        return fallback


def parse_date(date):
    """
    Parses a YYYY-MM-DD date into a Python datetime, or returns the input string
    on failure.
    """
    if date is None or len(date) != 10:
        return date

    try:
        year = int(date[0:4])
        month = int(date[5:7])
        day = int(date[8:10])
        return datetime.date(year, month, day)
    except (ValueError, OverflowError, TypeError):
        return date


def line_result(headers, items, types, filing_id_included, should_parse_date):
    """
    Formats the results of the line callback according to specified headers and types
    """

    def convert_item(i):
        item = array_get(items, i, None)
        if item is None:
            return None
        if types is None:
            return item
        # Offset types if filing id is included to account for string type at beginning
        fec_type = array_get(types, i - (1 if filing_id_included else 0), ord(b"s"))
        if fec_type == ord(b"s"):
            return item
        if fec_type == ord(b"d"):
            # Convert standard YYYY-MM-DD date to Pythonic date object if the date is
            # to be parsed
            return parse_date(item) if should_parse_date else item
        if fec_type == ord(b"f"):
            try:
                return float(item)
            except ValueError:
                return item

        logger.warning("Unrecognized type: %s", chr(fec_type))
        return item

    # Build up result object
    result = {}
    # Used to handle missing header #'s
    # (we don't expect this to happen, but FEC filings sometimes have
    # more values than headers in a particular row. If this happens,
    # we can still capture the data with a `__missing_header_#` key)
    missing_header = 1
    for i in range(max(len(headers), len(items))):
        if i < len(headers):
            header = headers[i]
        else:
            header = f"__missing_header_{missing_header}"
            missing_header += 1
        item = convert_item(i)
        result[header] = item

    return result


def provide_read_callback(file_handle):
    """
    Provides a C callback to read from a given file stream
    """
    return BUFFER_READ(make_read_buffer(file_handle))


def provide_write_callback(open_function):
    """
    Provides a C callback to write to file given a function to open file streams
    """
    # Initialize parsing cache
    cache = WriterCache(open_function)

    def write_callback(filename, extension, contents, num_bytes):
        writer = cache.get_writer(filename, extension)
        writer.write(contents[:num_bytes])

    return (CUSTOM_WRITE(write_callback), cache.close)


def provide_line_callback(queue, filing_id_included, should_parse_date):
    """
    Provides a C callback to return parsed lines given a queue to handle threading

    The threading allows the parent caller to yield result lines as they are returned.
    See https://stackoverflow.com/a/9968886 for more context
    """
    # Given a form type, this tells you the fields that are included in the CSV
    header_cache: dict[str, list[str]] = {}

    def line_callback(
        form_type_bytes: bytes, line_bytes: bytes, types_bytes: bytes | None
    ):
        form_type = form_type_bytes.decode("utf8")
        line_items = parse_csv_line(line_bytes.decode("utf8"))

        if form_type not in header_cache:
            # This was the first line, so it contains the headers
            header_cache[form_type] = line_items
            # We only return data, not the headers
            return

        header = header_cache[form_type]
        record_dict = line_result(
            header,
            line_items,
            types_bytes,
            filing_id_included,
            should_parse_date,
        )
        result = (form_type, record_dict)
        queue.put(result)
        queue.join()

    return line_callback
