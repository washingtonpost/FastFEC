#include "fec.h"
#include "encoding.h"
#include "writer.h"
#include "csv.h"
#include "mappings.h"
#include "buffer.h"
#include "string_utils.h"
#include "regex.h"
#include <string.h>
#include <stdarg.h>

char *HEADER = "header";
char *SCHEDULE_COUNTS = "SCHEDULE_COUNTS_";
char *FEC_VERSION_NUMBER = "fec_ver_#";
char *FEC = "FEC";

void ctxWarn(FEC_CONTEXT *ctx, const char *message, ...)
{
  va_list args;
  va_start(args, message);

  if (ctx->warn)
  {
    fprintf(stderr, "Warning: ");
    vfprintf(stderr, message, args);
  }
  va_end(args);
}

void freeSafe(void *ptr)
{
  if (ptr != NULL)
  {
    free(ptr);
  }
}

int _lineReMatch(FEC_CONTEXT *ctx, pcre *regex)
{
  char *str = ctx->persistentMemory->line->str;
  int len = ctx->currentLineLength;
  return pcre_exec(regex, NULL, str, len, 0, 0, NULL, 0) >= 0;
}

int lineContainsF99Start(FEC_CONTEXT *ctx)
{
  static pcre *regex = NULL;
  if (regex == NULL)
  {
    regex = newRegex("^\\s*\\[BEGIN ?TEXT\\]\\s*$");
  }
  return _lineReMatch(ctx, regex);
}

int lineContainsF99End(FEC_CONTEXT *ctx)
{
  static pcre *regex = NULL;
  if (regex == NULL)
  {
    regex = newRegex("^\\s*\\[END ?TEXT\\]\\s*$");
  }
  return _lineReMatch(ctx, regex);
}

FEC_CONTEXT *newFecContext(PERSISTENT_MEMORY_CONTEXT *persistentMemory, BufferRead bufferRead, int inputBufferSize, CustomWriteFunction customWriteFunction, int outputBufferSize, CustomLineFunction customLineFunction, int writeToFile, void *file, char *filingId, char *outputDirectory, int includeFilingId, int silent, int warn)
{
  FEC_CONTEXT *ctx = (FEC_CONTEXT *)malloc(sizeof(FEC_CONTEXT));
  ctx->persistentMemory = persistentMemory;
  ctx->buffer = newBuffer(inputBufferSize, bufferRead);
  ctx->file = file;
  ctx->writeContext = newWriteContext(outputDirectory, filingId, writeToFile, outputBufferSize, customWriteFunction, customLineFunction);
  ctx->filingId = filingId;
  ctx->version = 0;
  ctx->versionLength = 0;
  ctx->summary = 0;
  ctx->f99Text = 0;
  ctx->currentLineHasAscii28 = 0;
  ctx->currentLineLength = 0;
  ctx->formType = NULL;
  ctx->numFields = 0;
  ctx->headers = NULL;
  ctx->types = NULL;
  ctx->includeFilingId = includeFilingId;
  ctx->silent = silent;
  ctx->warn = warn;
  return ctx;
}

void freeFecContext(FEC_CONTEXT *ctx)
{
  freeBuffer(ctx->buffer);
  freeSafe(ctx->version);
  freeSafe(ctx->f99Text);
  freeSafe(ctx->formType);
  freeSafe(ctx->types);
  freeWriteContext(ctx->writeContext);
  free(ctx);
}

// Return 1 on success, 0 on failure
static int updateCurrentFormSchema(FEC_CONTEXT *ctx, const char *form, int formLength)
{
  // If type mappings are unchanged from before we can return early
  if ((ctx->formType != NULL) && (strncmp(ctx->formType, form, formLength) == 0))
  {
    return 1;
  }

  FORM_SCHEMA *schema = formSchemaLookup(ctx->version, ctx->versionLength, form, formLength);
  if (schema == NULL)
  {
    ctxWarn(ctx, "No mappings found for version %s and form ", ctx->version, form);
    return 0;
  }

  // Copy form type
  freeSafe(ctx->formType);
  ctx->formType = malloc(formLength + 1);
  strncpy(ctx->formType, form, formLength);
  ctx->formType[formLength] = 0;

  // Copy header string
  freeSafe(ctx->headers);
  ctx->headers = malloc(strlen(schema->headerString) + 1);
  strcpy(ctx->headers, schema->headerString);

  // Copy number of fields
  ctx->numFields = schema->numFields;

  // Copy field types
  freeSafe(ctx->types);
  ctx->types = malloc(strlen(schema->fieldTypes) + 1);
  strcpy(ctx->types, schema->fieldTypes);

  formSchemaFree(schema);

  return 1;
}

