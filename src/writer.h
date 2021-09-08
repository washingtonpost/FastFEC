#pragma once

#include "memory.h"

static const char extension[] = ".csv";

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

void writeN(WRITE_CONTEXT *context, char *filename, char *string, int nchars);

void write(WRITE_CONTEXT *context, char *filename, char *string);

void writeChar(WRITE_CONTEXT *context, char *filename, char c);

void freeWriteContext(WRITE_CONTEXT *context);