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
const std::set<std::string> NO_PARAMS = {};

// Event parameter validation map
const std::unordered_map<std::string, std::pair<std::set<std::string>, std::set<std::string>>> EVENT_PARAMETER_SETS = {
    {"TODO", {NO_PARAMS, NO_PARAMS}},
};

} // namespace Telemetry
#endif // PARAMETER_SETS_HPP
