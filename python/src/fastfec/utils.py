"""
A Python library to interface with FastFEC.

This library provides methods to
  * parse a .fec file line by line, yieling a parsed result
  * parse a .fec file into parsed output .csv files
"""

import csv
import datetime
import logging
import os
import pathlib
from ctypes import CFUNCTYPE, POINTER, c_char, c_char_p, c_int, c_size_t, c_void_p, memmove
from glob import glob

logger = logging.getLogger("fastfec")

# Directories used for locating the shared library
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
PARENT_DIR = pathlib.Path(SCRIPT_DIR).parent.absolute()

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

    This method tries searching in package directories, with a fallback to the local
    zig build directory for development work.
    """
    prefixes = ["fastfec", "libfastfec"]

    suffixes = ["so", "dylib", "dll"]
    directories = [
        SCRIPT_DIR,
        PARENT_DIR,
        os.path.join(PARENT_DIR, "zig-out/lib"),  # For local dev
    ]

    # Search in parent directory
    for root_dir in directories:
        for prefix in prefixes:
            for suffix in suffixes:
                files = glob(os.path.join(root_dir, f"{prefix}*.{suffix}"))
                if files:
                    if len(files) > 1:
                        logger.warning("Expected just one library file")
                    return os.path.join(PARENT_DIR, files[0])

    raise LookupError("Unable to find libfastfec")


def as_bytes(text):
    """
    Converts text to bytes, or leaves intact if already bytes or none
    """
    if isinstance(text, str):
        # Convert to bytes if in string form
        return text.encode("utf8")
    return text


class WriteCache:  # pylint: disable=too-few-public-methods
    """
    Class to store cache information for the custom write function
    """

    def __init__(self):
        self.file_descriptors = {}  # Store all open file descriptors
        self.last_filename = None  # The last opened filename
        self.last_fd = None  # The last file descriptor


class LineCache:  # pylint: disable=too-few-public-methods
    """
    Class to store cache information for the custom line function
    """

    def __init__(self):
        self.headers = {}  # Store all headers given form type
        self.last_form_type = None  # The last opened form type
        self.last_headers = None  # The last headers used


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
    write_cache = WriteCache()

    def write_callback(filename, extension, contents, num_bytes):
        if filename == write_cache.last_filename:
            # Same filename as last call? Reuse file handle
            write_cache.last_fd.write(contents[:num_bytes])
        else:
            path = filename + extension
            # Grab the file descriptor from the cache if possible
            file_descriptor = write_cache.file_descriptors.get(path)
            if not file_descriptor:
                # Open the file
                write_cache.file_descriptors[path] = open_function(path.decode("utf8"), mode="wb")
                file_descriptor = write_cache.file_descriptors[path]
            write_cache.last_filename = filename
            write_cache.last_fd = file_descriptor
            # Write to the file descriptor
            file_descriptor.write(contents[:num_bytes])

    def free_file_descriptors():
        for file_descriptor in write_cache.file_descriptors.values():
            file_descriptor.close()

    return [CUSTOM_WRITE(write_callback), free_file_descriptors]


def provide_line_callback(queue, filing_id_included, should_parse_date):
    """
    Provides a C callback to return parsed lines given a queue to handle threading

    The threading allows the parent caller to yield result lines as they are returned.
    See https://stackoverflow.com/a/9968886 for more context
    """
    # Initialize parsing cache
    line_cache = LineCache()

    def line_callback(form_type, line, types):
        def yield_result(result):
            # Yield in parent function by utilizing the passed in queue
            queue.put(result)
            queue.join()

        if form_type == line_cache.last_form_type:
            # Same form type as past form — return immediately
            yield_result(
                (
                    form_type.decode("utf8"),
                    line_result(
                        line_cache.last_headers,
                        parse_csv_line(line.decode("utf8")),
                        types,
                        filing_id_included,
                        should_parse_date,
                    ),
                )
            )
        else:
            # Grab the headers from the cache if possible
            headers = line_cache.headers.get(form_type)
            first_line = False
            if not headers:
                # The headers have not yet encountered. They
                # are always in the first line, so this line
                # will contain them.
                line_cache.headers[form_type] = parse_csv_line(line.decode("utf8"))
                headers = line_cache.headers[form_type]
                first_line = True
            line_cache.last_form_type = form_type
            line_cache.last_headers = headers
            if not first_line:
                # Format the result and return it (if not a header)
                yield_result(
                    (
                        form_type.decode("utf8"),
                        line_result(
                            headers,
                            parse_csv_line(line.decode("utf8")),
                            types,
                            filing_id_included,
                            should_parse_date,
                        ),
                    )
                )

    return line_callback
