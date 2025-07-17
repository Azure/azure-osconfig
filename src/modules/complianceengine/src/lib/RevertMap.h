// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_REVERT_MAP_H
#define COMPLIANCEENGINE_REVERT_MAP_H

#include <map>

namespace ComplianceEngine
{
template <typename K, typename V>
std::map<V, K> RevertMap(const std::map<K, V>& input)
{
    std::map<V, K> result;
    for (auto it = input.begin(); it != input.end(); ++it)
    {
        result.insert(std::make_pair(it->second, it->first));
    }
    return result;
}
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_REVERT_MAP_H
