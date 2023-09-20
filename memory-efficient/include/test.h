#pragma once
#include "defines.h"

#define ASSERT(cond)                                     \
    {                                                         \
        if (cond) {                                           \
        } else {                                              \
            reportAssertFail(#cond, NULL, __FILE__, __LINE__); \
        }                                                     \
    }

#define ASSERT_MSG(cond, msg)                                     \
    {                                                         \
        if (cond) {                                           \
        } else {                                              \
            reportAssertFail(#cond, msg, __FILE__, __LINE__); \
        }                                                     \
    }

void reportAssertFail(const char* expression, const char* message, const char* filename, int line);

typedef void (*test_function)(void);

int performTests(const char* programName);
