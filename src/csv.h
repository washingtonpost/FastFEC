#pragma once

#include "memory.h"
#include "writer.h"

struct field_info
{
  int num_commas;
  int num_quotes;
};
typedef struct field_info FIELD_INFO;

struct csv_line_parser
{
  STRING *line;
  FIELD_INFO fieldInfo;
  int position;
  int start;
  int end;
  int columnIndex;
};
// Parses a single line of comma (or ascii28) separated data
typedef struct csv_line_parser CSV_LINE_PARSER;

void csvParserInit(CSV_LINE_PARSER *parser, STRING *line);

void processFieldChar(char c, FIELD_INFO *info);

void writeDelimeter(WRITE_CONTEXT *context, const char *filename, const char *extension);
void writeNewline(WRITE_CONTEXT *context, const char *filename, const char *extension);
void csvWriteField(WRITE_CONTEXT *context, const char *filename, const char *extension, const char *str, int length, FIELD_INFO *info);
// Prints a string in the form YYYYMMDD to the form YYYY-MM-DD
// Returns 1 for success, 0 for failure to parse
int csvWriteFieldDate(WRITE_CONTEXT *wctx, const char *filename, const char *str, int length, FIELD_INFO *field);
// 1 for success, 0 for warning
int csvWriteFieldFloat(WRITE_CONTEXT *wctx, const char *filename, const char *str, int length, FIELD_INFO *field);

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
void readField(CSV_LINE_PARSER *parser, int useAscii28);

// Advance the parser to the next field. No-op if isParseDone().
void advanceField(CSV_LINE_PARSER *parser);

// Trim whitespace by adjusting start and end pointers in
// write context
void stripWhitespace(CSV_LINE_PARSER *parser);

// The parse is done if a newline or NULL is encountered
int isParseDone(CSV_LINE_PARSER *parser);
