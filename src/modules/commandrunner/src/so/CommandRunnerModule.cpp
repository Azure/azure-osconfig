// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <Command.h>
#include <CommandRunner.h>
#include <ScopeGuard.h>
#include <Mmi.h>

void __attribute__((constructor)) InitModule()
{
    CommandRunnerLog::OpenLog();
    OsConfigLogInfo(CommandRunnerLog::Get(), "CommandRunner module loaded");
}

void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(CommandRunnerLog::Get(), "CommandRunner module unloaded");
    CommandRunnerLog::CloseLog();
}

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(CommandRunnerLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(CommandRunnerLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
    }};

    // Get the static information from the CommandRunner module
    status = CommandRunner::GetInfo(clientName, payload, payloadSizeBytes);

    return status;
}

MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;
    MMI_HANDLE handle = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(CommandRunnerLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
    }};

    if (nullptr != clientName)
    {
        CommandRunner* commandRunner = new (std::nothrow) CommandRunner(clientName, maxPayloadSizeBytes);
        if (nullptr != commandRunner)
        {
            handle = reinterpret_cast<MMI_HANDLE>(commandRunner);
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "MmiOpen failed to allocate memory");
            status = ENOMEM;        }
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiOpen called with null clientName");
        status = EINVAL;
    }

    return handle;
}

void MmiClose(MMI_HANDLE clientSession)
{
    CommandRunner* commandRunner = reinterpret_cast<CommandRunner*>(clientSession);
    if (nullptr != commandRunner)
    {
        delete commandRunner;
    }
}

int MmiSet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    int status = MMI_OK;
    CommandRunner* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(CommandRunnerLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiSet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiSet called with null clientSession");
        status = EINVAL;
    }
    else
    {
        session = reinterpret_cast<CommandRunner*>(clientSession);
        status = session->Set(componentName, objectName, payload, payloadSizeBytes);
    }

    return status;
}

int MmiGet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;
    CommandRunner* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(CommandRunnerLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiGet called with null clientSession");
        status = EINVAL;
    }
    else
    {
        session = reinterpret_cast<CommandRunner*>(clientSession);
        status = session->Get(componentName, objectName, payload, payloadSizeBytes);
    }

    return status;
}

void MmiFree(MMI_JSON_STRING payload)
{
    if (nullptr != payload)
    {
        delete[] payload;
    }
}
