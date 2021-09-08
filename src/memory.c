#include "memory.h"
#include "string.h"

const size_t DEFAULT_STRING_SIZE = 256;

STRING *newString(size_t size)
{
  STRING *s = malloc(sizeof(STRING));
  s->str = malloc(size);
  s->n = size;
  return s;
}

STRING *fromString(const char *str)
{
  STRING *s = newString(strlen(str) + 1);
  strcpy(s->str, str);
  return s;
}

void setString(STRING *s, const char *str)
{
  growStringTo(s, strlen(str) + 1);
  strcpy(s->str, str);
}

void freeString(STRING *s)
{
  free(s->str);
  free(s);
}

int growStringTo(STRING *str, size_t newSize)
{
  // Grow the size
  if (newSize > str->n)
  {
    str->n = newSize;
  }
  else
  {
    // No need to reallocate
    return 1;
  }

  // Reallocate the space
  str->str = realloc(str->str, str->n);

  // Check if the reallocation failed
  if (str->str == 0)
  {
    return 0;
  }
  return 1;
}

int growString(STRING *str)
{
  // Double the space available
  return growStringTo(str, str->n * 2);
}

void copyString(STRING *src, STRING *dst)
{
  // Check if dst has enough space
  growStringTo(dst, src->n + 1);
  // Copy the strings
  strcpy(dst->str, src->str);
}

PERSISTENT_MEMORY_CONTEXT *newPersistentMemoryContext()
{
  PERSISTENT_MEMORY_CONTEXT *ctx = malloc(sizeof(PERSISTENT_MEMORY_CONTEXT));
  ctx->rawLine = newString(DEFAULT_STRING_SIZE);
  ctx->line = newString(DEFAULT_STRING_SIZE);
  ctx->bufferLine = newString(DEFAULT_STRING_SIZE);
  return ctx;
}

void freePersistentMemoryContext(PERSISTENT_MEMORY_CONTEXT *context)
{
  freeString(context->rawLine);
  freeString(context->line);
  freeString(context->bufferLine);
  free(context);
}