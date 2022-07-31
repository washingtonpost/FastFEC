#include "cli.h"
#include "compat.h"

const char *FLAG_FILING_ID = "--include-filing-id";
const char FLAG_FILING_ID_SHORT = 'i';
const char *FLAG_SILENT = "--silent";
const char FLAG_SILENT_SHORT = 's';
const char *FLAG_WARN = "--warn";
const char FLAG_WARN_SHORT = 'w';
const char *FLAG_DISABLE_STDIN = "--no-stdin";
const char FLAG_DISABLE_STDIN_SHORT = 'x';
const char *FLAG_URL = "--print-url";
const char FLAG_URL_SHORT = 'p';

CLI_CONTEXT *newCliContext()
{
  CLI_CONTEXT *ctx = (CLI_CONTEXT *)malloc(sizeof(CLI_CONTEXT));
  ctx->piped = 0;
  ctx->includeFilingId = 0;
  ctx->silent = 0;
  ctx->warn = 0;
  ctx->printUrl = 0;
  ctx->shouldPrintUsage = 0;
  ctx->shouldPrintSpecifyFilingId = 0;
  ctx->shouldPrintUrlOnly = 0;
  return ctx;
}

void parseArgs(CLI_CONTEXT *ctx, int isPiped, int argc, char *argv[])
{
  ctx->piped = isPiped;
  // For parsing flags
  int flagOffset = 0;

  if (argc < 2)
  {
    // Not enough args
    ctx->shouldPrintUsage = 1;
    return;
  }

  // Regexes and constants for filename handling
  const char *error;
  int errorOffset;
  ctx->filingIdOnly = pcre_compile("^\\s*([0-9]+)\\s*$", 0, &error, &errorOffset, NULL);
  if (ctx->filingIdOnly == NULL)
  {
    fprintf(stderr, "Regex filing ID compilation failed at offset %d: %s\n", errorOffset, error);
    exit(1);
  }
  ctx->extractNumber = pcre_compile("^.*?([0-9]+)(\\.[^\\.]+)?\\s*$", 0, &error, &errorOffset, NULL);
  if (ctx->extractNumber == NULL)
  {
    fprintf(stderr, "Regex number extraction compilation failed at offset %d: %s\n", errorOffset, error);
    exit(1);
  }

  // Try to extract flags
  while (1 + flagOffset < argc && argv[1 + flagOffset][0] == '-')
  {
    if (strcmp(argv[1 + flagOffset], FLAG_FILING_ID) == 0)
    {
      ctx->includeFilingId = 1;
      flagOffset++;
    }
    else if (strcmp(argv[1 + flagOffset], FLAG_SILENT) == 0)
    {
      ctx->silent = 1;
      flagOffset++;
    }
    else if (strcmp(argv[1 + flagOffset], FLAG_WARN) == 0)
    {
      ctx->warn = 1;
      flagOffset++;
    }
    else if (strcmp(argv[1 + flagOffset], FLAG_DISABLE_STDIN) == 0)
    {
      ctx->piped = 0;
      flagOffset++;
    }
    else if (strcmp(argv[1 + flagOffset], FLAG_URL) == 0)
    {
      ctx->printUrl = 1;
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
          ctx->includeFilingId = 1;
          matched = 1;
        }
        else if (argv[1 + flagOffset][i] == FLAG_SILENT_SHORT)
        {
          ctx->silent = 1;
          matched = 1;
        }
        else if (argv[1 + flagOffset][i] == FLAG_WARN_SHORT)
        {
          ctx->warn = 1;
          matched = 1;
        }
        else if (argv[1 + flagOffset][i] == FLAG_DISABLE_STDIN_SHORT)
        {
          ctx->piped = 0;
          matched = 1;
        }
        else if (argv[1 + flagOffset][i] == FLAG_URL_SHORT)
        {
          ctx->printUrl = 1;
          matched = 1;
        }
        else
        {
          ctx->shouldPrintUsage = 1;
          return;
        }
      }

      if (matched)
      {
        flagOffset++;
      }
      else
      {
        ctx->shouldPrintUsage = 1;
        return;
      }
    }
  }

  // Set the name
  if (flagOffset + 1 >= argc)
  {
    // Ensure there's more args to pull from
    ctx->shouldPrintUsage = 1;
    return;
  }
  ctx->name = ctx->piped ? NULL : argv[1 + flagOffset];

  // Strings that will carry decoded values for filing name/id
  ctx->outputDirectory = malloc(8);
  strcpy(ctx->outputDirectory, "output" DIR_SEPARATOR);
  char *fecExtension = ".fec";
  // Rewrite output directory if set in cli
  if (argc > 2 + flagOffset)
  {
    ctx->outputDirectory = realloc(ctx->outputDirectory, strlen(argv[2 + flagOffset]) + 1);
    strcpy(ctx->outputDirectory, argv[2 + flagOffset]);

    // Ensure output directory ends with a trailing slash
    if (ctx->outputDirectory[strlen(ctx->outputDirectory) - 1] != DIR_SEPARATOR_CHAR)
    {
      ctx->outputDirectory = realloc(ctx->outputDirectory, strlen(ctx->outputDirectory) + 2);
      strcat(ctx->outputDirectory, DIR_SEPARATOR);
    }
  }

  // Pull out ID/override ID parameter, depending on how input is piped
  if (ctx->piped)
  {
    if (argc > 1 + flagOffset)
    {
      ctx->fecId = malloc(strlen(argv[1 + flagOffset]) + 1);
      strcpy(ctx->fecId, argv[1 + flagOffset]);
    }
    else
    {
      ctx->shouldPrintUsage = 1;
      return;
    }
  }
  else
  {
    if (argc > 3 + flagOffset)
    {
      ctx->fecId = malloc(strlen(argv[3 + flagOffset]) + 1);
      strcpy(ctx->fecId, argv[3 + flagOffset]);
    }
  }

  // Check name format and grab matching ID
  int matches[6];
  if (!ctx->piped && (pcre_exec(ctx->filingIdOnly, NULL, ctx->name, strlen(ctx->name), 0, 0, matches, 3) >= 0))
  {
    // Name is a purely numeric filing ID
    int start = matches[0];
    int end = matches[1];

    // Initialize FEC id
    ctx->fecId = malloc(end - start + 1);
    strncpy(ctx->fecId, ctx->name + start, end - start);
    ctx->fecId[end - start] = '\0';
  }
  else if (!ctx->piped)
  {
    // Normalize the filename
    ctx->fecName = malloc(strlen(ctx->name) + 1);
    strcpy(ctx->fecName, ctx->name);
    ctx->fecName[strlen(ctx->fecName)] = '\0';

    // Try to extract the ID from the file name
    if (ctx->fecId == NULL && pcre_exec(ctx->extractNumber, NULL, ctx->fecName, strlen(ctx->fecName), 0, 0, matches, 6) >= 0)
    {
      int start = matches[2];
      int end = matches[3];

      // Initialize FEC id
      ctx->fecId = malloc(end - start + 1);
      strncpy(ctx->fecId, ctx->name + start, end - start);
      ctx->fecId[end - start] = '\0';
    }
  }

  if (ctx->fecId == NULL)
  {
    ctx->shouldPrintSpecifyFilingId = 1;
    ctx->shouldPrintUsage = 1;
    return;
  }

  if (ctx->printUrl)
  {
    // Handle printing URL
    if (ctx->piped || ctx->includeFilingId || ctx->silent || ctx->warn)
    {
      ctx->shouldPrintUrlOnly = 1;
      return;
    }

    // Initialize FEC URL (and backup URL)
    const char *docqueryUrl = "https://docquery.fec.gov/dcdev/posted/";
    const char *docqueryUrlAlt = "https://docquery.fec.gov/paper/posted/";
    ctx->fecUrl = NULL;
    ctx->fecBackupUrl = NULL;

    ctx->fecUrl = malloc(strlen(docqueryUrl) + strlen(ctx->fecId) + strlen(fecExtension) + 1);
    strcpy(ctx->fecUrl, docqueryUrl);
    strcat(ctx->fecUrl, ctx->fecId);
    strcat(ctx->fecUrl, fecExtension);
    ctx->fecUrl[strlen(ctx->fecUrl)] = '\0';
    ctx->fecBackupUrl = malloc(strlen(docqueryUrlAlt) + strlen(ctx->fecId) + strlen(fecExtension) + 1);
    strcpy(ctx->fecBackupUrl, docqueryUrlAlt);
    strcat(ctx->fecBackupUrl, ctx->fecId);
    strcat(ctx->fecBackupUrl, fecExtension);
  }
}

void freeCliContext(CLI_CONTEXT *ctx)
{
  if (ctx->outputDirectory)
  {
    free(ctx->outputDirectory);
    ctx->outputDirectory = NULL;
  }
  if (ctx->fecId)
  {
    free(ctx->fecId);
    ctx->fecId = NULL;
  }
  if (ctx->fecName)
  {
    free(ctx->fecName);
    ctx->fecName = NULL;
  }
  if (ctx->fecUrl)
  {
    free(ctx->fecUrl);
    ctx->fecUrl = NULL;
  }
  if (ctx->fecBackupUrl)
  {
    free(ctx->fecBackupUrl);
    ctx->fecBackupUrl = NULL;
  }
  if (ctx->filingIdOnly)
  {
    pcre_free(ctx->filingIdOnly);
    ctx->filingIdOnly = NULL;
  }
  if (ctx->extractNumber)
  {
    pcre_free(ctx->extractNumber);
    ctx->extractNumber = NULL;
  }
  free(ctx);
}