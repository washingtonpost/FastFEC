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
  // bool. If true, then each field in the input will be written as-is, even if
  // there are fewer or more fields than we expect (based on the form type and version).
  // If false, then we will add extra empty fields if there are too few,
  // and we will truncate extra fields if there are too many.
  // For more info see https://github.com/washingtonpost/FastFEC/issues/24
  int raw;

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
    char *outputDirectory,
    char *filingId,
    int silent,
    int warn,
    int raw);
EXPORT void freeFecContext(FEC_CONTEXT *context);
EXPORT int parseFec(FEC_CONTEXT *ctx);
