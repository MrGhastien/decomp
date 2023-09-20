#include "defines.h"
#include "test.h"
#include "prime-count.h"

#include <stdio.h>

int log_test() {
    printf("ln(1) == %zu\n", naturalLog(1));
    ASSERT(naturalLog(1) == 0);

    printf("ln(3) == %zu\n", naturalLog(2));
    ASSERT(naturalLog(2) == 2);

    for (ulong x = 1; x < 20; x++) {
        printf("ln(%zu) == %zu\n", x, naturalLog(x));
    }
    return 0;
}

int li_test() {
    ulong power = 1;
    for (ulong x = 1; x < 15; x++) {
        power *= 10;
        printf("li(10^%zu) == %zu\n", x, approxPrimeCount(power));
    }
}
