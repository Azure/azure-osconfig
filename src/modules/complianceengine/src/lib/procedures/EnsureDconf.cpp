// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ProcedureMap.h>
#include <StringTools.h>
#include <algorithm>
#include <functional>
#include <map>

namespace ComplianceEngine
{
Result<Status> AuditEnsureDconf(const AuditEnsureDconfParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    const auto key = EscapeForShell(params.key);
    static const std::map<DConfOperation, std::function<bool(const std::string&, const std::string&)>> ops{
        {DConfOperation::Eq, [](const std::string& x, const std::string& y) { return x == y; }},
        {DConfOperation::Ne, [](const std::string& x, const std::string& y) { return x != y; }}};
    const auto op = ops.find(params.operation);
    if (op == ops.end())
    {
        return Error("Not supported operation '" + std::to_string(params.operation) + "'", EINVAL);
    }

    Result<std::string> dconfRead = context.ExecuteCommand("dconf read \"" + key + "\"");
    if (!dconfRead.HasValue())
    {
        return Error("Failed to execute dconf read " + key + " error: " + dconfRead.Error().message, dconfRead.Error().code);
    }
    auto dconfVal = dconfRead.Value();

    // remove newline character
    if (*dconfVal.rbegin() == '\n')
    {
        dconfVal.erase(dconfVal.size() - 1);
    }

    auto isCompliant = op->second(dconfVal, params.value);
    if (isCompliant)
    {
        return indicators.Compliant("Dconf read " + key + " " + std::to_string(params.operation) + " value " + params.value);
    }
    else
    {
        return indicators.NonCompliant("Dconf read " + key + " " + std::to_string(params.operation) + " value " + params.value);
    }
    return indicators.NonCompliant("Dconf read " + key + " " + std::to_string(params.operation) + " value " + params.value + " Imposible");
}

} // namespace ComplianceEngine
