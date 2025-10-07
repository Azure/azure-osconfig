// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_JSON_H
#define COMPLIANCEENGINE_JSON_H

#include <Result.h>
#include <memory>
#include <parson.h>

struct json_value_t;

namespace ComplianceEngine
{
using JsonWrapperPointerType = std::unique_ptr<json_value_t, void (*)(json_value_t*)>;
struct JsonWrapper : public JsonWrapperPointerType
{
    // Parse a string-encoded JSON
    static Result<JsonWrapper> FromString(const char* input);

    // Parse a string-encoded JSON
    static Result<JsonWrapper> FromString(const std::string& input);

    // Parse a base64-encoded JSON
    static Result<JsonWrapper> FromBase64(const std::string& input);

    // Parse a JSON string as input (a JSON-encoded string)
    // The function guarantees that after successful parsing
    // the stored JSON value is a JSON string.
    static Result<JsonWrapper> FromJsonString(const std::string& input);

    JsonWrapper();
    JsonWrapper(const JsonWrapper&) = delete;
    JsonWrapper& operator=(const JsonWrapper&) = delete;
    JsonWrapper(JsonWrapper&&) = default;
    JsonWrapper& operator=(JsonWrapper&&) = default;
    ~JsonWrapper() = default;

private:
    // Construct the wrapper from an existing json_value_t pointer
    explicit JsonWrapper(JsonWrapperPointerType&& value);
};
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_JSON_H
