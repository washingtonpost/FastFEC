#include <stdio.h>
#include <string.h>
#include "minunit.h"
#include "memory.h"
#include "fec.h"

int tests_run = 0;

static char *all_tests()
{
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
