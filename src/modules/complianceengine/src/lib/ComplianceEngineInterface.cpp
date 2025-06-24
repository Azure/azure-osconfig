// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ComplianceEngineInterface.h"

#include "CommonContext.h"
#include "CommonUtils.h"
#include "Engine.h"
#include "JsonWrapper.h"
#include "Logging.h"
#include "Mmi.h"
#include "Result.h"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <exception>
#include <fstream>
#include <parson.h>
#include <set>
#include <sstream>

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
        OsConfigLogInfo(engine.Log(), "MmiGet(%p, %s, %s, %.*s)", clientSession, componentName, objectName, *payloadSizeBytes, *payload);
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

        OsConfigLogInfo(engine.Log(), "MmiSet(%p, %s, %s, %.*s, %d) returned %s", clientSession, componentName, objectName, payloadSizeBytes, payload,
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

namespace
{
using ComplianceEngine::Error;
using ComplianceEngine::Result;

// Defines the type of the benchmark, e.g., CIS
enum class BenchmarkType
{
    CIS
};

// Defines CIS benchmark information
struct CISBenchmarkInfo
{
    // Defines the Linux distribution, e.g., Ubuntu, CentOS
    std::string distribution;

    // Defines the version of the Linux distribution, e.g., 20.04, 8
    std::string version;

    // Defines the version of the benchmark, e.g., v1.0.0
    std::string benchmarkVersion;

    // Defines the benchmark secsion, e.g. 1.1.1
    std::string section;
};

Result<BenchmarkType> ParseBenchmarkType(const std::string& payloadKey)
{
    if (payloadKey.empty() || payloadKey[0] != '/')
    {
        return Error("Invalid payload key format");
    }

    auto it = payloadKey.find('/', 1);
    if (it == std::string::npos)
    {
        return Error("Invalid payload key format");
    }
    auto typeStr = payloadKey.substr(1, it - 1);
    if (typeStr == "cis")
    {
        return BenchmarkType::CIS;
    }

    return Error("Unsupported benchmark type: " + typeStr);
}

Result<CISBenchmarkInfo> ParseCISBenchmarkInfo(std::string payloadKey)
{
    std::string token;
    std::stringstream ss(payloadKey);
    // skip the first token which is expected to be empty due to leading '/'
    if (!std::getline(ss, token, '/') || !token.empty())
    {
        return Error("Invalid payload key format");
    }

    CISBenchmarkInfo result;
    if (!std::getline(ss, result.distribution, '/') || result.distribution.empty())
    {
        return Error("Invalid CIS benchmark payload key format: missing distribution");
    }
    if (!std::getline(ss, result.version, '/') || result.version.empty())
    {
        return Error("Invalid CIS benchmark payload key format: missing version");
    }
    if (!std::getline(ss, result.benchmarkVersion, '/') || result.benchmarkVersion.empty())
    {
        return Error("Invalid CIS benchmark payload key format: missing benchmark version");
    }

    // Parse the section, which can be multiple tokens separated by '/'
    while (std::getline(ss, token, '/'))
    {
        if (!result.section.empty())
        {
            result.section.append(".");
        }
        result.section.append(std::move(token));
    }

    return result;
}

Result<std::map<std::string, std::string>> ParseEtcOsRelease()
{
    std::map<std::string, std::string> osReleaseInfo;
    std::ifstream file("/etc/os-release");
    if (!file.is_open())
    {
        return Error("Failed to open /etc/os-release");
    }

    std::string line;
    while (std::getline(file, line))
    {
        auto pos = line.find('=');
        if (pos == std::string::npos)
        {
            continue; // Skip lines without '='
        }
        auto key = line.substr(0, pos);
        auto value = line.substr(pos + 1);
        if (!value.empty() && value.front() == '"' && value.back() == '"')
        {
            value = value.substr(1, value.size() - 2); // Remove surrounding quotes
        }
        osReleaseInfo[key] = value;
    }

    return osReleaseInfo;
}
} // namespace

int ComplianceEngineValidatePayload(const char* resourceId, const char* ruleId, const char* payloadKey, OsConfigLogHandle log)
{
    // parse the /etc/os-release and check whether the payloadKey defines the same distribution as the one in the file
    // the payloadKey is formatted as a path: /<benchmark>/<benchmark_specific_format>
    // In case the payloadKey starts with /cis, it becomes: /cis/<distribution>/<version>/<benchmark_version>/<section1>/<section2>/...

    if (nullptr == resourceId || nullptr == ruleId || nullptr == payloadKey)
    {
        OsConfigLogError(log, "ComplianceEngineValidatePayload called with invalid arguments");
        return EINVAL;
    }

    static const auto osReleaseInfo = ParseEtcOsRelease();
    if (!osReleaseInfo.HasValue())
    {
        OsConfigLogError(log, "ComplianceEngineValidatePayload failed to parse /etc/os-release: %s", osReleaseInfo.Error().message.c_str());
        return EINVAL;
    }

    auto benchmarkType = ParseBenchmarkType(payloadKey);
    if (!benchmarkType.HasValue())
    {
        OsConfigLogError(log, "ComplianceEngineValidatePayload failed to parse benchmark type: %s", benchmarkType.Error().message.c_str());
        return EINVAL;
    }

    if (benchmarkType.Value() == BenchmarkType::CIS)
    {
        auto benchmarkInfo = ParseCISBenchmarkInfo(payloadKey + strlen("/cis"));
        if (!benchmarkInfo.HasValue())
        {
            OsConfigLogError(log, "ComplianceEngineValidatePayload failed to parse CIS benchmark info: %s", benchmarkInfo.Error().message.c_str());
            return EINVAL;
        }

        // Here you would typically validate the resourceId and ruleId against the parsed benchmarkInfo
        // For now, we just log the information
        OsConfigLogDebug(log, "Validating CIS benchmark for distribution: %s, version: %s, benchmark version: %s, section: %s",
            benchmarkInfo.Value().distribution.c_str(), benchmarkInfo.Value().version.c_str(), benchmarkInfo.Value().benchmarkVersion.c_str(),
            benchmarkInfo.Value().section.c_str());

        auto it = osReleaseInfo.Value().find("ID");
        if (it == osReleaseInfo.Value().end())
        {
            OsConfigLogError(log, "ComplianceEngineValidatePayload: /etc/os-release does not contain 'ID' field");
            return EINVAL;
        }

        if (it->second != benchmarkInfo.Value().distribution)
        {
            OsConfigLogInfo(log, "ComplianceEngineValidatePayload: Distribution mismatch. Expected: %s, Found: %s",
                benchmarkInfo.Value().distribution.c_str(), it != osReleaseInfo.Value().end() ? it->second.c_str() : "unknown");
            return EINVAL;
        }

        it = osReleaseInfo.Value().find("VERSION_ID");
        if (it == osReleaseInfo.Value().end())
        {
            OsConfigLogError(log, "ComplianceEngineValidatePayload: /etc/os-release does not contain 'VERSION_ID' field");
            return EINVAL;
        }
        if (it->second != benchmarkInfo.Value().version)
        {
            OsConfigLogInfo(log, "ComplianceEngineValidatePayload: Version mismatch. Expected: %s, Found: %s", benchmarkInfo.Value().version.c_str(),
                it != osReleaseInfo.Value().end() ? it->second.c_str() : "unknown");
            return EINVAL;
        }
        return 0;
    }

    OsConfigLogError(log, "ComplianceEngineValidatePayload: Unsupported benchmark type or format");
    return EINVAL;
}
