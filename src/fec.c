#include "fec.h"
#include "encoding.h"
#include "writer.h"
#include "csv.h"
#include "mappings.h"
#include "buffer.h"
#include <string.h>

char *HEADER = "header";
char *SCHEDULE_COUNTS = "SCHEDULE_COUNTS_";
char *FEC_VERSION_NUMBER = "fec_ver_#";
char *FEC = "FEC";

char *COMMA_FEC_VERSIONS[] = {"1", "2", "3", "5"};
int NUM_COMMA_FEC_VERSIONS = sizeof(COMMA_FEC_VERSIONS) / sizeof(char *);

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
  ctx->useAscii28 = 0; // default to using comma parsing unless a version is set
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

  // Compile regexes
  const char *error;
  int errorOffset;

  ctx->f99TextStart = pcre_compile("^\\s*\\[BEGIN ?TEXT\\]\\s*$", PCRE_CASELESS, &error, &errorOffset, NULL);
  if (ctx->f99TextStart == NULL)
  {
    fprintf(stderr, "PCRE f99 text start compilation failed at offset %d: %s\n", errorOffset, error);
    exit(1);
  }
  ctx->f99TextEnd = pcre_compile("^\\s*\\[END ?TEXT\\]\\s*$", PCRE_CASELESS, &error, &errorOffset, NULL);
  if (ctx->f99TextEnd == NULL)
  {
    fprintf(stderr, "PCRE f99 text end compilation failed at offset %d: %s\n", errorOffset, error);
    exit(1);
  }

  return ctx;
}

void freeFecContext(FEC_CONTEXT *ctx)
{
  freeBuffer(ctx->buffer);
  if (ctx->version)
  {
    free(ctx->version);
  }
  if (ctx->f99Text)
  {
    free(ctx->f99Text);
  }
  if (ctx->formType != NULL)
  {
    free(ctx->formType);
  }
  if (ctx->types != NULL)
  {
    free(ctx->types);
  }
  pcre_free(ctx->f99TextStart);
  pcre_free(ctx->f99TextEnd);
  freeWriteContext(ctx->writeContext);
  free(ctx);
}

int isParseDone(PARSE_CONTEXT *parseContext)
{
  // The parse is done if a newline is encountered or EOF
  char c = parseContext->line->str[parseContext->position];
  return (c == 0) || (c == '\n');
}

