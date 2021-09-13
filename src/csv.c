
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

void writeDelimeter(WRITE_CONTEXT *context, char *filename)
{
  writeChar(context, filename, ',');
}

void writeNewline(WRITE_CONTEXT *context, char *filename)
{
  writeChar(context, filename, '\n');
}

static inline int endOfField(char c)
{
  return (c == ',') || (c == '\n') || (c == 0);
}

void readAscii28Field(PARSE_CONTEXT *parseContext)
{
  parseContext->start = parseContext->position;
  char c = parseContext->line->str[parseContext->position];
  processFieldChar(c, parseContext->fieldInfo);
  char startChar = c;
  char endChar = 0;
  while (c != 0 && c != 28 && c != '\n')
  {
    endChar = c;
    parseContext->position += 1;
    c = parseContext->line->str[parseContext->position];
    processFieldChar(c, parseContext->fieldInfo);
  }
  parseContext->end = parseContext->position;
  if (startChar == '"' && endChar == '"')
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

void readCsvField(PARSE_CONTEXT *parseContext)
{
  char c = parseContext->line->str[parseContext->position];
  if (endOfField(c))
  {
    // Empty field
    parseContext->start = parseContext->position;
    parseContext->end = parseContext->position;
    return;
  }

  processFieldChar(c, parseContext->fieldInfo);

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
        return;
      }
    }
    (parseContext->position)++;
  }
}

void advanceField(PARSE_CONTEXT *context)
{
  context->columnIndex++;
  context->position++;
}

void writeField(WRITE_CONTEXT *context, char *filename, STRING *line, int start, int end, FIELD_INFO *info)
{
  int escaped = info->num_commas || info->num_quotes;
  int copyDirectly = !info->num_quotes;

  if (escaped)
  {
    // Start of escape quote
    writeChar(context, filename, '"');
  }
  if (copyDirectly)
  {
    // No need for char-by-char writing
    writeN(context, filename, line->str + start, end - start);
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
        write(context, filename, "\"\"");
      }
      else
      {
        // Normal character
        writeChar(context, filename, c);
      }
    }
  }
  if (escaped)
  {
    // End of escape quote
    writeChar(context, filename, '"');
  }
}

int isWhitespace(PARSE_CONTEXT *context, int position)
{
  char c = context->line->str[position];
  return (c == ' ') || (c == '\t') || (c == '\n');
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
