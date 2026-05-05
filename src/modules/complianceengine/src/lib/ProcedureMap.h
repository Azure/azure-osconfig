// This file is auto-generated. Do not edit manually.
#ifndef COMPLIANCEENGINE_PROCEDURE_MAP_H
#define COMPLIANCEENGINE_PROCEDURE_MAP_H

#include <ApparmorProfileState.h>
#include <AuditdRules.h>
#include <CommandOutputMatch.h>
#include <DconfValue.h>
#include <DefaultUmask.h>
#include <FileExists.h>
#include <FilePermissions.h>
#include <FileRegexMatch.h>
#include <FilesystemMountOption.h>
#include <FirewallOpenPorts.h>
#include <FirewalldZoneTargets.h>
#include <GsettingsValue.h>
#include <KernelModule.h>
#include <LogFilePermissions.h>
#include <LoginDefsOption.h>
#include <MountPointExists.h>
#include <MtaLocalOnly.h>
#include <NoDuplicateEntries.h>
#include <NoShadowPrimaryGroup.h>
#include <NoShellAccountsLocked.h>
#include <NoUnownedFiles.h>
#include <NoWorldWritableFiles.h>
#include <PackageInstalled.h>
#include <PasswdGroupsExist.h>
#include <PasswordChangeDate.h>
#include <RootPathSecurity.h>
#include <SCE.h>
#include <ShadowField.h>
#include <ShellTimeout.h>
#include <SshKeyPermissions.h>
#include <SshdOption.h>
#include <SysctlValue.h>
#include <SystemAccountShell.h>
#include <SystemdConfig.h>
#include <SystemdUnitState.h>
#include <TestingProcedures.h>
#include <UfwStatus.h>
#include <UniqueGroupId.h>
#include <UniqueUserId.h>
#include <UserDotFilePermissions.h>
#include <UserHomeDirectoryPermissions.h>
#include <WirelessDisabled.h>
#include <XdmcpDisabled.h>

namespace ComplianceEngine
{
// Forward declaration, defined in Bindings.h
template <typename Params>
struct Bindings;

// Forward declaration, defined in Bindings.h
template <typename Enum>
const std::map<std::string, Enum>& MapEnum();

// Maps the Behavior enum labels to the enum values.
template <>
inline const std::map<std::string, Behavior>& MapEnum<Behavior>()
{
    static const std::map<std::string, Behavior> map = {
        {"check_if_exists", Behavior::CheckIfExists},
        {"at_least_one_exists", Behavior::AtLeastOneExists},
        {"all_exist", Behavior::AllExist},
        {"any_exist", Behavior::AnyExist},
        {"none_exist", Behavior::NoneExist},
        {"only_one_exists", Behavior::OnlyOneExists},
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

// Maps the Operation enum labels to the enum values.
template <>
inline const std::map<std::string, Operation>& MapEnum<Operation>()
{
    static const std::map<std::string, Operation> map = {
        {"pattern match", Operation::Match},
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

// Maps the SystemdConfigValueOperator enum labels to the enum values.
template <>
inline const std::map<std::string, SystemdConfigValueOperator>& MapEnum<SystemdConfigValueOperator>()
{
    static const std::map<std::string, SystemdConfigValueOperator> map = {
        {"lt", SystemdConfigValueOperator::LessThan},
        {"le", SystemdConfigValueOperator::LessOrEqual},
        {"gt", SystemdConfigValueOperator::GreaterThan},
        {"ge", SystemdConfigValueOperator::GreaterOrEqual},
        {"eq", SystemdConfigValueOperator::Equal},
    };
    return map;
}

// Defines the bindings for the ApparmorProfileStateParams structure.
template <>
struct Bindings<ApparmorProfileStateParams>
{
    using T = ApparmorProfileStateParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::enforce);
};

// Defines the bindings for the AuditdRulesParams structure.
template <>
struct Bindings<AuditdRulesParams>
{
    using T = AuditdRulesParams;
    static constexpr size_t size = 3;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::searchItem, &T::excludeOption, &T::requiredOptions);
};

// Defines the bindings for the CommandOutputMatchParams structure.
template <>
struct Bindings<CommandOutputMatchParams>
{
    using T = CommandOutputMatchParams;
    static constexpr size_t size = 4;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::command, &T::awk, &T::regex, &T::type);
};

// Defines the bindings for the DconfValueParams structure.
template <>
struct Bindings<DconfValueParams>
{
    using T = DconfValueParams;
    static constexpr size_t size = 3;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::key, &T::value, &T::operation);
};

// Defines the bindings for the FileExistsParams structure.
template <>
struct Bindings<FileExistsParams>
{
    using T = FileExistsParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::filename);
};

// Defines the bindings for the FilePermissionsParams structure.
template <>
struct Bindings<FilePermissionsParams>
{
    using T = FilePermissionsParams;
    static constexpr size_t size = 6;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::filename, &T::owner, &T::group, &T::permissions, &T::mask, &T::behavior);
};

// Defines the bindings for the FilePermissionsCollectionParams structure.
template <>
struct Bindings<FilePermissionsCollectionParams>
{
    using T = FilePermissionsCollectionParams;
    static constexpr size_t size = 8;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::directory, &T::recurse, &T::ext, &T::owner, &T::group, &T::permissions, &T::mask, &T::behavior);
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

