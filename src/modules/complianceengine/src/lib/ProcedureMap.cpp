// This file is auto-generated. Do not edit manually.
#include <ProcedureMap.h>
#include <Bindings.h>
#include <RevertMap.h>

namespace ComplianceEngine
{
// ApparmorProfileState.h:15
const char* Bindings<ApparmorProfileStateParams>::names[] = {"enforce"};

// AuditdRules.h:22
const char* Bindings<AuditdRulesParams>::names[] = {"searchItem", "excludeOption", "requiredOptions"};

// CommandOutputMatch.h:43
const char* Bindings<CommandOutputMatchParams>::names[] = {"command", "awk", "regex", "type"};

// DconfValue.h:31
const char* Bindings<DconfValueParams>::names[] = {"key", "value", "operation"};

// FileExists.h:15
const char* Bindings<FileExistsParams>::names[] = {"filename"};

// FilePermissions.h:36
const char* Bindings<FilePermissionsParams>::names[] = {"filename", "owner", "group", "permissions", "mask", "behavior"};

// FilePermissions.h:68
const char* Bindings<FilePermissionsCollectionParams>::names[] = {"directory", "recurse", "ext", "owner", "group", "permissions", "mask", "behavior"};

// FileRegexMatch.h:64
const char* Bindings<AuditFileRegexMatchParams>::names[] = {"path", "filenamePattern", "matchOperation", "matchPattern", "stateOperation", "statePattern", "ignoreCase", "behavior"};

// FilesystemMountOption.h:31
const char* Bindings<FilesystemMountOptionParams>::names[] = {"mountpoint", "optionsSet", "optionsNotSet", "test_fstab", "test_mtab", "test_mount"};

// GsettingsValue.h:56
const char* Bindings<GsettingsValueParams>::names[] = {"schema", "key", "keyType", "operation", "value"};

// KernelModule.h:15
const char* Bindings<KernelModuleUnavailableParams>::names[] = {"moduleName"};

// LogFilePermissions.h:15
const char* Bindings<LogFilePermissionsParams>::names[] = {"path"};

// LoginDefsOption.h:23
const char* Bindings<LoginDefsOptionParams>::names[] = {"option", "value", "comparison"};

// MountPointExists.h:15
const char* Bindings<MountPointExistsParams>::names[] = {"mountPoint"};

// NoDuplicateEntries.h:24
const char* Bindings<NoDuplicateEntriesParams>::names[] = {"filename", "delimiter", "column", "context"};

// NoShellAccountsLocked.h:21
const char* Bindings<NoShellAccountsLockedParams>::names[] = {"excludeUsers", "skip_below_uid_min", "skip_invalid_shells"};

// PackageInstalled.h:37
const char* Bindings<PackageInstalledParams>::names[] = {"packageName", "minPackageVersion", "packageManager", "test_cachePath"};

// PasswordChangeDate.h:15
const char* Bindings<PasswordChangeDateParams>::names[] = {"test_etcShadowPath"};

// SCE.h:18
const char* Bindings<SCEParams>::names[] = {"scriptName", "ENVIRONMENT"};

// ShadowField.h:90
const char* Bindings<ShadowFieldParams>::names[] = {"username", "username_operation", "field", "value", "operation", "test_etcShadowPath"};

// SshKeyPermissions.h:24
const char* Bindings<SshKeyPermissionsParams>::names[] = {"type"};

// SshdOption.h:64
const char* Bindings<SshdOptionParams>::names[] = {"option", "value", "op", "mode", "readExtraConfigs"};

// SysctlValue.h:21
const char* Bindings<SysctlValueParams>::names[] = {"sysctlName", "value"};

// SystemdConfig.h:55
const char* Bindings<SystemdConfigValueParams>::names[] = {"parameter", "valueRegex", "op", "value", "file", "block", "dir", "passOnNotFound"};

// SystemdUnitState.h:28
const char* Bindings<SystemdUnitStateParams>::names[] = {"unitName", "ActiveState", "LoadState", "UnitFileState", "Unit"};

// TestingProcedures.h:15
const char* Bindings<TestingProcedureParams>::names[] = {"message"};

// TestingProcedures.h:29
const char* Bindings<TestingProcedureParametrizedParams>::names[] = {"result"};

// TestingProcedures.h:38
const char* Bindings<TestingProcedureGetParamValuesParams>::names[] = {"KEY1", "KEY2", "KEY3"};

// UfwStatus.h:16
const char* Bindings<AuditUfwStatusParams>::names[] = {"statusRegex"};

// UniqueGroupId.h:22
const char* Bindings<UniqueGroupIdParams>::names[] = {"group", "gid", "test_etcGroupPath"};

// UniqueUserId.h:26
const char* Bindings<UniqueUserIdParams>::names[] = {"username", "uid", "gid", "test_etcPasswdPath"};

// WirelessDisabled.h:15
const char* Bindings<WirelessDisabledParams>::names[] = {"test_sysfs_class_net"};

const ProcedureMap Evaluator::mProcedureMap = {
    {"ApparmorProfileState", {MakeHandler(AuditApparmorProfileState), nullptr}},
    {"AuditFailure", {MakeHandler(AuditAuditFailure), nullptr}},
    {"AuditGetParamValues", {MakeHandler(AuditAuditGetParamValues), nullptr}},
    {"AuditNotApplicable", {MakeHandler(AuditAuditNotApplicable), nullptr}},
    {"AuditSuccess", {MakeHandler(AuditAuditSuccess), nullptr}},
    {"AuditdRules", {MakeHandler(AuditAuditdRules), nullptr}},
    {"CommandOutputMatch", {MakeHandler(AuditCommandOutputMatch), nullptr}},
    {"DconfValue", {MakeHandler(AuditDconfValue), nullptr}},
    {"DefaultUmask", {MakeHandler(AuditDefaultUmask), nullptr}},
    {"FileExists", {MakeHandler(AuditFileExists), nullptr}},
    {"FilePermissions", {MakeHandler(AuditFilePermissions), MakeHandler(RemediateFilePermissions)}},
    {"FilePermissionsCollection", {MakeHandler(AuditFilePermissionsCollection), MakeHandler(RemediateFilePermissionsCollection)}},
    {"FileRegexMatch", {MakeHandler(AuditFileRegexMatch), nullptr}},
    {"FilesystemMountOption", {MakeHandler(AuditFilesystemMountOption), MakeHandler(RemediateFilesystemMountOption)}},
    {"FirewalldZoneTargets", {MakeHandler(AuditFirewalldZoneTargets), nullptr}},
    {"GsettingsValue", {MakeHandler(AuditGsettingsValue), nullptr}},
    {"Ip6tablesOpenPorts", {MakeHandler(AuditIp6tablesOpenPorts), nullptr}},
    {"IptablesOpenPorts", {MakeHandler(AuditIptablesOpenPorts), nullptr}},
    {"KernelModuleUnavailable", {MakeHandler(AuditKernelModuleUnavailable), nullptr}},
    {"LogFilePermissions", {MakeHandler(AuditLogFilePermissions), MakeHandler(RemediateLogFilePermissions)}},
    {"LoginDefsOption", {MakeHandler(AuditLoginDefsOption), nullptr}},
    {"MountPointExists", {MakeHandler(AuditMountPointExists), nullptr}},
    {"MtaLocalOnly", {MakeHandler(AuditMtaLocalOnly), nullptr}},
    {"NoDuplicateEntries", {MakeHandler(AuditNoDuplicateEntries), nullptr}},
    {"NoShadowPrimaryGroup", {MakeHandler(AuditNoShadowPrimaryGroup), nullptr}},
    {"NoShellAccountsLocked", {MakeHandler(AuditNoShellAccountsLocked), nullptr}},
    {"NoUnownedFiles", {MakeHandler(AuditNoUnownedFiles), nullptr}},
    {"NoWorldWritableFiles", {MakeHandler(AuditNoWorldWritableFiles), nullptr}},
    {"PackageInstalled", {MakeHandler(AuditPackageInstalled), nullptr}},
    {"PasswdGroupsExist", {MakeHandler(AuditPasswdGroupsExist), MakeHandler(RemediatePasswdGroupsExist)}},
    {"PasswordChangeDate", {MakeHandler(AuditPasswordChangeDate), nullptr}},
    {"RemediationFailure", {nullptr, MakeHandler(RemediateRemediationFailure)}},
    {"RemediationNotApplicable", {nullptr, MakeHandler(RemediateRemediationNotApplicable)}},
    {"RemediationParametrized", {nullptr, MakeHandler(RemediateRemediationParametrized)}},
    {"RemediationSuccess", {nullptr, MakeHandler(RemediateRemediationSuccess)}},
    {"RootPathSecurity", {MakeHandler(AuditRootPathSecurity), nullptr}},
    {"SCE", {MakeHandler(AuditSCE), MakeHandler(RemediateSCE)}},
    {"ShadowField", {MakeHandler(AuditShadowField), nullptr}},
    {"ShellTimeout", {MakeHandler(AuditShellTimeout), nullptr}},
    {"SshKeyPermissions", {MakeHandler(AuditSshKeyPermissions), MakeHandler(RemediateSshKeyPermissions)}},
    {"SshdOption", {MakeHandler(AuditSshdOption), nullptr}},
    {"SysctlValue", {MakeHandler(AuditSysctlValue), nullptr}},
    {"SystemAccountShell", {MakeHandler(AuditSystemAccountShell), nullptr}},
    {"SystemdConfigValue", {MakeHandler(AuditSystemdConfigValue), nullptr}},
    {"SystemdUnitState", {MakeHandler(AuditSystemdUnitState), nullptr}},
    {"UfwOpenPorts", {MakeHandler(AuditUfwOpenPorts), nullptr}},
    {"UfwStatus", {MakeHandler(AuditUfwStatus), nullptr}},
    {"UniqueGroupId", {MakeHandler(AuditUniqueGroupId), nullptr}},
    {"UniqueUserId", {MakeHandler(AuditUniqueUserId), nullptr}},
    {"UserDotFilePermissions", {MakeHandler(AuditUserDotFilePermissions), MakeHandler(RemediateUserDotFilePermissions)}},
    {"UserHomeDirectoryPermissions", {MakeHandler(AuditUserHomeDirectoryPermissions), MakeHandler(RemediateUserHomeDirectoryPermissions)}},
    {"WirelessDisabled", {MakeHandler(AuditWirelessDisabled), nullptr}},
    {"XdmcpDisabled", {MakeHandler(AuditXdmcpDisabled), nullptr}},
};
} // namespace ComplianceEngine

namespace std
{
string to_string(const ComplianceEngine::Behavior value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::Behavior>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::RegexType value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::RegexType>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::DConfOperation value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::DConfOperation>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::Operation value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::Operation>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::IgnoreCase value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::IgnoreCase>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::GsettingsKeyType value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::GsettingsKeyType>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::GsettingsOperationType value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::GsettingsOperationType>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::PackageManagerType value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::PackageManagerType>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::ComparisonOperation value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::ComparisonOperation>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::Field value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::Field>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::SshKeyType value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::SshKeyType>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::EnsureSshdOptionOperation value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::EnsureSshdOptionOperation>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::EnsureSshdOptionMode value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::EnsureSshdOptionMode>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

string to_string(const ComplianceEngine::SystemdConfigValueOperator value) noexcept(false)
{
    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::SystemdConfigValueOperator>();
    static const auto revmap = ComplianceEngine::RevertMap(map);
    const auto it = revmap.find(value);
    if (revmap.end() == it)
    {
        throw std::out_of_range("Invalid enum value");
    }
    return it->second;
}

} // namespace std