void ctxWriteSubstr(FEC_CONTEXT *ctx, char *filename, int start, int end, FIELD_INFO *field)
{
  const char *str = ctx->persistentMemory->line->str + start;
  int len = end - start;
  writeField(ctx->writeContext, filename, CSV_EXTENSION, str, len, field);
}

void writeQuotedString(WRITE_CONTEXT *ctx, char *filename, char *str, int length)
{
  for (int i = 0; i < length; i++)
  {
    char c = str[i];
    if (c == '"')
    {
      // Write two quotes since the field is quoted
      writeChar(ctx, filename, CSV_EXTENSION, '"');
      writeChar(ctx, filename, CSV_EXTENSION, '"');
    }
    else
    {
      writeChar(ctx, filename, CSV_EXTENSION, c);
    }
  }
}

// Write a date field by separating the output with dashes
void writeDateField(FEC_CONTEXT *ctx, char *filename, int start, int end, FIELD_INFO *field)
{
  if (start == end)
  {
    // Empty field
    return;
  }
  if (end - start != 8)
  {
    // Could not parse date, write string as is and log warning
    ctxWarn(ctx, "Date fields must be exactly 8 chars long, not %d\n", end - start);
    ctxWriteSubstr(ctx, filename, start, end, field);
    return;
  }

  ctxWriteSubstr(ctx, filename, start, start + 4, field);
  writeChar(ctx->writeContext, filename, CSV_EXTENSION, '-');
  ctxWriteSubstr(ctx, filename, start + 4, start + 6, field);
  writeChar(ctx->writeContext, filename, CSV_EXTENSION, '-');
  ctxWriteSubstr(ctx, filename, start + 6, start + 8, field);
}

void writeFloatField(FEC_CONTEXT *ctx, char *filename, int start, int end, FIELD_INFO *field)
{
  char *doubleStr;
  char *conversionFloat = ctx->persistentMemory->line->str + start;
  double value = strtod(conversionFloat, &doubleStr);

  if (doubleStr == conversionFloat)
  {
    // Could not convert to a float, write string as is and log warning
    ctxWarn(ctx, "Could not parse float field\n");
    ctxWriteSubstr(ctx, filename, start, end, field);
    return;
  }

  // Write the value
  writeDouble(ctx->writeContext, filename, value);
}

// Grab a line from the input file.
// Return 0 if there are no lines left.
// If there is a line, decode it into
// ctx->persistentMemory->line.
int grabLine(FEC_CONTEXT *ctx)
{
  int bytesRead = readLine(ctx->buffer, ctx->persistentMemory->rawLine, ctx->file);
  if (bytesRead <= 0)
  {
    return 0;
  }

  // Decode the line
  LINE_INFO info;
  ctx->currentLineLength = decodeLine(&info, ctx->persistentMemory->rawLine, ctx->persistentMemory->line);
  // Store whether the current line has ascii separators
  // (determines whether we use CSV or ascii28 split line parsing)
  ctx->currentLineHasAscii28 = info.ascii28;
  return 1;
}

// Check if the line starts with the prefix
int lineStartsWith(STRING *line, const char *prefix)
{
  size_t prefixLength = strlen(prefix);
  return line->n >= prefixLength && strncmp(line->str, prefix, prefixLength) == 0;
}

// Return whether the line starts with "/*"
int lineStartsWithLegacyHeader(STRING *line)
{
  return lineStartsWith(line, "/*");
}

// Return whether the line starts with the '[' character (ignoring whitespace)
int lineMightStartWithF99(STRING *line)
{
  int i = 0;
  while (i < line->n && strIsWhitespace(line->str[i]))
  {
    i++;
  }
  return line->str[i] == '[';
}

// Consume whitespace, advancing a position index at the same time
static void consumeWhitespace(STRING *line, int *i)
{
  while (*i < line->n)
  {
    char c = line->str[*i];
    int isWhitespace = (c == ' ') || (c == '\t');
    if (!isWhitespace)
    {
      break;
    }
    (*i)++;
  }
}

// Consume characters until the specified character is reached.
// Returns the position of the final character consumed excluding
// trailing whitespace.
int consumeUntil(STRING *line, int *i, char c)
{
  // Store the last non-whitespace character
  int finalNonwhitespace = *i;
  while (*i < line->n)
  {
    // Grab the current character
    char current = line->str[*i];
    if ((current == c) || (current == 0))
    {
      // If the character is the one we're looking for, break
      break;
    }
    else if (!strIsWhitespace(current))
    {
      finalNonwhitespace = (*i) + 1;
    }
    (*i)++;
  }
  return finalNonwhitespace;
}

