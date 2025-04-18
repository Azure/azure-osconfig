// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"

#include "JsonWrapper.h"
#include "Logging.h"
#include "Result.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <parson.h>
#include <sstream>
#include <string>
#include <utility>

namespace compliance
{
using std::map;
using std::string;

Evaluator::Evaluator(std::string ruleName, const struct json_object_t* json, const ParameterMap& parameters, ContextInterface& context)
    : mJson(json),
      mParameters(parameters),
      mContext(context)
{
    mIndicators.Push(std::move(ruleName));
}

Result<AuditResult> Evaluator::ExecuteAudit(const PayloadFormatter& formatter)
{
    auto result = EvaluateProcedure(mJson, Action::Audit);
    if (!result.HasValue())
    {
        OsConfigLogError(mContext.GetLogHandle(), "Evaluation failed: %s", result.Error().message.c_str());
        return result.Error();
    }

    mIndicators.Back().status = result.Value();
    mIndicators.Pop();
    auto payloadResult = formatter.Format(mIndicators);
    if (!payloadResult.HasValue())
    {
        OsConfigLogError(mContext.GetLogHandle(), "Failed to format payload: %s", payloadResult.Error().message.c_str());
        return AuditResult{result.Value(), std::string("Failed to format payload: ") + payloadResult.Error().message};
    }

    return AuditResult{result.Value(), payloadResult.Value()};
}

Result<Status> Evaluator::ExecuteRemediation()
{
    auto result = EvaluateProcedure(mJson, Action::Remediate);
    if (!result.HasValue())
    {
        OsConfigLogError(mContext.GetLogHandle(), "Evaluation failed: %s", result.Error().message.c_str());
        return result.Error();
    }

    mIndicators.Back().status = result.Value();
    mIndicators.Pop();
    return result;
}

Result<Status> Evaluator::EvaluateProcedure(const JSON_Object* object, const Action action)
{
    if (nullptr == object)
    {
        OsConfigLogError(mContext.GetLogHandle(), "invalid argument");
        return Error("invalid json argument", EINVAL);
    }

    const char* name = json_object_get_name(object, 0);
    const auto* value = json_object_get_value_at(object, 0);
    if ((nullptr == name) || (nullptr == value))
    {
        OsConfigLogError(mContext.GetLogHandle(), "Rule name or value is null");
        return Error("Rule name or value is null");
    }

    if (!strcmp(name, "anyOf") || !strcmp(name, "allOf"))
    {
        mIndicators.Push(name);
        const auto result = EvaluateList(value, action, !strcmp(name, "anyOf") ? ListAction::AnyOf : ListAction::AllOf);
        if (!result.HasValue())
        {
            OsConfigLogError(mContext.GetLogHandle(), "Evaluation failed: %s", result.Error().message.c_str());
            return result;
        }
        mIndicators.Back().status = result.Value();
        mIndicators.Pop();
        return result.Value();
    }

    if (!strcmp(name, "not"))
    {
        mIndicators.Push("not");
        const auto result = EvaluateNot(value, action);
        if (!result.HasValue())
        {
            OsConfigLogError(mContext.GetLogHandle(), "Evaluation failed: %s", result.Error().message.c_str());
            return result;
        }
        mIndicators.Back().status = result.Value();
        mIndicators.Pop();
        return result.Value();
    }

    mIndicators.Push(name);
    auto result = EvaluateBuiltinProcedure(name, value, action);
    if (!result.HasValue())
    {
        OsConfigLogError(mContext.GetLogHandle(), "Evaluation failed: %s", result.Error().message.c_str());
        return result;
    }
    mIndicators.Back().status = result.Value();
    mIndicators.Pop();

    return result.Value();
}

Result<Status> Evaluator::EvaluateList(const json_value_t* value, const Action action, const ListAction listAction)
{
    const char* actionName = listAction == ListAction::AnyOf ? "anyOf" : "allOf";
    OsConfigLogDebug(mContext.GetLogHandle(), "Evaluating %s operator", actionName);

    if (nullptr == value)
    {
        OsConfigLogError(mContext.GetLogHandle(), "invalid argument");
        return Error("invalid argument", EINVAL);
    }

    if (json_value_get_type(value) != JSONArray)
    {
        OsConfigLogError(mContext.GetLogHandle(), "%s value is not an array", actionName);
        return Error(std::string(actionName) + " value is not an array", EINVAL);
    }

    const auto* array = json_value_get_array(value);
    size_t count = json_array_get_count(array);
    for (size_t i = 0; i < count; ++i)
    {
        const auto* subObject = json_array_get_object(array, i);
        const auto result = EvaluateProcedure(subObject, action);
        if (!result.HasValue())
        {
            OsConfigLogError(mContext.GetLogHandle(), "Evaluation failed: %s", result.Error().message.c_str());
            return result;
        }

        if (result.Value() == Status::Compliant && listAction == ListAction::AnyOf)
        {
            OsConfigLogDebug(mContext.GetLogHandle(), "Evaluation returned compliant status at index %zu", i);
            return Status::Compliant;
        }

        if (result.Value() == Status::NonCompliant && listAction == ListAction::AllOf)
        {
            OsConfigLogDebug(mContext.GetLogHandle(), "Evaluation returned non-compliant status at index %zu", i);
            return Status::NonCompliant;
        }
    }

    return listAction == ListAction::AnyOf ? Status::NonCompliant : Status::Compliant;
}

Result<Status> Evaluator::EvaluateNot(const json_value_t* value, const Action action)
{
    OsConfigLogDebug(mContext.GetLogHandle(), "Evaluating not operator");

    if (nullptr == value)
    {
        OsConfigLogError(mContext.GetLogHandle(), "invalid argument");
        return Error("invalid argument", EINVAL);
    }

    if (json_value_get_type(value) != JSONObject)
    {
        OsConfigLogError(mContext.GetLogHandle(), "not value is not an object");
        return Error("not value is not an object", EINVAL);
    }

    // NOT can be only used as an audit!
    if (action != Action::Audit)
    {
        OsConfigLogInfo(mContext.GetLogHandle(), "not used in remediation: falling back to audit mode. Some issues may not be remediated.");
    }

    auto result = EvaluateProcedure(json_value_get_object(value), Action::Audit);
    if (!result.HasValue())
    {
        OsConfigLogError(mContext.GetLogHandle(), "Evaluation failed: %s", result.Error().message.c_str());
        return result;
    }

    if (result.Value() == Status::Compliant)
    {
        OsConfigLogDebug(mContext.GetLogHandle(), "Evaluation returned compliant status");
        return Status::NonCompliant;
    }

    OsConfigLogDebug(mContext.GetLogHandle(), "Evaluation returned non-compliant status");
    return Status::Compliant;
}

Result<map<string, string>> Evaluator::GetBuiltinProcedureArguments(const json_value_t* value) const
{
    map<string, string> result;

    if ((nullptr == value) || (json_value_get_type(value) != JSONObject))
    {
        OsConfigLogError(mContext.GetLogHandle(), "invalid argument");
        return Error("invalid argument", EINVAL);
    }

    const auto* args_object = json_value_get_object(value);
    const size_t count = json_object_get_count(args_object);
    for (size_t i = 0; i < count; ++i)
    {
        const char* key = json_object_get_name(args_object, i);
        JSON_Value* val = json_object_get_value_at(args_object, i);

        if ((nullptr == key) || (nullptr == val))
        {
            OsConfigLogError(mContext.GetLogHandle(), "Key or value is null");
            return Error("Key or value is null", EINVAL);
        }

        if (json_value_get_type(val) != JSONString)
        {
            OsConfigLogError(mContext.GetLogHandle(), "Argument type is not a string for a key '%s'", key);
            return Error("Argument type is not a string", EINVAL);
        }

        auto it = result.insert({key, json_value_get_string(val)}).first;
        const auto& paramValue = it->second;
        if (!paramValue.empty() && paramValue[0] == '$')
        {
            auto paramSubstitution = mParameters.find(paramValue.substr(1));
            if (paramSubstitution == mParameters.end())
            {
                OsConfigLogError(mContext.GetLogHandle(), "Unknown parameter '%s'", paramValue.c_str());
                return Error("Unknown parameter", EINVAL);
            }
            it->second = paramSubstitution->second;
        }
    }

    return result;
}

Result<Status> Evaluator::EvaluateBuiltinProcedure(const string& procedureName, const json_value_t* value, const Action action)
{
    OsConfigLogDebug(mContext.GetLogHandle(), "Evaluating builtin procedure '%s'", procedureName.c_str());

    if ((nullptr == value) || (json_value_get_type(value) != JSONObject))
    {
        OsConfigLogError(mContext.GetLogHandle(), "invalid argument");
        return Error("invalid argument");
    }

    auto arguments = GetBuiltinProcedureArguments(value);
    if (!arguments.HasValue())
    {
        OsConfigLogError(mContext.GetLogHandle(), "Failed to get builtin procedure arguments: %s", arguments.Error().message.c_str());
        return arguments.Error();
    }

    const auto procedure = mProcedureMap.find(procedureName);
    if (procedure == mProcedureMap.end())
    {
        OsConfigLogError(mContext.GetLogHandle(), "Unknown function '%s'", procedureName.c_str());
        return Error("Unknown function '" + procedureName + "'", ENOENT);
    }

    action_func_t fn = nullptr;
    if (action == Action::Remediate)
    {
        fn = procedure->second.remediate;
        if (nullptr == fn)
        {
            OsConfigLogInfo(mContext.GetLogHandle(), "No remediation function found for '%s', using audit function", procedureName.c_str());
            fn = procedure->second.audit;
        }
    }
    else
    {
        fn = procedure->second.audit;
    }
    if (nullptr == fn)
    {
        OsConfigLogError(mContext.GetLogHandle(), "Function not found");
        return Error("Function not found", ENOENT);
    }

    auto result = fn(arguments.Value(), mIndicators, mContext);
    if (!result.HasValue())
    {
        OsConfigLogError(mContext.GetLogHandle(), "Builtin procedure evaluation failed: %s", result.Error().message.c_str());
        return result.Error();
    }

    return result.Value();
}

void NestedListFormatter::FormatNode(const IndicatorsTree::Node& node, std::ostringstream& result, int depth) const
{
    for (const auto& child : node.children)
    {
        for (int i = 0; i < depth; ++i)
        {
            result << "  ";
        }
        result << "[Begin] " << child->procedureName << "\n";
        assert(child);
        FormatNode(*child.get(), result, depth + 1);
    }

    for (const auto& indicator : node.indicators)
    {
        for (int i = 0; i < depth; ++i)
        {
            result << "  ";
        }
        if (indicator.status == Status::Compliant)
        {
            result << "[Compliant] " << indicator.message << "\n";
        }
        else
        {
            result << "[NonCompliant] " << indicator.message << "\n";
        }
    }

    for (int i = 0; i < depth - 1; ++i)
    {
        result << "  ";
    }
    result << (node.status == Status::Compliant ? "[Compliant] " : "[NonCompliant] ") << node.procedureName << "\n";
}

Result<std::string> NestedListFormatter::Format(const IndicatorsTree& indicators) const
{
    std::ostringstream result;
    const auto* node = indicators.GetRootNode();
    assert(nullptr != node);
    result << "[Begin] " << node->procedureName << "\n";
    FormatNode(*node, result, 1);
    return result.str();
}

void CompactListFormatter::FormatNode(const IndicatorsTree::Node& node, std::ostringstream& result) const
{
    for (const auto& indicator : node.indicators)
    {
        if (indicator.status == Status::Compliant)
        {
            result << "[Compliant] " << indicator.message << "\n";
        }
        else
        {
            result << "[NonCompliant] " << indicator.message << "\n";
        }
    }

    for (const auto& child : node.children)
    {
        assert(child);
        FormatNode(*child.get(), result);
    }
}

Result<std::string> CompactListFormatter::Format(const IndicatorsTree& indicators) const
{
    std::ostringstream result;
    const auto* node = indicators.GetRootNode();
    assert(nullptr != node);
    FormatNode(*node, result);
    return result.str();
}

Optional<Error> JsonFormatter::FormatNode(const IndicatorsTree::Node& node, json_value_t* jsonValue) const
{
    auto* array = json_value_get_array(jsonValue);
    assert(nullptr != array);

    for (const auto& child : node.children)
    {
        assert(child);
        auto* value = json_value_init_object();
        if (nullptr == value)
        {
            return Error("Failed to create JSON object", ENOMEM);
        }

        auto* object = json_value_get_object(value);
        assert(nullptr != object);

        if (JSONSuccess != json_object_set_string(object, "procedure", child->procedureName.c_str()))
        {
            json_value_free(value);
            return Error("Failed to set JSON object string", ENOMEM);
        }

        if (JSONSuccess != json_object_set_string(object, "status", child->status == Status::Compliant ? "Compliant" : "NonCompliant"))
        {
            json_value_free(value);
            return Error("Failed to set JSON object string", ENOMEM);
        }

        auto* childArray = json_value_init_array();
        if (nullptr == childArray)
        {
            json_value_free(value);
            return Error("Failed to create JSON array", ENOMEM);
        }

        auto error = FormatNode(*child.get(), childArray);
        if (error)
        {
            json_value_free(value);
            json_value_free(childArray);
            return error;
        }

        if (JSONSuccess != json_object_set_value(object, "indicators", childArray))
        {
            json_value_free(value);
            json_value_free(childArray);
            return Error("Failed to set JSON object value", ENOMEM);
        }

        if (JSONSuccess != json_array_append_value(array, value))
        {
            json_value_free(value);
            return Error("Failed to append JSON value", ENOMEM);
        }
    }

    for (const auto& indicator : node.indicators)
    {
        auto* value = json_value_init_object();
        if (nullptr == value)
        {
            return Error("Failed to create JSON object", ENOMEM);
        }

        auto* object = json_value_get_object(value);
        assert(nullptr != object);

        if (JSONSuccess != json_object_set_string(object, "message", indicator.message.c_str()))
        {
            json_value_free(value);
            return Error("Failed to set JSON object string", ENOMEM);
        }

        if (JSONSuccess != json_object_set_string(object, "status", indicator.status == Status::Compliant ? "Compliant" : "NonCompliant"))
        {
            json_value_free(value);
            return Error("Failed to set JSON object string", ENOMEM);
        }

        if (JSONSuccess != json_array_append_value(array, value))
        {
            json_value_free(value);
            return Error("Failed to append JSON value", ENOMEM);
        }
    }

    return Optional<Error>();
}

Result<std::string> JsonFormatter::Format(const IndicatorsTree& indicators) const
{
    auto* json = json_value_init_array();
    if (nullptr == json)
    {
        return Error("Failed to create JSON object", ENOMEM);
    }

    const auto* node = indicators.GetRootNode();
    assert(nullptr != node);
    auto error = FormatNode(*node, json);
    if (error)
    {
        json_value_free(json);
        return error.Value();
    }

    // Convert the JSON value to a string
    char* jsonString = json_serialize_to_string_pretty(json);
    json_value_free(json);
    auto result = std::string(jsonString);
    json_free_serialized_string(jsonString);
    return result;
}

void MmiFormatter::FormatNode(const IndicatorsTree::Node& node, std::ostringstream& result) const
{
    if (node.procedureName == "anyOf" || node.procedureName == "allOf")
    {
        bool first = true;
        result << "{ " << node.procedureName << ": [";
        for (const auto& child : node.children)
        {
            if (!first)
            {
                result << ", ";
            }
            FormatNode(*child.get(), result);
            first = false;
        }
        result << "]} == ";
        if (node.status == Status::Compliant)
        {
            result << "TRUE";
        }
        else
        {
            result << "FALSE";
        }
    }
    else if (node.procedureName == "not")
    {
        result << "{ " << node.procedureName << ": ";
        FormatNode(*node.children[0].get(), result);
        result << "} == ";
        if (node.status == Status::Compliant)
        {
            result << "FALSE";
        }
        else
        {
            result << "TRUE";
        }
    }
    else
    {
        result << "{ " << node.procedureName << ": ";

        bool first = true;
        for (const auto& indicator : node.indicators)
        {
            if (!first)
            {
                result << ", ";
            }
            result << indicator.message;
            first = false;
        }
        result << " } == ";
        if (node.status == Status::Compliant)
        {
            result << "TRUE";
        }
        else
        {
            result << "FALSE";
        }
    }
}

Result<std::string> MmiFormatter::Format(const IndicatorsTree& indicators) const
{
    std::ostringstream result;
    const auto* node = indicators.GetRootNode();
    assert(nullptr != node);
    if (node->children.empty())
    {
        return Error("No children found", EINVAL);
    }
    node = node->children[0].get();
    assert(nullptr != node);
    FormatNode(*node, result);
    return (indicators.GetRootNode()->status == Status::Compliant ? "PASS" : "") + result.str();
}

} // namespace compliance
