// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_MMI_RESULTS_H
#define COMPLIANCEENGINE_MMI_RESULTS_H

#include <string>

namespace ComplianceEngine
{
enum class Status
{
    Compliant,
    NonCompliant,
    NotApplicable
};

// Combines two statuses under allOf (conjunction) three-valued logic, matching
// Evaluator::EvaluateList(ListAction::AllOf): NonCompliant dominates, then
// NotApplicable is sticky, then Compliant. Used by the assessor to aggregate
// independent per-rule results into an overall benchmark status the same way the
// engine aggregates a rule's allOf sub-results.
inline Status CombineAllOf(Status a, Status b) noexcept
{
    if (Status::NonCompliant == a || Status::NonCompliant == b)
    {
        return Status::NonCompliant;
    }
    if (Status::NotApplicable == a || Status::NotApplicable == b)
    {
        return Status::NotApplicable;
    }
    return Status::Compliant;
}

struct AuditResult
{
    Status status = Status::NonCompliant;
    std::string payload;

    AuditResult(Status status, std::string payload)
        : status(status),
          payload(std::move(payload))
    {
    }
};
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_MMI_RESULTS_H
