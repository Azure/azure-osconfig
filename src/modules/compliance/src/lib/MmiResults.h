// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_MMI_RESULTS_H
#define COMPLIANCE_MMI_RESULTS_H

#include <string>

namespace compliance
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
} // namespace compliance

#endif // COMPLIANCE_MMI_RESULTS_H
