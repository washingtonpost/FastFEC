#include "memory.h"
#include "writer.h"
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#include "compat.h"

#ifndef PATH_MAX_LENGTH
#define PATH_MAX_LENGTH 4096 /* # chars in a path name including nul */
#endif
#ifndef EEXIST
#define EEXIST 17
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG 63
#endif

const char *NUMBER_FORMAT = "%.2f";

// From https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
int mkdir_p(const char *path)
{
  /* Adapted from http://stackoverflow.com/a/2336245/119527 */
  const size_t len = strlen(path);
  char _path[PATH_MAX_LENGTH];
  char *p;

  errno = 0;

  /* Copy string so its mutable */
  if (len > sizeof(_path) - 1)
  {
    errno = ENAMETOOLONG;
    return -1;
  }
  strcpy(_path, path);

  /* Iterate the string */
  for (p = _path + 1; *p; p++)
  {
    if (*p == DIR_SEPARATOR_CHAR)
    {
      /* Temporarily truncate */
      *p = '\0';

#if defined(_WIN32)
      int mkdirResult = mkdir(_path);
#else
      int mkdirResult = mkdir(_path, S_IRWXU);
#endif
      if (mkdirResult != 0)
      {
        if (errno != EEXIST)
          return -1;
      }

      *p = DIR_SEPARATOR_CHAR;
    }
  }

#if defined(_WIN32)
  int mkdirResult = mkdir(_path);
#else
  int mkdirResult = mkdir(_path, S_IRWXU);
#endif
  if (mkdirResult != 0)
  {
    if (errno != EEXIST)
      return -1;
  }

  return 0;
}

// Normalize a file name by converting slashes to dashes
// Adapted from https://stackoverflow.com/a/32496721
void normalize_filename(char *filename)
{
  char *current_pos = strchr(filename, DIR_SEPARATOR_CHAR);
  while (current_pos)
  {
    *current_pos = '-';
    current_pos = strchr(current_pos, DIR_SEPARATOR_CHAR);
  }
}

BUFFER_FILE *newBufferFile(int bufferSize)
{
  BUFFER_FILE *bufferFile = (BUFFER_FILE *)malloc(sizeof(BUFFER_FILE));
  bufferFile->buffer = malloc(bufferSize);
  bufferFile->bufferPos = 0;
  bufferFile->bufferSize = bufferSize;
  return bufferFile;
}

void freeBufferFile(BUFFER_FILE *bufferFile)
{
  free(bufferFile->buffer);
  free(bufferFile);
}

WRITE_CONTEXT *newWriteContext(char *outputDirectory, char *filingId, int writeToFile, int bufferSize, CustomWriteFunction customWriteFunction, CustomLineFunction customLineFunction)
{
  WRITE_CONTEXT *context = (WRITE_CONTEXT *)malloc(sizeof(WRITE_CONTEXT));
  context->outputDirectory = outputDirectory;
  context->filingId = filingId;
  context->writeToFile = writeToFile;
  context->bufferSize = bufferSize;
  context->filenames = NULL;
  context->extensions = NULL;
  context->bufferFiles = NULL;
  context->files = NULL;
  context->nfiles = 0;
  context->lastname = NULL;
  context->lastBufferFile = NULL;
  context->lastfile = NULL;
  context->local = 0;
  context->localBuffer = NULL;
  context->useCustomLine = customLineFunction != NULL;
  context->customLineBuffer = context->useCustomLine ? newString(DEFAULT_STRING_SIZE) : NULL;
  context->customWriteFunction = customWriteFunction;
  context->customLineFunction = customLineFunction;
  initializeCustomWriteContext(context);
  return context;
}

void initializeLocalWriteContext(WRITE_CONTEXT *writeContext, STRING *line)
{
  writeContext->local = 1;
  writeContext->localBuffer = line;
  writeContext->localBufferPosition = 0;
  // Ensure the line is empty
  writeContext->localBuffer->str[0] = 0;
}

void initializeCustomWriteContext(WRITE_CONTEXT *writeContext)
{
  if (!writeContext->useCustomLine)
  {
    return;
  }

  writeContext->customLineBufferPosition = 0;
  // Ensure the line is empty
  writeContext->customLineBuffer->str[0] = 0;
}

