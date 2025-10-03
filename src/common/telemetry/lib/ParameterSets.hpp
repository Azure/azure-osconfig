// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PARAMETER_SETS_HPP
#define PARAMETER_SETS_HPP

#include <string>
#include <set>
#include <unordered_map>

namespace Telemetry
{

// Common parameters used across all events
const std::set<std::string> COMMON_REQUIRED_PARAMS = {
    "DistroName",
    "CorrelationId",
    "Version",
    "Timestamp"
};
inline std::set<std::string> AddCommonParams(const std::set<std::string>& eventParams) {
    std::set<std::string> merged = COMMON_REQUIRED_PARAMS;
    merged.insert(eventParams.begin(), eventParams.end());
    return merged;
}

// BaselineRun
const std::set<std::string> BASELINE_RUN_SPECIFIC_REQUIRED_PARAMS = {
    "BaselineName",
    "Mode",
    "DurationSeconds"
};
const std::set<std::string> BASELINE_RUN_OPTIONAL_PARAMS = {
    // No optional params for now
};

// RuleComplete
const std::set<std::string> RULE_COMPLETE_SPECIFIC_REQUIRED_PARAMS = {
    "ComponentName",
    "ObjectName",
    "ObjectResult",
    "Microseconds"
};
const std::set<std::string> RULE_COMPLETE_OPTIONAL_PARAMS = {
    // No optional params for now
};

// StatusTrace
const std::set<std::string> STATUS_TRACE_SPECIFIC_REQUIRED_PARAMS = {
    "FileName",
    "LineNumber",
    "ScenarioName",
    "FunctionName",
    "RuleCodename",
    "CallingFunctionName",
    "Microseconds",
    "ResultCode"
};
const std::set<std::string> STATUS_TRACE_OPTIONAL_PARAMS = {
    // No optional params for now
};

// Event parameter validation map
const std::unordered_map<std::string, std::pair<std::set<std::string>, std::set<std::string>>> EVENT_PARAMETER_SETS = {
    {"BaselineRun", {AddCommonParams(BASELINE_RUN_SPECIFIC_REQUIRED_PARAMS), BASELINE_RUN_OPTIONAL_PARAMS}},
    {"RuleComplete", {AddCommonParams(RULE_COMPLETE_SPECIFIC_REQUIRED_PARAMS), RULE_COMPLETE_OPTIONAL_PARAMS}},
    {"StatusTrace", {AddCommonParams(STATUS_TRACE_SPECIFIC_REQUIRED_PARAMS), STATUS_TRACE_OPTIONAL_PARAMS}}
};

} // namespace Telemetry
#endif // PARAMETER_SETS_HPP