// Defines the bindings for the FilesystemMountOptionParams structure.
template <>
struct Bindings<FilesystemMountOptionParams>
{
    using T = FilesystemMountOptionParams;
    static constexpr size_t size = 6;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::mountpoint, &T::optionsSet, &T::optionsNotSet, &T::test_fstab, &T::test_mtab, &T::test_mount);
};

// Defines the bindings for the GsettingsValueParams structure.
template <>
struct Bindings<GsettingsValueParams>
{
    using T = GsettingsValueParams;
    static constexpr size_t size = 5;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::schema, &T::key, &T::keyType, &T::operation, &T::value);
};

// Defines the bindings for the KernelModuleUnavailableParams structure.
template <>
struct Bindings<KernelModuleUnavailableParams>
{
    using T = KernelModuleUnavailableParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::moduleName);
};

// Defines the bindings for the LogFilePermissionsParams structure.
template <>
struct Bindings<LogFilePermissionsParams>
{
    using T = LogFilePermissionsParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::path);
};

// Defines the bindings for the LoginDefsOptionParams structure.
template <>
struct Bindings<LoginDefsOptionParams>
{
    using T = LoginDefsOptionParams;
    static constexpr size_t size = 3;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::option, &T::value, &T::comparison);
};

// Defines the bindings for the MountPointExistsParams structure.
template <>
struct Bindings<MountPointExistsParams>
{
    using T = MountPointExistsParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::mountPoint);
};

// Defines the bindings for the NoDuplicateEntriesParams structure.
template <>
struct Bindings<NoDuplicateEntriesParams>
{
    using T = NoDuplicateEntriesParams;
    static constexpr size_t size = 4;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::filename, &T::delimiter, &T::column, &T::context);
};

// Defines the bindings for the NoShellAccountsLockedParams structure.
template <>
struct Bindings<NoShellAccountsLockedParams>
{
    using T = NoShellAccountsLockedParams;
    static constexpr size_t size = 3;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::excludeUsers, &T::skip_below_uid_min, &T::skip_invalid_shells);
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

// Defines the bindings for the PasswordChangeDateParams structure.
template <>
struct Bindings<PasswordChangeDateParams>
{
    using T = PasswordChangeDateParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::test_etcShadowPath);
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

// Defines the bindings for the ShadowFieldParams structure.
template <>
struct Bindings<ShadowFieldParams>
{
    using T = ShadowFieldParams;
    static constexpr size_t size = 6;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::username, &T::username_operation, &T::field, &T::value, &T::operation, &T::test_etcShadowPath);
};

// Defines the bindings for the SshKeyPermissionsParams structure.
template <>
struct Bindings<SshKeyPermissionsParams>
{
    using T = SshKeyPermissionsParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::type);
};

// Defines the bindings for the SshdOptionParams structure.
template <>
struct Bindings<SshdOptionParams>
{
    using T = SshdOptionParams;
    static constexpr size_t size = 5;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::option, &T::value, &T::op, &T::mode, &T::readExtraConfigs);
};

// Defines the bindings for the SysctlValueParams structure.
template <>
struct Bindings<SysctlValueParams>
{
    using T = SysctlValueParams;
    static constexpr size_t size = 2;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::sysctlName, &T::value);
};

// Defines the bindings for the SystemdConfigValueParams structure.
template <>
struct Bindings<SystemdConfigValueParams>
{
    using T = SystemdConfigValueParams;
    static constexpr size_t size = 8;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::parameter, &T::valueRegex, &T::op, &T::value, &T::file, &T::block, &T::dir, &T::passOnNotFound);
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

// Defines the bindings for the UniqueGroupIdParams structure.
template <>
struct Bindings<UniqueGroupIdParams>
{
    using T = UniqueGroupIdParams;
    static constexpr size_t size = 3;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::group, &T::gid, &T::test_etcGroupPath);
};

// Defines the bindings for the UniqueUserIdParams structure.
template <>
struct Bindings<UniqueUserIdParams>
{
    using T = UniqueUserIdParams;
    static constexpr size_t size = 4;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::username, &T::uid, &T::gid, &T::test_etcPasswdPath);
};

// Defines the bindings for the WirelessDisabledParams structure.
template <>
struct Bindings<WirelessDisabledParams>
{
    using T = WirelessDisabledParams;
    static constexpr size_t size = 1;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::test_sysfs_class_net);
};

} // namespace ComplianceEngine

namespace std
{
// Returns a string representation of the Behavior enum value.
string to_string(ComplianceEngine::Behavior value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the RegexType enum value.
string to_string(ComplianceEngine::RegexType value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the DConfOperation enum value.
string to_string(ComplianceEngine::DConfOperation value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the Operation enum value.
string to_string(ComplianceEngine::Operation value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the IgnoreCase enum value.
string to_string(ComplianceEngine::IgnoreCase value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the GsettingsKeyType enum value.
string to_string(ComplianceEngine::GsettingsKeyType value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the GsettingsOperationType enum value.
string to_string(ComplianceEngine::GsettingsOperationType value) noexcept(false); // NOLINT(*-identifier-naming)

// Returns a string representation of the PackageManagerType enum value.
string to_string(ComplianceEngine::PackageManagerType value) noexcept(false); // NOLINT(*-identifier-naming)

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

// Returns a string representation of the SystemdConfigValueOperator enum value.
string to_string(ComplianceEngine::SystemdConfigValueOperator value) noexcept(false); // NOLINT(*-identifier-naming)

} // namespace std
#endif // COMPLIANCEENGINE_PROCEDURE_MAP_H
