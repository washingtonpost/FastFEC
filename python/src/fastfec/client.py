"""A Python library to interface with FastFEC.

This library provides methods to
  * parse a .fec file line by line, yieling a parsed result
  * parse a .fec file into parsed output .csv files
"""
from __future__ import annotations

import contextlib
import io
import pathlib
from ctypes import CDLL, c_char_p, c_int, c_void_p
from queue import Queue
from threading import Thread
from typing import Any, Generator

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
    """Python wrapper for the fastfec library."""

    def __init__(self) -> None:
        self.__init_lib()

        # Initialize
        self.persistent_memory_context = self.libfastfec.newPersistentMemoryContext()

    def parse(
        self,
        file_handle: io.BinaryIO,
        include_filing_id: str | None = None,
        should_parse_date: bool = True,
    ) -> Generator[tuple[str, dict[str, Any]], None, None]:
        """Parses the input file line-by-line.

        Arguments:
        ---------
            file_handle -- An input stream for reading a .fec file
            include_filing_id -- If set, prepend a column into each outputted csv
                                 for filing_id with the specified filing id.
            should_parse_date -- If True, date fields are parsed to datetime.date.
                                 If False, date fields are returned as raw YYYY-MM-DD
                                 strings. This would mainly be set to false for
                                 performance reasons.

        Returns:
        -------
            A generator that receives the form name and a dictionary
            object describing each line in the file
        """
        # Set up a queue to be able to yield output
        queue = Queue()
        done_processing = object()  # A custom object to signal the end of processing

        # Prepare the filing id to include, if specified
        filing_id = as_bytes(include_filing_id)
        filing_id_included = filing_id is not None

        # Provide a custom line callback
        buffer_read_fn = provide_read_callback(file_handle)
        line_callback_fn = CUSTOM_LINE(
            provide_line_callback(queue, filing_id_included, should_parse_date),
        )
        fec_context = self.libfastfec.newFecContext(
            self.persistent_memory_context,  # persistentMemory
            buffer_read_fn,  # bufferRead
            BUFFER_SIZE,  # inputBufferSize
            CUSTOM_WRITE(0),  # customWriteFunction
            BUFFER_SIZE,  # outputBufferSize
            line_callback_fn,  # customLineFunction
            0,  # writeToFile
            None,  # file
            None,  # outputDirectory
            filing_id,  # filingId
            1,  # silent
            0,  # warn
        )

        # Run the parsing in a separate thread. It's essentially still single-threaded
        # but this provides a mechanism to yield the results of a callback function
        # from the caller. See https://stackoverflow.com/a/9968886 for more context
        def task():
            self.libfastfec.parseFec(fec_context)
            queue.put(done_processing)  # Signal processing is over

        Thread(target=task, args=(), daemon=True).start()

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

    def parse_as_files(
        self,
        file_handle: io.BinaryIO,
        output_directory: str | pathlib.Path,
        include_filing_id: str | None = None,
    ) -> int:
        """Parses the input file into output files in the output directory.

        Parent directories will be automatically created as needed.

        Arguments:
        ---------
            file_handle -- An input stream for reading a .fec file
            output_directory -- A directory in which to place output parsed .csv files
            include_filing_id -- If set, prepend a column `filing_id` into each
                                 outputted csv filled with the specified value.

        Returns:
        -------
            A status code. 1 indicates a successful parse, 0 an unsuccessful one.
        """
        out_path = pathlib.Path(output_directory)

        # Custom open method
        def open_output_file(form_type: str, *args, **kwargs):
            form_type = form_type.replace("/", "-")
            path = out_path / form_type
            path.parent.mkdir(exist_ok=True, parents=True)
            # pylint: disable=consider-using-with,unspecified-encoding,bad-option-value
            return open(path, *args, **kwargs)

        return self.parse_as_files_custom(
            file_handle,
            open_output_file,
            include_filing_id=include_filing_id,
        )

    def parse_as_files_custom(
        self,
        file_handle: io.BinaryIO,
        open_function,
        include_filing_id: str | None = None,
    ) -> int:
        """Parses the input file into output files.

        Arguments:
        ---------
            file_handle -- An input stream for reading a .fec file
            open_function -- A function to open an output file for writing, given
                             a form type. This can be set to customize the output
                             stream for each parsed .csv file
            include_filing_id -- If set, prepend a column `filing_id` into each
                                 outputted csv filled with the specified value.

        Returns:
        -------
            A status code. 1 indicates a successful parse, 0 an unsuccessful one.
        """
        # Set callbacks
        buffer_read_fn = provide_read_callback(file_handle)
        write_callback_fn, free_file_descriptors = provide_write_callback(open_function)

        # Prepare the filing id to include, if specified
        filing_id = as_bytes(include_filing_id)

        # Initialize fastfec context
        fec_context = self.libfastfec.newFecContext(
            self.persistent_memory_context,  # persistentMemory
            buffer_read_fn,  # bufferRead
            BUFFER_SIZE,  # inputBufferSize
            write_callback_fn,  # customWriteFunction
            BUFFER_SIZE,  # outputBufferSize
            CUSTOM_LINE(0),  # customLineFunction
            0,  # writeToFile
            None,  # file
            None,  # outputDirectory
            filing_id,  # filingId
            1,  # silent
            0,  # warn
        )

        # Parse
        result = self.libfastfec.parseFec(fec_context)

        # Free memory and file descriptors
        self.libfastfec.freeFecContext(fec_context)
        free_file_descriptors()

        return result

    def free(self) -> None:
        """Frees all the allocated memory from the fastfec library."""
        self.libfastfec.freePersistentMemoryContext(self.persistent_memory_context)

    def __init_lib(self) -> None:
        # Find the fastfec library
        self.libfastfec = CDLL(find_fastfec_lib())

        # Lay out arg/res types for C callbacks
        self.libfastfec.newPersistentMemoryContext.argtypes = []
        self.libfastfec.newPersistentMemoryContext.restype = c_void_p

        self.libfastfec.newFecContext.argtypes = [
            c_void_p,  # persistentMemory
            BUFFER_READ,  # bufferRead
            c_int,  # inputBufferSize
            CUSTOM_WRITE,  # customWriteFunction
            c_int,  # outputBufferSize
            CUSTOM_LINE,  # customLineFunction
            c_int,  # writeToFile
            c_void_p,  # file
            c_char_p,  # outputDirectory
            c_char_p,  # filingId
            c_int,  # silent
            c_int,  # warn
        ]
        self.libfastfec.newFecContext.restype = c_void_p
        self.libfastfec.parseFec.argtypes = [c_void_p]
        self.libfastfec.parseFec.restype = c_int
        self.libfastfec.freeFecContext.argtypes = [c_void_p]
        self.libfastfec.freePersistentMemoryContext.argtypes = [c_void_p]


@contextlib.contextmanager
def FastFEC() -> Generator[LibFastFEC, None, None]:  # pylint: disable=invalid-name
    """A convenience method to run fastfec and free memory afterwards.

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