int lookupMappings(FEC_CONTEXT *ctx, PARSE_CONTEXT *parseContext, int formStart, int formEnd)
{
  if ((ctx->formType != NULL) && (strncmp(ctx->formType, parseContext->line->str + formStart, formEnd - formStart) == 0))
  {
    // Type mappings are unchanged from before; can return early
    return 1;
  }

  // Clear last form type information if present
  if (ctx->formType != NULL)
  {
    free(ctx->formType);
  }
  // Set last form type to store it for later
  ctx->formType = malloc(formEnd - formStart + 1);
  strncpy(ctx->formType, parseContext->line->str + formStart, formEnd - formStart);
  ctx->formType[formEnd - formStart] = 0;

  // Grab the field mapping given the form version
  for (int i = 0; i < numHeaders; i++)
  {
    // Try to match the regex to version
    if (pcre_exec(ctx->persistentMemory->headerVersions[i], NULL, ctx->version, ctx->versionLength, 0, 0, NULL, 0) >= 0)
    {
      // Match! Test regex against form type
      if (pcre_exec(ctx->persistentMemory->headerFormTypes[i], NULL, parseContext->line->str + formStart, formEnd - formStart, 0, 0, NULL, 0) >= 0)
      {
        // Matched form type
        ctx->headers = (char *)(headers[i][2]);
        STRING *headersCsv = fromString(ctx->headers);
        if (ctx->types != NULL)
        {
          free(ctx->types);
        }
        ctx->numFields = 0;
        ctx->types = malloc(strlen(ctx->headers) + 1); // at least as big as it needs to be

        // Initialize a parse context for reading each header field
        PARSE_CONTEXT headerFields;
        headerFields.line = headersCsv;
        headerFields.fieldInfo = NULL;
        headerFields.position = 0;
        headerFields.start = 0;
        headerFields.end = 0;
        headerFields.columnIndex = 0;

        // Iterate each field and build up the type info
        while (!isParseDone(&headerFields))
        {
          readCsvField(&headerFields);

          // Match type info
          int matched = 0;
          for (int j = 0; j < numTypes; j++)
          {
            // Try to match the type regex to version
            if (pcre_exec(ctx->persistentMemory->typeVersions[j], NULL, ctx->version, ctx->versionLength, 0, 0, NULL, 0) >= 0)
            {
              // Try to match type regex to form type
              if (pcre_exec(ctx->persistentMemory->typeFormTypes[j], NULL, parseContext->line->str + formStart, formEnd - formStart, 0, 0, NULL, 0) >= 0)
              {
                // Try to match type regex to header
                if (pcre_exec(ctx->persistentMemory->typeHeaders[j], NULL, headerFields.line->str + headerFields.start, headerFields.end - headerFields.start, 0, 0, NULL, 0) >= 0)
                {
                  // Match! Print out type information
                  ctx->types[headerFields.columnIndex] = types[j][3][0];
                  matched = 1;
                  break;
                }
              }
            }
          }

          if (!matched)
          {
            // Unmatched type — default to 's' for string type
            ctx->types[headerFields.columnIndex] = 's';
          }

          if (isParseDone(&headerFields))
          {
            break;
          }
          advanceField(&headerFields);
        }

        // Add null terminator
        ctx->types[headerFields.columnIndex + 1] = 0;
        ctx->numFields = headerFields.columnIndex + 1;

        // Free up unnecessary line memory
        freeString(headersCsv);

        // Done; return
        return 1;
      }
    }
  }

  // Unmatched — error
  fprintf(stderr, "Error: Unmatched for version %s and form type %s\n", ctx->version, ctx->formType);
  return -1;
}

void writeSubstrToWriter(FEC_CONTEXT *ctx, WRITE_CONTEXT *writeContext, char *filename, const char *extension, int start, int end, FIELD_INFO *field)
{
  writeField(writeContext, filename, extension, ctx->persistentMemory->line, start, end, field);
}

void writeSubstr(FEC_CONTEXT *ctx, char *filename, const char *extension, int start, int end, FIELD_INFO *field)
{
  writeSubstrToWriter(ctx, ctx->writeContext, filename, extension, start, end, field);
}

void writeQuotedCsvField(FEC_CONTEXT *ctx, char *filename, const char *extension, char *line, int length)
{
  for (int i = 0; i < length; i++)
  {
    char c = line[i];
    if (c == '"')
    {
      // Write two quotes since the field is quoted
      writeChar(ctx->writeContext, filename, extension, '"');
      writeChar(ctx->writeContext, filename, extension, '"');
    }
    else
    {
      writeChar(ctx->writeContext, filename, extension, c);
    }
  }
}

// Write a date field by separating the output with dashes
void writeDateField(FEC_CONTEXT *ctx, char *filename, const char *extension, int start, int end, FIELD_INFO *field)
{
  if (start == end)
  {
    // Empty field
    return;
  }
  if (end - start != 8)
  {
    // Could not parse date, write string as is and log warning
    if (ctx->warn)
    {
      fprintf(stderr, "Warning: Date fields must be exactly 8 chars long, not %d\n", end - start);
    }
    writeSubstr(ctx, filename, extension, start, end, field);
    return;
  }

  writeSubstrToWriter(ctx, ctx->writeContext, filename, extension, start, start + 4, field);
  writeChar(ctx->writeContext, filename, extension, '-');
  writeSubstrToWriter(ctx, ctx->writeContext, filename, extension, start + 4, start + 6, field);
  writeChar(ctx->writeContext, filename, extension, '-');
  writeSubstrToWriter(ctx, ctx->writeContext, filename, extension, start + 6, start + 8, field);
}

