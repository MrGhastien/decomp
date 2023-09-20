#include "progress.h"
#include "primes.h"
#include "decomposition.h"

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <pthread.h>


static ulong iterCount = 0;
static ulong progress = 0;

/* ===== Text operations ===== */

ulong strlen(const char* str) {
    ulong len = 0;
    while (*str) {
        len++;
        str++;
    }
    return len;
}

char *replaceExt(const char* path, const char* ext) {
    ulong extLen = strlen(ext);
    ulong pathLen = strlen(path);
    char* end = path + pathLen;
    char* dotPos = end;
    while (dotPos > path && *dotPos != '.' && *dotPos != '\\' && *dotPos != '/') {
        dotPos--;
    }
    if (*dotPos == '.' && dotPos > path && *(dotPos - 1) != '\\' && *(dotPos - 1) != '/') {
        //replace
        ulong oldExtLen = pathLen - (dotPos - path + 1);
        char *newPath = malloc(sizeof *newPath * (pathLen + extLen - oldExtLen + 1));
        ulong i = 0;
        while (path != dotPos) {
            newPath[i] = *path;
            path++;
            i++;
        }
        newPath[i] = '.';
        i++;
        while (*ext) {
            newPath[i] = *ext;
            i++;
            ext++;
        }
        newPath[i] = 0;
        return newPath;
    } else {
        //add
        char* newPath = malloc(sizeof *newPath * (pathLen + extLen + 2));
        ulong i = 0;
        while (*path) {
            newPath[i] = *path;
            path++;
            i++;
        }
        newPath[i] = '.';
        i++;
        while (*ext) {
            newPath[i] = *ext;
            i++;
            ext++;
        }
        newPath[i] = 0;

        return newPath;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        err(-1, "You must specify a maximum");
    }
    ulong limit = strtoul(argv[1], NULL, 10);
    ulong threadCount = 1;
    if (argc > 2) {
        // Take next argument as the thread count
        threadCount = strtoul(argv[2], NULL, 10);
    }
    initProgressReporter(threadCount);

    FILE* file = fopen("primes.txt", "w");
    ulong maxPrimeCount = limit;
    ulong* primes = malloc(sizeof *primes * maxPrimeCount);
    printf("Counting primes, %zu worker threads...\n", threadCount);
    ulong primeCount = findPrimes(primes, maxPrimeCount, threadCount);
    //Trimming the array to save memory
    primes = realloc(primes, primeCount * sizeof *primes);
    printf("\nFound %zu prime numbers.\n", primeCount);
    for (ulong i = 0; i < primeCount; i++) {
        fprintf(file, "%zu\n", primes[i]);
    }
    fclose(file);

    printf("Factorizing, %zu worker threads...\n", threadCount);
    launchDecomposition(primes, primeCount, limit, "output.txt", threadCount);
    printf("\n");

    free(primes);
    shutdownProgressReporter();
    printf("\n");
    return 0;
}
