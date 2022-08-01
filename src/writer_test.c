#include <stdio.h>
#include <string.h>
#include "minunit.h"
#include "writer.h"

int tests_run = 0;

char outputFile[100];
int outputFilePosition = 0;

char outputLine[100];

void resetOutput()
{
  // Reset the output file/line
  outputFilePosition = 0;
  outputFile[0] = '\0';
  outputLine[0] = '\0';
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

void writeToLine(char *filename, char *line, char *types)
{
  // Write to line
  strcpy(outputLine, line);
}

char *testFile = "test";
char *testExt = ".txt";

static char *testWriter()
{
  resetOutput();

  WRITE_CONTEXT *ctx = newWriteContext(NULL, NULL, 0, 3, writeToFile, writeToLine);

  // Write a small string that won't flush the buffer
  writeString(ctx, testFile, testExt, "hi");
  mu_assert("expected file contents to be \"\"", strcmp(outputFile, "") == 0);
  mu_assert("expected line contents to be \"\"", strcmp(outputLine, "") == 0);

  // Write an additional string to cause the buffer to overflow
  writeString(ctx, testFile, testExt, " there");
  mu_assert("expected file contents to be \"hi the\"", strcmp(outputFile, "hi the") == 0);
  mu_assert("expected line contents to be \"\"", strcmp(outputLine, "") == 0);

  // Write a newline
  writeChar(ctx, testFile, testExt, '\n');
  endLine(ctx, NULL);

  // This will flush all remaining buffers
  freeWriteContext(ctx);
  mu_assert("expected file contents to be \"hi there\n\"", strcmp(outputFile, "hi there\n") == 0);
  mu_assert("expected line contents to be \"hi there\n\"", strcmp(outputLine, "hi there\n") == 0);

  return 0;
}

static char *testWriterEndOnBufferSize()
{
  resetOutput();

  WRITE_CONTEXT *ctx = newWriteContext(NULL, NULL, 0, 3, writeToFile, writeToLine);

  // Write a small string that won't flush the buffer
  writeString(ctx, testFile, testExt, "hi");
  mu_assert("expected file contents to be \"\"", strcmp(outputFile, "") == 0);
  mu_assert("expected line contents to be \"\"", strcmp(outputLine, "") == 0);

  // Write an additional string to cause the buffer to overflow
  writeString(ctx, testFile, testExt, " there!");
  mu_assert("expected file contents to be \"hi there!\"", strcmp(outputFile, "hi there!") == 0);
  mu_assert("expected line contents to be \"\"", strcmp(outputLine, "") == 0);

  // Write a newline
  writeChar(ctx, testFile, testExt, '\n');
  endLine(ctx, NULL);

  // This will flush all remaining buffers
  freeWriteContext(ctx);
  mu_assert("expected file contents to be \"hi there!\n\"", strcmp(outputFile, "hi there!\n") == 0);
  mu_assert("expected line contents to be \"hi there!\n\"", strcmp(outputLine, "hi there!\n") == 0);

  return 0;
}

static char *testWriterMassiveBuffer()
{
  resetOutput();

  WRITE_CONTEXT *ctx = newWriteContext(NULL, NULL, 0, 300, writeToFile, writeToLine);

  // Write a small string that won't flush the buffer
  writeString(ctx, testFile, testExt, "hi");
  mu_assert("expected file contents to be \"\"", strcmp(outputFile, "") == 0);
  mu_assert("expected line contents to be \"\"", strcmp(outputLine, "") == 0);

  // Write an additional string that still won't cause overflow
  writeString(ctx, testFile, testExt, " there!");
  mu_assert("expected file contents to be \"\"", strcmp(outputFile, "") == 0);
  mu_assert("expected line contents to be \"\"", strcmp(outputLine, "") == 0);

  // Write a newline
  writeChar(ctx, testFile, testExt, '\n');
  endLine(ctx, NULL);

  // This will flush all remaining buffers
  freeWriteContext(ctx);
  mu_assert("expected file contents to be \"hi there!\n\"", strcmp(outputFile, "hi there!\n") == 0);
  mu_assert("expected line contents to be \"hi there!\n\"", strcmp(outputLine, "hi there!\n") == 0);

  return 0;
}

static char *testLineBuffer()
{
  resetOutput();

  WRITE_CONTEXT *ctx = newWriteContext(NULL, NULL, 0, 300, writeToFile, writeToLine);

  // Write a small string that won't flush the buffer
  writeString(ctx, testFile, testExt, "hi there\n");
  endLine(ctx, NULL);
  mu_assert("expected line contents to be \"hi there\n\"", strcmp(outputLine, "hi there\n") == 0);

  writeString(ctx, testFile, testExt, "how are you today?\n");
  endLine(ctx, NULL);
  mu_assert("expected line contents to be \"how are you today?\n\"", strcmp(outputLine, "how are you today?\n") == 0);

  freeWriteContext(ctx);

  return 0;
}

static char *all_tests()
{
  mu_run_test(testWriter);
  mu_run_test(testWriterEndOnBufferSize);
  mu_run_test(testWriterMassiveBuffer);
  mu_run_test(testLineBuffer);
  return 0;
}

int main(int argc, char **argv)
{
  printf("\nWriter tests\n");
  char *result = all_tests();
  if (result != 0)
  {
    printf("%s\n", result);
  }
  else
  {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n\n", tests_run);

  return result != 0;
}
