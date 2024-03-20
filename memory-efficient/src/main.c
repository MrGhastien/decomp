#include "progress.h"
#include "primes.h"
#include "decomposition.h"
#include "darray.h"
#include "test.h"

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

ulong streq(const char* a, const char* b) {
    char ca = *a, cb = *b;
    while (ca && cb) {
        if(ca != cb)
            return FALSE;
        a++;
        b++;
        ca = *a;
        cb = *b;
    }
    //Check the null character too,
    //because one string might be longer than the other.
    return ca == cb; 
}

char *replaceExt(const char* path, const char* ext) {
    ulong extLen = strlen(ext);
    ulong pathLen = strlen(path);
    const char* end = path + pathLen;
    const char* dotPos = end;
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


    const char* primeLiteralPath = "primes.txt";
    const char* primeBinaryPath = "primes.bin";
    if(streq(argv[1], "test")) {
        return performTests(argv[0]);
    }
    if(streq(argv[1], "-s")) {
        FILE* binaryFile = fopen(primeBinaryPath, "rb");
        if(!binaryFile) {
            perror("Could not open prime number cache");
            return 4;
        }
        DecompData data = {
            .firstNumber = strtoul(argv[2], NULL, 10),
            .lastNumber = data.firstNumber + 1,
            .outputFile = stdout,
            .primeListFile = binaryFile,
            .tableSize = 1,
            .threadId = 0,
        };
        fread(&data.primeCount, sizeof data.primeCount, 1, binaryFile);
        decompose(&data);
        fclose(binaryFile);
        return 0;
    }
    ulong limit = strtoul(argv[1], NULL, 10);
    ulong threadCount = 1;
    if (argc > 2) {
        // Take next argument as the thread count
        threadCount = strtoul(argv[2], NULL, 10);
    }
    initProgressReporter(threadCount);

    printf("Counting primes, %zu worker threads...\n", threadCount);
    ulong* darrayPrimes = darrayCreate(64, sizeof(ulong));
    findPrimes(&darrayPrimes, limit, threadCount);

    ulong primeCount = darrayLength(darrayPrimes);

    printf("\nFound %zu prime numbers.\n", primeCount);
    FILE* literalFile = fopen(primeLiteralPath, "w");
    FILE* binaryFile = fopen(primeBinaryPath, "wb");
    for (ulong i = 0; i < primeCount; i++) {
        fprintf(literalFile, "%zu\n", darrayPrimes[i]);
    }
    fwrite(&primeCount, sizeof primeCount, 1, binaryFile);
    fwrite(darrayPrimes, sizeof(*darrayPrimes), primeCount, binaryFile);
    fclose(literalFile);
    fclose(binaryFile); //We'll reopen this file for each thread during decomposition
    darrayDestroy(darrayPrimes);

    printf("Factorizing, %zu worker threads...\n", threadCount);
    launchDecomposition("./primes.bin", primeCount, limit, "output.txt", threadCount);
    printf("\n");

    shutdownProgressReporter();
    printf("\n");
    return 0;
}
