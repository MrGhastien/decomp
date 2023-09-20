#include "defines.h"
#include <math.h>

#define LOG_ITER_COUNT 100
#define LI_ITER_COUNT 10
#define EULER_CST 0.577215664901532860606512090082402431042159
#define LI2 1.0451637801174927848445888891946131365226155781512015758329091440

static ulong highBitPosition(ulong x) {
    ulong r = 0;
    while (x >>= 1) {
        r++;
    }
    return r;
}

static ulong isqrt(ulong num) {
    ulong x = num >> 1;
    for (uint i = 0; i < ISQRT_STEPS; i++) {
        x = (x + (num / x)) >> 1;
    }
    return x;
}

ulong naturalLog(ulong x) {
    if(x == 1)
        return 0;
    ulong n = highBitPosition(x);
    long double y = (long double)x / (1 << n);

    long double result = logl(y) + n * M_LN2;
    return llroundl(result);
}

ulong factorial(ulong x) {
    ulong result = 1;
    for (size_t n = 2; n <= x; n++) {
        result *= n;
    }
    return result;
}

ulong integralLog(ulong x) {
    ulong numIncrements = 1000000;
    long double dt = (x - 2) / (long double)numIncrements;
    long double sum = 0;
    for (long double t = 2; t <= x; t += dt) {
        sum += dt / logl(t);
    }
    return llroundl(sum);
}

ulong approxPrimeCount(ulong limit) {
    if(limit <= 1)
        return 0;
    return integralLog(limit) * 1.01;
}
