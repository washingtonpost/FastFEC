#include "urlopen.h"
#include "encoding.h"
#include "fec.h"
#include <stdlib.h>
#include <pcre.h>
#include <string.h>

void printUsage(char *argv[])
{
  printf("Usage: %s <id, file, or url> [output directory=output] [override id]\n", argv[0]);
}

/* Small main program to retrieve from a url using fgets and fread saving the
 * output to two test files (note the fgets method will corrupt binary files if
 * they contain 0 chars */
int main(int argc, char *argv[])
{
  URL_FILE *handle;
  const char *url;

  if (argc < 2)
  {
    printUsage(argv);
    exit(1);
  }

  // Regexes and constants for URL handling
  const char *error;
  int errorOffset;
  pcre *filingIdOnly = pcre_compile("^\\s*([0-9]+)\\s*$", 0, &error, &errorOffset, NULL);
  if (filingIdOnly == NULL)
  {
    printf("PCRE filing ID compilation failed at offset %d: %s\n", errorOffset, error);
    exit(1);
  }
  pcre *extractNumber = pcre_compile("^.*?([0-9]+)(\\.[^\\.]+)?\\s*$", 0, &error, &errorOffset, NULL);
  if (extractNumber == NULL)
  {
    printf("PCRE number extraction compilation failed at offset %d: %s\n", errorOffset, error);
    exit(1);
  }

  const char *docqueryUrl = "https://docquery.fec.gov/dcdev/posted/";
  const char *docqueryUrlAlt = "https://docquery.fec.gov/paper/posted/";

  url = argv[1];

  // Strings that will carry the decoded values for the filing URL and ID,
  // where URL can be a local file or a remote URL
  char *fecUrl = NULL;
  char *fecBackupUrl = NULL;
  char *fecId = NULL;
  char *outputDirectory = "output/";
  char *fecExtension = ".fec";
  if (argc > 2)
  {
    outputDirectory = argv[2];

    // Ensure output directory ends with a trailing slash
    if (outputDirectory[strlen(outputDirectory) - 1] != '/')
    {
      outputDirectory = realloc(outputDirectory, strlen(outputDirectory) + 2);
      strcat(outputDirectory, "/");
    }
  }
  if (argc > 3)
  {
    fecId = argv[3];
  }

  // Check URL format and grab matching ID
  int matches[6];
  if (pcre_exec(filingIdOnly, NULL, url, strlen(url), 0, 0, matches, 3) >= 0)
  {
    // URL is a purely numeric filing ID
    int start = matches[0];
    int end = matches[1];

    // Initialize FEC id
    fecId = malloc(end - start + 1);
    strncpy(fecId, url + start, end - start);
    fecId[end - start] = '\0';

    // Initialize FEC URL (and backup URL)
    fecUrl = malloc(strlen(docqueryUrl) + strlen(fecId) + strlen(fecExtension) + 1);
    strcpy(fecUrl, docqueryUrl);
    strcat(fecUrl, fecId);
    strcat(fecUrl, fecExtension);
    fecUrl[strlen(fecUrl)] = '\0';
    fecBackupUrl = malloc(strlen(docqueryUrlAlt) + strlen(fecId) + strlen(fecExtension) + 1);
    strcpy(fecBackupUrl, docqueryUrlAlt);
    strcat(fecBackupUrl, fecId);
    strcat(fecBackupUrl, fecExtension);
  }
  else
  {
    // URL is a local file or a remote URL
    fecUrl = malloc(strlen(url) + 1);
    strcpy(fecUrl, url);
    fecUrl[strlen(fecUrl)] = '\0';

    // Try to extract the ID from the URL
    if (fecId == NULL && pcre_exec(extractNumber, NULL, fecUrl, strlen(fecUrl), 0, 0, matches, 6) >= 0)
    {
      int start = matches[2];
      int end = matches[3];

      // Initialize FEC id
      fecId = malloc(end - start + 1);
      strncpy(fecId, url + start, end - start);
      fecId[end - start] = '\0';
    }
  }

  if (fecId == NULL)
  {
    printf("Could not extract ID from URL. Please specify an ID manually\n");
    printUsage(argv);

    // Clear associated memory
    if (fecUrl != NULL)
    {
      free(fecUrl);
    }
    if (fecBackupUrl != NULL)
    {
      free(fecBackupUrl);
    }
    pcre_free(filingIdOnly);
    pcre_free(extractNumber);

    exit(1);
  }

  printf("About to parse filing ID %s\n", fecId);
  printf("Trying URL: %s\n", fecUrl);

  handle = url_fopen(fecUrl, "r");
  if (!handle)
  {
    printf("couldn't open url %s\n", fecUrl);

    if (fecBackupUrl != NULL)
    {
      printf("Trying backup URL: %s\n", fecBackupUrl);
      handle = url_fopen(fecBackupUrl, "r");

      if (!handle)
      {
        printf("couldn't open backup url\n");
        return 2;
      }
    }
  }

  // Initialize persistent memory context
  PERSISTENT_MEMORY_CONTEXT *persistentMemory = newPersistentMemoryContext();
  // Initialize FEC context
  FEC_CONTEXT *fec = newFecContext(persistentMemory, ((GetLine)(&url_getline)), handle, fecId, outputDirectory);

  // Parse the fec file
  int fecParseResult = parseFec(fec);

  // Clear up memory
  freeFecContext(fec);
  freePersistentMemoryContext(persistentMemory);
  pcre_free(filingIdOnly);
  pcre_free(extractNumber);
  if (fecUrl != NULL)
  {
    free(fecUrl);
  }
  if (fecBackupUrl != NULL)
  {
    free(fecBackupUrl);
  }
  if (fecId != NULL)
  {
    free(fecId);
  }

  // Close file handles
  url_fclose(handle);

  if (!fecParseResult)
  {
    perror("Parsing FEC failed\n");
    return 3;
  }

  printf("Done; parsing successful!\n");
  return 0; /* all done */
}