#pragma once
#include "defines.h"

#define DARRAY_CAPACITY_FIELD 0
#define DARRAY_LENGTH_FIELD 1
#define DARRAY_STRIDE_FIELD 2

ulong _darrayGetField(const void* darray, ulong field);

void* darrayCreate(ulong capacity, ulong stride);

void darrayDestroy(void* darray);

void darrayClear(void* darray);

void _darrayAdd(void** darrayp, const void* element);
void darrayRemove(void* darray, ulong index);
void _darrayInsert(void** darrayp, const void* element, ulong index);

#define darrayAdd(darray, elem)               \
    {                                         \
        typeof(elem) holder = elem;                  \
        _darrayAdd((void**)darray, &holder);         \
    }

#define darrayInsert(darray, elem, index)           \
    {                                         \
        typeof(elem) holder = elem;                  \
        _darrayInsert((void**)darray, &holder, index);  \
    }

#define darraySet(darray, elem, index)  \
    {                                 \
        if (index >= darrayLength(darray)) \
            err(4, "Index out of bounds : index = %zu, length = %zu\n", index, length); \
        darray[index] = elem;        \
    }

#define darraySetOrAdd(darray, elem, index)                                             \
    {                                                                                   \
        ulong length = darrayLength(darray);                                            \
        if (index > length)                                                             \
            err(4, "Index out of bounds : index = %zu, length = %zu\n", index, length); \
        else if (index == length) {                                                     \
            typeof(elem) holder;                                                        \
            darray = _darrayAdd(darray, &holder);                                       \
        } else {                                                                        \
                darray[index] = elem;\
        }\
    }

#define darrayCapacity(darray) _darrayGetField(darray, DARRAY_CAPACITY_FIELD)
#define darrayLength(darray) _darrayGetField(darray, DARRAY_LENGTH_FIELD)
#define darrayStride(darray) _darrayGetField(darray, DARRAY_STRIDE_FIELD)
