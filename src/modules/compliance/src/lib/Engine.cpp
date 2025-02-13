// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Engine.h"

#include <CommonUtils.h>
#include "Evaluator.h"
#include "Base64.h"

#include "parson.h"
#include <string>
#include <vector>
#include <map>
#include <sstream>

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

    Engine::Engine(void* log) noexcept : mLog{ log }
    {
    }

    Engine::Engine() noexcept : mLog{ OpenLog(cLogFile, cRolledLogFile) }, mLocalLog{ true }
    {
    }

    Engine::~Engine()
    {
        if(mLocalLog)
        {
            CloseLog(&mLog);
        }
    }

    void Engine::setMaxPayloadSize(unsigned int value) noexcept
    {
        mMaxPayloadSize = value;
    }

    unsigned int Engine::getMaxPayloadSize() const noexcept
    {
        return mMaxPayloadSize;
    }

    OSCONFIG_LOG_HANDLE Engine::log() const noexcept
    {
        return mLog;
    }

    const char* Engine::getMoguleInfo() const noexcept
    {
        return cModuleInfo;
    }

    Result<Engine::AuditResult> Engine::mmiGet(const char* objectName)
    {
        if (nullptr == objectName)
        {
            return Error("Invalid argument", EINVAL);
        }

        OsConfigLogInfo(log(), "Engine::mmiGet(%s)", objectName);
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
        if (nullptr == procedure.audit())
        {
            return Error("Failed to get 'audit' object");
        }

        Evaluator evaluator(procedure.audit(), procedure.parameters(), log());
        auto rc = evaluator.ExecuteAudit(&result.payload, &result.payloadSize);
        if (!rc.has_value())
        {
            return rc.error();
        }

        result.result = rc.value();
        return Result<AuditResult>(std::move(result));
    }

    Result<JsonWrapper> Engine::decodeB64JSON(const char* input) const
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

    Optional<Error> Engine::setProcedure(const std::string& ruleName, const char* payload, const int payloadSizeBytes)
    {
        if (ruleName.empty())
        {
            return Error("Rule name is empty", EINVAL);
        }

        mDatabase.erase(ruleName);
        auto ruleJSON = decodeB64JSON(std::string(payload, payloadSizeBytes).c_str());
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
        if(nullptr == procedure.audit())
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

    Optional<Error> Engine::initAudit(const std::string& ruleName, const char* payload, const int payloadSizeBytes)
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

        auto error = it->second.updateUserParameters(std::string(payload, payloadSizeBytes));
        if (error)
        {
            return error.value();
        }

        return Optional<Error>();
    }

    Result<bool> Engine::executeRemediation(const std::string& ruleName, const char* payload, const int payloadSizeBytes)
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

        auto error = procedure.updateUserParameters(std::string(payload, payloadSizeBytes));
        if (error)
        {
            return error.value();
        }

        Evaluator evaluator(procedure.remediation(), procedure.parameters(), log());
        auto result = evaluator.ExecuteRemediation();
        if (!result.has_value())
        {
            return result;
        }

        return result;
    }

    Result<bool> Engine::mmiSet(const char* objectName, const char* payload, const int payloadSizeBytes)
    {
        if (nullptr == objectName)
        {
            OsConfigLogError(log(), "Object name is null");
            return Error("Invalid argument", EINVAL);
        }

        if ((nullptr == payload) || (payloadSizeBytes < 0))
        {
            OsConfigLogError(log(), "Invalid argument: payload is null or payloadSizeBytes is <= 0");
            return Error("Invalid argument", EINVAL);
        }

        OsConfigLogInfo(log(), "Engine::mmiSet(%s, %.*s)", objectName, payloadSizeBytes, payload);
        constexpr const char* remediatePrefix = "remediate";
        constexpr const char* initPrefix = "init";
        constexpr const char* procedurePrefix = "procedure";
        auto ruleName = std::string(objectName);
        if (ruleName.find(procedurePrefix) == 0)
        {
            auto error = setProcedure(ruleName.substr(strlen(procedurePrefix)), payload, payloadSizeBytes);
            if (error)
            {
                return error.value();
            }

            return true;
        }
        else if (ruleName.find(initPrefix) == 0)
        {
            auto error = initAudit(ruleName.substr(strlen(initPrefix)), payload, payloadSizeBytes);
            if(error)
            {
                OsConfigLogInfo(log(), "Failed to init audit: %s", error->message.c_str());
                return error.value();
            }

            return true;
        }
        else if (ruleName.find(remediatePrefix) == 0)
        {
            return executeRemediation(ruleName.substr(strlen(remediatePrefix)), payload, payloadSizeBytes);
        }

        OsConfigLogError(log(), "Invalid object name: Must start with %s, %s or %s prefix", initPrefix, procedurePrefix, remediatePrefix);
        return Error("Invalid object name");
    }
}
