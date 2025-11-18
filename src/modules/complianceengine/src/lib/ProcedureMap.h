// This file is auto-generated. Do not edit manually.
#ifndef COMPLIANCEENGINE_PROCEDURE_MAP_H
#define COMPLIANCEENGINE_PROCEDURE_MAP_H

#include <AuditdRulesCheck.h>
#include <EnsureAccountsWithoutShellAreLocked.h>
#include <EnsureAllGroupsFromEtcPasswdExistInEtcGroup.h>
#include <EnsureApparmorProfiles.h>
#include <EnsureDconf.h>
#include <EnsureDefaultShellTimeoutIsConfigured.h>
#include <EnsureDefaultUserUmaskIsConfigured.h>
#include <EnsureFileExists.h>
#include <EnsureFilePermissions.h>
#include <EnsureFilesystemOption.h>
#include <EnsureFirewallOpenPorts.h>
#include <EnsureGroupIsOnlyGroupWith.h>
#include <EnsureGsettings.h>
#include <EnsureInteractiveUsersDotFilesAccessIsConfigured.h>
#include <EnsureInteractiveUsersHomeDirectoriesAreConfigured.h>
#include <EnsureKernelModule.h>
#include <EnsureLogfileAccess.h>
#include <EnsureMTAsLocalOnly.h>
#include <EnsureMountPointExists.h>
#include <EnsureNoDuplicateEntriesExist.h>
#include <EnsureNoUnowned.h>
#include <EnsureNoUserHasPrimaryShadowGroup.h>
#include <EnsureNoWritables.h>
#include <EnsurePasswordChangeIsInPast.h>
#include <EnsureRootPath.h>
#include <EnsureShadowContains.h>
#include <EnsureSshKeyPerms.h>
#include <EnsureSshdOption.h>
#include <EnsureSysctl.h>
#include <EnsureSystemAccountsDoNotHaveValidShell.h>
#include <EnsureUserIsOnlyAccountWith.h>
#include <EnsureWirelessIsDisabled.h>
#include <EnsureXdmcp.h>
#include <ExecuteCommandGrep.h>
#include <FileRegexMatch.h>
#include <PackageInstalled.h>
#include <SCE.h>
#include <SystemdConfig.h>
#include <SystemdUnitState.h>
#include <TestingProcedures.h>
#include <UfwStatus.h>

