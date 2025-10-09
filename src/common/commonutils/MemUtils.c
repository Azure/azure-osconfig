// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

typedef struct PointerNode
{
    void* pointer;
    struct PointerNode* next;
} PointerNode;

static PointerNode* g_start = NULL;

void* SafeMalloc(size_t size, OsConfigLogHandle log)
{
#ifndef TEST_CODE
    UNUSED(log);
    return malloc(size);
#else
    PointerNode* node = NULL;
    void* pointer = NULL;

    if (0 == size)
    {
        OsConfigLogError(log, "SafeMalloc: requested size is 0 bytes, nothing to allocate");
    }
    else if (size >= SIZE_MAX)
    {
        OsConfigLogError(log, "SafeMalloc: requested size %zu exceeds maximum allocatable size", size);
    }
    else if (NULL == (pointer = malloc(size)))
    {
        OsConfigLogError(log, "SafeMalloc: memory allocation of %zu bytes failed", size);
    }
    else
    {
        if (NULL != (node = malloc(sizeof(PointerNode))))
        {
            node->pointer = pointer;
            node->next = g_start;
            g_start = node;

            memset(pointer, 0, size);

            OsConfigLogInfo(log, "SafeMalloc: allocated pointer %p", pointer);
        }
        else
        {
            OsConfigLogError(log, "SafeMalloc: failed to allocate tracking node");
            FREE_MEMORY(pointer);
        }
    }

    return pointer;
#endif
}

bool SafeFree(void** p, OsConfigLogHandle log)
{
#ifndef TEST_CODE
    UNUSED(log);
    MEMORY_FREE(*p);
    return true;
#else
    if ((NULL == p) || (NULL == *p))
    {
        OsConfigLogError(log, "SafeFree: called with a NULL pointer argument");
        return false;
    }

    void* pointer = *p;
    PointerNode* current = g_start;
    PointerNode* previous = NULL;

    while (NULL != current)
    {
        if (current->pointer == pointer)
        {
            OsConfigLogInfo(log, "SafeFree: freeing pointer %p", pointer);

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

    OsConfigLogError(log, "SafeFree: pointer %p not tracked or already freed", pointer);
    return false;
#endif
}

void SafeFreeAll(OsConfigLogHandle log)
{
#ifndef TEST_CODE
    UNUSED(log);
#else
    PointerNode* current = g_start;
    PointerNode* next = NULL;

    while (NULL != current)
    {
        OsConfigLogInfo(log, "SafeFreeAll: freeing pointer %p", current);
        next = current->next;
        FREE_MEMORY(current->pointer);
        FREE_MEMORY(current);
        current = next;
    }

    g_start = NULL;
#endif
}

size_t GetNumberOfUnfreedPointers(void)
{
#ifndef TEST_CODE
    return 0;
#else
    size_t count = 0;
    PointerNode* current = g_start;

    while (NULL != current)
    {
        if (NULL != current->pointer)
        {
            count += 1;
        }

        current = current->next;
    }

    return count;
#endif
}

static void DumpTrackedPointers(OsConfigLogHandle log)
{
#ifndef TEST_CODE
    UNUSED(log);
#else
    PointerNode* current = g_start;
    size_t leaks = GetNumberOfUnfreedPointers();
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
#endif
}

void MemoryCleanup(OsConfigLogHandle log)
{
#ifndef TEST_CODE
    UNUSED(log);
#else
    size_t leaks = GetNumberOfUnfreedPointers();
    if (leaks > 0)
    {
        OsConfigLogError(log, "Memory leak detected: %zu unfreed pointers", leaks);
        DumpTrackedPointers(log);
        SafeFreeAll(log);
    }
#endif
}
