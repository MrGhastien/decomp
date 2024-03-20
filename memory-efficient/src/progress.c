#include "progress.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define REFRESH_INTERVAL 500

typedef enum { STATUS_WAIT = 0, STATUS_RUN, STATUS_EXIT } ThreadStatus;

static ulong maxProgress = 0;
static ulong* iterations;
static ulong* progress;
static size_t s_threadCount;
static bool initialized = FALSE;

static pthread_t reportThread;
static ThreadStatus threadStatus = STATUS_WAIT;
static sem_t reportSemaphore;

static void printBar(ulong val, ulong max, ulong length) {
    ulong filledPixels = max == 0 ? 0 : (val * length) / max;
    printf("[");
    ulong i = 0;
    for (; i < filledPixels; i++) {
        printf("\e[32m#");
    }
    printf("\e[0m");
    for (; i < length; i++) {
        printf("_");
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

static ulong computeSpeed(ulong iterations) {
    return (ulong)(iterations / (REFRESH_INTERVAL / 1000.0));
}

static void printProgress(bool firstPrint) {
    ulong totalSpeed = computeSpeed(sumIterations());
    ulong totalProgress = sumProgress();
    if(s_threadCount > 1) {
        if (!firstPrint)
            printf("\r\e[%zuA", s_threadCount);

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
            printf(" - %zu iter/s\n", computeSpeed(iterations[i]));
        }
        printf("\e[1m");
        printBar(totalProgress, maxProgress, 40);
        printf(" - %zu iter/s", totalSpeed);
        printf("\e[22m");
    } else {
        if(!firstPrint)
            printf("\r");
        printBar(totalProgress, maxProgress, 40);
        printf(" - %zu iter/s", totalSpeed);
    }
    if (totalSpeed > 0) {
        ulong seconds = (maxProgress - totalProgress) / totalSpeed;
        if(seconds / 60 > 999)
            printf("%s", " (ETA: --:--)");
        printf(" (ETA: %03zu:%02zu)", seconds / 60, seconds % 60);
    } else {
        printf("%s", " (ETA: --:--)");
    }
    printf("%s", "                    ");
    fflush(stdout);
}

void startProgressReport(ulong max) {
    if(!initialized)
        return;
    maxProgress = max;
    clearArray(progress, s_threadCount);
    clearArray(iterations, s_threadCount);
    threadStatus = STATUS_RUN;
    printProgress(TRUE);
    sem_post(&reportSemaphore);
}

void stopProgressReport() {
    if(!initialized)
        return;
    threadStatus = STATUS_WAIT;
    printProgress(FALSE);
}

void registerProgress(ulong threadId) {
    if(!initialized)
        return;
    iterations[threadId]++;
    progress[threadId]++;
}

static void *reportProgress(void *ptr) {
    while(threadStatus != STATUS_EXIT) {
        //I use semaphores here instead of condition viariables because
        //most of the time, the condition is signaled before this thread
        //starts waiting for it to be signaled, Thus the progress is not shown at all.
        sem_wait(&reportSemaphore);
        while (threadStatus == STATUS_RUN) {
            printProgress(FALSE);
            clearArray(iterations, s_threadCount);
            usleep(REFRESH_INTERVAL * 1000);
        }
    }
    pthread_exit(NULL);
}

void initProgressReporter(size_t threadCount) {
    if(initialized)
        return;
    initialized = TRUE;
    s_threadCount = threadCount;
    threadStatus = STATUS_WAIT;
    sem_init(&reportSemaphore, 0, 0);
    iterations = calloc(threadCount, sizeof *iterations);
    progress = calloc(threadCount, sizeof *progress);
    pthread_create(&reportThread, NULL, reportProgress, NULL);
}

void shutdownProgressReporter() {
    if(!initialized)
        return;
    threadStatus = STATUS_EXIT;
    sem_post(&reportSemaphore);
    pthread_join(reportThread, NULL);
    sem_destroy(&reportSemaphore);
    free(iterations);
    free(progress);
}
