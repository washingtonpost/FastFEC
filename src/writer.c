#include "memory.h"
#include "writer.h"
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

#define PATH_MAX_LENGTH 4096 /* # chars in a path name including nul */

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
    if (*p == '/')
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

      *p = '/';
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
  char *current_pos = strchr(filename, '/');
  while (current_pos)
  {
    *current_pos = '-';
    current_pos = strchr(current_pos, '/');
  }
}

WRITE_CONTEXT *newWriteContext(char *outputDirectory, char *filingId)
{
  WRITE_CONTEXT *context = (WRITE_CONTEXT *)malloc(sizeof(WRITE_CONTEXT));
  context->outputDirectory = outputDirectory;
  context->filingId = filingId;
  context->filenames = NULL;
  context->files = NULL;
  context->nfiles = 0;
  context->lastname = NULL;
  context->lastfile = NULL;
  context->local = 0;
  context->localBuffer = NULL;
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
    context->files = (FILE **)malloc(sizeof(FILE *));
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
        context->lastfile = context->files[i];
        return 0;
      }
    }

    // File is not open, open it
    context->filenames = (char **)realloc(context->filenames, sizeof(char *) * (context->nfiles + 1));
    context->files = (FILE **)realloc(context->files, sizeof(FILE *) * (context->nfiles + 1));
  }
  // Open and write to file
  context->filenames[context->nfiles] = malloc(strlen(filename + 1));
  strcpy(context->filenames[context->nfiles], filename);
  // Derive the full path to the file
  char *fullpath = (char *)malloc(sizeof(char) * (strlen(context->outputDirectory) + strlen(filename) + 1 + strlen(context->filingId) + strlen(extension) + 1));
  strcpy(fullpath, context->outputDirectory);
  strcat(fullpath, context->filingId);

  // Ensure the directory exists (will silently fail if it does)
  mkdir_p(fullpath);

  // Add the normalized filename to path
  strcat(fullpath, "/");
  char *normalizedFilename = malloc(strlen(filename + 1));
  strcpy(normalizedFilename, filename);
  normalize_filename(normalizedFilename);
  strcat(fullpath, normalizedFilename);
  strcat(fullpath, extension);

  context->files[context->nfiles] = fopen(fullpath, "w");
  // Free the derived file paths
  free(normalizedFilename);
  free(fullpath);
  context->lastname = context->filenames[context->nfiles];
  context->lastfile = context->files[context->nfiles];
  context->nfiles++;
  return 1;
}

void writeN(WRITE_CONTEXT *context, char *filename, const char *extension, char *string, int nchars)
{
  if (context->local == 0)
  {
    // Write to file
    getFile(context, filename, extension);
    fwrite(string, sizeof(char), nchars, context->lastfile);
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
  if (context->local == 0)
  {
    // Write to file
    getFile(context, filename, extension);
    fputc(c, context->lastfile);
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
  if (context->local == 0)
  {
    // Write to file
    getFile(context, filename, extension);
    fprintf(context->lastfile, NUMBER_FORMAT, d);
  }
  else
  {
    // Write to local buffer
    char str[100]; // should be able to fit any double
    sprintf(str, NUMBER_FORMAT, d);
    writeString(context, filename, extension, str);
  }
}

void freeWriteContext(WRITE_CONTEXT *context)
{
  for (int i = 0; i < context->nfiles; i++)
  {
    free(context->filenames[i]);
    fclose(context->files[i]);
  }
  if (context->filenames != NULL)
  {
    free(context->filenames);
    free(context->files);
  }
  free(context);
}
