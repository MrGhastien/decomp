#include "decomposition.h"
#include "progress.h"

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

static ulong decomposeSingle(const ulong *primes, ulong *factors, size_t primeCount, ulong number) {
    ulong p;
    ulong j = 0;
    ulong greatestFactorIndex = 0;
    factors[0] = 0;
    while (sqr(p = primes[j]) <= number) {
        if (number % p == 0) {
            number /= p;
            factors[j]++;
            greatestFactorIndex = j;
        } else {
            j++;
            factors[j] = 0;
        }
    }
    if (number > 1) {
        ulong index = indexOfPrime(primes, primeCount, number);
        if (index != -1) {
            for (size_t i = j + 1; i < index; i++) {
                factors[i] = 0;
            }
            factors[index] = 1;
            greatestFactorIndex = index;
        } else {
            err(17, "Decomposition ended with a non-prime number different from 1 : %zu\n", number);
        }
    }
    return greatestFactorIndex;
}

static void writeFactorsToFile(const ulong *primes, const ulong *factors, size_t primeCount, ulong number, ulong maxFactorIndex, FILE* file) {
    pthread_mutex_lock(&fileMutex);
    bool first = TRUE;
    fprintf(file, "%zu", number);
    for (ulong j = 0; j <= maxFactorIndex; j++) {
        ulong p = primes[j];
        ulong c = factors[j];
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
    pthread_mutex_unlock(&fileMutex);
}

typedef struct {
    ulong firstNumber;
    ulong lastNumber;
    const ulong* primes;
    size_t primeCount;
    size_t tableSize;
    FILE* file;
    ulong threadId;
} DecompData;

static void* decompose(void *input) {
    DecompData* data = (DecompData*)input;
    //ulong factors[data->primeCount];
    ulong* factors = malloc(sizeof *factors * data->primeCount);
    for (ulong i = data->firstNumber; i < data->lastNumber; i++) {
        //clearArray(factors, primeCount);
        ulong maxFactorIndex = decomposeSingle(data->primes, factors, data->primeCount, i);
        //Saving to file
        writeFactorsToFile(data->primes, factors, data->primeCount, i, maxFactorIndex, data->file);
        registerProgress(data->threadId);
    }
    free(factors);
    return NULL;
}

void launchDecomposition(const ulong *primes, size_t primeCount, size_t tableSize, const char *filePath, size_t threadCount) {
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
        input->primes = primes;
        input->tableSize = tableSize;
        input->threadId = i;
        pthread_create(&threads[i], NULL, decompose, input);
    }

    for (size_t i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
    }
    fclose(file);
    pthread_mutex_destroy(&fileMutex);
    stopProgressReport();
}