void endLine(WRITE_CONTEXT *writeContext, char *types)
{
  if (!writeContext->useCustomLine)
  {
    return;
  }

  writeContext->customLineFunction(writeContext->lastname, writeContext->customLineBuffer->str, types);
  writeContext->customLineBufferPosition = 0;
  // Ensure the line is empty
  writeContext->customLineBuffer->str[0] = 0;
}

int getFile(WRITE_CONTEXT *context, char *filename, const char *extension)
{
  if ((context->lastname != NULL) && (strcmp(context->lastname, filename) == 0))
  {
    // Same file as last time, just write to it
    return 0;
  }

  // Different file than last time, open it
  if (context->filenames == NULL)
  {
    // No files open, so open the file
    context->filenames = (char **)malloc(sizeof(char *));
    context->extensions = (char **)malloc(sizeof(char *));
    context->bufferFiles = (BUFFER_FILE **)malloc(sizeof(BUFFER_FILE *));
    if (context->writeToFile)
    {
      context->files = (FILE **)malloc(sizeof(FILE *));
    }
  }
  else
  {
    // See if file is already open
    for (int i = 0; i < context->nfiles; i++)
    {
      if (strcmp(context->filenames[i], filename) == 0)
      {
        // Write to existing file
        context->lastname = context->filenames[i];
        context->lastBufferFile = context->bufferFiles[i];
        if (context->writeToFile)
        {
          context->lastfile = context->files[i];
        }
        return 0;
      }
    }

    // File is not open, open it
    context->filenames = (char **)realloc(context->filenames, sizeof(char *) * (context->nfiles + 1));
    context->extensions = (char **)realloc(context->extensions, sizeof(char *) * (context->nfiles + 1));
    context->bufferFiles = (BUFFER_FILE **)realloc(context->bufferFiles, sizeof(BUFFER_FILE *) * (context->nfiles + 1));
    if (context->writeToFile)
    {
      context->files = (FILE **)realloc(context->files, sizeof(FILE *) * (context->nfiles + 1));
    }
  }
  // Open and write to file
  context->filenames[context->nfiles] = malloc(strlen(filename) + 1);
  context->extensions[context->nfiles] = malloc(strlen(extension) + 1);
  context->bufferFiles[context->nfiles] = newBufferFile(context->bufferSize);
  strcpy(context->filenames[context->nfiles], filename);
  strcpy(context->extensions[context->nfiles], extension);
  // Derive the full path to the file

  if (context->writeToFile)
  {
    // Ensure the directory exists (will silently fail if it does)
    char *fullpath = (char *)malloc(sizeof(char) * (strlen(context->outputDirectory) + strlen(filename) + 1 + strlen(context->filingId) + strlen(extension) + 1));
    strcpy(fullpath, context->outputDirectory);
    strcat(fullpath, context->filingId);
    mkdir_p(fullpath);

    // Add the normalized filename to path
    strcat(fullpath, DIR_SEPARATOR);
    char *normalizedFilename = malloc(strlen(filename) + 1);
    strcpy(normalizedFilename, filename);
    normalize_filename(normalizedFilename);
    strcat(fullpath, normalizedFilename);
    strcat(fullpath, extension);

    context->files[context->nfiles] = fopen(fullpath, "w");
    // Free the derived file paths
    free(normalizedFilename);
    free(fullpath);
  }
  context->lastname = context->filenames[context->nfiles];
  context->lastBufferFile = context->bufferFiles[context->nfiles];
  if (context->writeToFile)
  {
    context->lastfile = context->files[context->nfiles];
  }
  context->nfiles++;
  return 1;
}

void bufferFlush(WRITE_CONTEXT *context, char *filename, const char *extension, FILE *file, BUFFER_FILE *bufferFile)
{
  if (bufferFile->bufferPos == 0)
  {
    return;
  }
  if (context->customWriteFunction != NULL)
  {
    // Write to a custom write function
    context->customWriteFunction(filename, (char *)extension, bufferFile->buffer, bufferFile->bufferPos);
  }
  if (context->writeToFile)
  {
    fwrite(bufferFile->buffer, 1, bufferFile->bufferPos, file);
  }
  bufferFile->bufferPos = 0;
}

