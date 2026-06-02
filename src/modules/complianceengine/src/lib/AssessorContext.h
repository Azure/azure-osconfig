// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_ASSESSORCONTEXT_H
#define COMPLIANCEENGINE_ASSESSORCONTEXT_H

#include "CommonContext.h"
#include "Logging.h"

namespace ComplianceEngine
{

namespace
{
constexpr char assessorStatePath[] = "/tmp/compliance-engine";
} // namespace

class AssessorContext : public CommonContext
{
public:
    AssessorContext(OsConfigLogHandle log)
        : CommonContext(log, assessorStatePath)
    {
    }
};

} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_ASSESSORCONTEXT_H
