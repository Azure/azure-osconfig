// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_UNIT_STATE_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_UNIT_STATE_H

#include <Evaluator.h>
#include <Pattern.h>

namespace ComplianceEngine
{
struct SystemdUnitStateParams
{
    /// Name of the systemd unit
    std::string unitName;

    /// value of systemd ActiveState of unitName to match
    Optional<Pattern> activeState;

    /// value of systemd LoadState of unitName to match
    Optional<Pattern> loadState;

    /// value of systemd UnitFileState of unitName to match
    Optional<Pattern> unitFileState;

    /// value of systemd property Unit, used in systemd.timer, name of unit to run when timer elapses
    Optional<Pattern> unit;
};

Result<Status> AuditSystemdUnitState(const SystemdUnitStateParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_SYSTEMD_UNIT_STATE_H
