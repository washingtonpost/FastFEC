#include "encoding.h"
#include "fec.h"
#include "cli.h"
#include "path.h"

#define BUFFERSIZE 65536

void printUsage(char *argv[])
{
  fprintf(stderr, "\nUsage:\n    %s [flags] <id, file> [output directory=output] [override id]\nor: [some command] | %s [flags] <id> [output directory=output]\n", argv[0], argv[0]);
  fprintf(stderr, "\nOptional flags:\n");
  fprintf(stderr, "  %s, -%c: include a filing_id column at the beginning of\n                        every output CSV\n", FLAG_FILING_ID, FLAG_FILING_ID_SHORT);
  fprintf(stderr, "  %s, -%c        : suppress all stdout messages\n\n", FLAG_SILENT, FLAG_SILENT_SHORT);
  fprintf(stderr, "  %s, -%c        : show warning messages\n\n", FLAG_WARN, FLAG_WARN_SHORT);
  fprintf(stderr, "  %s, -%c        : disable piped input\n\n", FLAG_DISABLE_STDIN, FLAG_DISABLE_STDIN_SHORT);
  fprintf(stderr, "  %s, -%c        : print URLs from docquery.fec.gov\n\n", FLAG_URL, FLAG_URL_SHORT);
}

void printUrl(CLI_CONTEXT *ctx, char *argv[])
{
  fprintf(stderr, "\nFEC filing URLs:");
  fprintf(stderr, "\n  Primary: %s", ctx->fecUrl);
  fprintf(stderr, "\n  Converted paper: %s", ctx->fecBackupUrl);
  fprintf(stderr, "\n\nTry downloading the first URL. If that fails, try the second one.\n");
  fprintf(stderr, "\nIf you have curl (https://curl.se/download.html) installed, you\ncan download and parse a filing with:\n");
  fprintf(stderr, "\n  curl %s | %s %s\n\n", ctx->fecUrl, argv[0], ctx->fecId);
}

int main(int argc, char *argv[])
{
  // Determine whether the input is piped
  int isPiped = !isatty(fileno(stdin));

  // Parse the arguments into a CLI context
  CLI_CONTEXT *cli = newCliContext();
  parseArgs(cli, isPiped, argc, argv);

  // Print usage and exit
  if (cli->shouldPrintUsage)
  {
    // Determine additional messages to print
    if (cli->shouldPrintSpecifyFilingId)
    {
      fprintf(stderr, "Please specify a filing ID manually\n");
    }
    else if (cli->shouldPrintUrlOnly)
    {
      fprintf(stderr, "If printing docquery URLs, please specify no additional flags\n");
    }

    printUsage(argv);

    freeCliContext(cli);
    exit(1);
  }

  // Print docquery URLs and exit (successfully)
  if (cli->printUrl)
  {
    printUrl(cli, argv);
    exit(0);
  }

  // Run the program
  if (!cli->silent)
  {
    printf("About to parse filing ID %s\n", cli->fecId);
  }
  if (cli->piped)
  {
    if (!cli->silent)
    {
      printf("Using stdin\n");
    }
  }
  else
  {
    if (!cli->silent)
    {
      printf("Trying filename: %s\n", cli->fecName);
    }
  }

  // Get the file handle from stdin or the passed in filename
  FILE *handle = cli->piped ? stdin : fopen(cli->fecName, "r");
  if (!handle)
  {
    fprintf(stderr, "Couldn't open file: %s\n", cli->fecName);
    return 2;
  }

  const char *outputDir = pathJoin(cli->outputDirectory, cli->fecId);
  const char *filingId = cli->includeFilingId ? cli->fecId : NULL;
  PERSISTENT_MEMORY_CONTEXT *persistentMemory = newPersistentMemoryContext();
  int inBufferSize = BUFFERSIZE;
  int outBufferSize = BUFFERSIZE;
  CustomWriteFunction customWriteFunction = NULL;
  CustomLineFunction customLineFunction = NULL;
  int writeToFile = 1;
  int raw = 0;
  FEC_CONTEXT *fec = newFecContext(persistentMemory, ((BufferRead)(&readBuffer)), inBufferSize, customWriteFunction, outBufferSize, customLineFunction, writeToFile, handle, outputDir, filingId, cli->silent, cli->warn, raw);

  // Parse the fec file
  int fecParseResult = parseFec(fec);

  // Clear up memory
  freeFecContext(fec);
  freePersistentMemoryContext(persistentMemory);
  freeCliContext(cli);
  free(outputDir);

  // Close file handles
  if (!cli->piped)
  {
    fclose(handle);
  }

  if (!fecParseResult)
  {
    fprintf(stderr, "Parsing FEC failed\n");
    return 3;
  }

  if (!cli->silent)
  {
    printf("Done; parsing successful!\n");
  }
  return 0; /* all done */
}
