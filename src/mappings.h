#pragma once

#include "mappings_generated.h"

struct lookup_regexes
{
    pcre **headerVersions;
    pcre **headerFormTypes;
    pcre **typeVersions;
    pcre **typeFormTypes;
    pcre **typeHeaders;
};
typedef struct lookup_regexes LOOKUP_REGEXES;

// Functions to operate on mappings
static int numHeaders = sizeof(headers) / sizeof(headers[0]);
static int numTypes = sizeof(types) / sizeof(types[0]);

LOOKUP_REGEXES *getLookupRegexes(void);
