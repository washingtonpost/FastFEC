#include "urlopen.h"
#include "encoding.h"
#include "fec.h"
#include <stdlib.h>
#include <pcre.h>
#include <string.h>
#include <unistd.h>

const char *FLAG_FILING_ID = "--filingIdColumn";
const char FLAG_FILING_ID_SHORT = 'i';
const char *FLAG_SILENT = "--silent";
const char FLAG_SILENT_SHORT = 's';

void printUsage(char *argv[])
{
  fprintf(stderr, "\nUsage:\n    %s [flags] <id, file, or url> [output directory=output] [override id]\nor: [some command] | %s [flags] <id> [output directory=output]\n", argv[0], argv[0]);
  fprintf(stderr, "\nOptional flags:\n");
  fprintf(stderr, "  %s, -%c: include a filing_id column at the beginning of\n                        every output CSV\n", FLAG_FILING_ID, FLAG_FILING_ID_SHORT);
  fprintf(stderr, "  %s, -%c        : suppress all stdout messages\n\n", FLAG_SILENT, FLAG_SILENT_SHORT);
}

/* Small main program to retrieve from a url using fgets and fread saving the
 * output to two test files (note the fgets method will corrupt binary files if
 * they contain 0 chars */
int main(int argc, char *argv[])
{
  int piped = !isatty(fileno(stdin));
  int flagOffset = 0;

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
    fprintf(stderr, "PCRE filing ID compilation failed at offset %d: %s\n", errorOffset, error);
    exit(1);
  }
  pcre *extractNumber = pcre_compile("^.*?([0-9]+)(\\.[^\\.]+)?\\s*$", 0, &error, &errorOffset, NULL);
  if (extractNumber == NULL)
  {
    fprintf(stderr, "PCRE number extraction compilation failed at offset %d: %s\n", errorOffset, error);
    exit(1);
  }

  // Try to extract flags
  int includeFilingId = 0;
  int silent = 0;
  while (argv[1 + flagOffset][0] == '-')
  {
    if (strcmp(argv[1 + flagOffset], FLAG_FILING_ID) == 0)
    {
      includeFilingId = 1;
      flagOffset++;
    }
    else if (strcmp(argv[1 + flagOffset], FLAG_SILENT) == 0)
    {
      silent = 1;
      flagOffset++;
    }
    else
    {
      // Try to extract flags in short form
      int matched = 0;
      for (int i = 1; i < strlen(argv[1 + flagOffset]); i++)
      {
        if (argv[1 + flagOffset][i] == FLAG_FILING_ID_SHORT)
        {
          includeFilingId = 1;
          matched = 1;
        }
        else if (argv[1 + flagOffset][i] == FLAG_SILENT_SHORT)
        {
          silent = 1;
          matched = 1;
        }
        else
        {
          printUsage(argv);
          exit(1);
        }
      }

      if (matched)
      {
        flagOffset++;
      }
      else
      {
        printUsage(argv);
        exit(1);
      }
    }
  }

  const char *docqueryUrl = "https://docquery.fec.gov/dcdev/posted/";
  const char *docqueryUrlAlt = "https://docquery.fec.gov/paper/posted/";

  url = piped ? NULL : argv[1 + flagOffset];

  // Strings that will carry the decoded values for the filing URL and ID,
  // where URL can be a local file or a remote URL
  char *fecUrl = NULL;
  char *fecBackupUrl = NULL;
  char *fecId = NULL;
  char *outputDirectory = malloc(8);
  strcpy(outputDirectory, "output/");
  char *fecExtension = ".fec";
  if (argc > 2 + flagOffset)
  {
    outputDirectory = realloc(outputDirectory, strlen(argv[2 + flagOffset]) + 1);
    strcpy(outputDirectory, argv[2 + flagOffset]);

    // Ensure output directory ends with a trailing slash
    if (outputDirectory[strlen(outputDirectory) - 1] != '/')
    {
      outputDirectory = realloc(outputDirectory, strlen(outputDirectory) + 2);
      strcat(outputDirectory, "/");
    }
  }

  // Pull out ID/override ID parameter, depending on how input is piped
  if (piped)
  {
    if (argc > 1 + flagOffset)
    {
      fecId = malloc(strlen(argv[1 + flagOffset]) + 1);
      strcpy(fecId, argv[1 + flagOffset]);
    }
    else
    {
      printUsage(argv);
      exit(1);
    }
  }
  else
  {
    if (argc > 3 + flagOffset)
    {
      fecId = malloc(strlen(argv[3 + flagOffset]) + 1);
      strcpy(fecId, argv[3 + flagOffset]);
    }
  }

  // Check URL format and grab matching ID
  int matches[6];
  if (!piped && (pcre_exec(filingIdOnly, NULL, url, strlen(url), 0, 0, matches, 3) >= 0))
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
  else if (!piped)
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
    fprintf(stderr, "Please specify a filing ID manually\n");
    printUsage(argv);

    // Clear associated memory
    free(outputDirectory);
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

  if (!silent)
  {
    printf("About to parse filing ID %s\n", fecId);
  }
  if (piped)
  {
    if (!silent)
    {
      printf("Using stdin\n");
    }
  }
  else
  {
    if (!silent)
    {
      printf("Trying URL: %s\n", fecUrl);
    }
  }

  handle = url_fopen(piped ? NULL : fecUrl, "r", piped ? stdin : NULL);
  if (!handle)
  {
    if (fecBackupUrl != NULL)
    {
      if (!silent)
      {
        printf("Couldn't open URL %s\n", fecUrl);
        printf("Trying backup URL: %s\n", fecBackupUrl);
      }
      handle = url_fopen(fecBackupUrl, "r", NULL);

      if (!handle)
      {
        fprintf(stderr, "Couldn't open URL: %s\n", fecBackupUrl);
        return 2;
      }
    }
    else
    {
      fprintf(stderr, "Couldn't open URL %s\n", fecUrl);
      return 2;
    }
  }

  // Initialize persistent memory context
  PERSISTENT_MEMORY_CONTEXT *persistentMemory = newPersistentMemoryContext();
  // Initialize FEC context
  FEC_CONTEXT *fec = newFecContext(persistentMemory, ((GetLine)(&url_getline)), handle, fecId, outputDirectory, includeFilingId, silent);

  // Parse the fec file
  int fecParseResult = parseFec(fec);

  // Clear up memory
  freeFecContext(fec);
  freePersistentMemoryContext(persistentMemory);
  pcre_free(filingIdOnly);
  pcre_free(extractNumber);
  free(outputDirectory);
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
    fprintf(stderr, "Parsing FEC failed\n");
    return 3;
  }

  if (!silent)
  {
    printf("Done; parsing successful!\n");
  }
  return 0; /* all done */
}