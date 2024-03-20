#include "primes.h"
#include "progress.h"
#include "darray.h"
#include "prime-count.h"

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <err.h>

static ulong sqr(ulong num) { return num * num; }


static pthread_mutex_t tableMutex;
static sem_t* tableSemaphores;
static bool* finished;
static ulong* searchBuffer;
static ulong* threadProgress;
static sem_t* arrayAccessSemaphores;

typedef struct {
    ulong firstNumber;
    ulong lastNumber;
    ulong firstIndex;
    ulong lastIndex;
    ulong threadId;
    ulong threadCount;
} PrimeData;

static ulong getArrayLength(PrimeData* data) {
    ulong size = 0;
    for (size_t i = 0; i < data->threadId; i++) {
        size += threadProgress[i];
    }
    return size;
}

static ulong getPrimeNumber(PrimeData* data, ulong index) {
    ulong threadId = data->threadId;
    ulong i = 0;
    ulong length;
    if (data->threadId > 0) {
        //This is irrelevent in practice somehow, but for safety i leave it here
        while (searchBuffer[index] == 0) {
            //sem_wait(arrayAccessSemaphores + data->threadId - 1);
            usleep(5000000);
        }
    }
    return searchBuffer[index];
}

static bool hasPotentialFactor(PrimeData* data, ulong index, ulong number, ulong* outFactor) {
    *outFactor = getPrimeNumber(data, index);
    return sqr(*outFactor) <= number && *outFactor > 0; 
}

/** Computes the modular power of BASE to EXP modulo MOD.
 *  @param base The base of the exponentiation
 *  @param exp The exponent
 *  @param mod The modulo
 */

static ulong modpow(ulong base, ulong exp, ulong mod) {
    ulong result = 1;
    base = base % mod;
    while (exp > 0) {
        if(exp & 1)
            result = (result * base) % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    return result;
}

static bool fermatTest(PrimeData* data, ulong number) {
    ulong a = 2;
    return modpow(a, number - 1, number) == 1;
}

static bool isPrime(PrimeData* data, ulong number) {
    if(number == 2)
        return TRUE;
    if(!fermatTest(data, number))
      return FALSE;
    ulong p;
    for (ulong j = 0; hasPotentialFactor(data, j, number, &p); j++) {
        if (number % p == 0) {
            return FALSE;
        }
    }
    return TRUE;
}

static void *threadedFindPrimes(void *input) {
    PrimeData* data = input;
    for (ulong i = data->firstNumber; i < data->lastNumber; i++) {
        if (isPrime(data, i)) {
            //sem_wait(arrayAccessSemaphores + data->threadId);
            searchBuffer[threadProgress[data->threadId]++] = i;
            //sem_post(arrayAccessSemaphores + data->threadId);
        }
        //printBar(i, limit - 1, 30);
        registerProgress(data->threadId);
    }
    finished[data->threadId] = TRUE;
    pthread_exit(NULL);
}

static void createThreads(pthread_t* threadArray, PrimeData* threadInputs, ulong* searchBuffer, size_t threadCount, ulong searchLimit) {
    ulong previousLastNumber = 2;
    size_t iterCount = searchLimit - previousLastNumber;
    startProgressReport(iterCount);
    size_t perThread = iterCount / threadCount;
    size_t surplus = iterCount % threadCount;
    
    for (size_t i = 0; i < threadCount; i++) {
        sem_init(arrayAccessSemaphores + i, 0, 1);
        PrimeData* input = threadInputs + i;
        input->firstNumber = previousLastNumber;
        input->lastNumber = input->firstNumber + perThread;
        if (surplus > 0) {
            input->lastNumber++;
            surplus--;
        }
        input->firstIndex = approxPrimeCount(previousLastNumber - 1);
        threadProgress[i] = input->firstIndex;
        previousLastNumber = input->lastNumber;
        input->lastIndex = approxPrimeCount(input->lastNumber - 1);
        input->threadId = i;
        input->threadCount = threadCount;
        pthread_create(&threadArray[i], NULL, threadedFindPrimes, input);
    }

}

static void waitForThreads(pthread_t* threads, size_t threadCount) {
    for (size_t i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
    }
}

static void combineSearchResults(ulong** d_globalArray, ulong* searchBuffer, size_t threadCount, PrimeData* threadInputs) {
    for(size_t i = 0; i < threadCount; i++) {
        ulong length = threadProgress[i];
        for (size_t j = threadInputs[i].firstIndex; j < length; j++) {
            darrayAdd(d_globalArray, searchBuffer[j]);
        }
        sem_destroy(arrayAccessSemaphores + i);
    }
}

void findPrimes(ulong **darrayPrimesP, size_t limit, size_t threadCount) {
    pthread_mutex_init(&tableMutex, NULL);
    arrayAccessSemaphores = malloc(sizeof *arrayAccessSemaphores * threadCount);
    finished = calloc(threadCount, sizeof *finished);

    searchBuffer = malloc(sizeof *searchBuffer * approxPrimeCount(limit)); //Buffer containing all prime numbers
    //Each thread works in a different part of this array

    threadProgress = calloc(threadCount, sizeof *threadProgress);

    PrimeData threadInputs[threadCount];
    pthread_t threads[threadCount];

    createThreads(threads, threadInputs, searchBuffer, threadCount, limit);

    waitForThreads(threads, threadCount);

    combineSearchResults(darrayPrimesP, searchBuffer, threadCount, threadInputs);

    free(arrayAccessSemaphores);
    free(searchBuffer);
    pthread_mutex_destroy(&tableMutex);
    free(finished);
    stopProgressReport();
}
