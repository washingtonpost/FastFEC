"""
A Python library to interface with FastFEC.

Import the library helper via:

    from fastfec import FastFEC

Receive an instance of the LibFastFEC class that automatically handles freeing up used memory:

    with FastFEC() as fastfec:
        ...

Example usage:

    * Parse a filing line by line:

        from fastfec import FastFEC

        with open('12345.fec', 'rb') as f:
            with FastFEC() as fastfec:
                for form, line in fastfec.parse(f):
                    print("GOT", form, line)

    * Parse a filing to output files:

        import os
        from pathlib import Path
        from fastfec import FastFEC

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
"""

import contextlib
import csv
import datetime
import logging
import os
import pathlib
from ctypes import (CDLL, CFUNCTYPE, POINTER, c_char, c_char_p, c_int,
                    c_size_t, c_void_p, memmove)
from ctypes.util import find_library
from dataclasses import dataclass
from glob import glob
from queue import Queue
from threading import Thread

# Directories used for locating the shared library
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
PARENT_DIR = pathlib.Path(SCRIPT_DIR).parent.absolute()

# Buffer constants
BUFFER_SIZE = 1024 * 1024

# Callback function ctypes
BUFFER_READ = CFUNCTYPE(c_size_t, POINTER(c_char), c_int, c_void_p)
CUSTOM_WRITE = CFUNCTYPE(None, c_char_p, c_char_p,
                         c_char_p, c_int)
CUSTOM_LINE = CFUNCTYPE(None, c_char_p, c_char_p, c_char_p)


def make_read_buffer(file_input):
    """Creates a read buffer callback given an open input stream

    Arguments:
        file_input -- An input stream for the callback to consume

    Returns:
        A callback function that C code can wrap to consume the input stream"""

    def read_buffer(buffer, want, _):
        contents_raw = file_input.read(want)
        received = len(contents_raw)
        contents = c_char_p(contents_raw)
        memmove(buffer, contents, received)
        return received
    return read_buffer


