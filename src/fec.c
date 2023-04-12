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

static const char *HEADER = "header";
static const char *SCHEDULE_COUNTS = "SCHEDULE_COUNTS_";
static const char *FEC_VERSION_NUMBER = "fec_ver_#";

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

static int _lineReMatch(STRING *line, pcre *regex)
{
  return pcre_exec(regex, NULL, line->str, line->n, 0, 0, NULL, 0) >= 0;
}

static int _lineContainsF99Start(STRING *line)
{
  static pcre *regex = NULL;
  if (regex == NULL)
  {
    regex = newRegex("^\\s*\\[BEGIN ?TEXT\\]\\s*$");
  }
  return _lineReMatch(line, regex);
}

static int _lineContainsF99End(STRING *line)
{
  static pcre *regex = NULL;
  if (regex == NULL)
  {
    regex = newRegex("^\\s*\\[END ?TEXT\\]\\s*$");
  }
  return _lineReMatch(line, regex);
}

FEC_CONTEXT *newFecContext(
    PERSISTENT_MEMORY_CONTEXT *persistentMemory,
    BufferRead bufferRead,
    int inputBufferSize,
    CustomWriteFunction customWriteFunction,
    int outputBufferSize,
    CustomLineFunction customLineFunction,
    int writeToFile,
    void *file,
    char *filingId,
    char *outputDirectory,
    int silent,
    int warn)
{
  FEC_CONTEXT *ctx = malloc(sizeof(FEC_CONTEXT));
  ctx->buffer = newBuffer(inputBufferSize, bufferRead);
  ctx->file = file;
  ctx->writeContext = newWriteContext(outputDirectory, writeToFile, outputBufferSize, customWriteFunction, customLineFunction);
  ctx->version = NULL;

  ctx->persistentMemory = persistentMemory;
  ctx->currentLineHasAscii28 = 0;

  // Options
  ctx->filingId = filingId;
  ctx->silent = silent;
  ctx->warn = warn;

  ctx->currentForm = NULL;
  return ctx;
}

void freeFecContext(FEC_CONTEXT *ctx)
{
  freeBuffer(ctx->buffer);
  freeWriteContext(ctx->writeContext);
  freeString(ctx->version);
  formSchemaFree(ctx->currentForm);
  free(ctx);
}

// Get the schema for a form. Basically a cached version of formSchemaLookup,
// because ctx->version never changes within a single file.
static FORM_SCHEMA *ctxFormSchemaLookup(FEC_CONTEXT *ctx, const char *form, int formLength)
{
  // If type mappings are unchanged from before we can return early
  if (
      (ctx->currentForm != NULL) &&
      (ctx->currentForm->type != NULL) &&
      (strncmp(ctx->currentForm->type, form, formLength) == 0))
  {
    return ctx->currentForm;
  }

  FORM_SCHEMA *schema = formSchemaLookup(ctx->version, form, formLength);
  if (schema == NULL)
  {
    ctxWarn(ctx, "No mappings found for version %s and form %.s", ctx->version, formLength, form);
    return NULL;
  }

  formSchemaFree(ctx->currentForm);
  ctx->currentForm = schema;
  return ctx->currentForm;
}

static void writeQuotedString(WRITE_CONTEXT *ctx, const char *filename, const char *str, int length)
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
  // Store whether the current line has ascii separators
  // (determines whether we use CSV or ascii28 split line parsing)
  ctx->currentLineHasAscii28 = decodeLine(ctx->persistentMemory->rawLine, ctx->persistentMemory->line);
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
  size_t i = 0;
  while (i < line->n && strIsWhitespace(line->str[i]))
  {
    i++;
  }
  return line->str[i] == '[';
}

