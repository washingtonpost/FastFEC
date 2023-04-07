#include <stdio.h>
#include "regex.h"

pcre *newRegex(const char *pattern)
{
    const char *error;
    int errorOffset;
    pcre *regex = pcre_compile(pattern, PCRE_CASELESS, &error, &errorOffset, NULL);
    if (regex == NULL)
    {
        fprintf(stderr, "Regex '%s' compilation failed at offset %d: %s\n", pattern, errorOffset, error);
        exit(1);
    }
    return regex;
}
