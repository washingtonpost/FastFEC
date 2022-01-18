"""
A Python library to interface with FastFEC.

This library provides methods to
  * parse a .fec file line by line, yieling a parsed result
  * parse a .fec file into parsed output .csv files
"""

import contextlib
import os
import pathlib
from ctypes import CDLL, c_char_p, c_int, c_void_p
from queue import Queue
from threading import Thread

from .utils import (
    BUFFER_READ,
    BUFFER_SIZE,
    CUSTOM_LINE,
    CUSTOM_WRITE,
    as_bytes,
    find_fastfec_lib,
    provide_line_callback,
    provide_read_callback,
    provide_write_callback,
)


class LibFastFEC:
    """
    Python wrapper for the fastfec library
    """

    def __init__(self):
        self.__init_lib()

        # Initialize
        self.persistent_memory_context = self.libfastfec.newPersistentMemoryContext()

    def parse(self, file_handle, include_filing_id=None, should_parse_date=True):
        """
        Parses the input file line-by-line

        Arguments:
            file_handle -- An input stream for reading a .fec file
            include_filing_id -- If set, prepend a column into each outputted csv for filing_id
                                 with the specified filing id (defaults to None)
            should_parse_date -- If true, yields parsed datetime.date objects for date fields; if
                                 false, yields strings for date fields. This would mainly be set to
                                 false for performance reasons (defaults to true)

        Returns:
            A generator that receives the form name and a dictionary
            object describing each line in the file
        """
        # Set up a queue to be able to yield output
        queue = Queue()
        done_processing = object()  # A custom object to signal the end of processing

        # Prepare the filing id to include, if specified
        include_filing_id = as_bytes(include_filing_id)
        filing_id_included = include_filing_id is not None

        # Provide a custom line callback
        buffer_read_fn = provide_read_callback(file_handle)
        line_callback_fn = CUSTOM_LINE(provide_line_callback(queue, filing_id_included, should_parse_date))
        fec_context = self.libfastfec.newFecContext(
            self.persistent_memory_context,
            buffer_read_fn,
            BUFFER_SIZE,
            CUSTOM_WRITE(0),
            BUFFER_SIZE,
            line_callback_fn,
            0,
            None,
            include_filing_id,
            None,
            filing_id_included,
            1,
            0,
        )

        # Run the parsing in a separate thread. It's essentially still single-threaded
        # but this provides a mechanism to yield the results of a callback function
        # from the caller. See https://stackoverflow.com/a/9968886 for more context
        def task():
            self.libfastfec.parseFec(fec_context)
            queue.put(done_processing)  # Signal processing is over

        Thread(target=task, args=()).start()

        # Yield processed lines
        while True:
            line = queue.get()
            if line == done_processing:
                # End the task when done processing
                break
            yield line
            queue.task_done()

        # Free FEC context
        self.libfastfec.freeFecContext(fec_context)

    def parse_as_files(self, file_handle, output_directory, include_filing_id=None):
        """
        Parses the input file into output files in the output directory

        Parent directories will be automatically created as needed.

        Arguments:
            file_handle -- An input stream for reading a .fec file
            output_directory -- A directory in which to place output parsed .csv files
            include_filing_id -- If set, prepend a column into each outputted csv for filing_id
                                 with the specified filing id (defaults to None)

        Returns:
            A status code. 1 indicates a successful parse, 0 an unsuccessful one.
        """

        # Custom open method
        def open_output_file(filename, *args, **kwargs):
            filename = os.path.join(output_directory, filename)
            output_file = pathlib.Path(filename)
            output_file.parent.mkdir(exist_ok=True, parents=True)
            # pylint: disable=consider-using-with,unspecified-encoding,bad-option-value
            return open(filename, *args, **kwargs)

        return self.parse_as_files_custom(file_handle, open_output_file, include_filing_id=include_filing_id)

    def parse_as_files_custom(self, file_handle, open_function, include_filing_id=None):
        """
        Parses the input file into output files

        Arguments:
            file_handle -- An input stream for reading a .fec file
            open_function -- A function to open an output file for writing. This can be set to
                             customize the output stream for each parsed .csv file
            include_filing_id -- If set, prepend a column into each outputted csv for filing_id
                                 with the specified filing id (defaults to None)

        Returns:
            A status code. 1 indicates a successful parse, 0 an unsuccessful one.
        """
        # Set callbacks
        buffer_read_fn = provide_read_callback(file_handle)
        write_callback_fn, free_file_descriptors = provide_write_callback(open_function)

        # Prepare the filing id to include, if specified
        include_filing_id = as_bytes(include_filing_id)
        filing_id_included = include_filing_id is not None

        # Initialize fastfec context
        fec_context = self.libfastfec.newFecContext(
            self.persistent_memory_context,
            buffer_read_fn,
            BUFFER_SIZE,
            write_callback_fn,
            BUFFER_SIZE,
            CUSTOM_LINE(0),
            0,
            None,
            include_filing_id,
            None,
            filing_id_included,
            1,
            0,
        )

        # Parse
        result = self.libfastfec.parseFec(fec_context)

        # Free memory and file descriptors
        self.libfastfec.freeFecContext(fec_context)
        free_file_descriptors()

        return result

    def free(self):
        """
        Frees all the allocated memory from the fastfec library
        """
        self.libfastfec.freePersistentMemoryContext(self.persistent_memory_context)

    def __init_lib(self):
        # Find the fastfec library
        self.libfastfec = CDLL(find_fastfec_lib())

        # Lay out arg/res types for C callbacks
        self.libfastfec.newPersistentMemoryContext.argtypes = []
        self.libfastfec.newPersistentMemoryContext.restype = c_void_p

        self.libfastfec.newFecContext.argtypes = [
            c_void_p,
            BUFFER_READ,
            c_int,
            CUSTOM_WRITE,
            c_int,
            CUSTOM_LINE,
            c_int,
            c_void_p,
            c_char_p,
            c_char_p,
            c_int,
            c_int,
            c_int,
        ]
        self.libfastfec.newFecContext.restype = c_void_p
        self.libfastfec.parseFec.argtypes = [c_void_p]
        self.libfastfec.parseFec.restype = c_int
        self.libfastfec.freeFecContext.argtypes = [c_void_p]
        self.libfastfec.freePersistentMemoryContext.argtypes = [c_void_p]


@contextlib.contextmanager
def FastFEC():  # pylint: disable=invalid-name
    """
    A convenience method to run fastfec and free memory afterwards

    Usage:

        with FastFEC() as fastfec:
            # Run fastfec parse methods here
            #
            # The memory is freed afterwards, so no need to ever call
            # fastfec.free()
            ...
    """
    instance = LibFastFEC()
    yield instance
    instance.free()
