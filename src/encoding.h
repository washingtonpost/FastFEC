#pragma once

#include <stdio.h>
#include <stdint.h>
#include "memory.h"

struct lineInfo
{
  int ascii28;   // default false
  int asciiOnly; // default true
  int validUtf8; // default true
  int length;
};
typedef struct lineInfo LINE_INFO;

// Create a line info object by iterating the line
// and getting the line info for each character
void collectLineInfo(STRING *line, LINE_INFO *info);

// Ensure the passed in line is encoded in UTF-8 by transforming
// it to UTF-8 if necessary. The only other possible encodings
// are ASCII (no transformation necessary) and ISO-8859-1.
// Return the length of the resulting line.
int decodeLine(LINE_INFO *info, STRING *in, STRING *output);
