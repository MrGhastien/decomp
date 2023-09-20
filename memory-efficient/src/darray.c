#include "darray.h"
#include <stdlib.h>
#include <string.h>
#include <err.h>


void *darrayCreate(ulong capacity, ulong stride) {
    ulong headerSize = sizeof(ulong) * 3;
    ulong* alloc = malloc(headerSize + stride * capacity);
    //memset(alloc, 0, headerSize);
    alloc[DARRAY_CAPACITY_FIELD] = capacity;
    alloc[DARRAY_LENGTH_FIELD] = 0;
    alloc[DARRAY_STRIDE_FIELD] = stride;
    return (void*)alloc + headerSize;
}

void darrayDestroy(void *darray) {
    void* start = darray - sizeof(ulong) * 3;
    free(start);
}


ulong _darrayGetField(const void *darray, ulong field) {
    ulong* fieldPointer = (ulong*)darray - 3 + field;
    return *fieldPointer;
}

void darraySetField(void *darray, ulong field, ulong value) {
    ulong* header = (ulong*)darray - 3;
    header[field] = value;
}

void darrayClear(void *darray) {
    darraySetField(darray, DARRAY_LENGTH_FIELD, 0);
}

static void *resizeArray(void *darray) {
    ulong capacity = darrayCapacity(darray);
    ulong stride = darrayStride(darray);

    ulong headerSize = sizeof(ulong) * 3;
    void* header = darray - headerSize;

    capacity *= 1.5;
    header = realloc(header, headerSize + capacity * stride);
    darray = header + headerSize;
    darraySetField(darray, DARRAY_CAPACITY_FIELD, capacity);
    return darray;
}

void _darrayAdd(void **darrayp, const void *element) {
    void* darray = *darrayp;
    ulong capacity = darrayCapacity(darray);
    ulong length = darrayLength(darray);
    ulong stride = darrayStride(darray);

    if (length >= capacity) {
        //Resize array
        darray = resizeArray(darray);
        capacity = darrayCapacity(darray);
    }
    memcpy(darray + length * stride, element, stride);
    darraySetField(darray, DARRAY_LENGTH_FIELD, length + 1);
    *darrayp = darray;
}

void _darrayInsert(void **darrayp, const void *element, ulong index) {
    void* darray = *darrayp;
    ulong length = darrayLength(darray);
    if(index == length)
        return _darrayAdd(darrayp, element);
    ulong capacity = darrayCapacity(darray);
    ulong stride = darrayStride(darray);

    if (length >= capacity) {
        //Resize array
        darray = resizeArray(darray);
        capacity = darrayCapacity(darray);
    }
    memmove(darray + (index + 1) * stride, darray + index * stride, (length - index) * stride);
    memcpy(darray + index * stride, element, stride);
    darraySetField(darray, DARRAY_LENGTH_FIELD, length + 1);
    *darrayp = darray;
}