void startHeaderRow(FEC_CONTEXT *ctx, char *filename)
{
  // Write the filing ID header, if includeFilingId is specified
  if (ctx->includeFilingId)
  {
    writeString(ctx->writeContext, filename, CSV_EXTENSION, "filing_id");
    writeDelimeter(ctx->writeContext, filename, CSV_EXTENSION);
  }
}

void startDataRow(FEC_CONTEXT *ctx, char *filename)
{
  // Write the filing ID value, if includeFilingId is specified
  if (ctx->includeFilingId)
  {
    writeString(ctx->writeContext, filename, CSV_EXTENSION, ctx->filingId);
    writeDelimeter(ctx->writeContext, filename, CSV_EXTENSION);
  }
}

// Parse F99 text from a filing, writing the text to the specified
// file in escaped CSV form if successful. Returns 1 if successful,
// 0 otherwise.
int parseF99Text(FEC_CONTEXT *ctx, char *filename)
{
  int f99Mode = 0;
  int first = 1;

  while (1)
  {
    // Load the current line
    if (grabLine(ctx) == 0)
    {
      // End of file
      return 1;
    }

    if (f99Mode)
    {
      // See if we have reached the end boundary
      if (lineContainsF99End(ctx))
      {
        f99Mode = 0;
        break;
      }

      // Otherwise, write f99 information as a CSV field
      if (first)
      {
        // Write the delimeter at the beginning and a quote character
        // (the csv field will always be escaped so we can stream write
        // without having to calculate whether it's escaped later).
        writeDelimeter(ctx->writeContext, filename, CSV_EXTENSION);
        writeChar(ctx->writeContext, filename, CSV_EXTENSION, '"');
        first = 0;
      }

      writeQuotedString(ctx->writeContext, filename, ctx->persistentMemory->line->str, ctx->currentLineLength);
      continue;
    }

    // See if line begins with f99 text boundary by first seeing if it starts with "["
    if (lineMightStartWithF99(ctx->persistentMemory->line))
    {
      // Now, execute the proper regex (we don't want to do this for every line, as it's slow)
      if (lineContainsF99Start(ctx))
      {
        f99Mode = 1;
        continue;
      }
      else
      {
        // Invalid f99 text
        return 0;
      }
    }
    else if (strContainsNonWhitespace(ctx->persistentMemory->line->str, ctx->persistentMemory->line->n))
    {
      // The line should only contain non-whitespace that starts
      // f99 text
      return 0;
    }
  }
  // Successful extraction, end the quote delimiter
  writeChar(ctx->writeContext, filename, CSV_EXTENSION, '"');
  return 1;
}

void ctxWriteField(FEC_CONTEXT *ctx, char *filename, CSV_LINE_PARSER *parser, char type)
{
  if (type == 's')
  {
    ctxWriteSubstr(ctx, filename, parser->start, parser->end, &(parser->fieldInfo));
  }
  else if (type == 'd')
  {
    writeDateField(ctx, filename, parser->start, parser->end, &(parser->fieldInfo));
  }
  else if (type == 'f')
  {
    writeFloatField(ctx, filename, parser->start, parser->end, &(parser->fieldInfo));
  }
  else
  {
    fprintf(stderr, "Unknown type (%c) in %s\n", type, ctx->formType);
    exit(1);
  }
}

