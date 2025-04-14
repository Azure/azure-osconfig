// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"

#include "JsonWrapper.h"
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
Result<AuditResult> Evaluator::ExecuteAudit()
{
    auto result = EvaluateProcedure(mJson, Action::Audit);
    if (!result.HasValue())
    {
        OsConfigLogError(mContext->GetLogHandle(), "Evaluation failed: %s", result.Error().message.c_str());
        return result.Error();
    }
    std::string vlog;
    if (result.Value() == Status::Compliant)
    {
        vlog = "PASS" + mContext->ConsumeLogstream();
    }
    else
    {
        vlog = mContext->ConsumeLogstream();
    }
    return AuditResult{result.Value(), vlog};
}

Result<Status> Evaluator::ExecuteRemediation()
{
    auto result = EvaluateProcedure(mJson, Action::Remediate);
    if (!result.HasValue())
    {
        OsConfigLogError(mContext->GetLogHandle(), "Evaluation failed: %s", result.Error().message.c_str());
        return result.Error();
    }

    return result;
}

Result<Status> Evaluator::EvaluateProcedure(const JSON_Object* json, const Action action)
{
    std::ostream& logstream = mContext->GetLogstream();
    OsConfigLogHandle log = mContext->GetLogHandle();
    if (nullptr == json)
    {
        OsConfigLogError(log, "invalid json argument");
        return Error("invalid json argument");
    }

    const char* name = json_object_get_name(json, 0);
    JSON_Value* value = json_object_get_value_at(json, 0);
    if ((nullptr == name) || (nullptr == value))
    {
        OsConfigLogError(log, "Rule name or value is null");
        return Error("Rule name or value is null");
    }

    if (!strcmp(name, "anyOf"))
    {
        if (json_value_get_type(value) != JSONArray)
        {
            logstream << "ERROR: anyOf value is not an array";
            OsConfigLogError(log, "anyOf value is not an array");
            return Error("anyOf value is not an array");
        }
        JSON_Array* array = json_value_get_array(value);
        size_t count = json_array_get_count(array);
        logstream << "{ anyOf: [";
        for (size_t i = 0; i < count; ++i)
        {
            JSON_Object* subObject = json_array_get_object(array, i);
            auto result = EvaluateProcedure(subObject, action);
            if (!result.HasValue())
            {
                logstream << "] == FAILURE }";
                return result;
            }
            if (result.Value() == Status::NonCompliant)
            {
                if (i < count - 1)
                {
                    logstream << ", ";
                }
            }
            else
            {
                logstream << "] == TRUE }";
                return Status::Compliant;
            }
        }
        logstream << "] == FALSE }";
        return Status::NonCompliant;
    }
    else if (!strcmp(name, "allOf"))
    {
        if (json_value_get_type(value) != JSONArray)
        {
            logstream << "ERROR: allOf value is not an array";
            OsConfigLogError(log, "allOf value is not an array");
            return Error("allOf value is not an array");
        }
        auto array = json_value_get_array(value);
        size_t count = json_array_get_count(array);
        logstream << "{ allOf: [";
        for (size_t i = 0; i < count; ++i)
        {
            auto subObject = json_array_get_object(array, i);
            auto result = EvaluateProcedure(subObject, action);
            if (!result.HasValue())
            {
                logstream << "] == FAILURE }";
                return result;
            }
            if (result.Value() == Status::Compliant)
            {
                if (i < count - 1)
                {
                    logstream << ", ";
                }
            }
            else
            {
                logstream << "] == FALSE }";
                return Status::NonCompliant;
            }
        }
        logstream << "] == TRUE }";
        return Status::Compliant;
    }
    else if (!strcmp(name, "not"))
    {
        if (json_value_get_type(value) != JSONObject)
        {
            logstream << "ERROR: not value is not an object";
            return Error("not value is not an object");
        }
        logstream << "{ not: ";
        // NOT can be only used as an audit!
        auto result = EvaluateProcedure(json_value_get_object(value), Action::Audit);
        if (!result.HasValue())
        {
            logstream << "] == FAILURE }";
            return result;
        }
        if (result.Value() == Status::Compliant)
        {
            logstream << "] == FALSE }";
            return Status::NonCompliant;
        }
        else
        {
            logstream << "] == TRUE }";
            return Status::Compliant;
        }
    }
    else
    {
        if (json_value_get_type(value) != JSONObject)
        {
            logstream << "ERROR: value is not an object";
            OsConfigLogError(log, "value is not an object");
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
                logstream << "ERROR: Argument type is not a string '" << key << "' ";
                OsConfigLogError(log, "Argument type is not a string for a key '%s'", key);
                return Error("Argument type is not a string");
            }
            arguments[key] = json_value_get_string(val);
            if (!arguments[key].empty() && arguments[key][0] == '$')
            {
                auto f = mParameters.find(arguments[key].substr(1));
                if (f == mParameters.end())
                {
                    logstream << "ERROR: Unknown parameter " << arguments[key];
                    OsConfigLogError(log, "Unknown parameter '%s'", arguments[key].c_str());
                    return Error("Unknown parameter");
                }
                arguments[key] = f->second;
            }
        }

        auto procedure = mProcedureMap.find(name);
        if (procedure == mProcedureMap.end())
        {
            logstream << "ERROR: Unknown function " << name;
            OsConfigLogError(log, "Unknown function '%s'", name);
            return Error("Unknown function");
        }
        action_func_t fn = nullptr;
        if (action == Action::Remediate)
        {
            fn = procedure->second.second;
            if (nullptr == fn)
            {
                OsConfigLogInfo(log, "No remediation function found for '%s', using audit function", name);
                fn = procedure->second.first;
            }
        }
        else
        {
            fn = procedure->second.first;
        }
        if (nullptr == fn)
        {
            logstream << "ERROR: Function not found";
            OsConfigLogError(log, "Function not found");
            return Error("Function not found");
        }
        logstream << "{ " << name << ": ";
        auto result = fn(arguments, *mContext);
        logstream << " } == ";
        if (!result.HasValue())
        {
            logstream << "FAILURE";
            return {result.Error()};
        }

        if (result.Value() == true)
        {
            logstream << "TRUE";
        }
        else
        {
            logstream << "FALSE";
        }

        return result.Value() ? Status::Compliant : Status::NonCompliant;
    }
    return Error("Unreachable"); // unreachable
}

} // namespace compliance
