#pragma once

#include "memory.h"

// Ensure the passed in line is encoded in UTF-8 by transforming
// it to UTF-8 if necessary. The only other possible encodings
// are ASCII (no transformation necessary) and ISO-8859-1.
//
// Returns 1 if the line contains ascii 28, 0 otherwise.
int decodeLine(STRING *in, STRING *output);
