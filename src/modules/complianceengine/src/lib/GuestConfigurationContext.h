// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_GUESTCONFIGURATIONCONTEXT_H
#define COMPLIANCEENGINE_GUESTCONFIGURATIONCONTEXT_H

#include "CommonContext.h"
#include "Logging.h"

namespace ComplianceEngine
{

class GuestConfigurationContext : public CommonContext
{
public:
    GuestConfigurationContext(OsConfigLogHandle log)
        : CommonContext(log, sStatePath)
    {
    }

private:
    static constexpr char sStatePath[] = "/var/lib/GuestConfig";
};

} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_GUESTCONFIGURATIONCONTEXT_H
