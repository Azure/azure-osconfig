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

namespace ComplianceEngine
{
class ProcedureParameters : public std::map<std::string, std::string>
{
public:
    ProcedureParameters() = default;
    ProcedureParameters(const ProcedureParameters&) = default;
    ProcedureParameters(ProcedureParameters&&) = default;
    ProcedureParameters& operator=(const ProcedureParameters&) = default;
    ProcedureParameters& operator=(ProcedureParameters&&) = default;
    ~ProcedureParameters() = default;

    // Parse JSON representation of the input parameters
    static Result<ProcedureParameters> Parse(const json_object_t& input);

    // Parse key/value pairs in the form key1=value1 key2="value 2" key3='value 3'
    static Result<ProcedureParameters> Parse(const std::string& input);
};

class Procedure
{
    ProcedureParameters mParameters;
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

    void SetParameters(ProcedureParameters value);
    Optional<Error> UpdateUserParameters(const std::string& userParameters);
    Optional<Error> SetAudit(const json_value_t* rule);
    Optional<Error> SetRemediation(const json_value_t* rule);

private:
    Optional<Error> UpdateUserParameters(const ProcedureParameters& userParameters);
};
} // namespace ComplianceEngine

#endif // PROCEDURE_HPP
