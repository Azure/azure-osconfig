// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ComplianceInterface.h"

#include "CommonUtils.h"
#include "Engine.h"
#include "Logging.h"
#include "Mmi.h"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <exception>

using compliance::Engine;

static void* g_log = nullptr;

void ComplianceInitialize(void* log)
{
    UNUSED(log);
    g_log = log;
}

void ComplianceShutdown(void)
{
}

MMI_HANDLE ComplianceMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    auto* result = reinterpret_cast<void*>(new Engine(g_log));
    OsConfigLogInfo(g_log, "ComplianceMmiOpen(%s, %u) returning %p", clientName, maxPayloadSizeBytes, result);
    return result;
}

void ComplianceMmiClose(MMI_HANDLE clientSession)
{
    delete reinterpret_cast<Engine*>(clientSession);
}

int ComplianceMmiGetInfo(const char* clientName, char** payload, int* payloadSizeBytes)
{
    if ((nullptr == payload) || (nullptr == payloadSizeBytes))
    {
        OsConfigLogError(g_log, "ComplianceMmiGetInfo(%s, %p, %p) called with invalid arguments", clientName, payload, payloadSizeBytes);
        return EINVAL;
    }

    *payload = (char*)strdup(Engine::GetModuleInfo());
    if (!*payload)
    {
        OsConfigLogError(g_log, "ComplianceMmiGetInfo: failed to duplicate module info");
        return ENOMEM;
    }

    *payloadSizeBytes = (int)strlen(*payload);
    return MMI_OK;
}

int ComplianceMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes)
{
    if ((nullptr == componentName) || (nullptr == objectName) || (nullptr == payload) || (nullptr == payloadSizeBytes))
    {
        OsConfigLogError(g_log, "ComplianceMmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        return EINVAL;
    }

    if (nullptr == clientSession)
    {
        OsConfigLogError(g_log, "ComplianceMmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        return EINVAL;
    }

    if (0 != strcmp(componentName, "Compliance"))
    {
        OsConfigLogError(g_log, "ComplianceMmiGet called for an unsupported component name (%s)", componentName);
        return EINVAL;
    }
    auto& engine = *reinterpret_cast<Engine*>(clientSession);

    *payload = NULL;
    *payloadSizeBytes = 0;

    try
    {
        auto result = engine.MmiGet(objectName);
        if (!result.HasValue())
        {
            OsConfigLogError(engine.Log(), "ComplianceMmiGet failed: %s", result.Error().message.c_str());
            return result.Error().code;
        }

        *payload = strdup(result.Value().payload);
        *payloadSizeBytes = result.Value().payloadSize;
        OsConfigLogInfo(engine.Log(), "MmiGet(%p, %s, %s, %.*s)", clientSession, componentName, objectName, *payloadSizeBytes, *payload);
        return MMI_OK;
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(engine.Log(), "ComplianceMmiGet failed: %s", e.what());
    }

    return -1;
}

int ComplianceMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes)
{
    if ((nullptr == componentName) || (nullptr == objectName) || (nullptr == payload) || (0 > payloadSizeBytes))
    {
        OsConfigLogError(g_log, "ComplianceMmiSet(%s, %s, %.*s) called with invalid arguments", componentName, objectName, payloadSizeBytes, payload);
        return EINVAL;
    }

    if (nullptr == clientSession)
    {
        OsConfigLogError(g_log, "ComplianceMmiSet(%s, %s, %.*s) called outside of a valid session", componentName, objectName, payloadSizeBytes, payload);
        return EINVAL;
    }

    if (0 != strcmp(componentName, "Compliance"))
    {
        OsConfigLogError(g_log, "ComplianceMmiSet called for an unsupported component name (%s)", componentName);
        return EINVAL;
    }
    auto& engine = *reinterpret_cast<Engine*>(clientSession);

    try
    {
        auto result = engine.MmiSet(objectName, payload, payloadSizeBytes);
        if (!result.HasValue())
        {
            OsConfigLogError(engine.Log(), "ComplianceMmiSet failed: %s", result.Error().message.c_str());
            return result.Error().code;
        }

        OsConfigLogInfo(engine.Log(), "MmiSet(%p, %s, %s, %.*s, %d) returned %s", clientSession, componentName, objectName, payloadSizeBytes, payload,
            payloadSizeBytes, result.Value() ? "true" : "false");
        return MMI_OK;
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(engine.Log(), "ComplianceMmiSet failed: %s", e.what());
    }

    return -1;
}

void ComplianceMmiFree(char* payload)
{
    FREE_MEMORY(payload);
}
