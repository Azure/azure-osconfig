// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

typedef struct OsConfigPointerNode
{
    void* pointer;
    struct OsConfigPointerNode* next;
} OsConfigPointerNode;

static OsConfigPointerNode* g_head = NULL;
static uintptr_t g_minPointer = UINTPTR_MAX;
static uintptr_t g_maxPointer = 0;

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
        if (address < g_minPointer)
        {
            g_minPointer = address;
        }

        if ((address + size) > g_maxPointer)
        {
            g_maxPointer = address + size;
        }

        if (NULL != (node = malloc(sizeof(OsConfigPointerNode))))
        {
            node->pointer = pointer;
            node->next = g_head;
            g_head = node;

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
    void* pointer = *p;
    uintptr_t addrress = (uintptr_t)pointer;
    bool result = false;

    if ((NULL == p) || (NULL == *p))
    {
        OsConfigLogError(log, "SafeFree: called with a NULL pointer argument");
    }
    else if ((addrress < g_minPointer) || (addrress > g_maxPointer))
    {
        OsConfigLogError(log, "SafeFree: pointer '%p' is outside valid allocation range", pointer);
    }
    else
    {
        OsConfigPointerNode* previous = NULL;
        OsConfigPointerNode* current = g_head;

        while (current)
        {
            if (current->pointer == pointer)
            {
                if (previous)
                {
                    previous->next = current->next;
                }
                else
                {
                   g_head = current->next;
                }

                FREE_MEMORY(current);
                FREE_MEMORY(p);

                result = true;
                break;
            }

            previous = current;
            current = current->next;
        }
    }

    if (false == result)
    {
        OsConfigLogError(log, "SafeFree: pointer '%p' not tracked or already freed", pointer);
    }

    return result;
}
