#include "cli.h"
#include "minunit.h"

int tests_run = 0;

static char *testCliIncludeFilingId()
{
  CLI_CONTEXT *cli = newCliContext();

  const char *argv[] = {"fastfec", "-i", "13360.fec"};
  const int argc = sizeof(argv) / sizeof(argv[0]);
  parseArgs(cli, 0, argc, argv);

  mu_assert("Expected no piped", cli->piped == 0);
  mu_assert("Expected include filing id", cli->includeFilingId == 1);
  mu_assert("Expected no silent", cli->silent == 0);
  mu_assert("Expected no warn", cli->warn == 0);
  mu_assert("Expected no print url", cli->printUrl == 0);
  mu_assert("Expected no print usage", cli->shouldPrintUsage == 0);
  mu_assert("Expect file name to equal \"13360.fec\"", strcmp(cli->fecName, "13360.fec") == 0);
  mu_assert("Expect id to be 13360", strcmp(cli->fecId, "13360") == 0);

  freeCliContext(cli);

  return 0;
}

static char *testCliPrintUrl()
{
  CLI_CONTEXT *cli = newCliContext();

  const char *argv[] = {"fastfec", "-p", "12345"};
  const int argc = sizeof(argv) / sizeof(argv[0]);
  parseArgs(cli, 0, argc, argv);

  mu_assert("Expected no piped", cli->piped == 0);
  mu_assert("Expected no include filing id", cli->includeFilingId == 0);
  mu_assert("Expected no silent", cli->silent == 0);
  mu_assert("Expected no warn", cli->warn == 0);
  mu_assert("Expected print url", cli->printUrl == 1);
  mu_assert("Expected no print usage", cli->shouldPrintUsage == 0);
  mu_assert("Expect id to be 12345", strcmp(cli->fecId, "12345") == 0);

  freeCliContext(cli);

  return 0;
}

static char *testCliShowUsage1()
{
  CLI_CONTEXT *cli = newCliContext();

  const char *argv[] = {"fastfec", "-z", "12345"};
  const int argc = sizeof(argv) / sizeof(argv[0]);
  parseArgs(cli, 0, argc, argv);

  // Unknown flag
  mu_assert("Expected print usage", cli->shouldPrintUsage == 1);

  freeCliContext(cli);

  return 0;
}

static char *testCliShowUsage2()
{
  CLI_CONTEXT *cli = newCliContext();

  const char *argv[] = {"fastfec"};
  const int argc = sizeof(argv) / sizeof(argv[0]);
  parseArgs(cli, 0, argc, argv);

  // No args
  mu_assert("Expected print usage", cli->shouldPrintUsage == 1);

  freeCliContext(cli);

  return 0;
}

static char *testCliShowUsage3()
{
  CLI_CONTEXT *cli = newCliContext();

  const char *argv[] = {"fastfec", "--include-filing-id"};
  const int argc = sizeof(argv) / sizeof(argv[0]);
  parseArgs(cli, 0, argc, argv);

  // No args
  mu_assert("Expected print usage", cli->shouldPrintUsage == 1);

  freeCliContext(cli);

  return 0;
}

static char *testCliShowSpecifyFilingId()
{
  CLI_CONTEXT *cli = newCliContext();

  const char *argv[] = {"fastfec", "no_numbers.fec"};
  const int argc = sizeof(argv) / sizeof(argv[0]);
  parseArgs(cli, 0, argc, argv);

  // No discernable filing id
  mu_assert("Expected print usage", cli->shouldPrintUsage == 1);
  mu_assert("Expected specify filing id", cli->shouldPrintSpecifyFilingId == 1);

  freeCliContext(cli);

  return 0;
}

static char *testCliSilentWarnPipedIncludeFilingId()
{
  CLI_CONTEXT *cli = newCliContext();

  const char *argv[] = {"fastfec", "-sw", "--include-filing-id", "123"};
  const int argc = sizeof(argv) / sizeof(argv[0]);
  parseArgs(cli, 1, argc, argv);

  mu_assert("Expected piped", cli->piped == 1);
  mu_assert("Expected include filing id", cli->includeFilingId == 1);
  mu_assert("Expected silent", cli->silent == 1);
  mu_assert("Expected warn", cli->warn == 1);
  mu_assert("Expected no print url", cli->printUrl == 0);
  mu_assert("Expected no print usage", cli->shouldPrintUsage == 0);
  mu_assert("Expect id to be 123", strcmp(cli->fecId, "123") == 0);

  freeCliContext(cli);

  return 0;
}

static char *testCliPipedNoStdin()
{
  CLI_CONTEXT *cli = newCliContext();

  const char *argv[] = {"fastfec", "-sw", "--include-filing-id", "-x", "100.fec"};
  const int argc = sizeof(argv) / sizeof(argv[0]);
  parseArgs(cli, 1, argc, argv);

  mu_assert("Expected piped", cli->piped == 0);
  mu_assert("Expected include filing id", cli->includeFilingId == 1);
  mu_assert("Expected silent", cli->silent == 1);
  mu_assert("Expected warn", cli->warn == 1);
  mu_assert("Expected no print url", cli->printUrl == 0);
  mu_assert("Expected no print usage", cli->shouldPrintUsage == 0);
  mu_assert("Expect file name to equal \"100.fec\"", strcmp(cli->fecName, "100.fec") == 0);
  mu_assert("Expect id to be 100", strcmp(cli->fecId, "100") == 0);

  freeCliContext(cli);

  return 0;
}

static char *all_tests()
{
  mu_run_test(testCliIncludeFilingId);
  mu_run_test(testCliPrintUrl);
  mu_run_test(testCliShowUsage1);
  mu_run_test(testCliShowUsage2);
  mu_run_test(testCliShowUsage3);
  mu_run_test(testCliShowSpecifyFilingId);
  mu_run_test(testCliSilentWarnPipedIncludeFilingId);
  mu_run_test(testCliPipedNoStdin);
  return 0;
}

int main(int argc, char **argv)
{
  printf("\CLI tests\n");
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
