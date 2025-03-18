// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PROCEDURE_HPP
#define PROCEDURE_HPP

#include "JsonWrapper.h"
#include "Optional.h"
#include "Result.h"

#include <map>
#include <string>

struct json_value_t;
struct json_object_t;

namespace compliance
{
class Procedure
{
    std::map<std::string, std::string> mParameters;
    JsonWrapper mAuditRule;
    JsonWrapper mRemediationRule;

public:
    Procedure() = default;
    ~Procedure() = default;

    Procedure(const Procedure&) = delete;
    Procedure& operator=(const Procedure&) = delete;
    Procedure(Procedure&&) = default;
    Procedure& operator=(Procedure&&) = default;

    const std::map<std::string, std::string>& Parameters() const noexcept
    {
        return mParameters;
    }
    const json_object_t* Audit() const noexcept;
    const json_object_t* Remediation() const noexcept;

    void SetParameter(const std::string& key, std::string value);
    Optional<Error> UpdateUserParameters(const std::string& userParameters);
    Optional<Error> SetAudit(const json_value_t* rule);
    Optional<Error> SetRemediation(const json_value_t* rule);
};
} // namespace compliance

#endif // PROCEDURE_HPP
