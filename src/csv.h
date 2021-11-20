#pragma once

#include "memory.h"
#include "writer.h"

struct field_info
{
  int num_commas;
  int num_quotes;
};
typedef struct field_info FIELD_INFO;

struct parse_context
{
  STRING *line;
  FIELD_INFO *fieldInfo;
  int position;
  int start;
  int end;
  int columnIndex;
};
typedef struct parse_context PARSE_CONTEXT;

void processFieldChar(char c, FIELD_INFO *info);

void writeDelimeter(WRITE_CONTEXT *context, char *filename, const char *extension);

void writeNewline(WRITE_CONTEXT *context, char *filename, const char *extension);

static inline int endOfField(char c);

// Read a field from a file delimited by the character with the
// ascii code 28. If both the start and end of the field are `"`
// then return the field contents inside the quotes.
void readAscii28Field(PARSE_CONTEXT *parseContext);

// Read a CSV field in-place, modifying line and returning start and
// end positions of the unescaped field. Since CSV fields are always
// longer escaped than not, this will always work in-place.
void readCsvField(PARSE_CONTEXT *parseContext);

// Advance past the delimeter and increase the column index
void advanceField(PARSE_CONTEXT *parseContext);

void writeField(WRITE_CONTEXT *context, char *filename, const char *extension, STRING *line, int start, int end, FIELD_INFO *info);

int isWhitespaceChar(char c);

// Trim whitespace by adjusting start and end pointers in
// write context
void stripWhitespace(PARSE_CONTEXT *context);