namespace ComplianceEngine
{
// Forward declaration, defined in Bindings.h
template <typename Params>
struct Bindings;

// Forward declaration, defined in Bindings.h
template <typename Enum>
const std::map<std::string, Enum>& MapEnum();

// Maps the DConfOperation enum labels to the enum values.
template <>
inline const std::map<std::string, DConfOperation>& MapEnum<DConfOperation>()
{
    static const std::map<std::string, DConfOperation> map = {
        {"eq", DConfOperation::Eq},
        {"ne", DConfOperation::Ne},
    };
    return map;
}

// Maps the GsettingsKeyType enum labels to the enum values.
template <>
inline const std::map<std::string, GsettingsKeyType>& MapEnum<GsettingsKeyType>()
{
    static const std::map<std::string, GsettingsKeyType> map = {
        {"number", GsettingsKeyType::Number},
        {"string", GsettingsKeyType::String},
    };
    return map;
}

// Maps the GsettingsOperationType enum labels to the enum values.
template <>
inline const std::map<std::string, GsettingsOperationType>& MapEnum<GsettingsOperationType>()
{
    static const std::map<std::string, GsettingsOperationType> map = {
        {"eq", GsettingsOperationType::Equal},
        {"ne", GsettingsOperationType::NotEqual},
        {"lt", GsettingsOperationType::LessThan},
        {"gt", GsettingsOperationType::GreaterThan},
        {"is-unlocked", GsettingsOperationType::IsUnlocked},
    };
    return map;
}

// Maps the ComparisonOperation enum labels to the enum values.
template <>
inline const std::map<std::string, ComparisonOperation>& MapEnum<ComparisonOperation>()
{
    static const std::map<std::string, ComparisonOperation> map = {
        {"eq", ComparisonOperation::Equal},
        {"ne", ComparisonOperation::NotEqual},
        {"lt", ComparisonOperation::LessThan},
        {"le", ComparisonOperation::LessOrEqual},
        {"gt", ComparisonOperation::GreaterThan},
        {"ge", ComparisonOperation::GreaterOrEqual},
        {"match", ComparisonOperation::PatternMatch},
    };
    return map;
}

// Maps the Field enum labels to the enum values.
template <>
inline const std::map<std::string, Field>& MapEnum<Field>()
{
    static const std::map<std::string, Field> map = {
        {"username", Field::Username},
        {"password", Field::Password},
        {"last_change", Field::LastChange},
        {"min_age", Field::MinAge},
        {"max_age", Field::MaxAge},
        {"warn_period", Field::WarnPeriod},
        {"inactivity_period", Field::InactivityPeriod},
        {"expiration_date", Field::ExpirationDate},
        {"reserved", Field::Reserved},
        {"encryption_method", Field::EncryptionMethod},
    };
    return map;
}

// Maps the SshKeyType enum labels to the enum values.
template <>
inline const std::map<std::string, SshKeyType>& MapEnum<SshKeyType>()
{
    static const std::map<std::string, SshKeyType> map = {
        {"public", SshKeyType::Public},
        {"private", SshKeyType::Private},
    };
    return map;
}

// Maps the EnsureSshdOptionOperation enum labels to the enum values.
template <>
inline const std::map<std::string, EnsureSshdOptionOperation>& MapEnum<EnsureSshdOptionOperation>()
{
    static const std::map<std::string, EnsureSshdOptionOperation> map = {
        {"regex", EnsureSshdOptionOperation::Regex},
        {"match", EnsureSshdOptionOperation::Match},
        {"not_match", EnsureSshdOptionOperation::NotMatch},
        {"lt", EnsureSshdOptionOperation::LessThan},
        {"le", EnsureSshdOptionOperation::LessOrEqual},
        {"gt", EnsureSshdOptionOperation::GreaterThan},
        {"ge", EnsureSshdOptionOperation::GreaterOrEqual},
    };
    return map;
}

// Maps the EnsureSshdOptionMode enum labels to the enum values.
template <>
inline const std::map<std::string, EnsureSshdOptionMode>& MapEnum<EnsureSshdOptionMode>()
{
    static const std::map<std::string, EnsureSshdOptionMode> map = {
        {"regular", EnsureSshdOptionMode::Regular},
        {"all_matches", EnsureSshdOptionMode::AllMatches},
    };
    return map;
}

// Maps the RegexType enum labels to the enum values.
template <>
inline const std::map<std::string, RegexType>& MapEnum<RegexType>()
{
    static const std::map<std::string, RegexType> map = {
        {"P", RegexType::Perl},
        {"E", RegexType::Extended},
        {"Pv", RegexType::PerlInverted},
        {"Ev", RegexType::ExtendedInverted},
    };
    return map;
}

// Maps the Operation enum labels to the enum values.
template <>
inline const std::map<std::string, Operation>& MapEnum<Operation>()
{
    static const std::map<std::string, Operation> map = {
        {"pattern match", Operation::Match},
    };
    return map;
}

// Maps the Behavior enum labels to the enum values.
template <>
inline const std::map<std::string, Behavior>& MapEnum<Behavior>()
{
    static const std::map<std::string, Behavior> map = {
        {"all_exist", Behavior::AllExist},
        {"any_exist", Behavior::AnyExist},
        {"at_least_one_exists", Behavior::AtLeastOneExists},
        {"none_exist", Behavior::NoneExist},
        {"only_one_exists", Behavior::OnlyOneExists},
    };
    return map;
}

// Maps the IgnoreCase enum labels to the enum values.
template <>
inline const std::map<std::string, IgnoreCase>& MapEnum<IgnoreCase>()
{
    static const std::map<std::string, IgnoreCase> map = {
        {"matchPattern statePattern", IgnoreCase::Both},
        {"matchPattern", IgnoreCase::MatchPattern},
        {"statePattern", IgnoreCase::StatePattern},
    };
    return map;
}

// Maps the PackageManagerType enum labels to the enum values.
template <>
inline const std::map<std::string, PackageManagerType>& MapEnum<PackageManagerType>()
{
    static const std::map<std::string, PackageManagerType> map = {
        {"autodetect", PackageManagerType::Autodetect},
        {"rpm", PackageManagerType::RPM},
        {"dpkg", PackageManagerType::DPKG},
    };
    return map;
}

// Defines the bindings for the AuditAuditdRulesCheckParams structure.
template <>
struct Bindings<AuditAuditdRulesCheckParams>
{
    using T = AuditAuditdRulesCheckParams;
    static constexpr size_t size = 3;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::searchItem, &T::excludeOption, &T::requiredOptions);
};

// Defines the bindings for the AuditEnsureApparmorProfilesParams structure.
template <>
struct Bindings<AuditEnsureApparmorProfilesParams>
{
    using T = AuditEnsureApparmorProfilesParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::enforce);
};

