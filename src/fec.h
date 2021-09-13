#pragma once

#include "memory.h"
#include "urlopen.h"
#include "writer.h"

typedef ssize_t (*GetLine)(STRING *line, void *file);
typedef int (*PutLine)(char *line, void *file);

struct fec_context
{
  // A way to pull line info
  GetLine getLine;
  void *file;

  WRITE_CONTEXT *writeContext;

  char *version; // default null
  int useAscii28;
  int summary; // default false
  char *f99Text;

  // Supporting line information
  PERSISTENT_MEMORY_CONTEXT *persistentMemory;
  int currentLineHasAscii28;
};
typedef struct fec_context FEC_CONTEXT;

FEC_CONTEXT *newFecContext(PERSISTENT_MEMORY_CONTEXT *persistentMemory, GetLine getLine, void *file, char *filingId, char *outputDirectory);

void freeFecContext(FEC_CONTEXT *context);

int parseFec(FEC_CONTEXT *ctx);