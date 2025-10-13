// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

typedef struct TrackedPointerNode
{
    void* pointer;
    struct TrackedPointerNode* next;
} TrackedPointerNode;

static TrackedPointerNode* g_start = NULL;

void TrackedPointerInitialize(void)
{
    g_start = NULL;
}

void* TrackedPointerAlloc(size_t size, OsConfigLogHandle log)
{
    TrackedPointerNode* node = NULL;
    void* pointer = NULL;

    if (0 == size)
    {
        OsConfigLogError(log, "TrackedPointerAlloc: requested size is 0 bytes, nothing to allocate");
    }
    else if (size >= SIZE_MAX)
    {
        OsConfigLogError(log, "TrackedPointerAlloc: requested size %zu exceeds maximum allocatable size", size);
    }
    else if (NULL == (pointer = malloc(size)))
    {
        OsConfigLogError(log, "TrackedPointerAlloc: memory allocation of %zu bytes failed", size);
    }
    else
    {
        if (NULL != (node = malloc(sizeof(TrackedPointerNode))))
        {
            node->pointer = pointer;
            node->next = g_start;
            g_start = node;

            memset(pointer, 0, size);

            OsConfigLogInfo(log, "TrackedPointerAlloc: allocated pointer %p", pointer);
        }
        else
        {
            OsConfigLogError(log, "TrackedPointerAlloc: failed to allocate tracking node");
            FREE_MEMORY(pointer);
        }
    }

    return pointer;
}

bool TrackedPointerFree(void** p, OsConfigLogHandle log)
{
    if ((NULL == p) || (NULL == *p))
    {
        OsConfigLogError(log, "TrackedPointerFree: called with a NULL pointer argument");
        return false;
    }

    void* pointer = *p;
    TrackedPointerNode* current = g_start;
    TrackedPointerNode* previous = NULL;

    while (NULL != current)
    {
        if (current->pointer == pointer)
        {
            OsConfigLogInfo(log, "TrackedPointerFree: freeing pointer %p", pointer);

            if (NULL != previous)
            {
                previous->next = current->next;
            }
            else
            {
                g_start = current->next;
            }

            FREE_MEMORY(current->pointer);
            FREE_MEMORY(current);
            *p = NULL;
            return true;
        }

        previous = current;
        current = current->next;
    }

    OsConfigLogError(log, "TrackedPointerFree: pointer %p not tracked or already freed", pointer);
    return false;
}

void TrackedPointersFreeAll(OsConfigLogHandle log)
{
    TrackedPointerNode* current = g_start;
    TrackedPointerNode* next = NULL;

    while (NULL != current)
    {
        OsConfigLogInfo(log, "TrackedPointersFreeAll: freeing pointer %p", current);
        next = current->next;
        FREE_MEMORY(current->pointer);
        FREE_MEMORY(current);
        current = next;
    }

    g_start = NULL;
}

size_t GetNumberOfTrackedPointers(void)
{
    size_t count = 0;
    TrackedPointerNode* current = g_start;

    while (NULL != current)
    {
        if (NULL != current->pointer)
        {
            count += 1;
        }

        current = current->next;
    }

    return count;
}

static void DumpTrackedPointers(OsConfigLogHandle log)
{
    TrackedPointerNode* current = g_start;
    size_t leaks = GetNumberOfTrackedPointers();
    size_t index = 0;

    if (0 == leaks)
    {
        return;
    }

    while (NULL != current)
    {
        OsConfigLogError(log, "DumpTrackedPointers: node[%zu]: pointer %p", index, current->pointer);
        index += 1;
        if (index >= leaks)
        {
            break;
        }
        current = current->next;
    }
}

void TrackedPointersCleanup(OsConfigLogHandle log)
{
    size_t leaks = GetNumberOfTrackedPointers();
    if (leaks > 0)
    {
        OsConfigLogError(log, "Memory leak detected: %zu unfreed tracked pointers", leaks);
        DumpTrackedPointers(log);
        //TrackedPointersFreeAll(log);
    }

    g_start = NULL;
}
