#pragma once

#include "memory.h"
#include "urlopen.h"

typedef ssize_t (*GetLine)(STRING *line, void *file);
typedef int (*PutLine)(char *line, void *file);

struct fec_context
{
  // A way to pull line info
  GetLine getLine;
  void *file;

  PutLine putLine;
  void *outFile;

  STRING *version; // default null
  int summary;     // default false
  STRING *f99Text;

  // Supporting line information
  PERSISTENT_MEMORY_CONTEXT *persistentMemory;
};
typedef struct fec_context FEC_CONTEXT;

FEC_CONTEXT *newFecContext(PERSISTENT_MEMORY_CONTEXT *persistentMemory, GetLine getLine, void *file, PutLine putLine, void *outFile);

void freeFecContext(FEC_CONTEXT *context);

int parseFec(FEC_CONTEXT *ctx);