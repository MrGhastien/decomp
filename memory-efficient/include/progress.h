#pragma once

#include "defines.h"

#include <stdio.h>

void startProgressReport(ulong max);

void stopProgressReport();

void registerProgress(ulong threadId);

void initProgressReporter(size_t threadCount);
void shutdownProgressReporter();
