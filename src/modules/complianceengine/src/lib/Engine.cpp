// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Engine.h"

#include "Base64.h"
#include "Evaluator.h"
#include "JsonWrapper.h"
#include "Logging.h"
#include "Optional.h"
#include "Procedure.h"
#include "Result.h"

#include <Internal.h>
#include <cerrno>
#include <cstring>
#include <map>
#include <memory>
#include <parson.h>
#include <string>
#include <sys/stat.h>
#include <utility>

namespace ComplianceEngine
{
static constexpr const char* cModuleInfo =
    "{\"Name\": \"ComplianceEngine\","
    "\"Description\": \"Provides functionality to audit and remediate Security Baseline policies on device\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 0,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"\","
    "\"Components\": [\"ComplianceEngine\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

Engine::Engine(std::unique_ptr<ContextInterface> context, std::unique_ptr<PayloadFormatter> payloadFormatter) noexcept
    : mContext{std::move(context)},
      mFormatter{std::move(payloadFormatter)}
{
}

void Engine::SetMaxPayloadSize(unsigned int value) noexcept
{
    mMaxPayloadSize = value;
}

unsigned int Engine::GetMaxPayloadSize() const noexcept
{
    return mMaxPayloadSize;
}

OsConfigLogHandle Engine::Log() const noexcept
{
    return mContext->GetLogHandle();
}

Optional<Error> Engine::LoadDistributionInfo()
{
    struct stat st;
    if (0 == stat(DistributionInfo::cDefaultOverrideFilePath, &st))
    {
        // Override file exists, use it as distribution info source
        OsConfigLogDebug(Log(), "ComplianceEngineValidatePayload: Using %s for distribution info", DistributionInfo::cDefaultOverrideFilePath);
        auto overrideInfo = DistributionInfo::ParseOverrideFile(DistributionInfo::cDefaultOverrideFilePath);
        if (!overrideInfo.HasValue())
        {
            OSConfigTelemetryStatusTrace(mContext->GetTelemetryHandle(), "ParseOverrideFile", overrideInfo.Error().code);
            OsConfigLogError(Log(), "ComplianceEngineValidatePayload failed to parse %s: %s", DistributionInfo::cDefaultOverrideFilePath,
                overrideInfo.Error().message.c_str());
            return overrideInfo.Error();
        }

        mDistributionInfo = std::move(overrideInfo).Value();
    }
    else if (ENOENT == errno)
    {
        // Override file does not exist, use /etc/os-release
        OsConfigLogDebug(Log(), "ComplianceEngineValidatePayload: Using %s for distribution info", DistributionInfo::cDefaultEtcOsReleasePath);
        auto osReleaseInfo = DistributionInfo::ParseEtcOsRelease(DistributionInfo::cDefaultEtcOsReleasePath);
        if (!osReleaseInfo.HasValue())
        {
            OSConfigTelemetryStatusTrace(mContext->GetTelemetryHandle(), "ParseEtcOsRelease", osReleaseInfo.Error().code);
            OsConfigLogError(Log(), "ComplianceEngineValidatePayload failed to parse %s: %s", DistributionInfo::cDefaultEtcOsReleasePath,
                osReleaseInfo.Error().message.c_str());
            return osReleaseInfo.Error();
        }

        mDistributionInfo = std::move(osReleaseInfo).Value();
    }
    else
    {
        int status = errno;
        OSConfigTelemetryStatusTrace(mContext->GetTelemetryHandle(), "stat", status);
        OsConfigLogError(Log(), "ComplianceEngineValidatePayload failed to access %s: %s", DistributionInfo::cDefaultOverrideFilePath, strerror(status));
        return Error("Failed to access override file", status);
    }

    return Optional<Error>();
}

const Optional<DistributionInfo>& Engine::GetDistributionInfo() const noexcept
{
    return mDistributionInfo;
}

const char* Engine::GetModuleInfo() noexcept
{
    return cModuleInfo;
}

Result<AuditResult> Engine::MmiGet(const char* objectName)
{
    if (nullptr == objectName)
    {
        return Error("Invalid argument", EINVAL);
    }

    OsConfigLogDebug(Log(), "Engine::mmiGet(%s)", objectName);
    auto ruleName = std::string(objectName);
    constexpr const char* auditPrefix = "audit";
    if (ruleName.find(auditPrefix) != 0)
    {
        return Error("Invalid object name", EINVAL);
    }

    ruleName = ruleName.substr(strlen(auditPrefix));
    if (ruleName.empty())
    {
        return Error("Rule name is empty", EINVAL);
    }

    auto it = mDatabase.find(ruleName);
    if (it == mDatabase.end())
    {
        return Error("Rule not found", EINVAL);
    }
    const auto& procedure = it->second;
    if (nullptr == procedure.Audit())
    {
        return Error("Failed to get 'audit' object");
    }

    Evaluator evaluator(ruleName, procedure.Audit(), procedure.Parameters(), *mContext);
    return evaluator.ExecuteAudit(*mFormatter);
}

Result<JsonWrapper> Engine::DecodeB64Json(const std::string& input) const
{
    auto decodedString = Base64Decode(input);
    if (!decodedString.HasValue())
    {
        return decodedString.Error();
    }
    auto result = json_parse_string(decodedString.Value().c_str());
    if (nullptr == result)
    {
        return Error("Failed to parse JSON", EINVAL);
    }

    return JsonWrapper(result);
}

Optional<Error> Engine::SetProcedure(const std::string& ruleName, const std::string& payload)
{
    if (ruleName.empty())
    {
        return Error("Rule name is empty", EINVAL);
    }

    mDatabase.erase(ruleName);
    auto ruleJSON = DecodeB64Json(payload);
    if (!ruleJSON.HasValue())
    {
        // Fall back to plain JSON, both formats are supported
        ruleJSON = ComplianceEngine::ParseJson(payload.c_str());
        if (!ruleJSON.HasValue())
        {
            OSConfigTelemetryStatusTrace(mContext->GetTelemetryHandle(), "ComplianceEngine::ParseJson", ruleJSON.Error().code);
            OsConfigLogError(Log(), "Failed to parse JSON: %s", ruleJSON.Error().message.c_str());
            return ruleJSON.Error();
        }
    }

    auto object = json_value_get_object(ruleJSON.Value().get());
    if (nullptr == object)
    {
        return Error("Failed to parse JSON object");
    }

    auto jsonValue = json_object_get_value(object, "audit");
    if (nullptr == jsonValue)
    {
        return Error("Missing 'audit' object");
    }

    if (json_value_get_type(jsonValue) != JSONObject)
    {
        return Error("The 'audit' value is not an object");
    }

    auto procedure = Procedure{};
    auto error = procedure.SetAudit(jsonValue);
    if (error)
    {
        return error.Value();
    }
    if (nullptr == procedure.Audit())
    {
        OSConfigTelemetryStatusTrace(mContext->GetTelemetryHandle(), "Procedure::SetAudit", ENOMEM);
        OsConfigLogError(Log(), "Failed to copy 'audit' object");
        return Error("Out of memory");
    }

    jsonValue = json_object_get_value(object, "remediate");
    if (nullptr != jsonValue)
    {
        if (json_value_get_type(jsonValue) != JSONObject)
        {
            return Error("The 'remediate' value is not an object");
        }

        error = procedure.SetRemediation(jsonValue);
        if (error)
        {
            return error.Value();
        }
        if (nullptr == procedure.Remediation())
        {
            OSConfigTelemetryStatusTrace(mContext->GetTelemetryHandle(), "Procedure::Remediation", ENOMEM);
            OsConfigLogError(Log(), "Failed to copy 'remediate' object");
            return Error("Out of memory");
        }
    }

    jsonValue = json_object_get_value(object, "parameters");
    if (nullptr != jsonValue)
    {
        if (json_value_get_type(jsonValue) != JSONObject)
        {
            return Error("The 'parameters' value is not an object");
        }

        auto paramsObj = json_value_get_object(jsonValue);
        auto count = json_object_get_count(paramsObj);
        for (decltype(count) i = 0; i < count; ++i)
        {
            const char* key = json_object_get_name(paramsObj, i);
            const char* val = json_object_get_string(paramsObj, key);
            if ((nullptr == key) || (nullptr == val))
            {
                OSConfigTelemetryStatusTrace(mContext->GetTelemetryHandle(), "json_object_get_name", EINVAL);
                OsConfigLogError(Log(), "Failed to get parameter name and value");
                return Error("Failed to get parameter name and value");
            }

            procedure.SetParameter(key, val);
        }
    }
    mDatabase.emplace(std::move(ruleName), std::move(procedure));
    return Optional<Error>();
}

Optional<Error> Engine::InitAudit(const std::string& ruleName, const std::string& payload)
{
    if (ruleName.empty())
    {
        return Error("Rule name is empty", EINVAL);
    }

    auto it = mDatabase.find(ruleName);
    if (it == mDatabase.end())
    {
        return Error("Out-of-order operation: procedure must be set first", EINVAL);
    }

    auto error = it->second.UpdateUserParameters(payload);
    if (error)
    {
        return error.Value();
    }

    return Optional<Error>();
}

Result<Status> Engine::ExecuteRemediation(const std::string& ruleName, const std::string& payload)
{
    if (ruleName.empty())
    {
        return Error("Rule name is empty", EINVAL);
    }

    auto it = mDatabase.find(ruleName);
    if (it == mDatabase.end())
    {
        return Error("Out-of-order operation: procedure must be set first", EINVAL);
    }
    auto& procedure = it->second;
    if (nullptr == procedure.Remediation())
    {
        return Error("Failed to get 'remediate' object");
    }

    auto error = procedure.UpdateUserParameters(payload);
    if (error)
    {
        return error.Value();
    }

    Evaluator evaluator(ruleName, procedure.Remediation(), procedure.Parameters(), *mContext);
    return evaluator.ExecuteRemediation();
}

Result<Status> Engine::MmiSet(const char* objectName, const std::string& payload)
{
    if (nullptr == objectName)
    {
        OSConfigTelemetryStatusTrace(mContext->GetTelemetryHandle(), "objectName", EINVAL);
        OsConfigLogError(Log(), "Object name is null");
        return Error("Invalid argument", EINVAL);
    }

    OsConfigLogDebug(Log(), "Engine::MmiSet(%s, %s)", objectName, payload.c_str());
    constexpr const char* remediatePrefix = "remediate";
    constexpr const char* initPrefix = "init";
    constexpr const char* procedurePrefix = "procedure";
    auto ruleName = std::string(objectName);
    if (ruleName.find(procedurePrefix) == 0)
    {
        auto error = SetProcedure(ruleName.substr(strlen(procedurePrefix)), payload);
        if (error)
        {
            return error.Value();
        }

        return Status::Compliant;
    }

    if (ruleName.find(initPrefix) == 0)
    {
        auto error = InitAudit(ruleName.substr(strlen(initPrefix)), payload);
        if (error)
        {
            OsConfigLogInfo(Log(), "Failed to init audit: %s", error->message.c_str());
            return error.Value();
        }

        return Status::Compliant;
    }

    if (ruleName.find(remediatePrefix) == 0)
    {
        return ExecuteRemediation(ruleName.substr(strlen(remediatePrefix)), payload);
    }

    OSConfigTelemetryStatusTrace(mContext->GetTelemetryHandle(), "objectName", EINVAL);
    OsConfigLogError(Log(), "Invalid object name: Must start with %s, %s or %s prefix", initPrefix, procedurePrefix, remediatePrefix);
    return Error("Invalid object name");
}
} // namespace ComplianceEngine
