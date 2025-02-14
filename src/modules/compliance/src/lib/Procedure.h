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

    const std::map<std::string, std::string>& parameters() const noexcept
    {
        return mParameters;
    }
    const json_object_t* audit() const noexcept;
    const json_object_t* remediation() const noexcept;

    void setParameter(const std::string& key, std::string value);
    Optional<Error> updateUserParameters(const std::string& userParameters);
    Optional<Error> setAudit(const json_value_t* rule);
    Optional<Error> setRemediation(const json_value_t* rule);
};
} // namespace compliance

#endif // PROCEDURE_HPP
