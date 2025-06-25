// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ComplianceEngineInterface.h"

#include "CommonContext.h"
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

using ComplianceEngine::Engine;
using ComplianceEngine::JSONFromString;
using ComplianceEngine::ParseJson;
using ComplianceEngine::Status;

namespace
{
OsConfigLogHandle g_log = nullptr;
static const std::set<int> g_criticalErrors = {ENOMEM};
} // namespace

void ComplianceEngineInitialize(OsConfigLogHandle log)
{
    UNUSED(log);
    g_log = log;
}

void ComplianceEngineShutdown(void)
{
}

MMI_HANDLE ComplianceEngineMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    auto context = std::unique_ptr<ComplianceEngine::CommonContext>(new ComplianceEngine::CommonContext(g_log));
    if (nullptr == context)
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiOpen(%s, %u): failed to create context", clientName, maxPayloadSizeBytes);
        return nullptr;
    }
    auto* result = reinterpret_cast<void*>(new Engine(std::move(context)));
    OsConfigLogInfo(g_log, "ComplianceEngineMmiOpen(%s, %u) returning %p", clientName, maxPayloadSizeBytes, result);
    return result;
}

void ComplianceEngineMmiClose(MMI_HANDLE clientSession)
{
    delete reinterpret_cast<Engine*>(clientSession);
}

int ComplianceEngineMmiGetInfo(const char* clientName, char** payload, int* payloadSizeBytes)
{
    if ((nullptr == payload) || (nullptr == payloadSizeBytes))
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiGetInfo(%s, %p, %p) called with invalid arguments", clientName, payload, payloadSizeBytes);
        return EINVAL;
    }

    *payload = strdup(Engine::GetModuleInfo());
    if (!*payload)
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiGetInfo: failed to duplicate module info");
        return ENOMEM;
    }

    *payloadSizeBytes = (int)strlen(*payload);
    return MMI_OK;
}

int ComplianceEngineMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes)
{
    if ((nullptr == componentName) || (nullptr == objectName) || (nullptr == payload) || (nullptr == payloadSizeBytes))
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        return EINVAL;
    }

    if (nullptr == clientSession)
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        return EINVAL;
    }

    if (0 != strcmp(componentName, "ComplianceEngine"))
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiGet called for an unsupported component name (%s)", componentName);
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
                OsConfigLogError(engine.Log(), "ComplianceEngineMmiGet failed with a critical error: %s (errno: %d)", result.Error().message.c_str(),
                    result.Error().code);
                return result.Error().code;
            }
            else
            {
                OsConfigLogError(engine.Log(), "ComplianceEngineMmiGet failed with a non-critical error: %s (errno: %d)",
                    result.Error().message.c_str(), result.Error().code);
                result = ComplianceEngine::AuditResult(Status::NonCompliant, "Audit failed with a non-critical error: " + result.Error().message);
            }
        }

        auto json = JSONFromString(result.Value().payload.c_str());
        if (NULL == json)
        {
            OsConfigLogError(engine.Log(), "ComplianceEngineMmiGet failed: Failed to create JSON object from string");
            return ENOMEM;
        }
        else if (NULL == (*payload = json_serialize_to_string(json.get())))
        {
            OsConfigLogError(engine.Log(), "ComplianceEngineMmiGet failed: Failed to serialize JSON object");
            return ENOMEM;
        }
        *payloadSizeBytes = static_cast<int>(strlen(*payload));
        OsConfigLogDebug(engine.Log(), "MmiGet(%p, %s, %s, %.*s)", clientSession, componentName, objectName, *payloadSizeBytes, *payload);
        return MMI_OK;
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(engine.Log(), "ComplianceEngineMmiGet failed: %s", e.what());
    }

    return -1;
}

int ComplianceEngineMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes)
{
    if ((nullptr == componentName) || (nullptr == objectName) || (nullptr == payload) || (0 > payloadSizeBytes))
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiSet(%s, %s, %.*s) called with invalid arguments", componentName, objectName, payloadSizeBytes, payload);
        return EINVAL;
    }

    if (nullptr == clientSession)
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiSet(%s, %s, %.*s) called outside of a valid session", componentName, objectName, payloadSizeBytes, payload);
        return EINVAL;
    }

    if (0 != strcmp(componentName, "ComplianceEngine"))
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiSet called for an unsupported component name (%s)", componentName);
        return EINVAL;
    }
    auto& engine = *reinterpret_cast<Engine*>(clientSession);

    try
    {
        std::string payloadStr(payload, payloadSizeBytes);
        auto object = ParseJson(payloadStr.c_str());
        if (NULL == object || (JSONString != json_value_get_type(object.get()) && JSONObject != json_value_get_type(object.get())))
        {
            OsConfigLogError(engine.Log(), "ComplianceEngineMmiSet failed: Failed to parse JSON string");
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
                OsConfigLogError(engine.Log(), "ComplianceEngineMmiSet failed with a critical error: %s (errno: %d)", result.Error().message.c_str(),
                    result.Error().code);
                return result.Error().code;
            }
            else
            {
                OsConfigLogError(engine.Log(), "ComplianceEngineMmiSet failed with a non-critical error: %s (errno: %d)",
                    result.Error().message.c_str(), result.Error().code);
                return MMI_OK;
            }
        }

        OsConfigLogDebug(engine.Log(), "MmiSet(%p, %s, %s, %.*s, %d) returned %s", clientSession, componentName, objectName, payloadSizeBytes, payload,
            payloadSizeBytes, result.Value() == Status::Compliant ? "compliant" : "non-compliant");
        return MMI_OK;
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(engine.Log(), "ComplianceEngineMmiSet failed: %s", e.what());
    }

    return -1;
}

void ComplianceEngineMmiFree(char* payload)
{
    FREE_MEMORY(payload);
}
