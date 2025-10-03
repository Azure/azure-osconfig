// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ComplianceEngineInterface.h"

#include "BenchmarkInfo.h"
#include "CommonContext.h"
#include "CommonUtils.h"
#include "DistributionInfo.h"
#include "Engine.h"
#include "JsonWrapper.h"
#include "Logging.h"
#include "Mmi.h"
#include "Result.h"

#include <Telemetry.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <exception>
#include <fstream>
#include <parson.h>
#include <set>
#include <sstream>
#include <sys/stat.h>

using ComplianceEngine::CISBenchmarkInfo;
using ComplianceEngine::DistributionInfo;
using ComplianceEngine::Engine;
using ComplianceEngine::JSONFromString;
using ComplianceEngine::ParseJson;
using ComplianceEngine::Status;

namespace
{
static constexpr const char* cModuleTestClientName = "ModuleTestClient";
static constexpr const char* cNRPClientName = "ComplianceEngine";
OsConfigLogHandle g_log = nullptr;
static const std::set<int> g_criticalErrors = {ENOMEM};
} // namespace

// This function is called in library constructor by BaselineInitialize
void ComplianceEngineInitialize(OsConfigLogHandle log)
{
    g_log = log;
}

// This function is called in library destructor by BaselineInitialize
void ComplianceEngineShutdown(void)
{
}

MMI_HANDLE ComplianceEngineMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    auto context = std::unique_ptr<ComplianceEngine::CommonContext>(new ComplianceEngine::CommonContext(g_log));
    if (nullptr == context)
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiOpen(%s, %u): failed to create context", clientName, maxPayloadSizeBytes);
        OSConfigTelemetryStatusTrace("CommonContext", ENOMEM);
        return nullptr;
    }
    std::unique_ptr<ComplianceEngine::PayloadFormatter> formatter;
    if (!strcmp(clientName, cModuleTestClientName))
    {
        OsConfigLogInfo(g_log, "ComplianceEngineMmiOpen(%s) using DebugFormatter", clientName);
        formatter.reset(new ComplianceEngine::DebugFormatter());
    }
    else if (!strcmp(clientName, cNRPClientName))
    {
        OsConfigLogInfo(g_log, "ComplianceEngineMmiOpen(%s) using NestedListFormatter", clientName);
        formatter.reset(new ComplianceEngine::NestedListFormatter());
    }
    else
    {
        OsConfigLogInfo(g_log, "ComplianceEngineMmiOpen(%s) using JsonFormatter", clientName);
        formatter.reset(new ComplianceEngine::JsonFormatter());
    }

    auto* engine = new Engine(std::move(context), std::move(formatter));
    auto error = engine->LoadDistributionInfo();
    if (error)
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiOpen(%s, %u): failed to load distribution info: %s", clientName, maxPayloadSizeBytes, error->message.c_str());
        OSConfigTelemetryStatusTrace("LoadDistributionInfo", error->code);
        delete engine;
        return nullptr;
    }

    auto* result = reinterpret_cast<void*>(engine);
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
        OSConfigTelemetryStatusTrace("payload", EINVAL);
        return EINVAL;
    }

    *payload = strdup(Engine::GetModuleInfo());
    if (!*payload)
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiGetInfo: failed to duplicate module info");
        OSConfigTelemetryStatusTrace("strdup", ENOMEM);
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
        OSConfigTelemetryStatusTrace("payload", EINVAL);
        return EINVAL;
    }

    if (nullptr == clientSession)
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        OSConfigTelemetryStatusTrace("clientSession", EINVAL);
        return EINVAL;
    }

    if (0 != strcmp(componentName, "ComplianceEngine"))
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiGet called for an unsupported component name (%s)", componentName);
        OSConfigTelemetryStatusTrace("componentName", EINVAL);
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
                OSConfigTelemetryStatusTrace("MmiGet", result.Error().code);
                return result.Error().code;
            }
            else
            {
                OsConfigLogError(engine.Log(), "ComplianceEngineMmiGet failed with a non-critical error: %s (errno: %d)",
                    result.Error().message.c_str(), result.Error().code);
                OSConfigTelemetryStatusTrace("MmiGet", result.Error().code);
                result = ComplianceEngine::AuditResult(Status::NonCompliant, "Audit failed with a non-critical error: " + result.Error().message);
            }
        }

        auto payloadString = result.Value().payload;
        if (result.Value().status == Status::Compliant)
        {
            payloadString = "PASS" + payloadString;
        }

        auto json = JSONFromString(payloadString.c_str());
        if (NULL == json)
        {
            OsConfigLogError(engine.Log(), "ComplianceEngineMmiGet failed: Failed to create JSON object from string");
            OSConfigTelemetryStatusTrace("JSONFromString", ENOMEM);
            return ENOMEM;
        }
        else if (NULL == (*payload = json_serialize_to_string(json.get())))
        {
            OsConfigLogError(engine.Log(), "ComplianceEngineMmiGet failed: Failed to serialize JSON object");
            OSConfigTelemetryStatusTrace("json_serialize_to_string", ENOMEM);
            return ENOMEM;
        }
        *payloadSizeBytes = static_cast<int>(strlen(*payload));
        OsConfigLogDebug(engine.Log(), "MmiGet(%p, %s, %s, %.*s)", clientSession, componentName, objectName, *payloadSizeBytes, *payload);
        return MMI_OK;
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(engine.Log(), "ComplianceEngineMmiGet failed: %s", e.what());
        OSConfigTelemetryStatusTrace("MmiGet", -1);
    }

    return -1;
}

int ComplianceEngineMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes)
{
    if ((nullptr == componentName) || (nullptr == objectName) || (nullptr == payload) || (0 > payloadSizeBytes))
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiSet(%s, %s, %.*s) called with invalid arguments", componentName, objectName, payloadSizeBytes, payload);
        OSConfigTelemetryStatusTrace("payload", EINVAL);
        return EINVAL;
    }

    if (nullptr == clientSession)
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiSet(%s, %s, %.*s) called outside of a valid session", componentName, objectName, payloadSizeBytes, payload);
        OSConfigTelemetryStatusTrace("clientSession", EINVAL);
        return EINVAL;
    }

    if (0 != strcmp(componentName, "ComplianceEngine"))
    {
        OsConfigLogError(g_log, "ComplianceEngineMmiSet called for an unsupported component name (%s)", componentName);
        OSConfigTelemetryStatusTrace("componentName", EINVAL);
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
            OSConfigTelemetryStatusTrace("ParseJson", EINVAL);
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
                OSConfigTelemetryStatusTrace("MmiSet", result.Error().code);
                return result.Error().code;
            }
            else
            {
                OsConfigLogError(engine.Log(), "ComplianceEngineMmiSet failed with a non-critical error: %s (errno: %d)",
                    result.Error().message.c_str(), result.Error().code);
                OSConfigTelemetryStatusTrace("MmiSet", result.Error().code);
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
        OSConfigTelemetryStatusTrace("MmiSet", -1);
    }

    return -1;
}

void ComplianceEngineMmiFree(char* payload)
{
    FREE_MEMORY(payload);
}

int ComplianceEngineCheckApplicability(MMI_HANDLE clientSession, const char* payloadKey, OsConfigLogHandle log)
{
    // parse the /etc/os-release and check whether the payloadKey defines the same distribution as the one in the file
    // the payloadKey is formatted as a path: /<benchmark>/<benchmark_specific_format>
    // In case the payloadKey starts with /cis, it becomes: /cis/<distribution>/<version>/<benchmark_version>/<section1>/<section2>/...
    if ((nullptr == clientSession) || (nullptr == payloadKey))
    {
        OsConfigLogError(log, "ComplianceEngineValidatePayload called with invalid arguments");
        OSConfigTelemetryStatusTrace("clientSession", EINVAL);
        return EINVAL;
    }

    const auto& engine = *reinterpret_cast<Engine*>(clientSession);
    const auto& distributionInfo = engine.GetDistributionInfo();
    if (!distributionInfo.HasValue())
    {
        OsConfigLogError(log, "ComplianceEngineValidatePayload: Distribution info is not available");
        OSConfigTelemetryStatusTrace("GetDistributionInfo", EINVAL);
        return EINVAL;
    }

    auto benchmark = CISBenchmarkInfo::Parse(payloadKey);
    if (!benchmark.HasValue())
    {
        OsConfigLogError(log, "ComplianceEngineValidatePayload failed to parse benchmark: %s", benchmark.Error().message.c_str());
        OSConfigTelemetryStatusTrace("CISBenchmarkInfo::Parse", EINVAL);
        return EINVAL;
    }

    if (!benchmark->Match(distributionInfo.Value()))
    {
        OsConfigLogInfo(log, "This benchmark is not applicable for the current distribution");
        OsConfigLogInfo(log, "Current system identification: %s", std::to_string(distributionInfo.Value()).c_str());
        auto overridden = distributionInfo.Value();
        overridden.distribution = benchmark->distribution;
        overridden.version = benchmark->SanitizedVersion();
        OsConfigLogInfo(log, "To override this detection, place the following line inside the '%s' file: %s",
            DistributionInfo::cDefaultOverrideFilePath, std::to_string(overridden).c_str());
        return EINVAL;
    }

    return 0;
}
