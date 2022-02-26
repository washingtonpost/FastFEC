#include <stdio.h>
#include <string.h>
#include "minunit.h"
#include "buffer.h"
#include "memory.h"

int tests_run = 0;

char contents[20] = "The cat\nand the\nhat.";
int contentsPos = 0;

int contentsRead(char *buffer, int want)
{
  int remaining = strlen(contents) - contentsPos;
  if (remaining < want)
  {
    want = remaining;
  }
  memcpy(buffer, contents + contentsPos, want);
  contentsPos += want;
  return want;
}

static char *testShortBuffer()
{
  contentsPos = 0;
  BUFFER *buffer = newBuffer(3, (BufferRead)contentsRead);
  STRING *s = newString(100);

  // Read lines
  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"The cat\n\"", strcmp(s->str, "The cat\n") == 0);

  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"and the\n\"", strcmp(s->str, "and the\n") == 0);

  mu_assert("Expected line length 4", readLine(buffer, s, NULL) == 4);
  mu_assert("Expected line \"hat.\"", strcmp(s->str, "hat.") == 0);

  mu_assert("Expected line length 0", readLine(buffer, s, NULL) == 0);
  mu_assert("Expected line \"\"", strcmp(s->str, "") == 0);

  freeBuffer(buffer);
  freeString(s);

  return 0;
}

static char *testLongBuffer()
{
  contentsPos = 0;
  BUFFER *buffer = newBuffer(300, (BufferRead)contentsRead);
  STRING *s = newString(100);

  // Read lines
  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"The cat\n\"", strcmp(s->str, "The cat\n") == 0);

  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"and the\n\"", strcmp(s->str, "and the\n") == 0);

  mu_assert("Expected line length 4", readLine(buffer, s, NULL) == 4);
  mu_assert("Expected line \"hat.\"", strcmp(s->str, "hat.") == 0);

  mu_assert("Expected line length 0", readLine(buffer, s, NULL) == 0);
  mu_assert("Expected line \"\"", strcmp(s->str, "") == 0);

  freeBuffer(buffer);
  freeString(s);

  return 0;
}

static char *testAlmostFileLengthBuffer()
{
  contentsPos = 0;
  BUFFER *buffer = newBuffer(19, (BufferRead)contentsRead);
  STRING *s = newString(10);

  // Read lines
  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"The cat\n\"", strcmp(s->str, "The cat\n") == 0);

  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"and the\n\"", strcmp(s->str, "and the\n") == 0);

  mu_assert("Expected line length 4", readLine(buffer, s, NULL) == 4);
  mu_assert("Expected line \"hat.\"", strcmp(s->str, "hat.") == 0);

  mu_assert("Expected line length 0", readLine(buffer, s, NULL) == 0);
  mu_assert("Expected line \"\"", strcmp(s->str, "") == 0);

  freeBuffer(buffer);
  freeString(s);

  return 0;
}

static char *testDivisibleBuffer()
{
  contentsPos = 0;
  BUFFER *buffer = newBuffer(10, (BufferRead)contentsRead);
  STRING *s = newString(100);

  // Read lines
  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"The cat\n\"", strcmp(s->str, "The cat\n") == 0);

  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"and the\n\"", strcmp(s->str, "and the\n") == 0);

  mu_assert("Expected line length 4", readLine(buffer, s, NULL) == 4);
  mu_assert("Expected line \"hat.\"", strcmp(s->str, "hat.") == 0);

  mu_assert("Expected line length 0", readLine(buffer, s, NULL) == 0);
  mu_assert("Expected line \"\"", strcmp(s->str, "") == 0);

  freeBuffer(buffer);
  freeString(s);

  return 0;
}

static char *testByteBuffer()
{
  contentsPos = 0;
  BUFFER *buffer = newBuffer(1, (BufferRead)contentsRead);
  STRING *s = newString(100);

  // Read lines
  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"The cat\n\"", strcmp(s->str, "The cat\n") == 0);

  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"and the\n\"", strcmp(s->str, "and the\n") == 0);

  mu_assert("Expected line length 4", readLine(buffer, s, NULL) == 4);
  mu_assert("Expected line \"hat.\"", strcmp(s->str, "hat.") == 0);

  mu_assert("Expected line length 0", readLine(buffer, s, NULL) == 0);
  mu_assert("Expected line \"\"", strcmp(s->str, "") == 0);

  freeBuffer(buffer);
  freeString(s);

  return 0;
}

static char *testStringExpansion()
{
  contentsPos = 0;
  BUFFER *buffer = newBuffer(3, (BufferRead)contentsRead);
  STRING *s = newString(1);

  // Read lines
  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"The cat\n\"", strcmp(s->str, "The cat\n") == 0);
  mu_assert("Expected n 16", s->n == 16);

  mu_assert("Expected line length 8", readLine(buffer, s, NULL) == 8);
  mu_assert("Expected line \"and the\n\"", strcmp(s->str, "and the\n") == 0);

  mu_assert("Expected line length 4", readLine(buffer, s, NULL) == 4);
  mu_assert("Expected line \"hat.\"", strcmp(s->str, "hat.") == 0);

  mu_assert("Expected line length 0", readLine(buffer, s, NULL) == 0);
  mu_assert("Expected line \"\"", strcmp(s->str, "") == 0);

  freeBuffer(buffer);
  freeString(s);

  return 0;
}

static char *all_tests()
{
  mu_run_test(testShortBuffer);
  mu_run_test(testLongBuffer);
  mu_run_test(testAlmostFileLengthBuffer);
  mu_run_test(testDivisibleBuffer);
  mu_run_test(testByteBuffer);
  mu_run_test(testStringExpansion);
  return 0;
}

int main(int argc, char **argv)
{
  printf("\nBuffer tests\n");
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
