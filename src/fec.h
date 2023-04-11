#pragma once

#include "export.h"
#include "buffer.h"
#include "mappings.h"
#include "memory.h"
#include "writer.h"
#include "buffer.h"

struct fec_context
{
  // A way to pull lines
  BUFFER *buffer;
  void *file;

  // A way to write lines
  WRITE_CONTEXT *writeContext;

  char *filingId;
  char *version; // default null
  int versionLength;
  int summary; // default false
  char *f99Text;

  // Supporting line information
  PERSISTENT_MEMORY_CONTEXT *persistentMemory;
  int currentLineHasAscii28;

  // Flags
  int includeFilingId;
  int silent;
  int warn;

  // Parse cache
  FORM_SCHEMA *currentForm;
};
typedef struct fec_context FEC_CONTEXT;

EXPORT FEC_CONTEXT *newFecContext(PERSISTENT_MEMORY_CONTEXT *persistentMemory, BufferRead bufferRead, int inputBufferSize, CustomWriteFunction customWriteFunction, int outputBufferSize, CustomLineFunction customLineFunction, int writeToFile, void *file, char *filingId, char *outputDirectory, int includeFilingId, int silent, int warn);
EXPORT void freeFecContext(FEC_CONTEXT *context);
EXPORT int parseFec(FEC_CONTEXT *ctx);

EXPORT int versionUsesAscii28(char *version);