void bufferWrite(WRITE_CONTEXT *context, char *filename, const char *extension, FILE *file, BUFFER_FILE *bufferFile, char *string, int nchars)
{
  int offset = 0;
  while (nchars > 0)
  {
    int bytesToWrite = nchars;
    int remaining = bufferFile->bufferSize - bufferFile->bufferPos;
    if (bytesToWrite > remaining)
    {
      // Only write what is possible
      bytesToWrite = remaining;
    }
    // Copy bytes over
    memcpy(bufferFile->buffer + bufferFile->bufferPos, string + offset, bytesToWrite);
    bufferFile->bufferPos += bytesToWrite;

    // Flush if needed
    if (bufferFile->bufferPos >= bufferFile->bufferSize)
    {
      bufferFlush(context, filename, extension, file, bufferFile);
    }
    nchars -= bytesToWrite;
    offset += bytesToWrite;
  }
}

void writeN(WRITE_CONTEXT *context, char *filename, const char *extension, char *string, int nchars)
{
  if (context->local == 0)
  {
    // Write to file
    getFile(context, filename, extension);
    bufferWrite(context, filename, extension, context->lastfile, context->lastBufferFile, string, nchars);

    if (context->useCustomLine)
    {
      // Write to custom line function
      int newPosition = context->customLineBufferPosition + nchars;
      if (newPosition + 1 > context->customLineBuffer->n)
      {
        growStringTo(context->customLineBuffer, newPosition + 1);
      }
      memcpy(context->customLineBuffer->str + context->customLineBufferPosition, string, nchars);
      context->customLineBufferPosition = newPosition;
      // Add null terminator
      context->customLineBuffer->str[context->customLineBufferPosition] = 0;
    }
  }
  else
  {
    // Write to local buffer
    int newPosition = context->localBufferPosition + nchars;
    if (newPosition + 1 > context->localBuffer->n)
    {
      growStringTo(context->localBuffer, newPosition + 1);
    }
    memcpy(context->localBuffer->str + context->localBufferPosition, string, nchars);
    context->localBufferPosition = newPosition;
    // Add null terminator
    context->localBuffer->str[context->localBufferPosition] = 0;
  }
}

void writeString(WRITE_CONTEXT *context, char *filename, const char *extension, char *string)
{
  writeN(context, filename, extension, string, strlen(string));
}

void writeChar(WRITE_CONTEXT *context, char *filename, const char *extension, char c)
{
  if (context->local == 0 && (!context->useCustomLine))
  {
    // Write to file
    getFile(context, filename, extension);
    bufferWrite(context, filename, extension, context->lastfile, context->lastBufferFile, &c, 1);
  }
  else
  {
    // Write to local buffer
    char str[] = {c};
    writeN(context, filename, extension, str, 1);
  }
}

void writeDouble(WRITE_CONTEXT *context, char *filename, const char *extension, double d)
{
  // Write to local buffer
  char str[100]; // should be able to fit any double
  sprintf(str, NUMBER_FORMAT, d);
  writeString(context, filename, extension, str);
}

void freeWriteContext(WRITE_CONTEXT *context)
{
  for (int i = 0; i < context->nfiles; i++)
  {
    // Flush out any remaining file contents
    bufferFlush(context, context->filenames[i], context->extensions[i], context->writeToFile ? context->files[i] : NULL, context->bufferFiles[i]);

    // Free memory structures for each file
    free(context->filenames[i]);
    free(context->extensions[i]);
    freeBufferFile(context->bufferFiles[i]);
    if (context->writeToFile)
    {
      fclose(context->files[i]);
    }
  }
  if (context->filenames != NULL)
  {
    free(context->filenames);
  }
  if (context->files != NULL)
  {
    free(context->files);
  }
  if (context->bufferFiles != NULL)
  {
    free(context->bufferFiles);
  }
  if (context->extensions != NULL)
  {
    free(context->extensions);
  }
  if (context->customLineBuffer != NULL)
  {
    freeString(context->customLineBuffer);
  }
  free(context);
}
