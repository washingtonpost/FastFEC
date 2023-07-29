#include "fec.h"

extern size_t wasmBufferRead(char *buffer, int want, void *data);
extern void wasmBufferWrite(char *filename, char *extension, char *contents, int numBytes);

void wasmFec(int bufferSize)
{
  PERSISTENT_MEMORY_CONTEXT *persistentMemory = newPersistentMemoryContext();
  CustomLineFunction customLineFunction = NULL;
  int writeToFile = 0;
  FILE *file = NULL;
  char *filingId = NULL;
  char *outputDirectory = NULL;
  int silent = 1;
  int warn = 0;
  int raw = 0;
  FEC_CONTEXT *fec = newFecContext(persistentMemory, ((BufferRead)(&wasmBufferRead)), bufferSize, ((CustomWriteFunction)(&wasmBufferWrite)), bufferSize, customLineFunction, writeToFile, file, outputDirectory, filingId, silent, warn, raw);
  int fecParseResult = parseFec(fec);
  freeFecContext(fec);
  freePersistentMemoryContext(persistentMemory);
}
