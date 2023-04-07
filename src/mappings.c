#include "memory.h"
#include "regex.h"

LOOKUP_REGEXES *getLookupRegexes(void)
{
    static LOOKUP_REGEXES *lookup = NULL;
    if (lookup != NULL)
    {
        return lookup;
    }
    lookup = malloc(sizeof(*lookup));

    // Initialize all regular expressions
    lookup->headerVersions = malloc(sizeof(pcre *) * numHeaders);
    lookup->headerFormTypes = malloc(sizeof(pcre *) * numHeaders);

    lookup->typeVersions = malloc(sizeof(pcre *) * numTypes);
    lookup->typeFormTypes = malloc(sizeof(pcre *) * numTypes);
    lookup->typeHeaders = malloc(sizeof(pcre *) * numTypes);

    // Iterate and initialize all regexes
    for (int i = 0; i < numHeaders; i++)
    {
        lookup->headerVersions[i] = newRegex(headers[i][0]);
        lookup->headerFormTypes[i] = newRegex(headers[i][1]);
    }
    for (int i = 0; i < numTypes; i++)
    {
        lookup->typeVersions[i] = newRegex(types[i][0]);
        lookup->typeFormTypes[i] = newRegex(types[i][1]);
        lookup->typeHeaders[i] = newRegex(types[i][2]);
    }

    return lookup;
}
