// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PARAMETER_SETS_HPP
#define PARAMETER_SETS_HPP

#include <string>
#include <set>
#include <unordered_map>

namespace Telemetry
{

// Define parameter sets for each EventWrite_ method
const std::set<std::string> BASELINE_COMPLETE_REQUIRED_PARAMS = {
    "DistroName",
    "BaselineName",
    "Mode",
    "DurationSeconds",
    "CorrelationId",
    "Timestamp"    // TODO:  include common params
};

const std::set<std::string> BASELINE_COMPLETE_OPTIONAL_PARAMS = {
    "Version"
};

const std::set<std::string> RULE_COMPLETE_REQUIRED_PARAMS = {
    "ComponentName",
    "ObjectName",
    "ObjectResult",
    "Microseconds",
    "DistroName",
    "CorrelationId",
    "Version",
    "Timestamp"    // TODO:  include common params
};
const std::set<std::string> RULE_COMPLETE_OPTIONAL_PARAMS = {
    // No optional params for now
};

// Event parameter validation map
const std::unordered_map<std::string, std::pair<std::set<std::string>, std::set<std::string>>> EVENT_PARAMETER_SETS = {
    {"CompletedBaseline", {BASELINE_COMPLETE_REQUIRED_PARAMS, BASELINE_COMPLETE_OPTIONAL_PARAMS}},
    {"RuleComplete", {RULE_COMPLETE_REQUIRED_PARAMS, RULE_COMPLETE_OPTIONAL_PARAMS}}
};

} // namespace Telemetry
#endif // PARAMETER_SETS_HPP
