// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_ENSURE_PACKAGE_INSTALLED_H
#define COMPLIANCEENGINE_PROCEDURES_ENSURE_PACKAGE_INSTALLED_H

#include <Evaluator.h>

namespace ComplianceEngine
{
enum class PackageManagerType
{
    /// label: autodetect
    Autodetect,

    /// label: rpm
    RPM,

    /// label: dpkg
    DPKG,
};

struct PackageInstalledParams
{
    /// Package name
    std::string packageName;

    /// Minimum package version to check against (optional)
    Optional<std::string> minPackageVersion;

    /// Package manager, autodetected by default
    /// pattern: ^(rpm|dpkg)$
    Optional<PackageManagerType> packageManager = PackageManagerType::Autodetect;

    /// Cache path
    Optional<std::string> test_cachePath = std::string("/var/lib/GuestConfig/ComplianceEnginePackageCache");
};

Result<Status> AuditPackageInstalled(const PackageInstalledParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_ENSURE_PACKAGE_INSTALLED_H
