// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_EVALUATOR_H
#define COMPLIANCEENGINE_EVALUATOR_H

#include "ContextInterface.h"
#include "Indicators.h"
#include "MmiResults.h"
#include "Result.h"

#include <Optional.h>
#include <functional>
#include <map>
#include <memory>
#include <string>

struct json_object_t;
struct json_value_t;

namespace ComplianceEngine
{

// Define Action enum outside of class for forward declarations
enum class Action
{
    Audit,
    Remediate
};

// Forward declaration
class LuaEvaluator;

class PayloadFormatter
{
public:
    virtual ~PayloadFormatter() = default;

    virtual Result<std::string> Format(const IndicatorsTree& indicators) const = 0;
};

class NestedListFormatter : public PayloadFormatter
{
    void FormatNode(const IndicatorsTree::Node& node, std::ostringstream& output, int depth) const;

public:
    ~NestedListFormatter() override = default;

    Result<std::string> Format(const IndicatorsTree& indicators) const override;
};

class CompactListFormatter : public PayloadFormatter
{
    void FormatNode(const IndicatorsTree::Node& node, std::ostringstream& output) const;

public:
    ~CompactListFormatter() override = default;

    Result<std::string> Format(const IndicatorsTree& indicators) const override;
};

class JsonFormatter : public PayloadFormatter
{
    Optional<Error> FormatNode(const IndicatorsTree::Node& node, json_value_t* jsonValue) const;

public:
    ~JsonFormatter() override = default;

    Result<std::string> Format(const IndicatorsTree& indicators) const override;
};

class DebugFormatter : public PayloadFormatter
{
    void FormatNode(const IndicatorsTree::Node& node, std::ostringstream& output) const;

public:
    ~DebugFormatter() override = default;
    Result<std::string> Format(const IndicatorsTree& indicators) const override;
};

class LastIncomplianceFormatter : public PayloadFormatter
{
    void FormatNode(const IndicatorsTree::Node& node, std::ostringstream& output) const;

public:
    ~LastIncomplianceFormatter() override = default;

    Result<std::string> Format(const IndicatorsTree& indicators) const override;
};

using ParameterMap = std::map<std::string, std::string>;
using action_func_t = std::function<Result<Status>(const ParameterMap&, IndicatorsTree&, ContextInterface&)>;
struct ProcedureActions
{
    action_func_t audit;
    action_func_t remediate;
};
using ProcedureMap = std::map<std::string, ProcedureActions>;

class Evaluator
{
public:
    Evaluator(std::string ruleName, const struct json_object_t* json, const ParameterMap& parameters, ContextInterface& context);
    ~Evaluator();
    Evaluator(const Evaluator&) = delete;
    Evaluator(Evaluator&&) = delete;
    Evaluator& operator=(const Evaluator&) = delete;
    Evaluator& operator=(Evaluator&&) = delete;

    Result<AuditResult> ExecuteAudit(const PayloadFormatter& formatter);
    Result<Status> ExecuteRemediation();

    // Make procedure map public for access from Lua scripts
    static const ProcedureMap mProcedureMap;

private:
    enum class ListAction
    {
        AnyOf,
        AllOf
    };

    Result<Status> EvaluateProcedure(const struct json_object_t* object, Action action);
    Result<Status> EvaluateList(const struct json_value_t* value, Action action, ListAction listAction);
    Result<Status> EvaluateNot(const struct json_value_t* value, Action action);
    Result<Status> EvaluateLua(const struct json_value_t* value, Action action);
    Result<Status> EvaluateBuiltinProcedure(const std::string& procedureName, const struct json_value_t* value, Action action);
    Result<std::map<std::string, std::string>> GetBuiltinProcedureArguments(const json_value_t* value) const;
    const struct json_object_t* mJson;
    const ParameterMap& mParameters;
    ContextInterface& mContext;
    static const size_t cLogstreamMaxSize = 4096;

    // List of indicators which determine the final state of the evaluation
    IndicatorsTree mIndicators;

    // Lua evaluator instance for this evaluator
    std::unique_ptr<LuaEvaluator> mLuaEvaluator;
};

} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_EVALUATOR_H
