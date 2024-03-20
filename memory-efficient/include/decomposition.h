#pragma once
#include "defines.h"

#include <stdio.h>

typedef struct {
    ulong firstNumber;
    ulong lastNumber;
    FILE* primeListFile;
    size_t primeCount;
    size_t tableSize;
    FILE* outputFile;
    ulong threadId;
} DecompData;

void launchDecomposition(const char* primeListPath, size_t primeCount, size_t tableSize, const char *filePath, size_t threadCount);

void* decompose(void* input);
