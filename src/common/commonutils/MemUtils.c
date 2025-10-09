// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

typedef struct PointerNode
{
    void* pointer;
    struct PointerNode* next;
} PointerNode;

static PointerNode* g_start = NULL;

/*
void *malloc(size_t size);
void free(void *ptr);
*/

void* SafeMalloc(size_t size, OsConfigLogHandle log)
{
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
        }
        else
        {
            OsConfigLogError(log, "SafeMalloc: failed to allocate tracking node");
            FREE_MEMORY(pointer);
        }
    }

    return pointer;
}

bool SafeFree(void** p, OsConfigLogHandle log)
{
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

    OsConfigLogError(log, "SafeFree: pointer '%p' not tracked or already freed", pointer);
    return false;
}

void SafeFreeAll(void)
{
    PointerNode* current = g_start;
    PointerNode* next = NULL;

    while (NULL != current)
    {
        next = current->next;

        FREE_MEMORY(current->pointer);
        FREE_MEMORY(current);

        current = next;
    }

    g_start = NULL;
}

size_t GetNumberOfUnfreedPointers(void)
{
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
}

void MemoryCleanup(OsConfigLogHandle log)
{
    size_t leaks = GetNumberOfUnfreedPointers();

    if (leaks > 0)
    {
        OsConfigLogError(log, "Memory leak detected: %zu unfreed pointers", leaks);
        SafeFreeAll();
    }

    return;
}
