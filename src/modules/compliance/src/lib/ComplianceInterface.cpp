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
#include <parson.h>

using compliance::Engine;
using compliance::Status;

static OsConfigLogHandle g_log = nullptr;

void ComplianceInitialize(OsConfigLogHandle log)
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

    *payload = strdup(Engine::getModuleInfo());
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
        auto result = engine.mmiGet(objectName);
        if (!result.has_value())
        {
            OsConfigLogError(engine.log(), "ComplianceMmiGet failed: %s", result.error().message.c_str());
            return result.error().code;
        }

        *payload = strndup(result.value().payload.c_str(), result.value().payload.size());
        if (NULL == *payload)
        {
            OsConfigLogError(engine.log(), "ComplianceMmiGet: failed to allocate memory for payload");
            return ENOMEM;
        }
        *payloadSizeBytes = result.value().payload.size();
        OsConfigLogInfo(engine.log(), "MmiGet(%p, %s, %s, %.*s)", clientSession, componentName, objectName, *payloadSizeBytes, *payload);
        return MMI_OK;
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(engine.log(), "ComplianceMmiGet failed: %s", e.what());
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
        std::string payloadStr(payload, payloadSizeBytes);
        JSON_Value* object = json_parse_string(payloadStr.c_str());
        if (NULL == object || JSONString != json_value_get_type(object))
        {
            OsConfigLogError(engine.log(), "ComplianceMmiSet failed: Failed to parse JSON string");
            return EINVAL;
        }
        std::string realPayload = json_value_get_string(object);
        auto result = engine.mmiSet(objectName, realPayload);
        if (!result.has_value())
        {
            OsConfigLogError(engine.log(), "ComplianceMmiSet failed: %s", result.error().message.c_str());
            return result.error().code;
        }

        OsConfigLogInfo(engine.log(), "MmiSet(%p, %s, %s, %.*s, %d) returned %s", clientSession, componentName, objectName, payloadSizeBytes, payload,
            payloadSizeBytes, result.value() == Status::Compliant ? "compliant" : "non-compliant");
        return MMI_OK;
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(engine.log(), "ComplianceMmiSet failed: %s", e.what());
    }

    return -1;
}

void ComplianceMmiFree(char* payload)
{
    FREE_MEMORY(payload);
}
