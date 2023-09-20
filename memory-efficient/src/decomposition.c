#include "decomposition.h"
#include "progress.h"
#include "darray.h"
#include "primes.h"

#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <pthread.h>

static pthread_mutex_t fileMutex;

static ulong sqr(ulong num) { return num * num; }

static ulong indexOfPrime(const ulong *primes, ulong primeCount, ulong prime) {
    ulong left = 0, right = primeCount - 1;
    while(left <= right) {
        ulong mid = (right + left) / 2;
        if(mid >= primeCount)
            return -1;
        ulong midPrime = primes[mid];
        if (prime == midPrime) {
            return mid;
        } else if (prime < midPrime) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    return -1;
}

static ulong getNextPrime(FILE* primeList) {
    ulong buffer;
    ulong result = fread(&buffer, sizeof buffer, 1, primeList);
    if(result != 1) {
        if (feof(primeList)) {
            err(7, "No more primes, end of file !\n");
        } else {
            err(6, "Could not read prime list.\n");
        }
    }
    return buffer;
}

bool isPrime(FILE* primeListFile, ulong primeCount, ulong number) {
    bool isPrime = TRUE;
    ulong p;
    for (ulong j = 0; j < primeCount && sqr(p = getNextPrime(primeListFile)) <= number; j++) {
        if (number % p == 0) {
            isPrime = FALSE;
            break;
        }
    }
    return isPrime;
}

static void decomposeSingle(FILE* primeListFile, ulong** darrayFactorsP, ulong** darrayFactorCountsP, size_t primeCount, ulong number) {
    ulong p;
    ulong j = 0;
    ulong startingNumber = number;
    bool newFactor = 1;
    while (sqr(p = getNextPrime(primeListFile)) <= number) {
        if (number % p == 0) {
            number /= p;
            if (newFactor) {
                darrayAdd(darrayFactorsP, p);
                darrayAdd(darrayFactorCountsP, 1L);
                newFactor = 0;
            } else {
                (*darrayFactorCountsP)[darrayLength(*darrayFactorCountsP) - 1]++;
            }
        } else {
            j++;
            newFactor = 1;
        }
    }
    if (number > 1) {
        if (isPrime(primeListFile, primeCount, number)) {
            if (number != startingNumber) {
                darrayAdd(darrayFactorsP, number);
                darrayAdd(darrayFactorCountsP, 1L);
            }
        } else {
            // The remainder is not a prime number !
            // the while loop above did some shenanigans for us to be here
            err(17, "Decomposition ended with a non-prime number different from 1 : %zu\n", number);
        }
    }
}

static void writeFactorsToFile(const ulong *darrayFactors, const ulong* darrayFactorCounts, ulong number, FILE* file) {
    pthread_mutex_lock(&fileMutex);
    ulong factorCount = darrayLength(darrayFactors);
    if (factorCount != 0) {
        bool first = TRUE;
        fprintf(file, "%zu", number);
        for (ulong j = 0; j < factorCount; j++) {
            ulong p = darrayFactors[j];
            ulong c = darrayFactorCounts[j];
            if (c > 0) {
                if (first) {
                    fprintf(file, "%s", " = ");
                    first = FALSE;
                } else {
                    fprintf(file, "%s", " * ");
                }
                fprintf(file, "%zu", p);
                if (c > 1) {
                    fprintf(file, "^%zu", c);
                }
            }
            // fprintf(csvFile, ",%zu", c);
        }
        fprintf(file, "%s", "\n");
    }
    pthread_mutex_unlock(&fileMutex);
}

typedef struct {
    ulong firstNumber;
    ulong lastNumber;
    FILE* primeListFile;
    size_t primeCount;
    size_t tableSize;
    FILE* file;
    ulong threadId;
} DecompData;


static void* decompose(void *input) {
    DecompData* data = (DecompData*)input;
    //ulong factors[data->primeCount];
    ulong* darrayFactors = darrayCreate(4, sizeof(ulong));
    ulong* darrayFactorCounts = darrayCreate(4, sizeof(ulong));
    for (ulong i = data->firstNumber; i < data->lastNumber; i++) {
        darrayClear(darrayFactors);
        darrayClear(darrayFactorCounts);
        decomposeSingle(data->primeListFile, &darrayFactors, &darrayFactorCounts, data->primeCount, i);
        //Saving to file
        writeFactorsToFile(darrayFactors, darrayFactorCounts, i, data->file);
        registerProgress(data->threadId);
        fseek(data->primeListFile, 0, SEEK_SET);
    }
    darrayDestroy(darrayFactors);
    darrayDestroy(darrayFactorCounts);
    return NULL;
}

void launchDecomposition(const char* primeListPath, size_t primeCount, size_t tableSize, const char *filePath, size_t threadCount) {
    pthread_mutex_init(&fileMutex, NULL);
    startProgressReport(tableSize - 1);
    size_t perThread = tableSize / threadCount;
    size_t surplus = tableSize % threadCount;
    DecompData threadInputs[threadCount];
    pthread_t threads[threadCount];
    ulong previousLastNumber = 0;
    FILE* file = fopen(filePath, "w");
    for (size_t i = 0; i < threadCount; i++) {
        DecompData* input = threadInputs + i;
        input->firstNumber = previousLastNumber;
        input->lastNumber = previousLastNumber + perThread;
        if (surplus > 0) {
            input->lastNumber++;
            surplus--;
        }
        previousLastNumber = input->lastNumber;
        input->file = file;
        input->primeCount = primeCount;
        input->primeListFile = fopen(primeListPath, "rb");
        input->tableSize = tableSize;
        input->threadId = i;
        pthread_create(&threads[i], NULL, decompose, input);
    }

    for (size_t i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
        fclose(threadInputs[i].primeListFile);
    }
    fclose(file);
    pthread_mutex_destroy(&fileMutex);
    stopProgressReport();
}
