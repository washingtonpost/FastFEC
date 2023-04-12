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

  STRING *version; // The version of the filing, eg "8.3". Default NULL

  // Supporting line information
  PERSISTENT_MEMORY_CONTEXT *persistentMemory;
  int currentLineHasAscii28;

  // Options
  // If non-null, then the generated output will include a column
  // at the beginning of every generated file called `filing_id`
  // that is filled with this value.
  char *filingId;
  int silent; // bool
  int warn;   // bool

  // Parse cache
  FORM_SCHEMA *currentForm;
};
typedef struct fec_context FEC_CONTEXT;

EXPORT FEC_CONTEXT *newFecContext(
    PERSISTENT_MEMORY_CONTEXT *persistentMemory,
    BufferRead bufferRead,
    int inputBufferSize,
    CustomWriteFunction customWriteFunction,
    int outputBufferSize,
    CustomLineFunction customLineFunction,
    int writeToFile,
    void *file,
    char *filingId,
    char *outputDirectory,
    int silent,
    int warn);
EXPORT void freeFecContext(FEC_CONTEXT *context);
EXPORT int parseFec(FEC_CONTEXT *ctx);
