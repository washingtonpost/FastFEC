#pragma once

#include "encoding.h"
#include "fec.h"
#include <stdlib.h>
#include "pcre/pcre.h"
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

struct cli_context
{
  // Whether the command is receiving piped input
  int piped;
  // Whether to include the filing id in the output
  int includeFilingId;
  // Whether to suppress all stdout messages
  int silent;
  // Whether to show warning messages
  int warn;
  // Whether to print URLs from docquery instead of running commands
  int printUrl;
  // Whether usage should be printed
  int shouldPrintUsage;
  // Whether usage should be clarified with specifying a filing id manually
  int shouldPrintSpecifyFilingId;
  // Whether usage should be clarified with specifying print url exclusively
  int shouldPrintUrlOnly;
  // The name of the file to download
  const char *name;
  // The output directory
  char *outputDirectory;
  // The filing ID
  char *fecId;
  // The normalized FEC file name
  char *fecName;
  // The FEC URL from docquery (if requested)
  char *fecUrl;
  // The backup FEC URL from docquery (if requested)
  char *fecBackupUrl;
  // Regex's
  pcre *filingIdOnly;
  pcre *extractNumber;
};
typedef struct cli_context CLI_CONTEXT;

CLI_CONTEXT *newCliContext();

void parseArgs(CLI_CONTEXT *context, int piped, int argc, char *argv[]);

void freeCliContext(CLI_CONTEXT *context);

// CLI flags
extern const char *FLAG_FILING_ID;
extern const char FLAG_FILING_ID_SHORT;
extern const char *FLAG_SILENT;
extern const char FLAG_SILENT_SHORT;
extern const char *FLAG_WARN;
extern const char FLAG_WARN_SHORT;
extern const char *FLAG_DISABLE_STDIN;
extern const char FLAG_DISABLE_STDIN_SHORT;
extern const char *FLAG_URL;
extern const char FLAG_URL_SHORT;