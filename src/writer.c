#include "memory.h"
#include "writer.h"
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

// From https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
int mkdir_p(const char *path)
{
  /* Adapted from http://stackoverflow.com/a/2336245/119527 */
  const size_t len = strlen(path);
  char _path[PATH_MAX];
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

      if (mkdir(_path, S_IRWXU) != 0)
      {
        if (errno != EEXIST)
          return -1;
      }

      *p = '/';
    }
  }

  if (mkdir(_path, S_IRWXU) != 0)
  {
    if (errno != EEXIST)
      return -1;
  }

  return 0;
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
  return context;
}

void getFile(WRITE_CONTEXT *context, char *filename)
{
  if (context->lastname != NULL && strcmp(context->lastname, filename) == 0)
  {
    // Same file as last time, just write to it
    return;
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
        context->lastname = filename;
        context->lastfile = context->files[i];
        return;
      }
    }

    // File is not open, open it
    context->filenames = (char **)realloc(context->filenames, sizeof(char *) * (context->nfiles + 1));
    context->files = (FILE **)realloc(context->files, sizeof(FILE *) * (context->nfiles + 1));
  }
  // Open and write to file
  context->filenames[context->nfiles] = filename;
  // Derive the full path to the file
  char *fullpath = (char *)malloc(sizeof(char) * (strlen(context->outputDirectory) + strlen(filename) + 1 + strlen(context->filingId) + strlen(extension) + 1));
  strcpy(fullpath, context->outputDirectory);
  strcat(fullpath, filename);

  // Ensure the directory exists (will silently fail if it does)
  mkdir_p(fullpath);

  // Add filename to path
  strcat(fullpath, "/");
  strcat(fullpath, context->filingId);
  strcat(fullpath, extension);

  context->files[context->nfiles] = fopen(fullpath, "w");
  // Free the derived full path to the file
  free(fullpath);
  context->lastname = filename;
  context->lastfile = context->files[context->nfiles];
  context->nfiles++;
}

void writeN(WRITE_CONTEXT *context, char *filename, char *string, int nchars)
{
  getFile(context, filename);
  fwrite(string, sizeof(char), nchars, context->lastfile);
}

void write(WRITE_CONTEXT *context, char *filename, char *string)
{
  writeN(context, filename, string, strlen(string));
}

void writeChar(WRITE_CONTEXT *context, char *filename, char c)
{
  getFile(context, filename);
  fputc(c, context->lastfile);
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