// Consume whitespace, advancing a position index at the same time
static void consumeWhitespace(STRING *line, int *i)
{
  while ((size_t)*i < line->n)
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
  while ((size_t)*i < line->n)
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

void startHeaderRow(FEC_CONTEXT *ctx, const char *filename)
{
  // Write the filing ID header, if filingId is specified
  if (ctx->filingId)
  {
    writeString(ctx->writeContext, filename, CSV_EXTENSION, "filing_id");
    writeDelimeter(ctx->writeContext, filename);
  }
}

void startDataRow(FEC_CONTEXT *ctx, const char *filename)
{
  // Write the filing ID value, if filingId is specified
  if (ctx->filingId)
  {
    writeString(ctx->writeContext, filename, CSV_EXTENSION, ctx->filingId);
    writeDelimeter(ctx->writeContext, filename);
  }
}

// Parse F99 text from a filing, writing the text to the specified
// file in escaped CSV form if successful. Returns 1 if successful,
// 0 otherwise.
int parseF99Text(FEC_CONTEXT *ctx, const char *filename)
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
    STRING *line = ctx->persistentMemory->line;

    if (f99Mode)
    {
      // See if we have reached the end boundary
      if (_lineContainsF99End(line))
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
        writeDelimeter(ctx->writeContext, filename);
        writeChar(ctx->writeContext, filename, CSV_EXTENSION, '"');
        first = 0;
      }

      writeQuotedString(ctx->writeContext, filename, line->str, line->n);
      continue;
    }

    // See if line begins with f99 text boundary by first seeing if it starts with "["
    if (lineMightStartWithF99(line))
    {
      // Now, execute the proper regex (we don't want to do this for every line, as it's slow)
      if (_lineContainsF99Start(line))
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
    else if (strContainsNonWhitespace(line->str, line->n))
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

static void ctxWriteField(FEC_CONTEXT *ctx, const char *filename, CSV_FIELD *field, char type)
{
  WRITE_CONTEXT *wctx = ctx->writeContext;
  if (type == 's')
  {
    writeField(wctx, filename, field);
  }
  else if (type == 'd')
  {
    int convertFailed = (writeFieldDate(wctx, filename, field) == 0);
    if (convertFailed)
    {
      ctxWarn(ctx, "Date fields must be exactly 8 chars long, saw: %.s", field->length, field->chars);
    }
  }
  else if (type == 'f')
  {
    int convertFailed = (writeFieldFloat(wctx, filename, field) == 0);
    if (convertFailed)
    {
      ctxWarn(ctx, "Failed to convert field to float: %.s", field->length, field->chars);
    }
  }
  else
  {
    fprintf(stderr, "Unknown type (%c) in %s\n", type, ctx->currentForm->type);
    exit(1);
  }
}

// Parse a line from a filing, using FEC and form version
// information to map fields to headers and types.
// Return 1 if successful, or 0 if the line is not fully
// specified. Return 2 if there was a warning but it was
// still successful and the next line has already been grabbed.
// Return 3 if we encountered a mappings error.
int parseLine(FEC_CONTEXT *ctx, const char *filename, int headerRow)
{
  CSV_LINE_PARSER parser;
  CSV_FIELD *field = NULL;
  csvParserInit(&parser, ctx->persistentMemory->line);

  // Get the first field, which contains the form type
  if (isParseDone(&parser))
  {
    // empty line, shouldn't be done yet. error.
    return 0;
  }
  field = nextField(&parser, ctx->currentLineHasAscii28);
  stripWhitespace(field);
  FORM_SCHEMA *formSchema = ctxFormSchemaLookup(ctx, field->chars, field->length);
  if (formSchema == NULL)
  {
    return 3;
  }

  // Set filename if null to form type
  if (filename == NULL)
  {
    filename = formSchema->type;
  }

  // Iterate through rest of the fields
  while (!isParseDone(&parser))
  {
    field = nextField(&parser, ctx->currentLineHasAscii28);
    // If we have two columns the line is fully specified, so write header/line info
    if (parser.numFieldsRead == 2)
    {
      // Write header if necessary
      if (getFile(ctx->writeContext, filename, CSV_EXTENSION) == 1)
      {
        // File is newly opened, write headers
        startHeaderRow(ctx, filename);
        writeString(ctx->writeContext, filename, CSV_EXTENSION, formSchema->headerString);
        writeNewline(ctx->writeContext, filename);
        endLine(ctx->writeContext, formSchema->fieldTypes);
      }

      // Write form type
      startDataRow(ctx, filename);
      writeString(ctx->writeContext, filename, CSV_EXTENSION, formSchema->type);
    }

    char fieldType = 's'; // Default to string type
    if (parser.numFieldsRead - 1 < formSchema->numFields)
    {
      // Ensure the column index is in bounds
      fieldType = formSchema->fieldTypes[parser.numFieldsRead - 1];
    }
    else
    {
      ctxWarn(ctx, "Unexpected column in %s (%d): '%.*s'", formSchema->fieldTypes, parser.numFieldsRead, field->length, field->chars);
    }
    writeDelimeter(ctx->writeContext, filename);
    ctxWriteField(ctx, filename, field, fieldType);
  }

  if (parser.numFieldsRead <= 2)
  {
    // Two or fewer fields? The line isn't fully specified
    return 0;
  }

  if (parser.numFieldsRead != formSchema->numFields && !headerRow)
  {
    // Try to read F99 text
    if (!parseF99Text(ctx, filename))
    {
      ctxWarn(ctx, "mismatched number of fields (%d vs %d) (%s): \n", parser.numFieldsRead, formSchema->numFields, formSchema->type);
      ctxWarn(ctx, "'%s'\n", parser.line->str);
      // 2 indicates we won't grab the line again
      writeNewline(ctx->writeContext, filename);
      endLine(ctx->writeContext, formSchema->fieldTypes);
      return 2;
    }
  }

  // Parsing successful
  writeNewline(ctx->writeContext, filename);
  endLine(ctx->writeContext, formSchema->fieldTypes);
  return 1;
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
static int parseHeaderLegacy(FEC_CONTEXT *ctx)
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
      CSV_FIELD keyField = {
          .chars = key,
          .length = keyLength,
          .info = {.num_quotes = 0, .num_commas = 0},
      };
      for (int i = 0; i < keyLength; i++)
      {
        processFieldChar(key[i], &(keyField.info));
      }
      CSV_FIELD valueField = {
          .chars = value,
          .length = valueLength,
          .info = {.num_quotes = 0, .num_commas = 0},
      };
      for (int i = 0; i < valueLength; i++)
      {
        processFieldChar(value[i], &(valueField.info));
      }

      // Write commas as needed (only before fields that aren't first)
      if (!firstField)
      {
        writeDelimeter(ctx->writeContext, HEADER);
        writeDelimeter(&bufferWriteContext, NULL);
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
        ctx->version = fromChars(value, valueLength);
      }

      // Write the key/value pair
      writeField(ctx->writeContext, HEADER, &keyField);
      // Write the value to a buffer to be written later
      writeField(&bufferWriteContext, NULL, &valueField);
    }
  }
  writeNewline(ctx->writeContext, HEADER);
  endLine(ctx->writeContext, NULL);
  startDataRow(ctx, HEADER); // output the filing id if we have it
  writeString(ctx->writeContext, HEADER, CSV_EXTENSION, bufferWriteContext.localBuffer->str);
  writeNewline(ctx->writeContext, HEADER); // end with newline
  endLine(ctx->writeContext, NULL);
  return 1;
}

