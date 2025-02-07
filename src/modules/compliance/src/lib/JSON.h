// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_JSON_H
#define COMPLIANCE_JSON_H

#include <memory>

struct json_value_t;

namespace compliance
{
    struct JSONDeleter
    {
        void operator()(json_value_t* value) const;
    };
    using JSON = std::unique_ptr<json_value_t, JSONDeleter>;

    JSON parseJSON(const char* input);
} // namespace compliance

#endif // COMPLIANCE_JSON_H
