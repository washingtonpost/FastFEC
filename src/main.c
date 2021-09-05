#include "urlopen.h"
#include "encoding.h"
#include "fec.h"
#include <stdlib.h>

#define FREADFILE "fread.test"

/* Small main program to retrieve from a url using fgets and fread saving the
 * output to two test files (note the fgets method will corrupt binary files if
 * they contain 0 chars */
int main(int argc, char *argv[])
{
  URL_FILE *handle;
  FILE *outf;

  size_t nread;
  char buffer[256];
  const char *url;

  if (argc < 2)
  {
    // url = "https://docquery.fec.gov/dcdev/posted/13360.fec";
    // Test local files
    url = "1162172.fec";
    // url = "1533121.fec";
  }
  else
  {
    url = argv[1]; /* use passed url */
  }

  /* Copy from url with fread */
  outf = fopen(FREADFILE, "wb+");
  if (!outf)
  {
    perror("couldn't open fread output file\n");
    return 1;
  }

  handle = url_fopen(url, "r");
  if (!handle)
  {
    printf("couldn't url_fopen() %s\n", url);
    fclose(outf);
    return 2;
  }

  // Initialize persistent memory context
  PERSISTENT_MEMORY_CONTEXT *persistentMemory = newPersistentMemoryContext();
  // Initialize FEC context
  FEC_CONTEXT *fec = newFecContext(persistentMemory, ((GetLine)(&url_getline)), handle, ((PutLine)(&fputs)), outf);

  // Parse the fec file
  if (!parseFec(fec))
  {
    perror("Parsing FEC failed\n");
    return 3;
  }

  // Close file handles
  url_fclose(handle);
  fclose(outf);

  // Free up persistent memory
  freePersistentMemoryContext(persistentMemory);

  return 0; /* all done */
}