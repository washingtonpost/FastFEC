#include "fec.h"
#include "encoding.h"
#include <string.h>

FEC_CONTEXT *newFecContext(PERSISTENT_MEMORY_CONTEXT *persistentMemory, GetLine getLine, void *file, PutLine putLine, void *outFile)
{
  FEC_CONTEXT *ctx = (FEC_CONTEXT *)malloc(sizeof(FEC_CONTEXT));
  ctx->persistentMemory = persistentMemory;
  ctx->getLine = getLine;
  ctx->file = file;
  ctx->putLine = putLine;
  ctx->outFile = outFile;
  ctx->version = 0;
  ctx->summary = 0;
  ctx->f99Text = 0;
  return ctx;
}

void freeFecContext(FEC_CONTEXT *ctx)
{
  if (ctx->version)
  {
    free(ctx->version);
  }
  if (ctx->f99Text)
  {
    free(ctx->f99Text);
  }
  free(ctx);
}

// Grab a line from the input file.
// Return 0 if there are no lines left.
// If there is a line, decode it into
// ctx->persistentMemory->line.
int grabLine(FEC_CONTEXT *ctx)
{
  ssize_t bytesRead = ctx->getLine(ctx->persistentMemory->rawLine, ctx->file);
  if (bytesRead <= 0)
  {
    return 0;
  }

  // Decode the line
  LINE_INFO info;
  decodeLine(&info, ctx->persistentMemory->rawLine, ctx->persistentMemory->line);
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
    *c = lowercaseTable[*c];
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
    if ((current == c) || (current = 0))
    {
      // If the character is the one we're looking for, break
      break;
    }
    else if ((current != ' ') && (current == '\t'))
    {
      // If the character is not whitespace, advance finalNonwhitespace
      finalNonwhitespace = *position;
    }
    (*position)++;
  }
  return finalNonwhitespace;
}

void parseHeader(FEC_CONTEXT *ctx)
{
  // Check if the line starts with "/*"
  if (lineStartsWithLegacyHeader(ctx))
  {
    // Parse legacy header
    int scheduleCounts = 0; // init scheduleCounts to be false

    // Until the line starts with "/*" again, read lines
    do
    {
      grabLine(ctx);
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
      }

    } while (!lineStartsWithLegacyHeader(ctx));
  }
}

int parseFec(FEC_CONTEXT *ctx)
{
  while (1)
  {
    // Grab a line from the input file
    if (!grabLine(ctx))
    {
      // EOF
      break;
    }

    // Write the line out to file
    ctx->putLine(ctx->persistentMemory->line->str, ctx->outFile);
  }
  return 1;
}