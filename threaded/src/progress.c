#include "progress.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static ulong maxProgress = 0;
static ulong* iterations;
static ulong* progress;
static size_t s_threadCount;

static pthread_t reportThread;
static int threadStatus = 0;
static pthread_cond_t reportCondition;
static pthread_mutex_t reportMutex;

static void printBar(ulong val, ulong max, ulong length) {
    ulong filledPixels = max == 0 ? 0 : (val * length) / max;
    printf("[");
    ulong i = 0;
    for (; i < filledPixels; i++) {
        printf("#");
    }
    for (; i < length; i++) {
        printf(" ");
    }
    printf("] %zu / %zu %zu %%", val, max, max == 0 ? 0 : (val * 100) / max);
}

static ulong sumProgress() {
    ulong total = 0;
    for (size_t i = 0; i < s_threadCount; i++) {
        total += progress[i];
    }
    return total;
}

static ulong sumIterations() {
    ulong total = 0;
    for (size_t i = 0; i < s_threadCount; i++) {
        total += iterations[i];
    }
    return total;
}

static void clearArray(ulong *array, size_t size) {
    for (size_t i = 0; i < size; i++) {
        array[i] = 0;
    }
}

static void printProgress(bool lastPrint) {
    if(s_threadCount > 1) {
        ulong perThread = maxProgress / s_threadCount;
        ulong surplus = maxProgress % s_threadCount;
        for (size_t i = 0; i < s_threadCount; i++) {
          ulong max = perThread;
          if (surplus) {
            max++;
            surplus--;
          }
          printf("\e[2K");
          printBar(progress[i], max, 40);
          printf(" - %zu iter/s\n", iterations[i]);
        }
        printf("\e[1m");
        printBar(sumProgress(), maxProgress, 40);
        printf(" - %zu iter/s", sumIterations());
        printf("\e[22m");
        if (!lastPrint)
          printf("\r\e[%zuA", s_threadCount);
    } else {
        printf("\r");
        printBar(sumProgress(), maxProgress, 40);
        printf(" - %zu iter/s", sumIterations());
    }
    fflush(stdout);
}

void startProgressReport(ulong max) {
    maxProgress = max;
    clearArray(progress, s_threadCount);
    clearArray(iterations, s_threadCount);
    threadStatus = 1;
    pthread_cond_signal(&reportCondition);
}

void stopProgressReport() {
    threadStatus = 0;
    printProgress(TRUE);
}

void registerProgress(ulong threadId) {
    iterations[threadId]++;
    progress[threadId]++;
}

static void *reportProgress(void *ptr) {
    pthread_mutex_lock(&reportMutex);
    while(threadStatus != 2) {
        pthread_cond_wait(&reportCondition, &reportMutex);
        while (threadStatus == 1) {
            printProgress(FALSE);
            clearArray(iterations, s_threadCount);
            sleep(1);
        }
    }
    pthread_mutex_unlock(&reportMutex);
    pthread_exit(NULL);
}

void initProgressReporter(size_t threadCount) {
    s_threadCount = threadCount;
    pthread_mutex_init(&reportMutex, NULL);
    pthread_cond_init(&reportCondition, NULL);
    pthread_create(&reportThread, NULL, reportProgress, NULL);
    iterations = calloc(threadCount, sizeof *iterations);
    progress = calloc(threadCount, sizeof *progress);
}

void shutdownProgressReporter() {
    threadStatus = 2;
    pthread_cond_broadcast(&reportCondition);
    pthread_join(reportThread, NULL);
    pthread_mutex_destroy(&reportMutex);
    pthread_cond_destroy(&reportCondition);
    free(iterations);
    free(progress);
}
