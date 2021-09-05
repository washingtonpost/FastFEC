#include "fec.h"
#include "encoding.h"

FEC_CONTEXT *newFecContext(PERSISTENT_MEMORY_CONTEXT *persistentMemory, GetLine getLine, void *file, PutLine putLine, void *outFile)
{
  FEC_CONTEXT *ctx = (FEC_CONTEXT *)malloc(sizeof(FEC_CONTEXT));
  ctx->persistentMemory = persistentMemory;
  ctx->getLine = getLine;
  ctx->file = file;
  ctx->putLine = putLine;
  ctx->outFile = outFile;
  ctx->version = 0;
  ctx->summary = 0;
  ctx->f99Text = 0;
  return ctx;
}

void freeFecContext(FEC_CONTEXT *ctx)
{
  if (ctx->version)
  {
    free(ctx->version);
  }
  if (ctx->f99Text)
  {
    free(ctx->f99Text);
  }
  free(ctx);
}

int parseFec(FEC_CONTEXT *ctx)
{
  ssize_t bytesRead;
  while (1)
  {
    bytesRead = ctx->getLine(ctx->persistentMemory->rawLine, ctx->file);
    if (bytesRead <= 0)
    {
      break;
    }

    // Decode the line
    LINE_INFO info;
    decodeLine(&info, ctx->persistentMemory->rawLine, ctx->persistentMemory->line);

    ctx->putLine(ctx->persistentMemory->line->str, ctx->outFile);
  }
  return 1;
}