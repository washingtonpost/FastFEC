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

  // Initialize all regular expressions
  ctx->headerVersions = malloc(sizeof(pcre *) * numHeaders);
  ctx->headerFormTypes = malloc(sizeof(pcre *) * numHeaders);
  ctx->typeVersions = malloc(sizeof(pcre *) * numTypes);
  ctx->typeFormTypes = malloc(sizeof(pcre *) * numTypes);
  ctx->typeHeaders = malloc(sizeof(pcre *) * numTypes);

  // Iterate and initialize all header regexes
  const char *error;
  int errorOffset;

  for (int i = 0; i < numHeaders; i++)
  {
    ctx->headerVersions[i] = pcre_compile(headers[i][0], PCRE_CASELESS, &error, &errorOffset, NULL);
    if (ctx->headerVersions[i] == NULL)
    {
      fprintf(stderr, "Regex header version compilation for \"%s\" failed at offset %d: %s\n", headers[i][0], errorOffset, error);
      exit(1);
    }
    ctx->headerFormTypes[i] = pcre_compile(headers[i][1], PCRE_CASELESS, &error, &errorOffset, NULL);
    if (ctx->headerFormTypes[i] == NULL)
    {
      fprintf(stderr, "Regex header form type compilation for \"%s\" failed at offset %d: %s\n", headers[i][1], errorOffset, error);
      exit(1);
    }
  }

  // Iterate and initialize all type regexes
  for (int i = 0; i < numTypes; i++)
  {
    ctx->typeVersions[i] = pcre_compile(types[i][0], PCRE_CASELESS, &error, &errorOffset, NULL);
    if (ctx->typeVersions[i] == NULL)
    {
      fprintf(stderr, "Regex type version compilation for \"%s\" failed at offset %d: %s\n", types[i][0], errorOffset, error);
      exit(1);
    }
    ctx->typeFormTypes[i] = pcre_compile(types[i][1], PCRE_CASELESS, &error, &errorOffset, NULL);
    if (ctx->typeFormTypes[i] == NULL)
    {
      fprintf(stderr, "Regex type form type compilation for \"%s\" failed at offset %d: %s\n", types[i][1], errorOffset, error);
      exit(1);
    }
    ctx->typeHeaders[i] = pcre_compile(types[i][2], PCRE_CASELESS, &error, &errorOffset, NULL);
    if (ctx->typeHeaders[i] == NULL)
    {
      fprintf(stderr, "Regex type header compilation for \"%s\" failed at offset %d: %s\n", types[i][2], errorOffset, error);
      exit(1);
    }
  }

  return ctx;
}

void freePersistentMemoryContext(PERSISTENT_MEMORY_CONTEXT *context)
{
  freeString(context->rawLine);
  freeString(context->line);
  freeString(context->bufferLine);

  // Free all regexes
  for (int i = 0; i < numHeaders; i++)
  {
    pcre_free(context->headerVersions[i]);
    pcre_free(context->headerFormTypes[i]);
  }
  for (int i = 0; i < numTypes; i++)
  {
    pcre_free(context->typeVersions[i]);
    pcre_free(context->typeFormTypes[i]);
    pcre_free(context->typeHeaders[i]);
  }
  free(context->headerVersions);
  free(context->headerFormTypes);
  free(context->typeVersions);
  free(context->typeFormTypes);
  free(context->typeHeaders);

  free(context);
}
