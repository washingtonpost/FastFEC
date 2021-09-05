#include "urlopen.h"
#include "encoding.h"
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
    url = "https://docquery.fec.gov/dcdev/posted/13360.fec";
    // Test local files
    // url = "1162172.fec";
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

  size_t lineBufferSize = 10;
  STRING *rawline = newString(lineBufferSize);
  STRING *line = newString(lineBufferSize);

  if (rawline == 0)
  {
    perror("couldn't allocate line buffer\n");
    return 3;
  }

  ssize_t bytesRead;
  while (1)
  {
    bytesRead = url_getline(rawline, handle);
    if (bytesRead <= 0)
    {
      break;
      perror("couldn't get line\n");
      return 4;
    }

    // Decode the line
    LINE_INFO info;
    decodeLine(&info, rawline, line);

    fputs(line->str, outf);
  }

  url_fclose(handle);

  fclose(outf);

  freeString(rawline);
  freeString(line);

  return 0; /* all done */
}