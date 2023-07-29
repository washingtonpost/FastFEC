#pragma once

#include "pcre/pcre.h"

// Allocate and compile a regex. Exit if it fails. Uses PCRE_CASELESS.
pcre *newRegex(const char *pattern);
