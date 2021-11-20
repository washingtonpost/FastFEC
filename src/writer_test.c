#include <stdio.h>
#include <string.h>
#include "minunit.h"
#include "writer.h"

int tests_run = 0;

char outputFile[100];
int outputFilePosition = 0;

void resetOutputFile()
{
  // Reset the mock output file
  outputFilePosition = 0;
  outputFile[0] = '\0';
}

void writeToFile(char *filename, char *extension, char *contents, int numBytes)
{
  // Naively "write" to the file
  for (int i = 0; i < numBytes; i++)
  {
    outputFile[outputFilePosition] = contents[i];
    outputFilePosition++;
  }
  outputFile[outputFilePosition] = '\0';
}

char *testFile = "test";
char *testExt = ".txt";

static char *testWriter()
{
  resetOutputFile();

  WRITE_CONTEXT *ctx = newWriteContext(NULL, NULL, 3, writeToFile);

  // Write a small string that won't flush the buffer
  writeString(ctx, testFile, testExt, "hi");
  mu_assert("expected file contents to be \"\"", strcmp(outputFile, "") == 0);

  // Write an additional string to cause the buffer to overflow
  writeString(ctx, testFile, testExt, " there");
  mu_assert("expected file contents to be \"hi the\"", strcmp(outputFile, "hi the") == 0);

  // This will flush all remaining buffers
  freeWriteContext(ctx);
  mu_assert("expected file contents to be \"hi there\"", strcmp(outputFile, "hi there") == 0);

  return 0;
}

static char *testWriterEndOnBufferSize()
{
  resetOutputFile();

  WRITE_CONTEXT *ctx = newWriteContext(NULL, NULL, 3, writeToFile);

  // Write a small string that won't flush the buffer
  writeString(ctx, testFile, testExt, "hi");
  mu_assert("expected file contents to be \"\"", strcmp(outputFile, "") == 0);

  // Write an additional string to cause the buffer to overflow
  writeString(ctx, testFile, testExt, " there!");
  mu_assert("expected file contents to be \"hi there!\"", strcmp(outputFile, "hi there!") == 0);

  // This will flush all remaining buffers
  freeWriteContext(ctx);
  mu_assert("expected file contents to be \"hi there!\"", strcmp(outputFile, "hi there!") == 0);

  return 0;
}

static char *testWriterMassiveBuffer()
{
  resetOutputFile();

  WRITE_CONTEXT *ctx = newWriteContext(NULL, NULL, 300, writeToFile);

  // Write a small string that won't flush the buffer
  writeString(ctx, testFile, testExt, "hi");
  mu_assert("expected file contents to be \"\"", strcmp(outputFile, "") == 0);

  // Write an additional string that still won't cause overflow
  writeString(ctx, testFile, testExt, " there!");
  mu_assert("expected file contents to be \"\"", strcmp(outputFile, "") == 0);

  // This will flush all remaining buffers
  freeWriteContext(ctx);
  mu_assert("expected file contents to be \"hi there!\"", strcmp(outputFile, "hi there!") == 0);

  return 0;
}

static char *all_tests()
{
  mu_run_test(testWriter);
  mu_run_test(testWriterEndOnBufferSize);
  mu_run_test(testWriterMassiveBuffer);
  return 0;
}

int main(int argc, char **argv)
{
  char *result = all_tests();
  if (result != 0)
  {
    printf("%s\n", result);
  }
  else
  {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", tests_run);

  return result != 0;
}