void writeFloatField(FEC_CONTEXT *ctx, char *filename, const char *extension, int start, int end, FIELD_INFO *field)
{
  char *doubleStr;
  char *conversionFloat = ctx->persistentMemory->line->str + start;
  double value = strtod(conversionFloat, &doubleStr);

  if (doubleStr == conversionFloat)
  {
    // Could not convert to a float, write string as is and log warning
    if (ctx->warn)
    {
      fprintf(stderr, "Warning: Could not parse float field\n");
    }
    writeSubstr(ctx, filename, extension, start, end, field);
    return;
  }

  // Write the value
  writeDouble(ctx->writeContext, filename, extension, value);
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

char lowercaseTable[256] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 91, 92, 93, 94, 95,
    96, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

void lineToLowerCase(FEC_CONTEXT *ctx)
{
  // Convert the line to lower case
  char *c = ctx->persistentMemory->line->str;
  while (*c)
  {
    *c = lowercaseTable[(int)*c];
    c++;
  }
}

// Check if the line starts with the prefix
int lineStartsWith(FEC_CONTEXT *ctx, const char *prefix, const int prefixLength)
{
  return ctx->persistentMemory->line->n >= prefixLength && strncmp(ctx->persistentMemory->line->str, prefix, prefixLength) == 0;
}

// Return whether the line starts with "/*"
int lineStartsWithLegacyHeader(FEC_CONTEXT *ctx)
{
  return lineStartsWith(ctx, "/*", 2);
}

// Return whether the line starts with "schedule_counts"
int lineStartsWithScheduleCounts(FEC_CONTEXT *ctx)
{
  return lineStartsWith(ctx, "schedule_counts", 15);
}

// Return whether the line starts with the '[' character (ignoring whitespace)
int lineMightStartWithF99(FEC_CONTEXT *ctx)
{
  int i = 0;
  while (i < ctx->persistentMemory->line->n && isWhitespaceChar(ctx->persistentMemory->line->str[i]))
  {
    i++;
  }
  return ctx->persistentMemory->line->str[i] == '[';
}

// Return whether the line contains non-whitespace characters
int lineContainsNonwhitespace(FEC_CONTEXT *ctx)
{
  int i = 0;
  while (i < ctx->persistentMemory->line->n && isWhitespaceChar(ctx->persistentMemory->line->str[i]))
  {
    i++;
  }
  return ctx->persistentMemory->line->str[i] != 0;
}

// Consume whitespace, advancing a position pointer at the same time
void consumeWhitespace(FEC_CONTEXT *ctx, int *position)
{
  while (*position < ctx->persistentMemory->line->n)
  {
    if ((ctx->persistentMemory->line->str[*position] == ' ') || (ctx->persistentMemory->line->str[*position] == '\t'))
    {
      (*position)++;
    }
    else
    {
      break;
    }
  }
}

// Consume characters until the specified character is reached.
// Returns the position of the final character consumed excluding
// trailing whitespace.
int consumeUntil(FEC_CONTEXT *ctx, int *position, char c)
{
  // Store the last non-whitespace character
  int finalNonwhitespace = *position;
  while (*position < ctx->persistentMemory->line->n)
  {
    // Grab the current character
    char current = ctx->persistentMemory->line->str[*position];
    if ((current == c) || (current == 0))
    {
      // If the character is the one we're looking for, break
      break;
    }
    else if ((current != ' ') && (current != '\t') && (current != '\n'))
    {
      // If the character is not whitespace, advance finalNonwhitespace
      finalNonwhitespace = (*position) + 1;
    }
    (*position)++;
  }
  return finalNonwhitespace;
}

void initParseContext(FEC_CONTEXT *ctx, PARSE_CONTEXT *parseContext, FIELD_INFO *fieldInfo)
{
  parseContext->line = ctx->persistentMemory->line;
  parseContext->fieldInfo = fieldInfo;
  parseContext->position = 0;
  parseContext->start = 0;
  parseContext->end = 0;
  parseContext->columnIndex = 0;
}

void readField(FEC_CONTEXT *ctx, PARSE_CONTEXT *parseContext)
{
  // Reset field info
  parseContext->fieldInfo->num_quotes = 0;
  parseContext->fieldInfo->num_commas = 0;

  if (ctx->currentLineHasAscii28)
  {
    readAscii28Field(parseContext);
  }
  else
  {
    readCsvField(parseContext);
  }
}

void startHeaderRow(FEC_CONTEXT *ctx, char *filename, const char *extension)
{
  // Write the filing ID header, if includeFilingId is specified
  if (ctx->includeFilingId)
  {
    writeString(ctx->writeContext, filename, extension, "filing_id");
    writeDelimeter(ctx->writeContext, filename, extension);
  }
}

void startDataRow(FEC_CONTEXT *ctx, char *filename, const char *extension)
{
  // Write the filing ID value, if includeFilingId is specified
  if (ctx->includeFilingId)
  {
    writeString(ctx->writeContext, filename, extension, ctx->filingId);
    writeDelimeter(ctx->writeContext, filename, extension);
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
      if (pcre_exec(ctx->f99TextEnd, NULL, ctx->persistentMemory->line->str, ctx->currentLineLength, 0, 0, NULL, 0) >= 0)
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
        writeDelimeter(ctx->writeContext, filename, csvExtension);
        writeChar(ctx->writeContext, filename, csvExtension, '"');
        first = 0;
      }

      writeQuotedCsvField(ctx, filename, csvExtension, ctx->persistentMemory->line->str, ctx->currentLineLength);
      continue;
    }

    // See if line begins with f99 text boundary by first seeing if it starts with "["
    if (lineMightStartWithF99(ctx))
    {
      // Now, execute the proper regex (we don't want to do this for every line, as it's slow)
      if (pcre_exec(ctx->f99TextStart, NULL, ctx->persistentMemory->line->str, ctx->currentLineLength, 0, 0, NULL, 0) >= 0)
      {
        // Set f99 mode
        f99Mode = 1;
        continue;
      }
      else
      {
        // Invalid f99 text
        return 0;
      }
    }
    else if (lineContainsNonwhitespace(ctx))
    {
      // The line should only contain non-whitespace that starts
      // f99 text
      return 0;
    }
  }
  // Successful extraction, end the quote delimiter
  writeChar(ctx->writeContext, filename, csvExtension, '"');
  return 1;
}

