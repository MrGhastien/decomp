#include "primes.h"
#include "progress.h"

#include <pthread.h>

static ulong sqr(ulong num) { return num * num; }

static ulong isqrt(ulong num) {
    ulong x = num >> 1;
    for (uint i = 0; i < ISQRT_STEPS; i++) {
        x = (x + (num / x)) >> 1;
    }
    return x;
}

typedef struct {
    ulong firstNumber;
    ulong lastNumber;
    ulong outPrimeCount;
    ulong* primes;
    ulong threadId;
} PrimeData;

static void *threadedFindPrimes(void *input) {
    PrimeData* data = input;
    ulong index = 0;
    for (ulong i = data->firstNumber; i < data->lastNumber; i++) {
        bool isPrime = TRUE;
        for (ulong j = 0; j < index && sqr(data->primes[j]) <= i; j++) {
            if (i % data->primes[j] == 0) {
                isPrime = FALSE;
                break;
            }
        }
        if (isPrime) {
            data->primes[index] = i;
            index++;
        }
        //printBar(i, limit - 1, 30);
        registerProgress(data->threadId);
    }
    data->outPrimeCount = index;
}

ulong findPrimes(ulong *primes, size_t limit, size_t threadCount) {
    size_t iterCount = limit - 2;
    startProgressReport(iterCount);
    size_t perThread = iterCount / threadCount;
    size_t surplus = iterCount % threadCount;
    PrimeData threadInputs[threadCount];
    pthread_t threads[threadCount];
    ulong previousLastNumber = 2;
    ulong* resultTables[threadCount];
    for (size_t i = 0; i < threadCount; i++) {
        resultTables[i] = calloc(limit / threadCount, sizeof **resultTables);
    }
    for (size_t i = 0; i < threadCount; i++) {
        PrimeData* input = threadInputs + i;
        input->firstNumber = previousLastNumber;
        input->lastNumber = previousLastNumber + perThread;
        if (surplus > 0) {
            input->lastNumber++;
            surplus--;
        }
        previousLastNumber = input->lastNumber;
        input->primes = resultTables[i];
        input->threadId = i;
        pthread_create(&threads[i], NULL, threadedFindPrimes, input);
    }

    for (size_t i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
    }
    stopProgressReport();
    ulong primeCount = 0;
    for (size_t i = 0; i < threadCount; i++) {
        for(size_t j = 0; j < threadInputs[i].outPrimeCount; j++) {
            primes[j + primeCount] = threadInputs[i].primes[j];
        }
        free(resultTables[i]);
        primeCount += threadInputs[i].outPrimeCount;
    }
    return primeCount;
}
