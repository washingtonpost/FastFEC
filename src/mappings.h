#pragma once

struct form_schema
{
    // Comma-delimited list of fields
    // eg "rec_type,form_type,back_reference_tran_id_number,text"
    char *headerString;
    int numFields;
    // Type codes. One char for each field in headerString, then null-terminated.
    char *fieldTypes;
};
typedef struct form_schema FORM_SCHEMA;

// Lookup the FORM_SCHEMA for a given version and form type.
// You must call formSchemaFree() on the result when you're done with it.
FORM_SCHEMA *formSchemaLookup(const char *version, int versionLength, const char *form, int formLength);
// Safe to pass in NULL
void formSchemaFree(FORM_SCHEMA *schema);