// Parse a line from a filing, using FEC and form version
// information to map fields to headers and types.
// Return 1 if successful, or 0 if the line is not fully
// specified. Return 2 if there was a warning but it was
// still successful and the next line has already been grabbed.
// Return 3 if we encountered a mappings error.
int parseLine(FEC_CONTEXT *ctx, char *filename, int headerRow)
{
  CSV_LINE_PARSER parser;
  csvParserInit(&parser, ctx->persistentMemory->line);

  // Iterate through fields
  while (!isParseDone(&parser))
  {
    readField(&parser, ctx->currentLineHasAscii28);
    if (parser.columnIndex == 0)
    {
      // Set the form version to the first column
      // (with whitespace removed)
      stripWhitespace(&parser);
      char *form = parser.line->str + parser.start;
      int formLength = parser.end - parser.start;
      if (!updateCurrentFormSchema(ctx, form, formLength))
      {
        // Mappings error
        return 3;
      }

      // Set filename if null to form type
      if (filename == NULL)
      {
        filename = ctx->formType;
      }
    }
    else
    {
      // If column index is 1, then there are at least two columns
      // and the line is fully specified, so write header/line info
      if (parser.columnIndex == 1)
      {
        // Write header if necessary
        if (getFile(ctx->writeContext, filename, CSV_EXTENSION) == 1)
        {
          // File is newly opened, write headers
          startHeaderRow(ctx, filename);
          writeString(ctx->writeContext, filename, CSV_EXTENSION, ctx->headers);
          writeNewline(ctx->writeContext, filename, CSV_EXTENSION);
          endLine(ctx->writeContext, ctx->types);
        }

        // Write form type
        startDataRow(ctx, filename);
        writeString(ctx->writeContext, filename, CSV_EXTENSION, ctx->formType);
      }

      // Write delimeter
      writeDelimeter(ctx->writeContext, filename, CSV_EXTENSION);

      // Get the type of the current field and write accordingly
      char type = 's'; // Default to string type
      if (parser.columnIndex < ctx->numFields)
      {
        // Ensure the column index is in bounds
        type = ctx->types[parser.columnIndex];
      }
      else
      {
        // Warning: column exceeding row length
        ctxWarn(ctx, "Unexpected column in %s (%d): ", ctx->formType, parser.columnIndex);
        for (int i = parser.start; i < parser.end; i++)
        {
          ctxWarn(ctx, "%c", parser.line->str[i]);
        }
        ctxWarn(ctx, "\n");
      }
      ctxWriteField(ctx, filename, &parser, type);
    }
    advanceField(&parser);
  }

  if (parser.columnIndex < 2)
  {
    // Fewer than two fields? The line isn't fully specified
    return 0;
  }

  if (parser.columnIndex + 1 != ctx->numFields && !headerRow)
  {
    // Try to read F99 text
    if (!parseF99Text(ctx, filename))
    {
      ctxWarn(ctx, "mismatched number of fields (%d vs %d) (%s): \n", parser.columnIndex + 1, ctx->numFields, ctx->formType);
      ctxWarn(ctx, "'%s'\n", parser.line->str);
      // 2 indicates we won't grab the line again
      writeNewline(ctx->writeContext, filename, CSV_EXTENSION);
      endLine(ctx->writeContext, ctx->types);
      return 2;
    }
  }

  // Parsing successful
  writeNewline(ctx->writeContext, filename, CSV_EXTENSION);
  endLine(ctx->writeContext, ctx->types);
  return 1;
}

// Set the FEC file version string
static void setVersion(FEC_CONTEXT *ctx, const char *chars, int length)
{
  ctx->version = malloc(length + 1);
  strncpy(ctx->version, chars, length);
  // Add null terminator
  ctx->version[length] = 0;
  ctx->versionLength = length;
}

