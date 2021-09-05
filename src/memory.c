#include "memory.h"
#include "string.h"

STRING *newString(size_t size)
{
  STRING *s = malloc(sizeof(STRING));
  s->str = malloc(size);
  s->n = size;
  return s;
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
  growStringTo(dst, src->n);
  // Copy the strings
  strcpy(dst->str, src->str);
}