// Parse a line from a filing, using FEC and form version
// information to map fields to headers and types.
// Return 1 if successful, or 0 if the line is not fully
// specified. Return 2 if there was a warning but it was
// still successful and the next line has already been grabbed.
int parseLine(FEC_CONTEXT *ctx, char *filename, int headerRow)
{
  // Parse fields
  PARSE_CONTEXT parseContext;
  FIELD_INFO fieldInfo;
  initParseContext(ctx, &parseContext, &fieldInfo);

  // Log the indices on the line where the form version is specified
  int formStart;
  int formEnd;

  // Iterate through fields
  while (!isParseDone(&parseContext))
  {
    readField(ctx, &parseContext);
    if (parseContext.columnIndex == 0)
    {
      // Set the form version to the first column
      // (with whitespace removed)
      stripWhitespace(&parseContext);
      formStart = parseContext.start;
      formEnd = parseContext.end;
      int mappings = lookupMappings(ctx, &parseContext, formStart, formEnd);

      // Failed to find a match; skip this filing
      if (mappings == -1) {
        return -1;
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
      if (parseContext.columnIndex == 1)
      {
        // Write header if necessary
        if (getFile(ctx->writeContext, filename, csvExtension) == 1)
        {
          // File is newly opened, write headers
          startHeaderRow(ctx, filename, csvExtension);
          writeString(ctx->writeContext, filename, csvExtension, ctx->headers);
          writeNewline(ctx->writeContext, filename, csvExtension);
          endLine(ctx->writeContext, ctx->types);
        }

        // Write form type
        startDataRow(ctx, filename, csvExtension);
        writeString(ctx->writeContext, filename, csvExtension, ctx->formType);
      }

      // Write delimeter
      writeDelimeter(ctx->writeContext, filename, csvExtension);

      // Get the type of the current field and write accordingly
      char type;
      if (parseContext.columnIndex < ctx->numFields)
      {
        // Ensure the column index is in bounds
        type = ctx->types[parseContext.columnIndex];
      }
      else
      {
        // Warning: column exceeding row length
        if (ctx->warn)
        {
          fprintf(stderr, "Unexpected column in %s (%d): ", ctx->formType, parseContext.columnIndex);
          for (int i = parseContext.start; i < parseContext.end; i++)
          {
            fprintf(stderr, "%c", ctx->persistentMemory->line->str[i]);
          }
          fprintf(stderr, "\n");
        }
        // Default to string type
        type = 's';
      }

      // Iterate possible types
      if (type == 's')
      {
        // String
        writeSubstr(ctx, filename, csvExtension, parseContext.start, parseContext.end, parseContext.fieldInfo);
      }
      else if (type == 'd')
      {
        // Date
        writeDateField(ctx, filename, csvExtension, parseContext.start, parseContext.end, parseContext.fieldInfo);
      }
      else if (type == 'f')
      {
        // Float
        writeFloatField(ctx, filename, csvExtension, parseContext.start, parseContext.end, parseContext.fieldInfo);
      }
      else
      {
        // Unknown type
        fprintf(stderr, "Unknown type (%c) in %s\n", type, ctx->formType);
        exit(1);
      }
    }

    if (isParseDone(&parseContext))
    {
      break;
    }
    advanceField(&parseContext);
  }

  if (parseContext.columnIndex < 2)
  {
    // Fewer than two fields? The line isn't fully specified
    return 0;
  }

  if (parseContext.columnIndex + 1 != ctx->numFields && !headerRow)
  {
    // Try to read F99 text
    if (!parseF99Text(ctx, filename))
    {
      if (ctx->warn)
      {
        fprintf(stderr, "Warning: mismatched number of fields (%d vs %d) (%s)\nLine: %s\n", parseContext.columnIndex + 1, ctx->numFields, ctx->formType, parseContext.line->str);
      }
      // 2 indicates we won't grab the line again
      writeNewline(ctx->writeContext, filename, csvExtension);
      endLine(ctx->writeContext, ctx->types);
      return 2;
    }
  }

  // Parsing successful
  writeNewline(ctx->writeContext, filename, csvExtension);
  endLine(ctx->writeContext, ctx->types);
  return 1;
}

// Set the FEC context version based on a substring of the current line
void setVersion(FEC_CONTEXT *ctx, int start, int end)
{
  ctx->version = malloc(end - start + 1);
  strncpy(ctx->version, ctx->persistentMemory->line->str + start, end - start);
  // Add null terminator
  ctx->version[end - start] = 0;
  ctx->versionLength = end - start;

  // Calculate whether to use ascii28 or not based on version
  char *dot = strchr(ctx->version, '.');
  int useCommaVersion = 0;
  if (dot != NULL)
  {
    // Check text of left of the dot to get main version
    int dotIndex = (int)(dot - ctx->version);

    for (int i = 0; i < NUM_COMMA_FEC_VERSIONS; i++)
    {
      if (strncmp(ctx->version, COMMA_FEC_VERSIONS[i], dotIndex) == 0)
      {
        useCommaVersion = 1;
        break;
      }
    }
  }

  ctx->useAscii28 = !useCommaVersion;
}

void parseHeader(FEC_CONTEXT *ctx)
{
  // Check if the line starts with "/*"
  if (lineStartsWithLegacyHeader(ctx))
  {
    // Parse legacy header
    startHeaderRow(ctx, HEADER, csvExtension);
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
      if (lineStartsWithLegacyHeader(ctx))
      {
        break;
      }

      // Turn the line into lowercase
      lineToLowerCase(ctx);

      // Check if the line starts with "schedule_counts"
      if (lineStartsWithScheduleCounts(ctx))
      {
        scheduleCounts = 1;
      }
      else
      {
        // Grab key value from "key=value" (strip whitespace)
        int i = 0;
        consumeWhitespace(ctx, &i);
        int keyStart = i;
        int keyEnd = consumeUntil(ctx, &i, '=');
        // Jump over '='
        i++;
        consumeWhitespace(ctx, &i);
        int valueStart = i;
        int valueEnd = consumeUntil(ctx, &i, 0);

        // Gather field metrics for CSV writing
        FIELD_INFO headerField = {.num_quotes = 0, .num_commas = 0};
        for (int i = keyStart; i < keyEnd; i++)
        {
          processFieldChar(ctx->persistentMemory->line->str[i], &headerField);
        }
        FIELD_INFO valueField = {.num_quotes = 0, .num_commas = 0};
        for (int i = valueStart; i < valueEnd; i++)
        {
          processFieldChar(ctx->persistentMemory->line->str[i], &valueField);
        }

        // Write commas as needed (only before fields that aren't first)
        if (!firstField)
        {
          writeDelimeter(ctx->writeContext, HEADER, csvExtension);
          writeDelimeter(&bufferWriteContext, NULL, NULL);
        }
        firstField = 0;

        // Write schedule counts prefix if set
        if (scheduleCounts)
        {
          writeString(ctx->writeContext, HEADER, csvExtension, SCHEDULE_COUNTS);
        }

        // If we match the FEC version column, set the version
        if (strncmp(ctx->persistentMemory->line->str + keyStart, FEC_VERSION_NUMBER, strlen(FEC_VERSION_NUMBER)) == 0)
        {
          setVersion(ctx, valueStart, valueEnd);
        }

        // Write the key/value pair
        writeSubstr(ctx, HEADER, csvExtension, keyStart, keyEnd, &headerField);
        // Write the value to a buffer to be written later
        writeSubstrToWriter(ctx, &bufferWriteContext, NULL, NULL, valueStart, valueEnd, &valueField);
      }
    }
    writeNewline(ctx->writeContext, HEADER, csvExtension);
    endLine(ctx->writeContext, ctx->types);
    startDataRow(ctx, HEADER, csvExtension); // output the filing id if we have it
    writeString(ctx->writeContext, HEADER, csvExtension, bufferWriteContext.localBuffer->str);
    writeNewline(ctx->writeContext, HEADER, csvExtension); // end with newline
    endLine(ctx->writeContext, ctx->types);
  }
  else
  {
    // Not a multiline legacy header — must be using a more recent version

    // Parse fields
    PARSE_CONTEXT parseContext;
    FIELD_INFO fieldInfo;
    initParseContext(ctx, &parseContext, &fieldInfo);

    int isFecSecondColumn = 0;

    // Iterate through fields
    while (!isParseDone(&parseContext))
    {
      readField(ctx, &parseContext);

      if (parseContext.columnIndex == 1)
      {
        // Check if the second column is "FEC"
        if (strncmp(ctx->persistentMemory->line->str + parseContext.start, FEC, strlen(FEC)) == 0)
        {
          isFecSecondColumn = 1;
        }
        else
        {
          // If not, the second column is the version
          setVersion(ctx, parseContext.start, parseContext.end);

          // Parse the header now that version is known
          parseLine(ctx, HEADER, 1);
        }
      }
      if (parseContext.columnIndex == 2 && isFecSecondColumn)
      {
        // Set the version
        setVersion(ctx, parseContext.start, parseContext.end);

        // Parse the header now that version is known
        parseLine(ctx, HEADER, 1);
        return;
      }

      if (isParseDone(&parseContext))
      {
        break;
      }
      advanceField(&parseContext);
    }
  }
}

int parseFec(FEC_CONTEXT *ctx)
{
  int skipGrabLine = 0;

  if (grabLine(ctx) == 0)
  {
    return 0;
  }

  // Parse the header
  parseHeader(ctx);

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
    int parseLineResult = parseLine(ctx, NULL, 0) ;
    
    skipGrabLine = parseLineResult == 2;

    if (parseLineResult == -1) {
      return -1;
      }
  }

  return 1;
}
