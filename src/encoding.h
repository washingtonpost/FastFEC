#pragma once

#include <stdio.h>
#include <stdint.h>
#include "memory.h"

struct lineInfo
{
  int ascii28;   // default false
  int validUtf8; // default true
};
typedef struct lineInfo LINE_INFO;

// Ensure the passed in line is encoded in UTF-8 by transforming
// it to UTF-8 if necessary. The only other possible encodings
// are ASCII (no transformation necessary) and ISO-8859-1.
void decodeLine(LINE_INFO *info, STRING *in, STRING *output);
