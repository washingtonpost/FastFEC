#include "memory.h"
#include "regex.h"

static const int NUM_TYPES = sizeof(types) / sizeof(types[0]);

LOOKUP_REGEXES *getLookupRegexes(void)
{
    static LOOKUP_REGEXES *lookup = NULL;
    if (lookup != NULL)
    {
        return lookup;
    }
    lookup = malloc(sizeof(*lookup));

    lookup->headerVersions = malloc(sizeof(pcre *) * NUM_HEADERS);
    lookup->headerFormTypes = malloc(sizeof(pcre *) * NUM_HEADERS);

    lookup->typeVersions = malloc(sizeof(pcre *) * NUM_TYPES);
    lookup->typeFormTypes = malloc(sizeof(pcre *) * NUM_TYPES);
    lookup->typeFieldNames = malloc(sizeof(pcre *) * NUM_TYPES);

    for (int i = 0; i < NUM_HEADERS; i++)
    {
        lookup->headerVersions[i] = newRegex(headers[i][0]);
        lookup->headerFormTypes[i] = newRegex(headers[i][1]);
    }
    for (int i = 0; i < NUM_TYPES; i++)
    {
        lookup->typeVersions[i] = newRegex(types[i][0]);
        lookup->typeFormTypes[i] = newRegex(types[i][1]);
        lookup->typeFieldNames[i] = newRegex(types[i][2]);
    }

    return lookup;
}

char lookupType(char *version, int versionLength, char *form, int formLength, char *fieldName, int fieldNameLength)
{
    LOOKUP_REGEXES *lookup = getLookupRegexes();
    for (int i = 0; i < NUM_TYPES; i++)
    {
        int versionMatch = pcre_exec(lookup->typeVersions[i], NULL, version, versionLength, 0, 0, NULL, 0) >= 0;
        if (!versionMatch)
        {
            continue;
        }
        int formMatch = pcre_exec(lookup->typeFormTypes[i], NULL, form, formLength, 0, 0, NULL, 0) >= 0;
        if (!formMatch)
        {
            continue;
        }
        int headerMatch = pcre_exec(lookup->typeFieldNames[i], NULL, fieldName, fieldNameLength, 0, 0, NULL, 0) >= 0;
        if (!headerMatch)
        {
            continue;
        }
        return types[i][3][0];
    }
    // Default to string
    return 's';
}