// Defines the bindings for the AuditEnsureDconfParams structure.
template <>
struct Bindings<AuditEnsureDconfParams>
{
    using T = AuditEnsureDconfParams;
    static constexpr size_t size = 3;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::key, &T::value, &T::operation);
};

// Defines the bindings for the AuditEnsureFileExistsParams structure.
template <>
struct Bindings<AuditEnsureFileExistsParams>
{
    using T = AuditEnsureFileExistsParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::filename);
};

// Defines the bindings for the EnsureFilePermissionsParams structure.
template <>
struct Bindings<EnsureFilePermissionsParams>
{
    using T = EnsureFilePermissionsParams;
    static constexpr size_t size = 5;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::filename, &T::owner, &T::group, &T::permissions, &T::mask);
};

// Defines the bindings for the EnsureFilePermissionsCollectionParams structure.
template <>
struct Bindings<EnsureFilePermissionsCollectionParams>
{
    using T = EnsureFilePermissionsCollectionParams;
    static constexpr size_t size = 7;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::directory, &T::recurse, &T::ext, &T::owner, &T::group, &T::permissions, &T::mask);
};

// Defines the bindings for the EnsureFilesystemOptionParams structure.
template <>
struct Bindings<EnsureFilesystemOptionParams>
{
    using T = EnsureFilesystemOptionParams;
    static constexpr size_t size = 6;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::mountpoint, &T::optionsSet, &T::optionsNotSet, &T::test_fstab, &T::test_mtab, &T::test_mount);
};

// Defines the bindings for the EnsureGroupIsOnlyGroupWithParams structure.
template <>
struct Bindings<EnsureGroupIsOnlyGroupWithParams>
{
    using T = EnsureGroupIsOnlyGroupWithParams;
    static constexpr size_t size = 3;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::group, &T::gid, &T::test_etcGroupPath);
};

// Defines the bindings for the EnsureGsettingsParams structure.
template <>
struct Bindings<EnsureGsettingsParams>
{
    using T = EnsureGsettingsParams;
    static constexpr size_t size = 5;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::schema, &T::key, &T::keyType, &T::operation, &T::value);
};

// Defines the bindings for the EnsureKernelModuleUnavailableParams structure.
template <>
struct Bindings<EnsureKernelModuleUnavailableParams>
{
    using T = EnsureKernelModuleUnavailableParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::moduleName);
};

// Defines the bindings for the EnsureLogfileAccessParams structure.
template <>
struct Bindings<EnsureLogfileAccessParams>
{
    using T = EnsureLogfileAccessParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::path);
};

// Defines the bindings for the EnsureMountPointExistsParams structure.
template <>
struct Bindings<EnsureMountPointExistsParams>
{
    using T = EnsureMountPointExistsParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::mountPoint);
};

// Defines the bindings for the EnsureNoDuplicateEntriesExistParams structure.
template <>
struct Bindings<EnsureNoDuplicateEntriesExistParams>
{
    using T = EnsureNoDuplicateEntriesExistParams;
    static constexpr size_t size = 4;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::filename, &T::delimiter, &T::column, &T::context);
};

// Defines the bindings for the EnsurePasswordChangeIsInPastParams structure.
template <>
struct Bindings<EnsurePasswordChangeIsInPastParams>
{
    using T = EnsurePasswordChangeIsInPastParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::test_etcShadowPath);
};

// Defines the bindings for the EnsureShadowContainsParams structure.
template <>
struct Bindings<EnsureShadowContainsParams>
{
    using T = EnsureShadowContainsParams;
    static constexpr size_t size = 6;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::username, &T::username_operation, &T::field, &T::value, &T::operation, &T::test_etcShadowPath);
};

// Defines the bindings for the EnsureSshKeyPermsParams structure.
template <>
struct Bindings<EnsureSshKeyPermsParams>
{
    using T = EnsureSshKeyPermsParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::type);
};

// Defines the bindings for the EnsureSshdOptionParams structure.
template <>
struct Bindings<EnsureSshdOptionParams>
{
    using T = EnsureSshdOptionParams;
    static constexpr size_t size = 5;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::option, &T::value, &T::op, &T::mode, &T::readExtraConfigs);
};

// Defines the bindings for the EnsureSysctlParams structure.
template <>
struct Bindings<EnsureSysctlParams>
{
    using T = EnsureSysctlParams;
    static constexpr size_t size = 2;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::sysctlName, &T::value);
};

