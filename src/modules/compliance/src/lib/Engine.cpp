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
static constexpr const char* cModuleInfo = "{\"Name\": \"Compliance\","
                                           "\"Description\": \"Provides functionality to audit and remediate Security Baseline policies on device\","
                                           "\"Manufacturer\": \"Microsoft\","
                                           "\"VersionMajor\": 2,"
                                           "\"VersionMinor\": 0,"
                                           "\"VersionInfo\": \"Dilithium\","
                                           "\"Components\": [\"Compliance\"],"
                                           "\"Lifetime\": 2,"
                                           "\"UserAccount\": 0}";

Engine::Engine(OsConfigLogHandle log) noexcept
    : mLog{log}
{
}

void Engine::setMaxPayloadSize(unsigned int value) noexcept
{
    mMaxPayloadSize = value;
}

unsigned int Engine::getMaxPayloadSize() const noexcept
{
    return mMaxPayloadSize;
}

OsConfigLogHandle Engine::log() const noexcept
{
    return mLog;
}

const char* Engine::getModuleInfo() noexcept
{
    return cModuleInfo;
}

Result<AuditResult> Engine::mmiGet(const char* objectName)
{
    if (nullptr == objectName)
    {
        return Error("Invalid argument", EINVAL);
    }

    OsConfigLogInfo(log(), "Engine::mmiGet(%s)", objectName);
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
    if (nullptr == procedure.audit())
    {
        return Error("Failed to get 'audit' object");
    }

    Evaluator evaluator(procedure.audit(), procedure.parameters(), log());
    return evaluator.ExecuteAudit();
}

Result<JsonWrapper> Engine::decodeB64JSON(const std::string& input) const
{
    auto decodedString = Base64Decode(input);
    if (!decodedString.has_value())
    {
        return decodedString.error();
    }
    auto result = json_parse_string(decodedString.value().c_str());
    if (nullptr == result)
    {
        return Error("Failed to parse JSON", EINVAL);
    }

    return JsonWrapper(result);
}

Optional<Error> Engine::setProcedure(const std::string& ruleName, const std::string& payload)
{
    if (ruleName.empty())
    {
        return Error("Rule name is empty", EINVAL);
    }

    mDatabase.erase(ruleName);
    auto ruleJSON = decodeB64JSON(payload);
    if (!ruleJSON.has_value())
    {
        return ruleJSON.error();
    }

    auto object = json_value_get_object(ruleJSON.value().get());
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
    auto error = procedure.setAudit(jsonValue);
    if (error)
    {
        return error.value();
    }
    if (nullptr == procedure.audit())
    {
        OsConfigLogError(log(), "Failed to copy 'audit' object");
        return Error("Out of memory");
    }

    jsonValue = json_object_get_value(object, "remediate");
    if (nullptr != jsonValue)
    {
        if (json_value_get_type(jsonValue) != JSONObject)
        {
            return Error("The 'remediate' value is not an object");
        }

        auto error = procedure.setRemediation(jsonValue);
        if (error)
        {
            return error.value();
        }
        if (nullptr == procedure.remediation())
        {
            OsConfigLogError(log(), "Failed to copy 'remediate' object");
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
                OsConfigLogError(log(), "Failed to get parameter name and value");
                return Error("Failed to get parameter name and value");
            }

            procedure.setParameter(key, val);
        }
    }
    mDatabase.emplace(std::move(ruleName), std::move(procedure));
    return Optional<Error>();
}

Optional<Error> Engine::initAudit(const std::string& ruleName, const std::string& payload)
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

    auto error = it->second.updateUserParameters(payload);
    if (error)
    {
        return error.value();
    }

    return Optional<Error>();
}

Result<Status> Engine::executeRemediation(const std::string& ruleName, const std::string& payload)
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
    if (nullptr == procedure.remediation())
    {
        return Error("Failed to get 'remediate' object");
    }

    auto error = procedure.updateUserParameters(payload);
    if (error)
    {
        return error.value();
    }

    Evaluator evaluator(procedure.remediation(), procedure.parameters(), log());
    return evaluator.ExecuteRemediation();
}

Result<Status> Engine::mmiSet(const char* objectName, const std::string& payload)
{
    if (nullptr == objectName)
    {
        OsConfigLogError(log(), "Object name is null");
        return Error("Invalid argument", EINVAL);
    }

    OsConfigLogInfo(log(), "Engine::mmiSet(%s, %s)", objectName, payload.c_str());
    constexpr const char* remediatePrefix = "remediate";
    constexpr const char* initPrefix = "init";
    constexpr const char* procedurePrefix = "procedure";
    auto ruleName = std::string(objectName);
    if (ruleName.find(procedurePrefix) == 0)
    {
        auto error = setProcedure(ruleName.substr(strlen(procedurePrefix)), payload);
        if (error)
        {
            return error.value();
        }

        return Status::Compliant;
    }

    if (ruleName.find(initPrefix) == 0)
    {
        auto error = initAudit(ruleName.substr(strlen(initPrefix)), payload);
        if (error)
        {
            OsConfigLogInfo(log(), "Failed to init audit: %s", error->message.c_str());
            return error.value();
        }

        return Status::Compliant;
    }

    if (ruleName.find(remediatePrefix) == 0)
    {
        return executeRemediation(ruleName.substr(strlen(remediatePrefix)), payload);
    }

    OsConfigLogError(log(), "Invalid object name: Must start with %s, %s or %s prefix", initPrefix, procedurePrefix, remediatePrefix);
    return Error("Invalid object name");
}
} // namespace compliance
