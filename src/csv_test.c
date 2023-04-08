#include <stdio.h>
#include <string.h>
#include "minunit.h"
#include "memory.h"
#include "csv.h"

int tests_run = 0;

static char *testCsvReading()
{
  CSV_LINE_PARSER parser;
  FIELD_INFO *fieldInfo = &(parser.fieldInfo);

  // Basic test
  STRING *csv = fromString("abc");
  csvParserInit(&parser, csv);
  readField(&parser, 0);
  mu_assert("error, start != 0", parser.start == 0);
  mu_assert("error, end != 3", parser.end == 3);
  mu_assert("error, position != 3", parser.position == 3);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quoted field
  setString(csv, "\"abc\"");
  csvParserInit(&parser, csv);
  readField(&parser, 0);
  mu_assert("error, start != 1", parser.start == 1);
  mu_assert("error, end != 4", parser.end == 4);
  mu_assert("error, position != 5", parser.position == 5);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quoted field with escaped quote
  setString(csv, "\"a\"\",a\"\"b,\"\"c,\"\"\"\"\",\"\"");
  csvParserInit(&parser, csv);
  parser.position = 3;
  readField(&parser, 0);
  mu_assert("error, start != 4", parser.start == 4);
  mu_assert("error, end != 14", parser.end == 14);
  mu_assert("error, position != 19", parser.position == 19);
  mu_assert("num quotes != 4", fieldInfo->num_quotes == 4);
  mu_assert("num commas != 3", fieldInfo->num_commas == 3);

  // Empty quoted field
  setString(csv, "\"\"");
  csvParserInit(&parser, csv);
  readField(&parser, 0);
  mu_assert("error, start != 1", parser.start == 1);
  mu_assert("error, end != 1", parser.end == 1);
  mu_assert("error, position != 2", parser.position == 2);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Empty string
  setString(csv, ",,");
  csvParserInit(&parser, csv);
  readField(&parser, 0);
  mu_assert("error, start != 0", parser.start == 0);
  mu_assert("error, end != 0", parser.end == 0);
  mu_assert("error, position != 0", parser.position == 0);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Doubly quoted field
  setString(csv, "\"\"\"FEC\"\"\"");
  csvParserInit(&parser, csv);
  readField(&parser, 0);
  char *resultString = malloc(parser.end - parser.start + 1);
  strncpy(resultString, parser.line->str + parser.start, parser.end - parser.start);
  resultString[parser.end - parser.start] = 0;
  mu_assert("field should be \"FEC\" without quotes", strcmp(resultString, "FEC") == 0);
  mu_assert("error, position != 9", parser.position == 9);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Reads last field when advancing
  setString(csv, "a,b,c\nd,e,f\n");
  csvParserInit(&parser, csv);
  readField(&parser, 0);
  advanceField(&parser);
  readField(&parser, 0);
  advanceField(&parser);
  readField(&parser, 0);
  resultString = realloc(resultString, parser.end - parser.start + 1);
  strncpy(resultString, parser.line->str + parser.start, parser.end - parser.start);
  resultString[parser.end - parser.start] = 0;
  mu_assert("field should be \"c\" without quotes", strcmp(resultString, "c") == 0);
  mu_assert("error, position != 5", parser.position == 5);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  freeString(csv);
  free(resultString);
  return 0;
}

static char *testAscii28Reading()
{
  CSV_LINE_PARSER parser;
  FIELD_INFO *fieldInfo = &(parser.fieldInfo);

  // Basic test
  STRING *line = fromString("abc");
  csvParserInit(&parser, line);
  readField(&parser, 1);
  mu_assert("error, start != 0", parser.start == 0);
  mu_assert("error, end != 3", parser.end == 3);
  mu_assert("error, position != 3", parser.position == 3);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quoted field
  setString(line, "\"abc\"");
  csvParserInit(&parser, line);
  readField(&parser, 1);
  mu_assert("error, start != 1", parser.start == 1);
  mu_assert("error, end != 4", parser.end == 4);
  mu_assert("error, position != 5", parser.position == 5);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quote at beginning but not end
  setString(line, "\"abc");
  csvParserInit(&parser, line);
  readField(&parser, 1);
  mu_assert("error, start != 0", parser.start == 0);
  mu_assert("error, end != 4", parser.end == 4);
  mu_assert("error, position != 4", parser.position == 4);
  mu_assert("num quotes != 1", fieldInfo->num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quote in middle
  setString(line, "ab\"c");
  csvParserInit(&parser, line);
  readField(&parser, 1);
  mu_assert("error, start != 0", parser.start == 0);
  mu_assert("error, end != 4", parser.end == 4);
  mu_assert("error, position != 4", parser.position == 4);
  mu_assert("num quotes != 1", fieldInfo->num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quote at end
  setString(line, "abc\"");
  csvParserInit(&parser, line);
  readField(&parser, 1);
  mu_assert("error, start != 0", parser.start == 0);
  mu_assert("error, end != 4", parser.end == 4);
  mu_assert("error, position != 4", parser.position == 4);
  mu_assert("num quotes != 1", fieldInfo->num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Ascii 28 delimiter
  setString(line, "\"ab\034c\"");
  csvParserInit(&parser, line);
  readField(&parser, 1);
  mu_assert("error, start != 0", parser.start == 0);
  mu_assert("error, end != 3", parser.end == 3);
  mu_assert("error, position != 3", parser.position == 3);
  mu_assert("num quotes != 1", fieldInfo->num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  freeString(line);
  return 0;
}

static char *testStripWhitespace()
{
  CSV_LINE_PARSER parser;

  // Basic test
  STRING *line = fromString("   abc    ");
  csvParserInit(&parser, line);
  readField(&parser, 1);

  mu_assert("error, start != 0", parser.start == 0);
  mu_assert("error, end != 10", parser.end == 10);
  mu_assert("error, position != 10", parser.position == 10);

  stripWhitespace(&parser);

  mu_assert("error, start != 3", parser.start == 3);
  mu_assert("error, end != 6", parser.end == 6);
  mu_assert("error, position != 10", parser.position == 10);

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
