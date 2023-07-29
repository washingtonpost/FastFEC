#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "pcre/pcre.h"
#include "export.h"

extern const size_t DEFAULT_STRING_SIZE;
struct string_type
{
  char *str; // Null-terminated string
  size_t n;  // Size of the string, including the null terminator
};
typedef struct string_type STRING;

// size must include the null terminator, so for "abc", size is 4
STRING *newString(size_t size);
STRING *fromString(const char *str);
// n is the number of characters to copy. Does not include the null terminator.
// The resulting string will have n characters then a null terminator.
STRING *fromChars(const char *chars, size_t n);

void setString(STRING *s, const char *str);

// OK to pass NULL
void freeString(STRING *s);

// Double the size of the string
int growString(STRING *str);

// Grow the string to the specified size (if needed)
// Size must include the null terminator, so for "abc", size is 4
int growStringTo(STRING *str, size_t newSize);

void copyString(STRING *src, STRING *dst);

struct persistent_memory_context
{
  STRING *rawLine;
  STRING *line;
  STRING *bufferLine;
};
typedef struct persistent_memory_context PERSISTENT_MEMORY_CONTEXT;

EXPORT PERSISTENT_MEMORY_CONTEXT *newPersistentMemoryContext(void);

EXPORT void freePersistentMemoryContext(PERSISTENT_MEMORY_CONTEXT *context);
