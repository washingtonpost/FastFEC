import sys
import time
from ctypes import *
import os
from smart_open import open

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))


BUFFER_SIZE = 1024 * 1024
TRANSFER_BUFFER_SIZE = 128 * 1024
BUFFER_READ = CFUNCTYPE(c_size_t, POINTER(c_char), c_int, c_void_p)
CUSTOM_WRITE = CFUNCTYPE(None, c_char_p, c_char_p,
                         c_char_p, c_int)


def make_read_buffer(f):
    def read_buffer(buffer, want, _):
        contents_raw = f.read(want)
        received = len(contents_raw)
        contents = c_char_p(contents_raw)
        memmove(buffer, contents, received)
        return received
    return read_buffer


file_descriptors = {}
last_filename = None
last_fd = None
filing_id = '1464847'

bytes_written = {"bytes": 0}


def write_callback(filename, extension, contents, numBytes):
    global last_filename, last_fd
    if filename == last_filename:
        last_fd.write(contents[:numBytes])
    else:
        path = filename + extension
        fd = file_descriptors.get(path)
        if not fd:
            file_descriptors[path] = open(
                os.path.join(f's3://elex-fec-test/test-architecture/test-fastfec-output/{filing_id}', path.decode('utf8')) + '.gz', mode='wb', transport_params=dict(buffer_size=TRANSFER_BUFFER_SIZE))
            fd = file_descriptors[path]
        last_filename = filename
        last_fd = fd
        fd.write(contents[:numBytes])
    bytes_written['bytes'] += numBytes
    print(bytes_written['bytes'] / 1024.0 / 1024.0 / 1024.0)


class FastFEC:

    def init_lib(self):
        self.libfastfec = CDLL(os.path.join(SCRIPT_DIR, "../bin/fastfec.so"))

        # Lay out arg/res types
        self.libfastfec.newPersistentMemoryContext.argtypes = [c_int]
        self.libfastfec.newPersistentMemoryContext.restype = c_void_p

        self.libfastfec.newFecContext.argtypes = [
            c_void_p, BUFFER_READ, c_int, CUSTOM_WRITE, c_int, c_void_p, c_char_p, c_char_p, c_int, c_int]
        self.libfastfec.newFecContext.restype = c_void_p

        self.libfastfec.parseFec.argtypes = [c_void_p]
        self.libfastfec.parseFec.restype = c_int

        self.libfastfec.freeFecContext.argtypes = [c_void_p]

        self.libfastfec.freePersistentMemoryContext.argtypes = [c_void_p]

    def __init__(self, file_handle):
        self.init_lib()
        self.file_handle = file_handle

        # Initialize
        self.buffer_read_fn = BUFFER_READ(
            make_read_buffer(file_handle))
        self.write_callback_fn = CUSTOM_WRITE(write_callback)

        self.persistent_memory_context = self.libfastfec.newPersistentMemoryContext(
            1)
        self.fec_context = self.libfastfec.newFecContext(
            self.persistent_memory_context, self.buffer_read_fn, BUFFER_SIZE, self.write_callback_fn, BUFFER_SIZE, None, None, None, 0, 1)

    def parse(self):
        print("Parsing (py)")
        print(f'Parsed; status {self.libfastfec.parseFec(self.fec_context)}')

    def free(self):
        self.libfastfec.freeFecContext(self.fec_context)
        self.libfastfec.freePersistentMemoryContext(
            self.persistent_memory_context)


f = open(
    f's3://elex-fec-test/test-architecture/test-filings/{filing_id}.fec', mode='rb', transport_params=dict(buffer_size=TRANSFER_BUFFER_SIZE))
fast_fec = FastFEC(f)
fast_fec.parse()

# Free memory
fast_fec.free()
f.close()
for path in file_descriptors:
    file_descriptors[path].close()
