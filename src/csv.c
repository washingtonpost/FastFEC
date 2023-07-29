
#include "memory.h"
#include "csv.h"
#include "writer.h"
#include "string_utils.h"

void csvParserInit(CSV_LINE_PARSER *parser, STRING *line)
{
  parser->line = line;
  parser->position = 0;
  parser->numFieldsRead = 0;
  parser->currentField.chars = NULL;
  parser->currentField.length = 0;
  parser->currentField.info.num_commas = 0;
  parser->currentField.info.num_quotes = 0;
}

int isParseDone(CSV_LINE_PARSER *parser)
{
  char c = parser->line->str[parser->position];
  return (c == 0) || (c == '\n');
}

void processFieldChar(char c, FIELD_INFO *info)
{
  if (info)
  {
    if (c == '"')
    {
      info->num_quotes++;
    }
    else if (c == ',')
    {
      info->num_commas++;
    }
  }
}

static void stripQuotes(CSV_FIELD *field)
{
  if ((field->length > 1) && (field->chars[0] == '"') && (field->chars[field->length - 1] == '"'))
  {
    // Bump the field positions to avoid the quotes
    (field->chars)++;
    (field->length) -= 2;
    (field->info.num_quotes) -= 2;
  }
}

static void readAscii28Field(CSV_LINE_PARSER *parser)
{
  int startPosition = parser->position;
  parser->currentField.chars = parser->line->str + parser->position;
  char c;
  while (1)
  {
    c = parser->line->str[parser->position];
    if (c == 0 || c == 28 || c == '\n')
    {
      break;
    }
    processFieldChar(c, &(parser->currentField.info));
    parser->position += 1;
  }
  parser->currentField.length = parser->position - startPosition;
  stripQuotes(&(parser->currentField));
}

static void readCsvSubfield(CSV_LINE_PARSER *parser)
{
  char c = parser->line->str[parser->position];
  // If first char is a quote, field is escaped
  int escaped = c == '"';
  int offset = 0;
  if (escaped)
  {
    (parser->position)++;
  }
  parser->currentField.chars = parser->line->str + parser->position;
  int startPosition = parser->position;
  while (1)
  {
    if (offset != 0)
    {
      // If the offset is non-zero, we need to shift characters
      parser->line->str[(parser->position) - offset] = parser->line->str[parser->position];
    }

    c = parser->line->str[parser->position];
    int is_EOF = (c == 0);
    int is_EOL = !escaped && ((c == ',') || (c == '\n'));
    if (is_EOF || is_EOL)
    {
      parser->currentField.length = (parser->position - startPosition) - offset;
      return;
    }
    processFieldChar(c, &(parser->currentField.info));
    if (escaped && c == '"')
    {
      // check if the quote is escaped.
      if (parser->line->str[parser->position + 1] != '"')
      {
        // If the quote is not escaped, then we're done.
        parser->currentField.length = (parser->position - startPosition) - offset;
        (parser->position)++;
        // Don't count trailing quote
        parser->currentField.info.num_quotes--;
        return;
      }
      // If the quote is escaped, then we need to skip it.
      (parser->position)++;
      offset++;
    }
    (parser->position)++;
  }
}

static void readCsvField(CSV_LINE_PARSER *parser)
{
  readCsvSubfield(parser);
  stripQuotes(&(parser->currentField));
}

CSV_FIELD *nextField(CSV_LINE_PARSER *parser, int useAscii28)
{
  // Reset field info
  parser->currentField.info.num_quotes = 0;
  parser->currentField.info.num_commas = 0;

  if (useAscii28)
  {
    readAscii28Field(parser);
  }
  else
  {
    readCsvField(parser);
  }
  parser->numFieldsRead++;
  if (!isParseDone(parser))
  {
    parser->position++;
  }
  return &(parser->currentField);
}

void writeDelimeter(WRITE_CONTEXT *context, const char *filename)
{
  writeChar(context, filename, CSV_EXTENSION, ',');
}

void writeNewline(WRITE_CONTEXT *context, const char *filename)
{
  writeChar(context, filename, CSV_EXTENSION, '\n');
}

void writeField(WRITE_CONTEXT *context, const char *filename, const CSV_FIELD *field)
{
  int hasQuotes = field->info.num_quotes > 0;
  int escaped = (field->info.num_commas > 0) || hasQuotes;
  if (escaped)
  {
    // Start of escape quote
    writeChar(context, filename, CSV_EXTENSION, '"');
  }
  if (!hasQuotes)
  {
    // No need for char-by-char writing
    writeN(context, filename, CSV_EXTENSION, field->chars, field->length);
  }
  else
  {
    // Copy string in char-by-char
    for (int i = 0; i < field->length; i++)
    {
      char c = field->chars[i];
      writeChar(context, filename, CSV_EXTENSION, c);
      if (c == '"')
      {
        // Double quotes, write again
        writeChar(context, filename, CSV_EXTENSION, c);
      }
    }
  }
  if (escaped)
  {
    // End of escape quote
    writeChar(context, filename, CSV_EXTENSION, '"');
  }
}

// 1 for success, 0 for failure
int writeFieldDate(WRITE_CONTEXT *wctx, const char *filename, const CSV_FIELD *field)
{
  if (field->length == 0)
  {
    // Empty field, not a problem
    return 1;
  }
  if (field->length != 8)
  {
    // write string as is
    writeField(wctx, filename, field);
    return 0;
  }

  writeN(wctx, filename, CSV_EXTENSION, field->chars, 4);
  writeChar(wctx, filename, CSV_EXTENSION, '-');
  writeN(wctx, filename, CSV_EXTENSION, field->chars + 4, 2);
  writeChar(wctx, filename, CSV_EXTENSION, '-');
  writeN(wctx, filename, CSV_EXTENSION, field->chars + 6, 2);
  return 1;
}

// 1 for success, 0 for warning
int writeFieldFloat(WRITE_CONTEXT *wctx, const char *filename, const CSV_FIELD *field)
{
  if (field->length == 0)
  {
    // Empty field, not a problem
    return 1;
  }
  char *parseEndLocation;
  double value = strtod(field->chars, &parseEndLocation);
  int conversionFailed = parseEndLocation == field->chars;
  if (conversionFailed)
  {
    // write string as is
    writeField(wctx, filename, field);
    return 0;
  }
  writeDouble(wctx, filename, value);
  return 1;
}

void stripWhitespace(CSV_FIELD *field)
{
  // Strip leading whitespace
  while (strIsWhitespace(field->chars[0]) && (field->length > 0))
  {
    (field->chars)++;
    (field->length)--;
  }

  // Strip trailing whitespace
  while (strIsWhitespace(field->chars[field->length - 1]) && (field->length > 0))
  {
    (field->length)--;
  }
}
