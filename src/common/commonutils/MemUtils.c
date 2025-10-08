// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

typedef struct OsConfigPointerNode
{
    void* pointer;
    struct OsConfigPointerNode* next;
} OsConfigPointerNode;

static OsConfigPointerNode* g_head = NULL;

void* SafeMalloc(size_t size, OsConfigLogHandle log)
{
    OsConfigPointerNode* node = NULL;
    void* pointer = NULL;
    uintptr_t address = 0;

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
    else if (0 != ((address = (uintptr_t)pointer) % 8))
    {
        OsConfigLogError(log, "SafeMalloc: pointer '%p' is not aligned to 8 bytes", pointer);
        FREE_MEMORY(pointer);
    }
    else
    {
        if (NULL != (node = malloc(sizeof(OsConfigPointerNode))))
        {
            node->pointer = pointer;
            node->next = g_head;
            g_head = node;

            memset(pointer, 0, size);

            OsConfigLogInfo(log, "SafeMalloc: pointer '%p' allocated", pointer);
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
    OsConfigPointerNode* current = g_head;

    while (current)
    {
        if (current->pointer == pointer)
        {
            if (NULL != previous)
            {
                previous->next = current->next;
            }
            else
            {
                g_head = current->next;
            }

            FREE_MEMORY(current->pointer);
            FREE_MEMORY(pointer);
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
    OsConfigPointerNode* current = g_head;
    OsConfigPointerNode* next = NULL;

    while (current)
    {
        next = current->next;
        FREE_MEMORY(current->pointer);
        FREE_MEMORY(current);
        current = next;
    }

    g_head = NULL;
}

size_t GetNumberOfUnfreedPointers(void)
{
    size_t count = 0;
    OsConfigPointerNode* current = g_head;

    while (current)
    {
        if (NULL != current->pointer)
        {
            ++count;
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

void DumpTrackedPointers(OsConfigLogHandle log)
{
    OsConfigPointerNode* current = g_head;
    size_t index = 0;

    while (current)
    {
        OsConfigLogError(log, "Node[%zu]: pointer = %p", index++, current->pointer);
        current = current->next;
    }
}