// Returns 0 on failure, 1 on success
static int parseHeaderNonLegacy(FEC_CONTEXT *ctx)
{
  CSV_LINE_PARSER parser;
  STRING *line = ctx->persistentMemory->line;
  csvParserInit(&parser, line);

  int isFecSecondColumn = 0;

  // Iterate through fields
  while (!isParseDone(&parser))
  {
    const CSV_FIELD *field = nextField(&parser, ctx->currentLineHasAscii28);
    if (parser.numFieldsRead == 2)
    {
      // Check if the second column is "FEC"
      if (strncmp(field->chars, "FEC", strlen("FEC")) == 0)
      {
        isFecSecondColumn = 1;
      }
      else
      {
        // If not, the second column is the version
        ctx->version = fromChars(field->chars, field->length);

        // Parse the header now that version is known
        if (parseLine(ctx, HEADER, 1) == 3)
        {
          return 0;
        }
      }
    }
    if (parser.numFieldsRead == 3 && isFecSecondColumn)
    {
      ctx->version = fromChars(field->chars, field->length);
      // Parse the header now that version is known
      return parseLine(ctx, HEADER, 1) != 3;
    }
  }
  return 1;
}

// Returns 0 on failure, 1 on success
static inline int parseHeader(FEC_CONTEXT *ctx)
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
    return 0; // file is empty
  }
  if (!parseHeader(ctx))
  {
    return 0; // couldn't parse header
  }

  // Loop through the file line by line
  while (1)
  {
    if (!skipGrabLine && grabLine(ctx) == 0)
    {
      return 1; // End of file
    }
    skipGrabLine = parseLine(ctx, NULL, 0) == 2;
  }
}
