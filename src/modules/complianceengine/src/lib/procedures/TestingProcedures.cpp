// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>
#include <TestingProcedures.h>
#include <vector>

namespace ComplianceEngine
{
Result<Status> RemediateRemediationFailure(const TestingProcedureParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(context);
    if (params.message.HasValue())
    {
        return indicators.NonCompliant(params.message.Value());
    }
    return Status::NonCompliant;
}

Result<Status> RemediateRemediationSuccess(const TestingProcedureParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(context);
    if (params.message.HasValue())
    {
        return indicators.NonCompliant(params.message.Value());
    }
    return Status::Compliant;
}

Result<Status> AuditAuditFailure(const TestingProcedureParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(context);
    if (params.message.HasValue())
    {
        return indicators.NonCompliant(params.message.Value());
    }
    return Status::NonCompliant;
}

Result<Status> AuditAuditSuccess(const TestingProcedureParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(context);
    if (params.message.HasValue())
    {
        return indicators.Compliant(params.message.Value());
    }
    return Status::Compliant;
}

Result<Status> RemediateRemediationParametrized(const TestingProcedureParametrizedParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(indicators);
    OsConfigLogInfo(context.GetLogHandle(), "remediationParametrized: %s", params.result.c_str());
    if (params.result == "success")
    {
        return Status::Compliant;
    }
    else if (params.result == "failure")
    {
        return Status::NonCompliant;
    }

    return Error("Invalid 'result' parameter");
}

Result<Status> AuditAuditGetParamValues(const TestingProcedureGetParamValuesParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(context);
    using P = TestingProcedureGetParamValuesParams;
    const std::vector<std::pair<const char*, Optional<std::string> P::*>> keys = {{"KEY1", &P::KEY1}, {"KEY2", &P::KEY2}, {"KEY3", &P::KEY3}};
    bool first = true;
    std::string rv;
    for (const auto& key : keys)
    {
        const auto& member = params.*key.second;
        if (member.HasValue())
        {
            if (!first)
            {
                rv += ", ";
            }
            rv += std::string(key.first) + "=" + member.Value();
            first = false;
        }
    }
    return indicators.Compliant(rv);
}
} // namespace ComplianceEngine
