// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"

#include "Logging.h"
#include "Result.h"

#include <cstring>
#include <map>
#include <parson.h>
#include <sstream>
#include <string>
#include <utility>

namespace compliance
{

Result<bool> Evaluator::ExecuteAudit(char** payload, int* payloadSizeBytes)
{
    if ((nullptr == payload) || (nullptr == payloadSizeBytes))
    {
        OsConfigLogError(mLog, "Invalid argument: payload or payloadSizeBytes is null");
        return Error("Payload or payloadSizeBytes is null");
    }

    auto result = EvaluateProcedure(mJson, Action::Audit);
    if (!result.has_value())
    {
        OsConfigLogError(mLog, "Evaluation failed: %s", result.error().message.c_str());
        return result.error();
    }

    std::string vlog = mLogstream.str().substr(0, cLogstreamMaxSize - (1 + strlen("PASS") + strlen("\"\"")));
    if (result.value() == true)
    {
        vlog = "\"PASS" + vlog + "\"";
    }
    else
    {
        vlog = "\"" + vlog + "\"";
    }
    *payload = strdup(vlog.c_str());
    *payloadSizeBytes = static_cast<int>(vlog.size());
    return result;
}

Result<bool> Evaluator::ExecuteRemediation()
{
    auto result = EvaluateProcedure(mJson, Action::Remediate);
    if (!result.has_value())
    {
        OsConfigLogError(mLog, "Evaluation failed: %s", result.error().message.c_str());
        return result.error();
    }

    return result;
}

Result<bool> Evaluator::EvaluateProcedure(const JSON_Object* json, const Action action)
{
    if (nullptr == json)
    {
        OsConfigLogError(mLog, "invalid json argument");
        return Error("invalid json argument");
    }

    const char* name = json_object_get_name(json, 0);
    JSON_Value* value = json_object_get_value_at(json, 0);
    if ((nullptr == name) || (nullptr == value))
    {
        OsConfigLogError(mLog, "Rule name or value is null");
        return Error("Rule name or value is null");
    }

    if (!strcmp(name, "anyOf"))
    {
        if (json_value_get_type(value) != JSONArray)
        {
            mLogstream << "ERROR: anyOf value is not an array";
            OsConfigLogError(mLog, "anyOf value is not an array");
            return Error("anyOf value is not an array");
        }
        JSON_Array* array = json_value_get_array(value);
        size_t count = json_array_get_count(array);
        mLogstream << "{ anyOf: [";
        for (size_t i = 0; i < count; ++i)
        {
            JSON_Object* subObject = json_array_get_object(array, i);
            auto result = EvaluateProcedure(subObject, action);
            if (!result.has_value())
            {
                mLogstream << "] == FAILURE }";
                return result;
            }
            if (result.value() == false)
            {
                if (i < count - 1)
                {
                    mLogstream << ", ";
                }
            }
            else
            {
                mLogstream << "] == TRUE }";
                return true;
            }
        }
        mLogstream << "] == FALSE }";
        return false;
    }
    else if (!strcmp(name, "allOf"))
    {
        if (json_value_get_type(value) != JSONArray)
        {
            mLogstream << "ERROR: allOf value is not an array";
            OsConfigLogError(mLog, "allOf value is not an array");
            return Error("allOf value is not an array");
        }
        auto array = json_value_get_array(value);
        size_t count = json_array_get_count(array);
        mLogstream << "{ allOf: [";
        for (size_t i = 0; i < count; ++i)
        {
            auto subObject = json_array_get_object(array, i);
            auto result = EvaluateProcedure(subObject, action);
            if (!result.has_value())
            {
                mLogstream << "] == FAILURE }";
                return result;
            }
            if (result.value() == true)
            {
                if (i < count - 1)
                {
                    mLogstream << ", ";
                }
            }
            else
            {
                mLogstream << "] == FALSE }";
                return false;
            }
        }
        mLogstream << "] == TRUE }";
        return true;
    }
    else if (!strcmp(name, "not"))
    {
        if (json_value_get_type(value) != JSONObject)
        {
            mLogstream << "ERROR: not value is not an object";
            return Error("not value is not an object");
        }
        mLogstream << "{ not: ";
        // NOT can be only used as an audit!
        auto result = EvaluateProcedure(json_value_get_object(value), Action::Audit);
        if (!result.has_value())
        {
            mLogstream << "] == FAILURE }";
            return result;
        }
        if (result.value() == true)
        {
            mLogstream << "] == FALSE }";
            return false;
        }
        else
        {
            mLogstream << "] == TRUE }";
            return true;
        }
    }
    else
    {
        if (json_value_get_type(value) != JSONObject)
        {
            mLogstream << "ERROR: value is not an object";
            OsConfigLogError(mLog, "value is not an object");
            return Error("value is not an object");
        }
        std::map<std::string, std::string> arguments;
        auto args_object = json_value_get_object(value);
        size_t count = json_object_get_count(args_object);
        for (size_t i = 0; i < count; ++i)
        {
            const char* key = json_object_get_name(args_object, i);
            JSON_Value* val = json_object_get_value_at(args_object, i);
            if (json_value_get_type(val) != JSONString)
            {
                mLogstream << "ERROR: Argument type is not a string '" << key << "' ";
                OsConfigLogError(mLog, "Argument type is not a string for a key '%s'", key);
                return Error("Argument type is not a string");
            }
            arguments[key] = json_value_get_string(val);
            if (!arguments[key].empty() && arguments[key][0] == '$')
            {
                auto f = mParameters.find(arguments[key].substr(1));
                if (f == mParameters.end())
                {
                    mLogstream << "ERROR: Unknown parameter " << arguments[key];
                    OsConfigLogError(mLog, "Unknown parameter '%s'", arguments[key].c_str());
                    return Error("Unknown parameter");
                }
                arguments[key] = f->second;
            }
        }

        auto procedure = mProcedureMap.find(name);
        if (procedure == mProcedureMap.end())
        {
            mLogstream << "ERROR: Unknown function " << name;
            OsConfigLogError(mLog, "Unknown function '%s'", name);
            return Error("Unknown function");
        }
        action_func_t fn;
        if (action == Action::Remediate)
        {
            fn = procedure->second.second;
            if (nullptr == fn)
            {
                OsConfigLogInfo(mLog, "No remediation function found for '%s', using audit function", name);
                fn = procedure->second.first;
            }
        }
        else
        {
            fn = procedure->second.first;
        }
        if (nullptr == fn)
        {
            mLogstream << "ERROR: Function not found";
            OsConfigLogError(mLog, "Function not found");
            return Error("Function not found");
        }
        mLogstream << "{ " << name << ": ";
        auto result = fn(arguments, mLogstream);
        mLogstream << " } == ";
        if (!result.has_value())
        {
            mLogstream << "FAILURE";
        }
        else if (result.value() == true)
        {
            mLogstream << "TRUE";
        }
        else
        {
            mLogstream << "FALSE";
        }
        return result;
    }
    return Error("Unreachable"); // unreachable
}

} // namespace compliance