// Returns 0 on failure, 1 on success
//
// Example header:
// /* Header
// FEC_Ver_# = 2.02
// Soft_Name = FECfile
// Soft_Ver# = 3
// Dec/NoDec = DEC
// Date_Fmat = CCYYMMDD
// NameDelim = ^
// Form_Name = F3XA
// FEC_IDnum = C00101766
// Committee = CONTINENTAL AIRLINES INC EMPLOYEE FUND FOR A BETTER AMERICA (FKA CONTINENTAL HOLDINGS PAC)
// Control_# = K245592Q
// Schedule_Counts:
// SA11A1    = 00139
// SA17      = 00001
// SB23      = 00008
// SB29      = 00003
// /* End Header
int parseHeaderLegacy(FEC_CONTEXT *ctx)
{
  startHeaderRow(ctx, HEADER);
  int scheduleCounts = 0; // init scheduleCounts to be false
  int firstField = 1;

  // Use a local buffer to store header values
  WRITE_CONTEXT bufferWriteContext;
  initializeLocalWriteContext(&bufferWriteContext, ctx->persistentMemory->bufferLine);

  // Until the line starts with "/*" again, read lines
  while (1)
  {
    if (grabLine(ctx) == 0)
    {
      break;
    }
    STRING *line = ctx->persistentMemory->line;
    if (lineStartsWithLegacyHeader(line))
    {
      break;
    }

    strToLowerCase(line->str);

    // Check if the line starts with "schedule_counts"
    if (lineStartsWith(line, "schedule_counts"))
    {
      scheduleCounts = 1;
    }
    else
    {
      // Grab key value from "key=value" (strip whitespace)
      int i = 0;
      consumeWhitespace(line, &i);
      const int keyStart = i;
      const int keyEnd = consumeUntil(line, &i, '=');
      const char *key = line->str + keyStart;
      const int keyLength = keyEnd - keyStart;
      // Jump over '='
      i++;
      consumeWhitespace(line, &i);
      const int valueStart = i;
      const int valueEnd = consumeUntil(line, &i, 0);
      const char *value = line->str + valueStart;
      const int valueLength = valueEnd - valueStart;

      // Gather field metrics for CSV writing
      FIELD_INFO headerField = {.num_quotes = 0, .num_commas = 0};
      for (int i = 0; i < keyLength; i++)
      {
        processFieldChar(key[i], &headerField);
      }
      FIELD_INFO valueField = {.num_quotes = 0, .num_commas = 0};
      for (int i = 0; i < valueLength; i++)
      {
        processFieldChar(value[i], &valueField);
      }

      // Write commas as needed (only before fields that aren't first)
      if (!firstField)
      {
        writeDelimeter(ctx->writeContext, HEADER, CSV_EXTENSION);
        writeDelimeter(&bufferWriteContext, NULL, NULL);
      }
      firstField = 0;

      // Write schedule counts prefix if set
      if (scheduleCounts)
      {
        writeString(ctx->writeContext, HEADER, CSV_EXTENSION, SCHEDULE_COUNTS);
      }

      // If we match the FEC version column, set the version
      if (strncmp(key, FEC_VERSION_NUMBER, strlen(FEC_VERSION_NUMBER)) == 0)
      {
        setVersion(ctx, value, valueLength);
      }

      // Write the key/value pair
      ctxWriteSubstr(ctx, HEADER, keyStart, keyEnd, &headerField);
      // Write the value to a buffer to be written later
      writeField(&bufferWriteContext, NULL, NULL, value, valueLength, &valueField);
    }
  }
  writeNewline(ctx->writeContext, HEADER, CSV_EXTENSION);
  endLine(ctx->writeContext, ctx->types);
  startDataRow(ctx, HEADER); // output the filing id if we have it
  writeString(ctx->writeContext, HEADER, CSV_EXTENSION, bufferWriteContext.localBuffer->str);
  writeNewline(ctx->writeContext, HEADER, CSV_EXTENSION); // end with newline
  endLine(ctx->writeContext, ctx->types);
  return 1;
}

// Returns 0 on failure, 1 on success
int parseHeaderNonLegacy(FEC_CONTEXT *ctx)
{
  // Parse fields
  CSV_LINE_PARSER parser;
  STRING *line = ctx->persistentMemory->line;
  csvParserInit(&parser, line);

  int isFecSecondColumn = 0;

  // Iterate through fields
  while (!isParseDone(&parser))
  {
    readField(&parser, ctx->currentLineHasAscii28);
    const char *fieldValue = parser.line->str + parser.start;
    const int fieldLength = parser.end - parser.start;
    if (parser.columnIndex == 1)
    {
      // Check if the second column is "FEC"
      if (strncmp(fieldValue, FEC, strlen(FEC)) == 0)
      {
        isFecSecondColumn = 1;
      }
      else
      {
        // If not, the second column is the version
        setVersion(ctx, fieldValue, fieldLength);

        // Parse the header now that version is known
        if (parseLine(ctx, HEADER, 1) == 3)
        {
          return 0;
        }
      }
    }
    if (parser.columnIndex == 2 && isFecSecondColumn)
    {
      setVersion(ctx, fieldValue, fieldLength);
      // Parse the header now that version is known
      return parseLine(ctx, HEADER, 1) != 3;
    }
    advanceField(&parser);
  }
  return 1;
}

// Returns 0 on failure, 1 on success
int parseHeader(FEC_CONTEXT *ctx)
{
  if (lineStartsWithLegacyHeader(ctx->persistentMemory->line))
  {
    return parseHeaderLegacy(ctx);
  }
  else
  {
    return parseHeaderNonLegacy(ctx);
  }
}

// Returns 0 on failure, 1 on success
int parseFec(FEC_CONTEXT *ctx)
{
  int skipGrabLine = 0;

  if (grabLine(ctx) == 0)
  {
    return 0;
  }

  // Parse the header
  if (!parseHeader(ctx))
  {
    return 0;
  }

  // Loop through parsing the entire file, line by
  // line.
  while (1)
  {
    // Load the current line
    if (!skipGrabLine && grabLine(ctx) == 0)
    {
      // End of file
      break;
    }

    // Parse the line and write its parsed output
    // to CSV files depending on version/form type
    skipGrabLine = parseLine(ctx, NULL, 0) == 2;
  }

  return 1;
}
