
#include "memory.h"
#include "csv.h"
#include "writer.h"

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

static inline int endOfField(char c)
{
  return (c == ',') || (c == '\n') || (c == 0);
}

void stripQuotes(PARSE_CONTEXT *parseContext)
{
  if ((parseContext->start != parseContext->end) && (parseContext->line->str[parseContext->start] == '"') && (parseContext->line->str[parseContext->end - 1] == '"'))
  {
    // Bump the field positions to avoid the quotes
    (parseContext->start)++;
    (parseContext->end)--;
    if (parseContext->fieldInfo)
    {
      (parseContext->fieldInfo->num_quotes) -= 2;
    }
  }
}

void readAscii28Field(PARSE_CONTEXT *parseContext)
{
  parseContext->start = parseContext->position;
  char c = parseContext->line->str[parseContext->position];
  processFieldChar(c, parseContext->fieldInfo);
  while (c != 0 && c != 28 && c != '\n')
  {
    parseContext->position += 1;
    c = parseContext->line->str[parseContext->position];
    processFieldChar(c, parseContext->fieldInfo);
  }
  parseContext->end = parseContext->position;
  stripQuotes(parseContext);
}

void readCsvSubfield(PARSE_CONTEXT *parseContext)
{
  char c = parseContext->line->str[parseContext->position];
  if (endOfField(c))
  {
    // Empty field
    parseContext->start = parseContext->position;
    parseContext->end = parseContext->position;
    return;
  }

  // If quoted, field is escaped
  int escaped = c == '"';
  int offset = 0;
  if (escaped)
  {
    (parseContext->position)++;
  }
  parseContext->start = parseContext->position;
  while (1)
  {
    if (offset != 0)
    {
      // If the offset is non-zero, we need to shift characters
      parseContext->line->str[(parseContext->position) - offset] = parseContext->line->str[parseContext->position];
    }

    c = parseContext->line->str[parseContext->position];
    if (c == 0)
    {
      // End of line
      parseContext->end = parseContext->position - offset;
      return;
    }
    if (!escaped && ((c == ',') || (c == '\n')))
    {
      // If not in escaped mode and the end of field is encountered
      // then we're done.
      parseContext->end = parseContext->position - offset;
      return;
    }
    processFieldChar(c, parseContext->fieldInfo);
    if (escaped && c == '"')
    {
      // If in escaped mode and a quote is encountered, then we need
      // to check if the quote is escaped.
      if (parseContext->line->str[parseContext->position + 1] == '"')
      {
        // If the quote is escaped, then we need to skip it.
        (parseContext->position)++;
        offset++;
      }
      else
      {
        // If the quote is not escaped, then we're done.
        parseContext->end = parseContext->position - offset;
        (parseContext->position)++;
        // Don't count trailing quote
        parseContext->fieldInfo->num_quotes--;
        return;
      }
    }
    (parseContext->position)++;
  }
}

void readCsvField(PARSE_CONTEXT *parseContext)
{
  readCsvSubfield(parseContext);
  stripQuotes(parseContext);
}

void advanceField(PARSE_CONTEXT *context)
{
  context->columnIndex++;
  context->position++;
}

void writeField(WRITE_CONTEXT *context, char *filename, const char *extension, STRING *line, int start, int end, FIELD_INFO *info)
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
    writeN(context, filename, extension, line->str + start, end - start);
  }
  else
  {
    // Copy string in char-by-char
    for (int i = start; i < end; i++)
    {
      char c = line->str[i];
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

int isWhitespaceChar(char c)
{
  return (c == ' ') || (c == '\t') || (c == '\n');
}

int isWhitespace(PARSE_CONTEXT *context, int position)
{
  return isWhitespaceChar(context->line->str[position]);
}

void stripWhitespace(PARSE_CONTEXT *context)
{
  // Strip leading whitespace
  while (isWhitespace(context, context->start) && (context->start < context->end))
  {
    (context->start)++;
  }

  // Strip trailing whitespace
  while (isWhitespace(context, context->end - 1) && (context->end > context->start))
  {
    (context->end)--;
  }
}
