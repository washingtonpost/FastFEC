#include "fec.h"

extern size_t wasmBufferRead(char *buffer, int want, void *data);
extern void wasmBufferWrite(char *filename, char *extension, char *contents, int numBytes);

void wasmFec(int bufferSize)
{
  PERSISTENT_MEMORY_CONTEXT *persistentMemory = newPersistentMemoryContext();
  FEC_CONTEXT *fec = newFecContext(persistentMemory, ((BufferRead)(&wasmBufferRead)), bufferSize, ((CustomWriteFunction)(&wasmBufferWrite)), bufferSize, NULL, 0, NULL, NULL, NULL, 0, 1, 0);
  int fecParseResult = parseFec(fec);
  freeFecContext(fec);
  freePersistentMemoryContext(persistentMemory);
}
