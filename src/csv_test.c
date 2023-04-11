#include <stdio.h>
#include <string.h>
#include "minunit.h"
#include "memory.h"
#include "csv.h"

int tests_run = 0;

static char *testCsvReading(void)
{
  CSV_LINE_PARSER parser;
  CSV_FIELD *field = &(parser.currentField);
  FIELD_INFO *fieldInfo = &(field->info);
  STRING *csv = fromString("");

  setString(csv, "");
  csvParserInit(&parser, csv);
  nextField(&parser, 0);
  mu_assert("expect position == 3", parser.position == 0);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect end == 0", field->length == 0);
  mu_assert("expect field.chars == NULL", strncmp(field->chars, "", 0) == 0);
  mu_assert("expect num quotes == 0", fieldInfo->num_quotes == 0);
  mu_assert("expect num commas == 0", fieldInfo->num_commas == 0);

  // Basic test
  setString(csv, "abc");
  csvParserInit(&parser, csv);
  nextField(&parser, 0);
  mu_assert("error, position != 3", parser.position == 3);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 3", field->length == 3);
  mu_assert("expect field.chars == abc", strncmp(field->chars, "abc", 3) == 0);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quoted field
  setString(csv, "\"abc\"");
  csvParserInit(&parser, csv);
  nextField(&parser, 0);
  mu_assert("error, position != 5", parser.position == 5);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 3", field->length == 3);
  mu_assert("expect field.chars == abc", strncmp(field->chars, "abc", 3) == 0);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quoted field with escaped quote
  setString(csv, "\"a\"\",a\"\"b,\"\"c,\"\"\"\"\",\"\"");
  csvParserInit(&parser, csv);
  parser.position = 3;
  nextField(&parser, 0);
  mu_assert("expect position at start of next field", parser.position == 20);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 11", field->length == 10);
  mu_assert("expect field.chars == ,a\"b,\"c,\"\"", strncmp(field->chars, ",a\"b,\"c,\"\"", 10) == 0);
  mu_assert("num quotes != 4", fieldInfo->num_quotes == 4);
  mu_assert("num commas != 3", fieldInfo->num_commas == 3);

  // Empty quoted field
  setString(csv, "\"\"");
  csvParserInit(&parser, csv);
  nextField(&parser, 0);
  mu_assert("error, position != 2", parser.position == 2);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 0", field->length == 0);
  mu_assert("expect field.chars == \"\"", strncmp(field->chars, "", 0) == 0);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Empty string
  setString(csv, ",,");
  csvParserInit(&parser, csv);
  nextField(&parser, 0);
  mu_assert("expect, position == 1", parser.position == 1);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 0", field->length == 0);
  mu_assert("expect field.chars == \"\"", strncmp(field->chars, "", 0) == 0);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Doubly quoted field
  setString(csv, "\"\"\"FEC\"\"\"");
  csvParserInit(&parser, csv);
  nextField(&parser, 0);
  mu_assert("error, position != 9", parser.position == 9);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 3", field->length == 3);
  mu_assert("field should be \"FEC\" without quotes", strncmp(field->chars, "FEC", 3) == 0);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Reads last field when advancing
  setString(csv, "a,b,c\nd,e,f\n");
  csvParserInit(&parser, csv);
  nextField(&parser, 0);
  nextField(&parser, 0);
  nextField(&parser, 0);
  mu_assert("error, position != 5", parser.position == 5);
  mu_assert("expect numFieldsRead == 3", parser.numFieldsRead == 3);
  mu_assert("expect field.length == 1", field->length == 1);
  mu_assert("field should be \"c\" without quotes", strncmp(field->chars, "c", 1) == 0);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  freeString(csv);
  return 0;
}

static char *testAscii28Reading(void)
{
  CSV_LINE_PARSER parser;
  CSV_FIELD *field = &(parser.currentField);
  FIELD_INFO *fieldInfo = &(field->info);

  // Basic test
  STRING *line = fromString("abc");
  csvParserInit(&parser, line);
  nextField(&parser, 1);
  mu_assert("error, position != 3", parser.position == 3);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 3", field->length == 3);
  mu_assert("expect field.chars == abc", strncmp(field->chars, "abc", 3) == 0);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quoted field only contains interior text
  setString(line, "\"abc\"");
  csvParserInit(&parser, line);
  nextField(&parser, 1);
  mu_assert("expect position == 5", parser.position == 5);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 3", field->length == 3);
  mu_assert("expect field.chars == abc", strncmp(field->chars, "abc", 3) == 0);
  mu_assert("num quotes != 0", fieldInfo->num_quotes == 0);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quote at beginning but not end
  setString(line, "\"abc");
  csvParserInit(&parser, line);
  nextField(&parser, 1);
  mu_assert("expect position == 4", parser.position == 4);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 4", field->length == 4);
  mu_assert("expect field.chars == \"abc", strncmp(field->chars, "\"abc", 3) == 0);
  mu_assert("num quotes != 1", fieldInfo->num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quote in middle
  setString(line, "ab\"c");
  csvParserInit(&parser, line);
  nextField(&parser, 1);
  mu_assert("error, position != 4", parser.position == 4);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 4", field->length == 4);
  mu_assert("expect field.chars == ab\"c", strncmp(field->chars, "ab\"c", 3) == 0);
  mu_assert("num quotes != 1", fieldInfo->num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Quote at end
  setString(line, "abc\"");
  csvParserInit(&parser, line);
  nextField(&parser, 1);
  mu_assert("error, position != 4", parser.position == 4);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 4", field->length == 4);
  mu_assert("expect field.chars == abc\"", strncmp(field->chars, "abc\"", 3) == 0);
  mu_assert("num quotes != 1", fieldInfo->num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  // Ascii 28 delimiter
  setString(line, "\"ab\034c\"");
  csvParserInit(&parser, line);
  nextField(&parser, 1);
  mu_assert("expect position right after asci28", parser.position == 4);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 3", field->length == 3);
  mu_assert("expect field.chars == \"ab", strncmp(field->chars, "\"ab", 3) == 0);
  mu_assert("num quotes != 1", fieldInfo->num_quotes == 1);
  mu_assert("num commas != 0", fieldInfo->num_commas == 0);

  freeString(line);
  return 0;
}

static char *testStripWhitespace()
{
  CSV_LINE_PARSER parser;

  // Basic test
  char *s = "   abc    ";
  STRING *line = fromString(s);
  csvParserInit(&parser, line);
  nextField(&parser, 1);

  mu_assert("expect position == 10", parser.position == 10);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == strlen(s)", parser.currentField.length == strlen(s));
  mu_assert("expect field.chars == line", strncmp(parser.currentField.chars, s, strlen(s)) == 0);

  stripWhitespace(&(parser.currentField));

  mu_assert("expect position == 10", parser.position == 10);
  mu_assert("expect numFieldsRead == 1", parser.numFieldsRead == 1);
  mu_assert("expect field.length == 3", parser.currentField.length == 3);
  mu_assert("expect field.chars == abc", strncmp(parser.currentField.chars, "abc", 3) == 0);

  freeString(line);
  return 0;
}

static char *all_tests(void)
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
