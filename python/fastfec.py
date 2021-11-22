import sys, getopt
from ctypes import *
import os
import pathlib
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


class FastFEC:
    def __init__(self, file_handle, output_directory, use_compression=False, make_dirs=False):
        self.init_lib()
        self.file_handle = file_handle
        self.use_compression=use_compression
        self.make_dirs = make_dirs

        # Initialize
        self.file_descriptors = {}
        self.last_filename = None
        self.last_fd = None
        self.bytes_written = 0
        self.buffer_read_fn = BUFFER_READ(
            make_read_buffer(file_handle))
        write_callback = self.provide_write_callback(output_directory)
        self.write_callback_fn = CUSTOM_WRITE(write_callback)

        self.persistent_memory_context = self.libfastfec.newPersistentMemoryContext(
            1)
        self.fec_context = self.libfastfec.newFecContext(
            self.persistent_memory_context, self.buffer_read_fn, BUFFER_SIZE, self.write_callback_fn, BUFFER_SIZE, None, None, None, 0, 1)

    def init_lib(self):
        # TODO: ensure this works on any platform
        self.libfastfec = CDLL(os.path.join(SCRIPT_DIR, "../zig-out/lib/libfastfec.dylib"))

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

    def provide_write_callback(self, output_directory):
        def write_callback(filename, extension, contents, numBytes):
            if filename == self.last_filename:
                self.last_fd.write(contents[:numBytes])
            else:
                path = filename + extension
                fd = self.file_descriptors.get(path)
                if not fd:
                    if self.make_dirs:
                        pathlib.Path(output_directory).mkdir(parents=True, exist_ok=True)
                    self.file_descriptors[path] = open(
                        os.path.join(output_directory, f'{path.decode("utf8")}{".gz" if self.use_compression else ""}'), mode='wb', transport_params=dict(buffer_size=TRANSFER_BUFFER_SIZE))
                    fd = self.file_descriptors[path]
                self.last_filename = filename
                self.last_fd = fd
                fd.write(contents[:numBytes])
            self.bytes_written += numBytes
            print(self.bytes_written / 1024.0 / 1024.0 / 1024.0)
        return write_callback

    def parse(self):
        print("Parsing (py)")
        print(f'Parsed; status {self.libfastfec.parseFec(self.fec_context)}')

    def free(self):
        self.libfastfec.freeFecContext(self.fec_context)
        self.libfastfec.freePersistentMemoryContext(
            self.persistent_memory_context)
        
        # Close open file descriptors
        for path in self.file_descriptors:
            self.file_descriptors[path].close()

def main(argv):
   filing_id = ''
   input_directory = ''
   output_directory = ''
   use_compression = False
   make_dirs = False
   try:
      opts, _ = getopt.getopt(argv, 'hf:i:o:z:m:')
   except getopt.GetoptError:
      print('fastfec.py -f <filing_id> -i <input_directory> -o <output_directory> -z <use_compression=false> -m <make_dirs=false>')
      sys.exit(2)
   for opt, arg in opts:
      if opt == '-h':
         print('fastfec.py -f <filing_id> -i <input_directory> -o <output_directory> -z <use_compression=false> -m <make_dirs=false>')
         sys.exit()
      elif opt in ("-f", "--filing_id"):
         filing_id = arg
      elif opt in ("-i", "--input_directory"):
         input_directory = arg
      elif opt in ("-o", "--output_directory"):
         output_directory = arg
      elif opt in ("-z", "--use_compression"):
         use_compression = arg.lower() != 'false'
      elif opt in ("-m", "--make_dirs"):
         make_dirs = arg.lower() != 'false'
   input_file = f'{os.path.join(input_directory, filing_id)}.fec'
   output_directory = os.path.join(output_directory, filing_id)
   print(f'Filing ID is {filing_id}')
   print(f'Input file is {input_file}')
   print(f'Output directory is {output_directory}')
   print(f'Make parent directories: {make_dirs}')

   f = open(
     input_file, mode='rb', transport_params=dict(buffer_size=TRANSFER_BUFFER_SIZE))
   fast_fec = FastFEC(f, output_directory, use_compression=use_compression, make_dirs=make_dirs)
   fast_fec.parse()

   # Free memory
   fast_fec.free()
   f.close()

if __name__ == "__main__":
   main(sys.argv[1:])