def find_fastfec_lib():
    """Scans for the fastfec shared library and returns the path of the found library

    This method tries searching in the parent directory first but has a fallback to
    the local zig build directory for development work.
    """
    prefixes = ["fastfec", "libfastfec"]
    suffixes = ["so", "dylib", "dll"]
    directories = [
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
                        logging.warning("Expected just one library file")
                    return os.path.join(PARENT_DIR, files[0])

    # Search in system library directory as last-ditch attempt
    system_library = find_library("fastfec")
    if system_library is None:
        system_library = find_library("libfastfec")
    return system_library


@dataclass
class WriteCache:
    """Class to store cache information for the custom write function"""
    file_descriptors = {}  # Store all open file descriptors
    last_filename = None  # The last opened filename
    last_fd = None  # The last file descriptor


@dataclass
class LineCache:
    """Class to store cache information for the custom line function"""
    headers = {}  # Store all headers given filename
    last_filename = None  # The last opened filename
    last_headers = None  # The last headers used


def csv_parse(line):
    """Parses a string holding a CSV line into a Python list"""
    return list(csv.reader([line]))[0]


def array_get(array, idx, fallback):
    """Retrieves the item at position idx in array, or fallback if out of bounds"""
    try:
        return array[idx]
    except IndexError:
        return fallback


def line_result(headers, items, types):
    """Formats the results of the line callback according to specified headers and types"""
    def convert_item(i):
        item = array_get(items, i, None)
        if item is None:
            return None
        if types is None:
            return item
        fec_type = array_get(types, i, ord(b's'))
        if fec_type == ord(b's'):
            return item
        if fec_type == ord(b'd'):
            # Convert standard YYYY-MM-DD date to Pythonic date object
            return datetime.date(*(int(component) for component in item.split('-')))
        if fec_type == ord(b'f'):
            return float(item)

        logging.warning("Unrecognized type: %s", chr(fec_type))
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
            header = f'__missing_header_{missing_header}'
            missing_header += 1
        item = convert_item(i)
        result[header] = item

    return result


def provide_read_callback(file_handle):
    """Provides a C callback to read from a given file stream"""
    return BUFFER_READ(
        make_read_buffer(file_handle))


def provide_write_callback(open_function):
    """Provides a C callback to write to file given a function to open file streams"""
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
                write_cache.file_descriptors[path] = open_function(
                    path.decode("utf8"), mode='wb')
                file_descriptor = write_cache.file_descriptors[path]
            write_cache.last_filename = filename
            write_cache.last_fd = file_descriptor
            # Write to the file descriptor
            file_descriptor.write(contents[:num_bytes])

    def free_file_descriptors():
        for path in write_cache.file_descriptors:
            write_cache.file_descriptors[path].close()

    return [CUSTOM_WRITE(write_callback), free_file_descriptors]


def provide_line_callback(queue):
    """Provides a C callback to return parsed lines given a queue to handle threading

    The threading allows the parent caller to yield result lines as they are returned"""
    # Initialize parsing cache
    line_cache = LineCache()

    def line_callback(filename, line, types):
        # Yield in parent function by utilizing the passed in queue
        if filename == line_cache.last_filename:
            queue.put(
                (filename.decode('utf8'),
                    line_result(line_cache.last_headers, csv_parse(line.decode('utf8')), types)))
            queue.join()
        else:
            # Grab the headers from the cache if possible
            headers = line_cache.headers.get(filename)
            first_line = False
            if not headers:
                # The headers have not yet encountered. They
                # are always in the first line, so this line
                # will contain them.
                line_cache.headers[filename] = csv_parse(
                    line.decode('utf8'))
                headers = line_cache.headers[filename]
                first_line = True
            line_cache.last_filename = filename
            line_cache.last_headers = headers
            if not first_line:
                # Format the result and return it (if not a header)
                queue.put((filename.decode('utf8'), line_result(headers, csv_parse(
                    line.decode('utf8')), types)))
                queue.join()
    return line_callback


class LibFastFEC:
    """Python wrapper for the fastfec library

    Usage:

        Initialize the library:

            fastfec = FastFEC()

        Call a parse method. Options:

        * parse: parse the input file line-by-line

            with open('file.fec', 'rb') as f:
                for form, line in fastfec.parse(f):
                    print("Got line:", form, line)

        * parse_as_files: parse the input file into output files

            import os
            from pathlib import Path

            # Custom open method
            def open_output_file(filename, *args, **kwargs):
                filename = os.path.join('custom_output_dir', filename)
                output_file = Path(filename)
                output_file.parent.mkdir(exist_ok=True, parents=True)
                return open(filename, *args, **kwargs)

            with open('file.fec', 'rb') as f:
                fastfec.parse_as_files(f, open=open_output_file)

        Close the library:

            fastfec.free()
        """

    def __init__(self):
        self.__init_lib()

        # Initialize
        self.persistent_memory_context = self.libfastfec.newPersistentMemoryContext()

    def parse(self, file_handle):
        """Parses the input file line-by-line

        Arguments:
            file_handle -- An input stream for reading a .fec file

        Returns:
            An iterator that receives the form name and a dictionary
            object describing each line in the file"""
        # Set up a queue to be able to yield output
        queue = Queue()
        done_processing = object()

        # Provide a custom line callback
        buffer_read_fn = provide_read_callback(file_handle)
        line_callback_fn = CUSTOM_LINE(
            provide_line_callback(queue))
        fec_context = self.libfastfec.newFecContext(
            self.persistent_memory_context, buffer_read_fn, BUFFER_SIZE, CUSTOM_WRITE(
                0),
            BUFFER_SIZE, line_callback_fn, 0, None, None, None, 0, 1, 0)

        # Run the parsing in a separate thread
        def task():
            self.libfastfec.parseFec(fec_context)
            queue.put(done_processing)

        Thread(target=task, args=()).start()

        # Yield processed lines
        while True:
            line = queue.get()
            if line == done_processing:
                break
            yield line
            queue.task_done()

        # Free FEC context
        self.libfastfec.freeFecContext(fec_context)

    def parse_as_files(self, file_handle, open_function=open):
        """Parses the input file into output files

        Arguments:
            file_handle -- An input stream for reading a .fec file
            open_function -- An optional function to open an output file for writing. This can be
                             set to customize the output stream for each parsed .csv file"""
        # Set callbacks
        buffer_read_fn = provide_read_callback(file_handle)
        write_callback_fn, free_file_descriptors = provide_write_callback(
            open_function)

        # Initialize fastfec context
        fec_context = self.libfastfec.newFecContext(
            self.persistent_memory_context, buffer_read_fn, BUFFER_SIZE,
            write_callback_fn, BUFFER_SIZE, CUSTOM_LINE(0), 0, None, None, None, 0, 1, 0)

        # Parse
        result = self.libfastfec.parseFec(fec_context)

        # Free memory and file descriptors
        self.libfastfec.freeFecContext(fec_context)
        free_file_descriptors()

        return result

    def free(self):
        """Frees all the allocated memory from the fastfec library"""
        self.libfastfec.freePersistentMemoryContext(
            self.persistent_memory_context)

    def __init_lib(self):
        # Find the fastfec library
        self.libfastfec = CDLL(find_fastfec_lib())

        # Lay out arg/res types for C callbacks
        self.libfastfec.newPersistentMemoryContext.argtypes = []
        self.libfastfec.newPersistentMemoryContext.restype = c_void_p

        self.libfastfec.newFecContext.argtypes = [
            c_void_p, BUFFER_READ, c_int, CUSTOM_WRITE, c_int, CUSTOM_LINE, c_int, c_void_p,
            c_char_p, c_char_p, c_int, c_int, c_int]
        self.libfastfec.newFecContext.restype = c_void_p

        self.libfastfec.parseFec.argtypes = [c_void_p]
        self.libfastfec.parseFec.restype = c_int

        self.libfastfec.freeFecContext.argtypes = [c_void_p]

        self.libfastfec.freePersistentMemoryContext.argtypes = [c_void_p]


@ contextlib.contextmanager
def FastFEC():  # pylint: disable=invalid-name
    """A convenience method to run fastfec and free memory afterwards

    Usage:

        with FastFEC() as fastfec:
            # Running fastfec parse methods
            # The memory is freed afterwards, so no need to ever call
            # fastfec.free()
            ...
    """
    instance = LibFastFEC()
    yield instance
    instance.free()
