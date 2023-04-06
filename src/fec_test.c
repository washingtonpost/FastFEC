#include <stdio.h>
#include <string.h>
#include "minunit.h"
#include "memory.h"
#include "fec.h"

int tests_run = 0;

static char *testVersionUsesAscii28()
{
    // No dots defaults to True
    mu_assert("expect, 1->True", versionUsesAscii28("1") == 1);
    mu_assert("expect, 2->True", versionUsesAscii28("2") == 1);
    mu_assert("expect, 1.1->False", versionUsesAscii28("1.1") == 0);
    mu_assert("expect, 2.8->False", versionUsesAscii28("2.8") == 0);
    mu_assert("expect, 3.4.3->False", versionUsesAscii28("3.4.3") == 0);
    mu_assert("expect, 8.4->True", versionUsesAscii28("8.4") == 1);
    return 0;
}

static char *all_tests()
{
    mu_run_test(testVersionUsesAscii28);
    return 0;
}

int main(int argc, char **argv)
{
    printf("\nfec tests\n");
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
