// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "JsonWrapper.h"

#include <Base64.h>
#include <parson.h>

namespace ComplianceEngine
{
Result<JsonWrapper> JsonWrapper::FromString(const char* input)
{
    auto result = JsonWrapperPointerType(json_parse_string(input), &json_value_free);
    if (nullptr == result)
    {
        return Error("Failed to parse JSON", EINVAL);
    }

    return JsonWrapper(std::move(result));
}

Result<JsonWrapper> JsonWrapper::FromString(const std::string& input)
{
    return FromString(input.c_str());
}

Result<JsonWrapper> JsonWrapper::FromBase64(const std::string& input)
{
    const auto decodedString = Base64Decode(input);
    if (!decodedString.HasValue())
    {
        return decodedString.Error();
    }

    return FromString(decodedString.Value());
}

Result<JsonWrapper> JsonWrapper::FromJsonString(const std::string& input)
{
    auto result = JsonWrapperPointerType(json_value_init_string(input.c_str()), &json_value_free);
    if (nullptr == result)
    {
        return Error("Failed to parse JSON-encoded string", EINVAL);
    }

    if (JSONString != json_value_get_type(result.get()))
    {
        return Error("Failed to parse a JSON-encoded string: the parsed value is not a string", EINVAL);
    }

    return JsonWrapper(std::move(result));
}

JsonWrapper::JsonWrapper()
    : JsonWrapperPointerType(nullptr, &json_value_free)
{
}

JsonWrapper::JsonWrapper(JsonWrapperPointerType&& value)
    : JsonWrapperPointerType(std::move(value))
{
}
} // namespace ComplianceEngine
