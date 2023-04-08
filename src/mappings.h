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

// Lookup the schema for a given version and form type.
// You must call freeSchema() on the result when you're done with it.
FORM_SCHEMA *lookupSchema(const char *version, int versionLength, const char *form, int formLength);
void freeSchema(FORM_SCHEMA *schema);
