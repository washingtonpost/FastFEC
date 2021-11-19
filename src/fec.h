#pragma once

#include "memory.h"
#include "urlopen.h"
#include "writer.h"
#include "buffer.h"

typedef int (*GetLine)(STRING *line, void *file);
typedef int (*PutLine)(char *line, void *file);

struct fec_context
{
  // A way to pull lines
  BUFFER *buffer;
  void *file;

  // A way to write lines
  CustomWriteFunction customWriteFunction;
  WRITE_CONTEXT *writeContext;

  char *filingId;
  char *version; // default null
  int versionLength;
  int useAscii28;
  int summary; // default false
  char *f99Text;

  // Supporting line information
  PERSISTENT_MEMORY_CONTEXT *persistentMemory;
  int currentLineHasAscii28;
  int currentLineLength;

  // Flags
  int includeFilingId;
  int silent;
  int suppress;

  // Parse cache
  char *formType;
  int numFields;
  char *headers; // pointer to static CSV header row info
  char *types;   // dynamically allocated string where each char indicates types

  // Special regex
  pcre *f99TextStart;
  pcre *f99TextEnd;
};
typedef struct fec_context FEC_CONTEXT;

FEC_CONTEXT *newFecContext(PERSISTENT_MEMORY_CONTEXT *persistentMemory, BufferRead bufferRead, int inputBufferSize, CustomWriteFunction customWriteFunction, int outputBufferSize, void *file, char *filingId, char *outputDirectory, int includeFilingId, int silent);

void freeFecContext(FEC_CONTEXT *context);

int parseFec(FEC_CONTEXT *ctx);