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

#include <cerrno>
#include <cstring>
#include <map>
#include <memory>
#include <parson.h>
#include <string>
#include <utility>

namespace compliance
{
static constexpr const char* cLogFile = "/var/log/osconfig_compliance.log";
static constexpr const char* cRolledLogFile = "/var/log/osconfig_compliance.bak";
static constexpr const char* cModuleInfo = "{\"Name\": \"Compliance\","
                                           "\"Description\": \"Provides functionality to audit and remediate Security Baseline policies on device\","
                                           "\"Manufacturer\": \"Microsoft\","
                                           "\"VersionMajor\": 2,"
                                           "\"VersionMinor\": 0,"
                                           "\"VersionInfo\": \"Dilithium\","
                                           "\"Components\": [\"Compliance\"],"
                                           "\"Lifetime\": 2,"
                                           "\"UserAccount\": 0}";

Engine::Engine(void* log) noexcept
    : mLog{ log }
{
}

Engine::Engine() noexcept
    : mLog{ OpenLog(cLogFile, cRolledLogFile) }
    , mLocalLog{ true }
{
}

Engine::~Engine()
{
    if (mLocalLog)
    {
        CloseLog(&mLog);
    }
}

void Engine::SetMaxPayloadSize(unsigned int value) noexcept
{
    mMaxPayloadSize = value;
}

unsigned int Engine::GetMaxPayloadSize() const noexcept
{
    return mMaxPayloadSize;
}

OSCONFIG_LOG_HANDLE Engine::Log() const noexcept
{
    return mLog;
}

const char* Engine::GetModuleInfo() noexcept
{
    return cModuleInfo;
}

Result<Engine::AuditResult> Engine::MmiGet(const char* objectName)
{
    if (nullptr == objectName)
    {
        return Error("Invalid argument", EINVAL);
    }

    OsConfigLogInfo(Log(), "Engine::mmiGet(%s)", objectName);
    auto result = AuditResult();
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

    Evaluator evaluator(procedure.Audit(), procedure.Parameters(), Log());
    auto rc = evaluator.ExecuteAudit(&result.payload, &result.payloadSize);
    if (!rc.HasValue())
    {
        return rc.Error();
    }

    result.result = rc.Value();
    return Result<AuditResult>(std::move(result));
}

Result<JsonWrapper> Engine::DecodeB64Json(const char* input) const
{
    if (nullptr == input)
    {
        return Error("Input is null", EINVAL);
    }
    std::string inputStr(input);
    if ((inputStr.length() > 2) && (inputStr[0] == '"') && (inputStr[inputStr.length() - 1] == '"'))
    {
        inputStr = inputStr.substr(1, inputStr.length() - 2);
    }
    auto decodedString = Base64Decode(inputStr);
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

Optional<Error> Engine::SetProcedure(const std::string& ruleName, const char* payload, const int payloadSizeBytes)
{
    if (ruleName.empty())
    {
        return Error("Rule name is empty", EINVAL);
    }

    mDatabase.erase(ruleName);
    auto ruleJSON = DecodeB64Json(std::string(payload, payloadSizeBytes).c_str());
    if (!ruleJSON.HasValue())
    {
        return ruleJSON.Error();
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

        auto error = procedure.SetRemediation(jsonValue);
        if (error)
        {
            return error.Value();
        }
        if (nullptr == procedure.Remediation())
        {
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
                OsConfigLogError(Log(), "Failed to get parameter name and value");
                return Error("Failed to get parameter name and value");
            }

            procedure.SetParameter(key, val);
        }
    }
    mDatabase.emplace(std::move(ruleName), std::move(procedure));
    return Optional<Error>();
}

Optional<Error> Engine::InitAudit(const std::string& ruleName, const char* payload, const int payloadSizeBytes)
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

    auto error = it->second.UpdateUserParameters(std::string(payload, payloadSizeBytes));
    if (error)
    {
        return error.Value();
    }

    return Optional<Error>();
}

Result<bool> Engine::ExecuteRemediation(const std::string& ruleName, const char* payload, const int payloadSizeBytes)
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

    auto error = procedure.UpdateUserParameters(std::string(payload, payloadSizeBytes));
    if (error)
    {
        return error.Value();
    }

    Evaluator evaluator(procedure.Remediation(), procedure.Parameters(), Log());
    auto result = evaluator.ExecuteRemediation();
    if (!result.HasValue())
    {
        return result;
    }

    return result;
}

Result<bool> Engine::MmiSet(const char* objectName, const char* payload, const int payloadSizeBytes)
{
    if (nullptr == objectName)
    {
        OsConfigLogError(Log(), "Object name is null");
        return Error("Invalid argument", EINVAL);
    }

    if ((nullptr == payload) || (payloadSizeBytes < 0))
    {
        OsConfigLogError(Log(), "Invalid argument: payload is null or payloadSizeBytes is <= 0");
        return Error("Invalid argument", EINVAL);
    }

    OsConfigLogInfo(Log(), "Engine::mmiSet(%s, %.*s)", objectName, payloadSizeBytes, payload);
    constexpr const char* remediatePrefix = "remediate";
    constexpr const char* initPrefix = "init";
    constexpr const char* procedurePrefix = "procedure";
    auto ruleName = std::string(objectName);
    if (ruleName.find(procedurePrefix) == 0)
    {
        auto error = SetProcedure(ruleName.substr(strlen(procedurePrefix)), payload, payloadSizeBytes);
        if (error)
        {
            return error.Value();
        }

        return true;
    }
    else if (ruleName.find(initPrefix) == 0)
    {
        auto error = InitAudit(ruleName.substr(strlen(initPrefix)), payload, payloadSizeBytes);
        if (error)
        {
            OsConfigLogInfo(Log(), "Failed to init audit: %s", error->message.c_str());
            return error.Value();
        }

        return true;
    }
    else if (ruleName.find(remediatePrefix) == 0)
    {
        return ExecuteRemediation(ruleName.substr(strlen(remediatePrefix)), payload, payloadSizeBytes);
    }

    OsConfigLogError(Log(), "Invalid object name: Must start with %s, %s or %s prefix", initPrefix, procedurePrefix, remediatePrefix);
    return Error("Invalid object name");
}
} // namespace compliance
