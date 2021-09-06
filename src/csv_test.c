#include <stdio.h>
#include "minunit.h"
#include "memory.h"
#include "csv.h"

int tests_run = 0;

static char *testCsvReading()
{
  int start, end, position;

  // Basic test
  STRING *csv = fromString("abc");
  position = 0;
  readCsvField(csv, &position, &start, &end);
  mu_assert("error, start != 0", start == 0);
  mu_assert("error, end != 3", end == 3);
  mu_assert("error, position != 3", position == 3);

  // Quoted field
  setString(csv, "\"abc\"");
  position = 0;
  readCsvField(csv, &position, &start, &end);
  mu_assert("error, start != 1", start == 1);
  mu_assert("error, end != 4", end == 4);
  mu_assert("error, position != 5", position == 5);

  // Quoted field with escaped quote
  setString(csv, "\"a\"\",a\"\"b,\"\"c,\"\"\"\"\",\"\"");
  position = 3;
  readCsvField(csv, &position, &start, &end);
  mu_assert("error, start != 4", start == 4);
  mu_assert("error, end != 14", end == 14);
  mu_assert("error, position != 19", position == 19);

  // Empty quoted field
  setString(csv, "\"\"");
  position = 0;
  readCsvField(csv, &position, &start, &end);
  mu_assert("error, start != 1", start == 1);
  mu_assert("error, end != 1", end == 1);
  mu_assert("error, position != 2", position == 2);

  freeString(csv);
  return 0;
}

static char *testAscii28Reading()
{
  int start, end, position;

  // Basic test
  STRING *line = fromString("abc");
  position = 0;
  readAscii28Field(line, &position, &start, &end);
  mu_assert("error, start != 0", start == 0);
  mu_assert("error, end != 3", end == 3);
  mu_assert("error, position != 3", position == 3);

  // Quoted field
  setString(line, "\"abc\"");
  position = 0;
  readAscii28Field(line, &position, &start, &end);
  mu_assert("error, start != 1", start == 1);
  mu_assert("error, end != 4", end == 4);
  mu_assert("error, position != 5", position == 5);

  // Quote at beginning but not end
  setString(line, "\"abc");
  position = 0;
  readAscii28Field(line, &position, &start, &end);
  mu_assert("error, start != 0", start == 0);
  mu_assert("error, end != 4", end == 4);
  mu_assert("error, position != 4", position == 4);

  // Quote in middle
  setString(line, "ab\"c");
  position = 0;
  readAscii28Field(line, &position, &start, &end);
  mu_assert("error, start != 0", start == 0);
  mu_assert("error, end != 4", end == 4);
  mu_assert("error, position != 4", position == 4);

  // Quote at end
  setString(line, "abc\"");
  position = 0;
  readAscii28Field(line, &position, &start, &end);
  mu_assert("error, start != 0", start == 0);
  mu_assert("error, end != 4", end == 4);
  mu_assert("error, position != 4", position == 4);

  // Ascii 28 delimiter
  setString(line, "\"ab\034c\"");
  position = 0;
  readAscii28Field(line, &position, &start, &end);
  mu_assert("error, start != 0", start == 0);
  mu_assert("error, end != 3", end == 3);
  mu_assert("error, position != 3", position == 3);

  freeString(line);
  return 0;
}

static char *all_tests()
{
  mu_run_test(testCsvReading);
  mu_run_test(testAscii28Reading);
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