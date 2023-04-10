
#include "memory.h"
#include "csv.h"
#include "writer.h"
#include "string_utils.h"

void csvParserInit(CSV_LINE_PARSER *parser, STRING *line)
{
  parser->line = line;
  parser->fieldInfo.num_commas = 0;
  parser->fieldInfo.num_quotes = 0;
  parser->position = 0;
  parser->start = 0;
  parser->end = 0;
  parser->columnIndex = 0;
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

void writeDelimeter(WRITE_CONTEXT *context, char *filename, const char *extension)
{
  writeChar(context, filename, extension, ',');
}

void writeNewline(WRITE_CONTEXT *context, char *filename, const char *extension)
{
  writeChar(context, filename, extension, '\n');
}

void stripQuotes(CSV_LINE_PARSER *parser)
{
  if ((parser->start != parser->end) && (parser->line->str[parser->start] == '"') && (parser->line->str[parser->end - 1] == '"'))
  {
    // Bump the field positions to avoid the quotes
    (parser->start)++;
    (parser->end)--;
    (parser->fieldInfo.num_quotes) -= 2;
  }
}

void readAscii28Field(CSV_LINE_PARSER *parser)
{
  parser->start = parser->position;
  char c = parser->line->str[parser->position];
  processFieldChar(c, &(parser->fieldInfo));
  while (c != 0 && c != 28 && c != '\n')
  {
    parser->position += 1;
    c = parser->line->str[parser->position];
    processFieldChar(c, &(parser->fieldInfo));
  }
  parser->end = parser->position;
  stripQuotes(parser);
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
  parser->start = parser->position;
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
      parser->end = parser->position - offset;
      return;
    }
    processFieldChar(c, &(parser->fieldInfo));
    if (escaped && c == '"')
    {
      // check if the quote is escaped.
      if (parser->line->str[parser->position + 1] != '"')
      {
        // If the quote is not escaped, then we're done.
        parser->end = parser->position - offset;
        (parser->position)++;
        // Don't count trailing quote
        parser->fieldInfo.num_quotes--;
        return;
      }
      // If the quote is escaped, then we need to skip it.
      (parser->position)++;
      offset++;
    }
    (parser->position)++;
  }
}

void readCsvField(CSV_LINE_PARSER *parser)
{
  readCsvSubfield(parser);
  stripQuotes(parser);
}

void readField(CSV_LINE_PARSER *parser, int useAscii28)
{
  // Reset field info
  parser->fieldInfo.num_quotes = 0;
  parser->fieldInfo.num_commas = 0;

  if (useAscii28)
  {
    readAscii28Field(parser);
  }
  else
  {
    readCsvField(parser);
  }
}

void advanceField(CSV_LINE_PARSER *parser)
{
  if (isParseDone(parser))
  {
    return;
  }
  parser->columnIndex++;
  parser->position++;
}

void writeField(WRITE_CONTEXT *context, char *filename, const char *extension, const char *str, int length, FIELD_INFO *info)
{
  int escaped = (info->num_commas > 0) || (info->num_quotes > 0);
  int copyDirectly = !(info->num_quotes > 0);

  if (escaped)
  {
    // Start of escape quote
    writeChar(context, filename, extension, '"');
  }
  if (copyDirectly)
  {
    // No need for char-by-char writing
    writeN(context, filename, extension, str, length);
  }
  else
  {
    // Copy string in char-by-char
    for (int i = 0; i < length; i++)
    {
      char c = str[i];
      if (c == '"')
      {
        // Double quotes
        writeString(context, filename, extension, "\"\"");
      }
      else
      {
        // Normal character
        writeChar(context, filename, extension, c);
      }
    }
  }
  if (escaped)
  {
    // End of escape quote
    writeChar(context, filename, extension, '"');
  }
}

int isWhitespace(CSV_LINE_PARSER *parser, int position)
{
  return strIsWhitespace(parser->line->str[position]);
}

void stripWhitespace(CSV_LINE_PARSER *parser)
{
  // Strip leading whitespace
  while (isWhitespace(parser, parser->start) && (parser->start < parser->end))
  {
    (parser->start)++;
  }

  // Strip trailing whitespace
  while (isWhitespace(parser, parser->end - 1) && (parser->end > parser->start))
  {
    (parser->end)--;
  }
}
