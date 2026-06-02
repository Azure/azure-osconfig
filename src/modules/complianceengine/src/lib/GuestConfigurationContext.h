// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_GUESTCONFIGURATIONCONTEXT_H
#define COMPLIANCEENGINE_GUESTCONFIGURATIONCONTEXT_H

#include "CommonContext.h"
#include "Logging.h"

namespace ComplianceEngine
{

namespace
{
constexpr char guestConfigStatePath[] = "/var/lib/GuestConfig";
} // namespace

class GuestConfigurationContext : public CommonContext
{
public:
    GuestConfigurationContext(OsConfigLogHandle log)
        : CommonContext(log, guestConfigStatePath)
    {
    }
};

} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_GUESTCONFIGURATIONCONTEXT_H
