#pragma once

#include "memory.h"
#include "writer.h"

struct field_info
{
  int num_commas;
  int num_quotes;
};
typedef struct field_info FIELD_INFO;

struct csv_field
{
  char *chars;
  size_t length;
  FIELD_INFO info;
};
typedef struct csv_field CSV_FIELD;

struct csv_line_parser
{
  STRING *line;
  int position;
  int columnIndex;
  CSV_FIELD currentField;
};

// ========== Parser API ============

// Parses a single line of comma (or ascii28) separated data
typedef struct csv_line_parser CSV_LINE_PARSER;
void csvParserInit(CSV_LINE_PARSER *parser, STRING *line);
// Read a field, either delimted by ascii28 or comma.
//
// For ascii28:
// If both the start and end of the field are `"`
// then return the field contents inside the quotes.
//
// For comma::
// Read a CSV field in-place, modifying line and returning start and
// end positions of the unescaped field. Since CSV fields are always
// longer escaped than not, this will always work in-place.
CSV_FIELD *readField(CSV_LINE_PARSER *parser, int useAscii28);
// Advance the parser to the next field. No-op if isParseDone().
void advanceField(CSV_LINE_PARSER *parser);
// The parse is done if a newline or NULL is encountered
int isParseDone(CSV_LINE_PARSER *parser);

// ========== Field API ============

// Trim whitespace in place
void stripWhitespace(CSV_FIELD *field);
void processFieldChar(char c, FIELD_INFO *info);

// ========== Write API ============

void writeDelimeter(WRITE_CONTEXT *context, const char *filename);
void writeNewline(WRITE_CONTEXT *context, const char *filename);
void writeField(WRITE_CONTEXT *context, const char *filename, const CSV_FIELD *field);
// Prints a string in the form YYYYMMDD to the form YYYY-MM-DD
// Returns 1 for success, 0 for failure to parse
int writeFieldDate(WRITE_CONTEXT *wctx, const char *filename, const CSV_FIELD *field);
// 1 for success, 0 for warning
int writeFieldFloat(WRITE_CONTEXT *wctx, const char *filename, const CSV_FIELD *field);
