#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <pthread.h>

#define TRUE 1
#define FALSE 0
#define ISQRT_STEPS 1000


typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned char bool;

static ulong iterCount = 0;
static ulong progress = 0;
static ulong maxProgress = 0;

static pthread_t reportThread;
static int threadStatus = 0;
static pthread_cond_t reportCondition;
static pthread_mutex_t reportMutex;

void printBar(ulong val, ulong max, ulong length) {
    ulong filledPixels = max == 0 ? 0 : (val * length) / max;
    printf("\r[");
    ulong i = 0;
    for (; i < filledPixels; i++) {
        printf("#");
    }
    for (; i < length; i++) {
        printf(" ");
    }
    printf("] %zu / %zu %zu %%", val, max, max == 0 ? 0 : (val * 100) / max);
}

ulong sqr(ulong num) { return num * num; }

ulong isqrt(ulong num) {
    ulong x = num >> 1;
    for (uint i = 0; i < ISQRT_STEPS; i++) {
        x = (x + (num / x)) >> 1;
    }
    return x;
}

void printProgress() {
    printBar(progress, maxProgress, 40);
    printf(" - %zu iter/s", iterCount);
    fflush(stdout);
}

void startProgressReport(ulong max) {
    maxProgress = max;
    progress = 0;
    threadStatus = 1;
    pthread_cond_signal(&reportCondition);
}

void stopProgressReport() {
    threadStatus = 0;
    printProgress();
}

void registerProgress() {
    iterCount++;
    progress++;
}

ulong findPrimes(ulong *primes, size_t limit) {
    ulong index = 0;
    startProgressReport(limit - 2);
    for (ulong i = 2; i < limit; i++) {
        bool isPrime = TRUE;
        ulong maxPrime = isqrt(i);
        for (ulong j = 2; j <= maxPrime; j++) {
            if (i % j == 0) {
                isPrime = FALSE;
                break;
            }
        }
        if (isPrime) {
            primes[index] = i;
            index++;
        }
        //printBar(i, limit - 1, 30);
        registerProgress();
    }
    stopProgressReport();
    return index;
}

ulong indexOfPrime(ulong *primes, ulong primeCount, ulong prime) {
    ulong left = 0, right = primeCount - 1;
    while(left <= right) {
        ulong mid = (right + left) / 2;
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

void clearArray(ulong* array, size_t size) {
    for (size_t i = 0; i < size; i++) {
        array[i] = 0;
    }
}

void decomposeNumber(ulong *primes, ulong *factors, size_t primeCount, ulong number) {
    ulong p;
    ulong j = 0;
    while (sqr(p = primes[j]) <= number) {
        if (number % p == 0) {
            number /= p;
            factors[j]++;
        } else {
            j++;
        }
    }
    if (number > 1) {
        ulong index = indexOfPrime(primes, primeCount, number);
        if (index != -1) {
            factors[index]++;
        }
    }
}

void writeFactorsToFile(ulong *primes, ulong *factors, size_t primeCount, ulong number, FILE* file) {
    bool first = TRUE;
    fprintf(file, "%zu", number);
    for (ulong j = 0; j < primeCount; j++) {
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
}

void decomp(ulong *primes, size_t primeCount, size_t tableSize, const char* path) {
    FILE* file = fopen(path, "w");
    startProgressReport(tableSize - 1);
    ulong factors[primeCount];
    for (ulong i = 0; i < tableSize; i++) {
        clearArray(factors, primeCount);
        decomposeNumber(primes, factors, primeCount, i);
        //Saving to file
        writeFactorsToFile(primes, factors, primeCount, i, file);
        registerProgress();
    }
    stopProgressReport();
    fclose(file);
}


void *reportProgress(void *ptr) {
    pthread_mutex_lock(&reportMutex);
    while(threadStatus != 2) {
        pthread_cond_wait(&reportCondition, &reportMutex);
        while (threadStatus == 1) {
            printProgress();
            iterCount = 0;
            sleep(1);
        }
    }
    pthread_mutex_unlock(&reportMutex);
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    pthread_mutex_init(&reportMutex, NULL);
    pthread_cond_init(&reportCondition, NULL);
    pthread_create(&reportThread, NULL, reportProgress, NULL);

    if (argc < 2) {
        err(-1, "You must specify a maximum");
    }
    ulong limit = strtoul(argv[1], NULL, 10);
    FILE* file = fopen("primes.txt", "w");
    ulong maxPrimeCount = limit;
    ulong* primes = malloc(sizeof *primes * maxPrimeCount);
    printf("%s\n", "Computing primes...");
    ulong primeCount = findPrimes(primes, maxPrimeCount);
    //Trimming the array to save memory
    primes = realloc(primes, primeCount * sizeof *primes);
    printf("\nFound %zu prime numbers.\n", primeCount);
    for (ulong i = 0; i < primeCount; i++) {
        fprintf(file, "%zu\n", primes[i]);
    }
    fclose(file);

    printf("Factorizing...\n");
    decomp(primes, primeCount, limit, "output.txt");
    printf("\n");

    free(primes);
    threadStatus = 2;
    pthread_cond_broadcast(&reportCondition);
    pthread_join(reportThread, NULL);
    pthread_mutex_destroy(&reportMutex);
    pthread_cond_destroy(&reportCondition);
    printf("\n");
    return 0;
}
