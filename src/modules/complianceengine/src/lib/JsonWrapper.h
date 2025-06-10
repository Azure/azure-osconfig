// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_JSON_H
#define COMPLIANCEENGINE_JSON_H

#include <memory>

struct json_value_t;

namespace ComplianceEngine
{
struct JsonWrapperDeleter
{
    void operator()(json_value_t* value) const;
};
using JsonWrapper = std::unique_ptr<json_value_t, JsonWrapperDeleter>;

JsonWrapper ParseJson(const char* input);
JsonWrapper JSONFromString(const char* input);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_JSON_H
