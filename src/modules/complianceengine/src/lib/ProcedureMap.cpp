// This file is auto-generated. Do not edit manually.
#include <ProcedureMap.h>
#include <Bindings.h>
#include <RevertMap.h>

namespace ComplianceEngine
{
// AuditdRulesCheck.h:22
const char* Bindings<AuditAuditdRulesCheckParams>::names[] = {"searchItem", "excludeOption", "requiredOptions"};

// EnsureApparmorProfiles.h:15
const char* Bindings<AuditEnsureApparmorProfilesParams>::names[] = {"enforce"};

// EnsureDconf.h:31
const char* Bindings<AuditEnsureDconfParams>::names[] = {"key", "value", "operation"};

// EnsureFileExists.h:15
const char* Bindings<AuditEnsureFileExistsParams>::names[] = {"filename"};

// EnsureFilePermissions.h:31
const char* Bindings<EnsureFilePermissionsParams>::names[] = {"filename", "owner", "group", "permissions", "mask"};

// EnsureFilePermissions.h:57
const char* Bindings<EnsureFilePermissionsCollectionParams>::names[] = {"directory", "ext", "owner", "group", "permissions", "mask"};

// EnsureFilesystemOption.h:31
const char* Bindings<EnsureFilesystemOptionParams>::names[] = {"mountpoint", "optionsSet", "optionsNotSet", "test_fstab", "test_mtab", "test_mount"};

// EnsureGroupIsOnlyGroupWith.h:22
const char* Bindings<EnsureGroupIsOnlyGroupWithParams>::names[] = {"group", "gid", "test_etcGroupPath"};

// EnsureGsettings.h:56
const char* Bindings<EnsureGsettingsParams>::names[] = {"schema", "key", "keyType", "operation", "value"};

// EnsureKernelModule.h:15
const char* Bindings<EnsureKernelModuleUnavailableParams>::names[] = {"moduleName"};

// EnsureLogfileAccess.h:15
const char* Bindings<EnsureLogfileAccessParams>::names[] = {"path"};

// EnsureMountPointExists.h:15
const char* Bindings<EnsureMountPointExistsParams>::names[] = {"mountPoint"};

// EnsureNoDuplicateEntriesExist.h:24
const char* Bindings<EnsureNoDuplicateEntriesExistParams>::names[] = {"filename", "delimiter", "column", "context"};

// EnsurePasswordChangeIsInPast.h:15
const char* Bindings<EnsurePasswordChangeIsInPastParams>::names[] = {"test_etcShadowPath"};

// EnsureShadowContains.h:90
const char* Bindings<EnsureShadowContainsParams>::names[] = {"username", "username_operation", "field", "value", "operation", "test_etcShadowPath"};

// EnsureSshKeyPerms.h:24
const char* Bindings<EnsureSshKeyPermsParams>::names[] = {"type"};

// EnsureSshdOption.h:61
const char* Bindings<EnsureSshdOptionParams>::names[] = {"option", "value", "op", "mode"};

// EnsureSysctl.h:20
const char* Bindings<EnsureSysctlParams>::names[] = {"sysctlName", "value"};

// EnsureUserIsOnlyAccountWith.h:26
const char* Bindings<EnsureUserIsOnlyAccountWithParams>::names[] = {"username", "uid", "gid", "test_etcPasswdPath"};

// EnsureWirelessIsDisabled.h:15
const char* Bindings<EnsureWirelessIsDisabledParams>::names[] = {"test_sysfs_class_net"};

// ExecuteCommandGrep.h:35
const char* Bindings<ExecuteCommandGrepParams>::names[] = {"command", "awk", "regex", "type"};

// FileRegexMatch.h:83
const char* Bindings<AuditFileRegexMatchParams>::names[] = {"path", "filenamePattern", "matchOperation", "matchPattern", "stateOperation", "statePattern", "ignoreCase", "behavior"};

// PackageInstalled.h:37
const char* Bindings<PackageInstalledParams>::names[] = {"packageName", "minPackageVersion", "packageManager", "test_cachePath"};

// SCE.h:18
const char* Bindings<SCEParams>::names[] = {"scriptName", "ENVIRONMENT"};

// SystemdConfig.h:25
const char* Bindings<SystemdParameterParams>::names[] = {"parameter", "valueRegex", "file", "dir"};

// SystemdUnitState.h:28
const char* Bindings<SystemdUnitStateParams>::names[] = {"unitName", "ActiveState", "LoadState", "UnitFileState", "Unit"};

// TestingProcedures.h:15
const char* Bindings<TestingProcedureParams>::names[] = {"message"};

// TestingProcedures.h:27
const char* Bindings<TestingProcedureParametrizedParams>::names[] = {"result"};

// TestingProcedures.h:36
const char* Bindings<TestingProcedureGetParamValuesParams>::names[] = {"KEY1", "KEY2", "KEY3"};

// UfwStatus.h:16
const char* Bindings<AuditUfwStatusParams>::names[] = {"statusRegex"};

const ProcedureMap Evaluator::mProcedureMap = {
    {"AuditFailure", {MakeHandler(AuditAuditFailure), nullptr}},
    {"AuditGetParamValues", {MakeHandler(AuditAuditGetParamValues), nullptr}},
    {"AuditSuccess", {MakeHandler(AuditAuditSuccess), nullptr}},
    {"AuditdRulesCheck", {MakeHandler(AuditAuditdRulesCheck), nullptr}},
    {"EnsureAccountsWithoutShellAreLocked", {MakeHandler(AuditEnsureAccountsWithoutShellAreLocked), nullptr}},
    {"EnsureAllGroupsFromEtcPasswdExistInEtcGroup", {MakeHandler(AuditEnsureAllGroupsFromEtcPasswdExistInEtcGroup), MakeHandler(RemediateEnsureAllGroupsFromEtcPasswdExistInEtcGroup)}},
    {"EnsureApparmorProfiles", {MakeHandler(AuditEnsureApparmorProfiles), nullptr}},
    {"EnsureDconf", {MakeHandler(AuditEnsureDconf), nullptr}},
    {"EnsureDefaultShellTimeoutIsConfigured", {MakeHandler(AuditEnsureDefaultShellTimeoutIsConfigured), nullptr}},
    {"EnsureDefaultUserUmaskIsConfigured", {MakeHandler(AuditEnsureDefaultUserUmaskIsConfigured), nullptr}},
    {"EnsureFileExists", {MakeHandler(AuditEnsureFileExists), nullptr}},
    {"EnsureFilePermissions", {MakeHandler(AuditEnsureFilePermissions), MakeHandler(RemediateEnsureFilePermissions)}},
    {"EnsureFilePermissionsCollection", {MakeHandler(AuditEnsureFilePermissionsCollection), MakeHandler(RemediateEnsureFilePermissionsCollection)}},
    {"EnsureFilesystemOption", {MakeHandler(AuditEnsureFilesystemOption), MakeHandler(RemediateEnsureFilesystemOption)}},
    {"EnsureGroupIsOnlyGroupWith", {MakeHandler(AuditEnsureGroupIsOnlyGroupWith), nullptr}},
    {"EnsureGsettings", {MakeHandler(AuditEnsureGsettings), nullptr}},
    {"EnsureInteractiveUsersDotFilesAccessIsConfigured", {MakeHandler(AuditEnsureInteractiveUsersDotFilesAccessIsConfigured), MakeHandler(RemediateEnsureInteractiveUsersDotFilesAccessIsConfigured)}},
    {"EnsureInteractiveUsersHomeDirectoriesAreConfigured", {MakeHandler(AuditEnsureInteractiveUsersHomeDirectoriesAreConfigured), MakeHandler(RemediateEnsureInteractiveUsersHomeDirectoriesAreConfigured)}},
    {"EnsureIp6tablesOpenPorts", {MakeHandler(AuditEnsureIp6tablesOpenPorts), nullptr}},
    {"EnsureIptablesOpenPorts", {MakeHandler(AuditEnsureIptablesOpenPorts), nullptr}},
    {"EnsureKernelModuleUnavailable", {MakeHandler(AuditEnsureKernelModuleUnavailable), nullptr}},
    {"EnsureLogfileAccess", {MakeHandler(AuditEnsureLogfileAccess), MakeHandler(RemediateEnsureLogfileAccess)}},
    {"EnsureMTAsLocalOnly", {MakeHandler(AuditEnsureMTAsLocalOnly), nullptr}},
    {"EnsureMountPointExists", {MakeHandler(AuditEnsureMountPointExists), nullptr}},
    {"EnsureNoDuplicateEntriesExist", {MakeHandler(AuditEnsureNoDuplicateEntriesExist), nullptr}},
    {"EnsureNoUnowned", {MakeHandler(AuditEnsureNoUnowned), nullptr}},
    {"EnsureNoUserHasPrimaryShadowGroup", {MakeHandler(AuditEnsureNoUserHasPrimaryShadowGroup), nullptr}},
    {"EnsureNoWritables", {MakeHandler(AuditEnsureNoWritables), nullptr}},
    {"EnsurePasswordChangeIsInPast", {MakeHandler(AuditEnsurePasswordChangeIsInPast), nullptr}},
    {"EnsureRootPath", {MakeHandler(AuditEnsureRootPath), nullptr}},
    {"EnsureShadowContains", {MakeHandler(AuditEnsureShadowContains), nullptr}},
    {"EnsureSshKeyPerms", {MakeHandler(AuditEnsureSshKeyPerms), MakeHandler(RemediateEnsureSshKeyPerms)}},
    {"EnsureSshdOption", {MakeHandler(AuditEnsureSshdOption), nullptr}},
    {"EnsureSysctl", {MakeHandler(AuditEnsureSysctl), nullptr}},
    {"EnsureSystemAccountsDoNotHaveValidShell", {MakeHandler(AuditEnsureSystemAccountsDoNotHaveValidShell), nullptr}},
    {"EnsureUfwOpenPorts", {MakeHandler(AuditEnsureUfwOpenPorts), nullptr}},
    {"EnsureUserIsOnlyAccountWith", {MakeHandler(AuditEnsureUserIsOnlyAccountWith), nullptr}},
    {"EnsureWirelessIsDisabled", {MakeHandler(AuditEnsureWirelessIsDisabled), nullptr}},
    {"EnsureXdmcp", {MakeHandler(AuditEnsureXdmcp), nullptr}},
    {"ExecuteCommandGrep", {MakeHandler(AuditExecuteCommandGrep), nullptr}},
    {"FileRegexMatch", {MakeHandler(AuditFileRegexMatch), nullptr}},
    {"PackageInstalled", {MakeHandler(AuditPackageInstalled), nullptr}},
    {"RemediationFailure", {nullptr, MakeHandler(RemediateRemediationFailure)}},
    {"RemediationParametrized", {nullptr, MakeHandler(RemediateRemediationParametrized)}},
    {"RemediationSuccess", {nullptr, MakeHandler(RemediateRemediationSuccess)}},
    {"SCE", {MakeHandler(AuditSCE), MakeHandler(RemediateSCE)}},
    {"SystemdParameter", {MakeHandler(AuditSystemdParameter), nullptr}},
    {"SystemdUnitState", {MakeHandler(AuditSystemdUnitState), nullptr}},
    {"UfwStatus", {MakeHandler(AuditUfwStatus), nullptr}},
};
} // namespace ComplianceEngine

namespace std
{
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

} // namespace std
