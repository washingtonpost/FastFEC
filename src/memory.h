#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "pcre/pcre.h"
#include "export.h"
#include "mappings.h"

extern const size_t DEFAULT_STRING_SIZE;
struct string_type
{
  char *str;
  size_t n;
};
typedef struct string_type STRING;

STRING *newString(size_t size);

STRING *fromString(const char *);

void setString(STRING *s, const char *str);

void freeString(STRING *s);

// Double the size of the string
int growString(STRING *str);

// Grow the string to the specified size.
// If the new size is less than the old size, keep the old size.
int growStringTo(STRING *str, size_t newSize);

void copyString(STRING *src, STRING *dst);

struct persistent_memory_context
{
  STRING *rawLine;
  STRING *line;
  STRING *bufferLine;

  pcre **headerVersions;
  pcre **headerFormTypes;
  pcre **typeVersions;
  pcre **typeFormTypes;
  pcre **typeHeaders;
};
typedef struct persistent_memory_context PERSISTENT_MEMORY_CONTEXT;

EXPORT PERSISTENT_MEMORY_CONTEXT *newPersistentMemoryContext();

EXPORT void freePersistentMemoryContext(PERSISTENT_MEMORY_CONTEXT *context);
