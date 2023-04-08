#pragma once

#include "mappings_generated.h"

struct lookup_regexes
{
    pcre **headerVersions;
    pcre **headerFormTypes;
    pcre **typeVersions;
    pcre **typeFormTypes;
    pcre **typeFieldNames;
};
typedef struct lookup_regexes LOOKUP_REGEXES;

static int NUM_HEADERS = sizeof(headers) / sizeof(headers[0]);

LOOKUP_REGEXES *getLookupRegexes(void);

// Lookup the type of a field, for a given version and form type.
char lookupType(char *version, int versionLength, char *form, int formLength, char *fieldName, int fieldNameLength);
