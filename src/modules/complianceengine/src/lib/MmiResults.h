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
    NonCompliant
};

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