// Defines the bindings for the EnsureUserIsOnlyAccountWithParams structure.
template <>
struct Bindings<EnsureUserIsOnlyAccountWithParams>
{
    using T = EnsureUserIsOnlyAccountWithParams;
    static constexpr size_t size = 4;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::username, &T::uid, &T::gid, &T::test_etcPasswdPath);
};

// Defines the bindings for the EnsureWirelessIsDisabledParams structure.
template <>
struct Bindings<EnsureWirelessIsDisabledParams>
{
    using T = EnsureWirelessIsDisabledParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::test_sysfs_class_net);
};

// Defines the bindings for the ExecuteCommandGrepParams structure.
template <>
struct Bindings<ExecuteCommandGrepParams>
{
    using T = ExecuteCommandGrepParams;
    static constexpr size_t size = 4;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::command, &T::awk, &T::regex, &T::type);
};

// Defines the bindings for the AuditFileRegexMatchParams structure.
template <>
struct Bindings<AuditFileRegexMatchParams>
{
    using T = AuditFileRegexMatchParams;
    static constexpr size_t size = 8;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::path, &T::filenamePattern, &T::matchOperation, &T::matchPattern, &T::stateOperation, &T::statePattern, &T::ignoreCase, &T::behavior);
};

// Defines the bindings for the PackageInstalledParams structure.
template <>
struct Bindings<PackageInstalledParams>
{
    using T = PackageInstalledParams;
    static constexpr size_t size = 4;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::packageName, &T::minPackageVersion, &T::packageManager, &T::test_cachePath);
};

// Defines the bindings for the SCEParams structure.
template <>
struct Bindings<SCEParams>
{
    using T = SCEParams;
    static constexpr size_t size = 2;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::scriptName, &T::ENVIRONMENT);
};

// Defines the bindings for the SystemdParameterParams structure.
template <>
struct Bindings<SystemdParameterParams>
{
    using T = SystemdParameterParams;
    static constexpr size_t size = 4;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::parameter, &T::valueRegex, &T::file, &T::dir);
};

// Defines the bindings for the SystemdUnitStateParams structure.
template <>
struct Bindings<SystemdUnitStateParams>
{
    using T = SystemdUnitStateParams;
    static constexpr size_t size = 5;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::unitName, &T::ActiveState, &T::LoadState, &T::UnitFileState, &T::Unit);
};

// Defines the bindings for the TestingProcedureParams structure.
template <>
struct Bindings<TestingProcedureParams>
{
    using T = TestingProcedureParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::message);
};

// Defines the bindings for the TestingProcedureParametrizedParams structure.
template <>
struct Bindings<TestingProcedureParametrizedParams>
{
    using T = TestingProcedureParametrizedParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::result);
};

// Defines the bindings for the TestingProcedureGetParamValuesParams structure.
template <>
struct Bindings<TestingProcedureGetParamValuesParams>
{
    using T = TestingProcedureGetParamValuesParams;
    static constexpr size_t size = 3;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::KEY1, &T::KEY2, &T::KEY3);
};

// Defines the bindings for the AuditUfwStatusParams structure.
template <>
struct Bindings<AuditUfwStatusParams>
{
    using T = AuditUfwStatusParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::statusRegex);
};

} // namespace ComplianceEngine

namespace std
{
// Returns a string representation of the DConfOperation enum value.
string to_string(ComplianceEngine::DConfOperation value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the GsettingsKeyType enum value.
string to_string(ComplianceEngine::GsettingsKeyType value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the GsettingsOperationType enum value.
string to_string(ComplianceEngine::GsettingsOperationType value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the ComparisonOperation enum value.
string to_string(ComplianceEngine::ComparisonOperation value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the Field enum value.
string to_string(ComplianceEngine::Field value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the SshKeyType enum value.
string to_string(ComplianceEngine::SshKeyType value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the EnsureSshdOptionOperation enum value.
string to_string(ComplianceEngine::EnsureSshdOptionOperation value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the EnsureSshdOptionMode enum value.
string to_string(ComplianceEngine::EnsureSshdOptionMode value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the RegexType enum value.
string to_string(ComplianceEngine::RegexType value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the Operation enum value.
string to_string(ComplianceEngine::Operation value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the Behavior enum value.
string to_string(ComplianceEngine::Behavior value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the IgnoreCase enum value.
string to_string(ComplianceEngine::IgnoreCase value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the PackageManagerType enum value.
string to_string(ComplianceEngine::PackageManagerType value) noexcept(false); // NOLINT(*-identifier-naming)

} // namespace std
#endif // COMPLIANCEENGINE_PROCEDURE_MAP_H
