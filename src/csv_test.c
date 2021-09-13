#include <stdio.h>
#include "minunit.h"
#include "memory.h"
#include "csv.h"

int tests_run = 0;

// Quick helper method to init parse contexts for testing
void initParseContextWithLine(PARSE_CONTEXT *parseContext, STRING *line)
{
  parseContext->line = line;
  parseContext->fieldInfo = NULL;
  parseContext->position = 0;
  parseContext->columnIndex = 0;
}

static char *testCsvReading()
{
  PARSE_CONTEXT parseContext;

  // Basic test
  STRING *csv = fromString("abc");
  initParseContextWithLine(&parseContext, csv);
  readCsvField(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 3", parseContext.end == 3);
  mu_assert("error, position != 3", parseContext.position == 3);

  // Quoted field
  setString(csv, "\"abc\"");
  initParseContextWithLine(&parseContext, csv);
  readCsvField(&parseContext);
  mu_assert("error, start != 1", parseContext.start == 1);
  mu_assert("error, end != 4", parseContext.end == 4);
  mu_assert("error, position != 5", parseContext.position == 5);

  // Quoted field with escaped quote
  setString(csv, "\"a\"\",a\"\"b,\"\"c,\"\"\"\"\",\"\"");
  initParseContextWithLine(&parseContext, csv);
  parseContext.position = 3;
  readCsvField(&parseContext);
  mu_assert("error, start != 4", parseContext.start == 4);
  mu_assert("error, end != 14", parseContext.end == 14);
  mu_assert("error, position != 19", parseContext.position == 19);

  // Empty quoted field
  setString(csv, "\"\"");
  initParseContextWithLine(&parseContext, csv);
  readCsvField(&parseContext);
  mu_assert("error, start != 1", parseContext.start == 1);
  mu_assert("error, end != 1", parseContext.end == 1);
  mu_assert("error, position != 2", parseContext.position == 2);

  freeString(csv);
  return 0;
}

static char *testAscii28Reading()
{
  PARSE_CONTEXT parseContext;

  // Basic test
  STRING *line = fromString("abc");
  initParseContextWithLine(&parseContext, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 3", parseContext.end == 3);
  mu_assert("error, position != 3", parseContext.position == 3);

  // Quoted field
  setString(line, "\"abc\"");
  initParseContextWithLine(&parseContext, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 1", parseContext.start == 1);
  mu_assert("error, end != 4", parseContext.end == 4);
  mu_assert("error, position != 5", parseContext.position == 5);

  // Quote at beginning but not end
  setString(line, "\"abc");
  initParseContextWithLine(&parseContext, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 4", parseContext.end == 4);
  mu_assert("error, position != 4", parseContext.position == 4);

  // Quote in middle
  setString(line, "ab\"c");
  initParseContextWithLine(&parseContext, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 4", parseContext.end == 4);
  mu_assert("error, position != 4", parseContext.position == 4);

  // Quote at end
  setString(line, "abc\"");
  initParseContextWithLine(&parseContext, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 4", parseContext.end == 4);
  mu_assert("error, position != 4", parseContext.position == 4);

  // Ascii 28 delimiter
  setString(line, "\"ab\034c\"");
  initParseContextWithLine(&parseContext, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 3", parseContext.end == 3);
  mu_assert("error, position != 3", parseContext.position == 3);

  // Empty string
  setString(line, ",,");
  initParseContextWithLine(&parseContext, line);
  readCsvField(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 0", parseContext.end == 0);
  mu_assert("error, position != 0", parseContext.position == 0);

  freeString(line);
  return 0;
}

static char *testStripWhitespace()
{
  PARSE_CONTEXT parseContext;

  // Basic test
  STRING *line = fromString("   abc    ");
  initParseContextWithLine(&parseContext, line);
  readAscii28Field(&parseContext);

  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 10", parseContext.end == 10);
  mu_assert("error, position != 10", parseContext.position == 10);

  stripWhitespace(&parseContext);

  mu_assert("error, start != 3", parseContext.start == 3);
  printf("%d\n", parseContext.end);
  mu_assert("error, end != 6", parseContext.end == 6);
  mu_assert("error, position != 10", parseContext.position == 10);

  freeString(line);
  return 0;
}

static char *all_tests()
{
  mu_run_test(testCsvReading);
  mu_run_test(testAscii28Reading);
  mu_run_test(testStripWhitespace);
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