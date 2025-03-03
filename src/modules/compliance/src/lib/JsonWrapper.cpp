// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "JsonWrapper.h"

#include "parson.h"

namespace compliance
{
void JsonWrapperDeleter::operator()(json_value_t* value) const
{
    json_value_free(value);
}

JsonWrapper parseJSON(const char* input)
{
    return JsonWrapper(json_parse_string(input), JsonWrapperDeleter());
}
JsonWrapper JSONFromString(const char* input)
{
    return JsonWrapper(json_value_init_string(input), JsonWrapperDeleter());
}
} // namespace compliance
