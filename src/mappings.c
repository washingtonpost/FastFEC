#include <string.h>
#include "csv.h"
#include "memory.h"
#include "regex.h"
#include "mappings_generated.h"

static int NUM_HEADERS = sizeof(headers) / sizeof(headers[0]);
static const int NUM_TYPES = sizeof(types) / sizeof(types[0]);

struct lookup_regexes
{
    pcre **headerVersions;
    pcre **headerFormTypes;
    pcre **typeVersions;
    pcre **typeFormTypes;
    pcre **typeFieldNames;
};
typedef struct lookup_regexes LOOKUP_REGEXES;

static LOOKUP_REGEXES *getLookupRegexes(void)
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

static char lookupType(const char *version, int versionLength, const char *form, int formLength, const char *fieldName, int fieldNameLength)
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

// Lookup the types for a given version, form type, and header string.
// The types are placed into the types array. This assumes that the types array
// is large enough to hold all the types, plus a null terminator.
// Returns the number of types/fields found.
static int lookupTypes(char *types, const char *version, int versionLength, const char *form, int formLength, const char *headerString)
{
    STRING *s = fromString(headerString);
    CSV_LINE_PARSER headerParser;
    csvParserInit(&headerParser, s);

    // Iterate each field in the header and build up the type info
    while (!isParseDone(&headerParser))
    {
        readField(&headerParser, 0);
        char *fieldName = headerParser.line->str + headerParser.start;
        int fieldNameLength = headerParser.end - headerParser.start;
        types[headerParser.columnIndex] = lookupType(version, versionLength, form, formLength, fieldName, fieldNameLength);
        advanceField(&headerParser);
    }
    freeString(s);
    // Add null terminator
    types[headerParser.columnIndex + 1] = 0;
    return headerParser.columnIndex + 1;
}

FORM_SCHEMA *formSchemaLookup(const char *version, int versionLength, const char *form, int formLength)
{
    // Find the headerString given the version and form type
    LOOKUP_REGEXES *lookup = getLookupRegexes();
    for (int i = 0; i < NUM_HEADERS; i++)
    {
        int versionMatch = pcre_exec(lookup->headerVersions[i], NULL, version, versionLength, 0, 0, NULL, 0) >= 0;
        if (!versionMatch)
        {
            continue;
        }
        int formMatch = pcre_exec(lookup->headerFormTypes[i], NULL, form, formLength, 0, 0, NULL, 0) >= 0;
        if (!formMatch)
        {
            continue;
        }

        // Now find the types given the version, form type, and header string
        // We aren't allocating any new memory here for the headerString,
        // only referencing the static data in the headers array. Don't free it later.
        char *headerString = headers[i][2];
        char *types = malloc(strlen(headerString) + 1); // at least as big as it needs to be
        int numFields = lookupTypes(types, version, versionLength, form, formLength, headerString);

        FORM_SCHEMA *result = malloc(sizeof(*result));
        result->headerString = headerString;
        result->numFields = numFields;
        result->fieldTypes = types;
        return result;
    }
    return NULL;
}

void formSchemaFree(FORM_SCHEMA *schema)
{
    if (schema == NULL)
    {
        return;
    }
    free(schema->fieldTypes);
    free(schema);
}
