// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_TESTING_PROCEDURES_H
#define COMPLIANCEENGINE_PROCEDURES_TESTING_PROCEDURES_H

#include <Evaluator.h>

namespace ComplianceEngine
{
struct TestingProcedureParams
{
    /// The message to be logged
    Optional<std::string> message;
};

Result<Status> RemediateRemediationFailure(const TestingProcedureParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> RemediateRemediationSuccess(const TestingProcedureParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> AuditAuditFailure(const TestingProcedureParams& params, IndicatorsTree& indicators, ContextInterface& context);
Result<Status> AuditAuditSuccess(const TestingProcedureParams& params, IndicatorsTree& indicators, ContextInterface& context);

struct TestingProcedureParametrizedParams
{
    /// Expected remediation result - success or failure
    /// pattern: (success|failure)
    std::string result;
};

Result<Status> RemediateRemediationParametrized(const TestingProcedureParametrizedParams& params, IndicatorsTree& indicators, ContextInterface& context);

struct TestingProcedureGetParamValuesParams
{
    Optional<std::string> KEY1;
    Optional<std::string> KEY2;
    Optional<std::string> KEY3;
};

Result<Status> AuditAuditGetParamValues(const TestingProcedureGetParamValuesParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_TESTING_PROCEDURES_H
