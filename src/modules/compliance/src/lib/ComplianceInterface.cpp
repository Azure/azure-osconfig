// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ComplianceInterface.h"

#include "CommonUtils.h"
#include "Engine.h"
#include "JsonWrapper.h"
#include "Logging.h"
#include "Mmi.h"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <exception>
#include <parson.h>
#include <set>

using compliance::Engine;
using compliance::JSONFromString;
using compliance::ParseJson;
using compliance::Status;

namespace
{
OsConfigLogHandle g_log = nullptr;
static const std::set<int> g_criticalErrors = {ENOMEM};
} // namespace

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

    *payload = strdup(Engine::GetModuleInfo());
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
            if (g_criticalErrors.find(result.Error().code) != g_criticalErrors.end())
            {
                OsConfigLogError(engine.Log(), "ComplianceMmiGet failed with a critical error: %s (errno: %d)", result.Error().message.c_str(),
                    result.Error().code);
                return result.Error().code;
            }
            else
            {
                OsConfigLogError(engine.Log(), "ComplianceMmiGet failed with a non-critical error: %s (errno: %d)", result.Error().message.c_str(),
                    result.Error().code);
                result = compliance::AuditResult(Status::NonCompliant, result.Error().message);
            }
        }

        auto json = JSONFromString(result.Value().payload.c_str());
        if (NULL == json)
        {
            OsConfigLogError(engine.Log(), "ComplianceMmiGet failed: Failed to create JSON object from string");
            return ENOMEM;
        }
        else if (NULL == (*payload = json_serialize_to_string(json.get())))
        {
            OsConfigLogError(engine.Log(), "ComplianceMmiGet failed: Failed to serialize JSON object");
            return ENOMEM;
        }
        *payloadSizeBytes = static_cast<int>(strlen(*payload));
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
        std::string payloadStr(payload, payloadSizeBytes);
        auto object = ParseJson(payloadStr.c_str());
        if (NULL == object || (JSONString != json_value_get_type(object.get()) && JSONObject != json_value_get_type(object.get())))
        {
            OsConfigLogError(engine.Log(), "ComplianceMmiSet failed: Failed to parse JSON string");
            return EINVAL;
        }
        std::string realPayload;
        if (json_value_get_type(object.get()) == JSONString)
        {
            realPayload = json_value_get_string(object.get());
        }
        else if (json_value_get_type(object.get()) == JSONObject)
        {
            char* tmp = json_serialize_to_string(object.get());
            realPayload = tmp;
            json_free_serialized_string(tmp);
        }
        auto result = engine.MmiSet(objectName, std::move(realPayload));
        if (!result.HasValue())
        {
            if (g_criticalErrors.find(result.Error().code) != g_criticalErrors.end())
            {
                OsConfigLogError(engine.Log(), "ComplianceMmiSet failed with a critical error: %s (errno: %d)", result.Error().message.c_str(),
                    result.Error().code);
                return result.Error().code;
            }
            else
            {
                OsConfigLogError(engine.Log(), "ComplianceMmiSet failed with a non-critical error: %s (errno: %d)", result.Error().message.c_str(),
                    result.Error().code);
                return MMI_OK;
            }
        }

        OsConfigLogInfo(engine.Log(), "MmiSet(%p, %s, %s, %.*s, %d) returned %s", clientSession, componentName, objectName, payloadSizeBytes, payload,
            payloadSizeBytes, result.Value() == Status::Compliant ? "compliant" : "non-compliant");
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
