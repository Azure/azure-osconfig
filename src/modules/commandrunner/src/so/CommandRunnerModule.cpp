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

std::map<std::string, std::shared_ptr<CommandRunner>> instances;

void __attribute__((constructor)) InitModule()
{
    CommandRunnerLog::OpenLog();
    OsConfigLogInfo(CommandRunnerLog::Get(), "CommandRunner module loaded");
}

void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(CommandRunnerLog::Get(), "CommandRunner module unloaded");
    for (auto it = instances.begin(); it != instances.end(); ++it)
    {
        it->second.reset();
    }
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

int PersistCacheToDisk()
{
    int status = 0;
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

    writer.StartArray();

    for (auto it = instances.begin(); it != instances.end(); ++it)
    {
        Command::Status commandStatus = it->second->GetStatusToPersist();

        writer.StartObject();

        writer.Key(g_clientName.c_str());
        writer.String(it->first.c_str());

        writer.Key(g_commandStatusValues.c_str());
        writer.StartArray();

        Command::Status::Serialize(writer, commandStatus, false);

        writer.EndArray();
        writer.EndObject();
    }

    writer.EndArray();

    if (sb.GetSize() > 0)
    {
        std::FILE* file = std::fopen(CACHEFILE, "w");
        if (nullptr == file)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Unable to open persisted cache: %s", CACHEFILE);
            status = EACCES;
        }
        else
        {
            int rc = std::fputs(sb.GetString(), file);

            if ((0 > rc) || (EOF == rc))
            {
                status = errno ? errno : EINVAL;
                OsConfigLogError(CommandRunnerLog::Get(), "Unable to save last command results to %s, error: %d %s", CACHEFILE, status, errno ? strerror(errno) : "-");
            }

            fflush(file);
            std::fclose(file);
        }
    }

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
        try
        {
            if (instances.find(clientName) == instances.end())
            {
                std::shared_ptr<CommandRunner> session = std::make_shared<CommandRunner>(clientName, maxPayloadSizeBytes, PersistCacheToDisk);
                instances[clientName] = session;
                handle = reinterpret_cast<MMI_HANDLE>(session.get());
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiOpen(%s, %d) failed, client already exists", clientName, maxPayloadSizeBytes);
                status = EINVAL;
            }
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "%s", e.what());
            status = EINVAL;
        }
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
    CommandRunner* session = reinterpret_cast<CommandRunner*>(clientSession);
    std::string clientName = session->GetClientName();

    if (instances.find(clientName) != instances.end())
    {
        instances[clientName].reset();
        instances.erase(clientName);
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