#include <stdio.h>
#include <string.h>
#include "minunit.h"
#include "memory.h"
#include "csv.h"

int tests_run = 0;

// Quick helper method to init parse contexts for testing
void initParseContextWithLine(PARSE_CONTEXT *parseContext, FIELD_INFO *fieldInfo, STRING *line)
{
  parseContext->line = line;
  parseContext->fieldInfo = NULL;
  parseContext->position = 0;
  parseContext->columnIndex = 0;
  fieldInfo->num_commas = 0;
  fieldInfo->num_quotes = 0;
  parseContext->fieldInfo = fieldInfo;
}

static char *testCsvReading()
{
  PARSE_CONTEXT parseContext;
  FIELD_INFO fieldInfo;

  // Basic test
  STRING *csv = fromString("abc");
  initParseContextWithLine(&parseContext, &fieldInfo, csv);
  readCsvField(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 3", parseContext.end == 3);
  mu_assert("error, position != 3", parseContext.position == 3);
  mu_assert("num quotes != 0", fieldInfo.num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  // Quoted field
  setString(csv, "\"abc\"");
  initParseContextWithLine(&parseContext, &fieldInfo, csv);
  readCsvField(&parseContext);
  mu_assert("error, start != 1", parseContext.start == 1);
  mu_assert("error, end != 4", parseContext.end == 4);
  mu_assert("error, position != 5", parseContext.position == 5);
  mu_assert("num quotes != 0", fieldInfo.num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  // Quoted field with escaped quote
  setString(csv, "\"a\"\",a\"\"b,\"\"c,\"\"\"\"\",\"\"");
  initParseContextWithLine(&parseContext, &fieldInfo, csv);
  parseContext.position = 3;
  readCsvField(&parseContext);
  mu_assert("error, start != 4", parseContext.start == 4);
  mu_assert("error, end != 14", parseContext.end == 14);
  mu_assert("error, position != 19", parseContext.position == 19);
  mu_assert("num quotes != 4", fieldInfo.num_quotes == 4);
  mu_assert("num commas != 3", fieldInfo.num_commas == 3);

  // Empty quoted field
  setString(csv, "\"\"");
  initParseContextWithLine(&parseContext, &fieldInfo, csv);
  readCsvField(&parseContext);
  mu_assert("error, start != 1", parseContext.start == 1);
  mu_assert("error, end != 1", parseContext.end == 1);
  mu_assert("error, position != 2", parseContext.position == 2);
  mu_assert("num quotes != 0", fieldInfo.num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  // Empty string
  setString(csv, ",,");
  initParseContextWithLine(&parseContext, &fieldInfo, csv);
  readCsvField(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 0", parseContext.end == 0);
  mu_assert("error, position != 0", parseContext.position == 0);
  mu_assert("num quotes != 0", fieldInfo.num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  // Doubly quoted field
  setString(csv, "\"\"\"FEC\"\"\"");
  initParseContextWithLine(&parseContext, &fieldInfo, csv);
  readCsvField(&parseContext);
  char *resultString = malloc(parseContext.end - parseContext.start + 1);
  strncpy(resultString, parseContext.line->str + parseContext.start, parseContext.end - parseContext.start);
  resultString[parseContext.end - parseContext.start] = 0;
  mu_assert("field should be \"FEC\" without quotes", strcmp(resultString, "FEC") == 0);
  mu_assert("error, position != 9", parseContext.position == 9);
  mu_assert("num quotes != 0", fieldInfo.num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  // Reads last field when advancing
  setString(csv, "a,b,c\nd,e,f\n");
  initParseContextWithLine(&parseContext, &fieldInfo, csv);
  readCsvField(&parseContext);
  advanceField(&parseContext);
  readCsvField(&parseContext);
  advanceField(&parseContext);
  readCsvField(&parseContext);
  resultString = realloc(resultString, parseContext.end - parseContext.start + 1);
  strncpy(resultString, parseContext.line->str + parseContext.start, parseContext.end - parseContext.start);
  resultString[parseContext.end - parseContext.start] = 0;
  mu_assert("field should be \"c\" without quotes", strcmp(resultString, "c") == 0);
  mu_assert("error, position != 5", parseContext.position == 5);
  mu_assert("num quotes != 0", fieldInfo.num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  freeString(csv);
  free(resultString);
  return 0;
}

static char *testAscii28Reading()
{
  PARSE_CONTEXT parseContext;
  FIELD_INFO fieldInfo;

  // Basic test
  STRING *line = fromString("abc");
  initParseContextWithLine(&parseContext, &fieldInfo, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 3", parseContext.end == 3);
  mu_assert("error, position != 3", parseContext.position == 3);
  mu_assert("num quotes != 0", fieldInfo.num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  // Quoted field
  setString(line, "\"abc\"");
  initParseContextWithLine(&parseContext, &fieldInfo, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 1", parseContext.start == 1);
  mu_assert("error, end != 4", parseContext.end == 4);
  mu_assert("error, position != 5", parseContext.position == 5);
  mu_assert("num quotes != 0", fieldInfo.num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  // Quote at beginning but not end
  setString(line, "\"abc");
  initParseContextWithLine(&parseContext, &fieldInfo, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 4", parseContext.end == 4);
  mu_assert("error, position != 4", parseContext.position == 4);
  mu_assert("num quotes != 1", fieldInfo.num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  // Quote in middle
  setString(line, "ab\"c");
  initParseContextWithLine(&parseContext, &fieldInfo, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 4", parseContext.end == 4);
  mu_assert("error, position != 4", parseContext.position == 4);
  mu_assert("num quotes != 1", fieldInfo.num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  // Quote at end
  setString(line, "abc\"");
  initParseContextWithLine(&parseContext, &fieldInfo, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 4", parseContext.end == 4);
  mu_assert("error, position != 4", parseContext.position == 4);
  mu_assert("num quotes != 1", fieldInfo.num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  // Ascii 28 delimiter
  setString(line, "\"ab\034c\"");
  initParseContextWithLine(&parseContext, &fieldInfo, line);
  readAscii28Field(&parseContext);
  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 3", parseContext.end == 3);
  mu_assert("error, position != 3", parseContext.position == 3);
  mu_assert("num quotes != 1", fieldInfo.num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo.num_commas == 0);

  freeString(line);
  return 0;
}

static char *testStripWhitespace()
{
  PARSE_CONTEXT parseContext;
  FIELD_INFO fieldInfo;

  // Basic test
  STRING *line = fromString("   abc    ");
  initParseContextWithLine(&parseContext, &fieldInfo, line);
  readAscii28Field(&parseContext);

  mu_assert("error, start != 0", parseContext.start == 0);
  mu_assert("error, end != 10", parseContext.end == 10);
  mu_assert("error, position != 10", parseContext.position == 10);

  stripWhitespace(&parseContext);

  mu_assert("error, start != 3", parseContext.start == 3);
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
  printf("\nCSV tests\n");
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
