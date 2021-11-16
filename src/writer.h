#pragma once

#include "memory.h"

static const char csvExtension[] = ".csv";

struct write_context
{
  char *outputDirectory;
  char *filingId;
  char **filenames;
  FILE **files;
  int nfiles;
  char *lastname;
  FILE *lastfile;
  int local;
  STRING *localBuffer;
  int localBufferPosition;
};
typedef struct write_context WRITE_CONTEXT;

WRITE_CONTEXT *newWriteContext(char *outputDirectory, char *filingId);

void initializeLocalWriteContext(WRITE_CONTEXT *writeContext, STRING *line);

// Return 0 if file is cached, or 1 if it is newly created for writing
int getFile(WRITE_CONTEXT *context, char *filename, const char *extension);

void writeN(WRITE_CONTEXT *context, char *filename, const char *extension, char *string, int nchars);

void writeString(WRITE_CONTEXT *context, char *filename, const char *extension, char *string);

void writeChar(WRITE_CONTEXT *context, char *filename, const char *extension, char c);

void writeDouble(WRITE_CONTEXT *context, char *filename, const char *extension, double d);

void freeWriteContext(WRITE_CONTEXT *context);
