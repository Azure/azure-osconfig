// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <version.h>
#include <ctype.h>
#include <parson.h>
#include <CommonUtils.h>
#include <UserUtils.h>
#include <SshUtils.h>
#include <Logging.h>
#include <Reasons.h>
#include <Asb.h>

#define PERF_LOG_FILE "/var/log/osconfig_asb_perf.log"
#define ROLLED_PERF_LOG_FILE "/var/log/osconfig_asb_perf.bak"

#define RETURN_REASON_IF_ZERO(call) {\
    if (0 == (call)) {\
        return reason;\
    }\
}\

#define RETURN_REASON_IF_NOT_ZERO(call) {\
    if (0 != (call)) {\
        return reason;\
    }\
}\

static const char* g_asbName = "Azure Security Baseline for Linux";

static const char* g_securityBaselineComponentName = "SecurityBaseline";

static const char* g_configurationFile = "/etc/osconfig/osconfig.json";

// Audit
static const char* g_auditEnsureLoggingLevelObject = "auditEnsureLoggingLevel";
static const char* g_auditEnsurePermissionsOnEtcIssueObject = "auditEnsurePermissionsOnEtcIssue";
static const char* g_auditEnsurePermissionsOnEtcIssueNetObject = "auditEnsurePermissionsOnEtcIssueNet";
static const char* g_auditEnsurePermissionsOnEtcHostsAllowObject = "auditEnsurePermissionsOnEtcHostsAllow";
static const char* g_auditEnsurePermissionsOnEtcHostsDenyObject = "auditEnsurePermissionsOnEtcHostsDeny";
static const char* g_auditEnsurePermissionsOnEtcSshSshdConfigObject = "auditEnsurePermissionsOnEtcSshSshdConfig";
static const char* g_auditEnsurePermissionsOnEtcShadowObject = "auditEnsurePermissionsOnEtcShadow";
static const char* g_auditEnsurePermissionsOnEtcShadowDashObject = "auditEnsurePermissionsOnEtcShadowDash";
static const char* g_auditEnsurePermissionsOnEtcGShadowObject = "auditEnsurePermissionsOnEtcGShadow";
static const char* g_auditEnsurePermissionsOnEtcGShadowDashObject = "auditEnsurePermissionsOnEtcGShadowDash";
static const char* g_auditEnsurePermissionsOnEtcPasswdObject = "auditEnsurePermissionsOnEtcPasswd";
static const char* g_auditEnsurePermissionsOnEtcPasswdDashObject = "auditEnsurePermissionsOnEtcPasswdDash";
static const char* g_auditEnsurePermissionsOnEtcGroupObject = "auditEnsurePermissionsOnEtcGroup";
static const char* g_auditEnsurePermissionsOnEtcGroupDashObject = "auditEnsurePermissionsOnEtcGroupDash";
static const char* g_auditEnsurePermissionsOnEtcAnacronTabObject = "auditEnsurePermissionsOnEtcAnacronTab";
static const char* g_auditEnsurePermissionsOnEtcCronDObject = "auditEnsurePermissionsOnEtcCronD";
static const char* g_auditEnsurePermissionsOnEtcCronDailyObject = "auditEnsurePermissionsOnEtcCronDaily";
static const char* g_auditEnsurePermissionsOnEtcCronHourlyObject = "auditEnsurePermissionsOnEtcCronHourly";
static const char* g_auditEnsurePermissionsOnEtcCronMonthlyObject = "auditEnsurePermissionsOnEtcCronMonthly";
static const char* g_auditEnsurePermissionsOnEtcCronWeeklyObject = "auditEnsurePermissionsOnEtcCronWeekly";
static const char* g_auditEnsurePermissionsOnEtcMotdObject = "auditEnsurePermissionsOnEtcMotd";
static const char* g_auditEnsureInetdNotInstalledObject = "auditEnsureInetdNotInstalled";
static const char* g_auditEnsureXinetdNotInstalledObject = "auditEnsureXinetdNotInstalled";
static const char* g_auditEnsureRshServerNotInstalledObject = "auditEnsureRshServerNotInstalled";
static const char* g_auditEnsureNisNotInstalledObject = "auditEnsureNisNotInstalled";
static const char* g_auditEnsureTftpdNotInstalledObject = "auditEnsureTftpdNotInstalled";
static const char* g_auditEnsureReadaheadFedoraNotInstalledObject = "auditEnsureReadaheadFedoraNotInstalled";
static const char* g_auditEnsureBluetoothHiddNotInstalledObject = "auditEnsureBluetoothHiddNotInstalled";
static const char* g_auditEnsureIsdnUtilsBaseNotInstalledObject = "auditEnsureIsdnUtilsBaseNotInstalled";
static const char* g_auditEnsureIsdnUtilsKdumpToolsNotInstalledObject = "auditEnsureIsdnUtilsKdumpToolsNotInstalled";
static const char* g_auditEnsureIscDhcpdServerNotInstalledObject = "auditEnsureIscDhcpdServerNotInstalled";
static const char* g_auditEnsureSendmailNotInstalledObject = "auditEnsureSendmailNotInstalled";
static const char* g_auditEnsureSldapdNotInstalledObject = "auditEnsureSldapdNotInstalled";
static const char* g_auditEnsureBind9NotInstalledObject = "auditEnsureBind9NotInstalled";
static const char* g_auditEnsureDovecotCoreNotInstalledObject = "auditEnsureDovecotCoreNotInstalled";
static const char* g_auditEnsureAuditdInstalledObject = "auditEnsureAuditdInstalled";
static const char* g_auditEnsurePrelinkIsDisabledObject = "auditEnsurePrelinkIsDisabled";
static const char* g_auditEnsureTalkClientIsNotInstalledObject = "auditEnsureTalkClientIsNotInstalled";
static const char* g_auditEnsureCronServiceIsEnabledObject = "auditEnsureCronServiceIsEnabled";
static const char* g_auditEnsureAuditdServiceIsRunningObject = "auditEnsureAuditdServiceIsRunning";
static const char* g_auditEnsureKernelSupportForCpuNxObject = "auditEnsureKernelSupportForCpuNx";
static const char* g_auditEnsureAllTelnetdPackagesUninstalledObject = "auditEnsureAllTelnetdPackagesUninstalled";
static const char* g_auditEnsureNodevOptionOnHomePartitionObject = "auditEnsureNodevOptionOnHomePartition";
static const char* g_auditEnsureNodevOptionOnTmpPartitionObject = "auditEnsureNodevOptionOnTmpPartition";
static const char* g_auditEnsureNodevOptionOnVarTmpPartitionObject = "auditEnsureNodevOptionOnVarTmpPartition";
static const char* g_auditEnsureNosuidOptionOnTmpPartitionObject = "auditEnsureNosuidOptionOnTmpPartition";
static const char* g_auditEnsureNosuidOptionOnVarTmpPartitionObject = "auditEnsureNosuidOptionOnVarTmpPartition";
static const char* g_auditEnsureNoexecOptionOnVarTmpPartitionObject = "auditEnsureNoexecOptionOnVarTmpPartition";
static const char* g_auditEnsureNoexecOptionOnDevShmPartitionObject = "auditEnsureNoexecOptionOnDevShmPartition";
static const char* g_auditEnsureNodevOptionEnabledForAllRemovableMediaObject = "auditEnsureNodevOptionEnabledForAllRemovableMedia";
static const char* g_auditEnsureNoexecOptionEnabledForAllRemovableMediaObject = "auditEnsureNoexecOptionEnabledForAllRemovableMedia";
static const char* g_auditEnsureNosuidOptionEnabledForAllRemovableMediaObject = "auditEnsureNosuidOptionEnabledForAllRemovableMedia";
static const char* g_auditEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject = "auditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts";
static const char* g_auditEnsureAllEtcPasswdGroupsExistInEtcGroupObject = "auditEnsureAllEtcPasswdGroupsExistInEtcGroup";
static const char* g_auditEnsureNoDuplicateUidsExistObject = "auditEnsureNoDuplicateUidsExist";
static const char* g_auditEnsureNoDuplicateGidsExistObject = "auditEnsureNoDuplicateGidsExist";
static const char* g_auditEnsureNoDuplicateUserNamesExistObject = "auditEnsureNoDuplicateUserNamesExist";
static const char* g_auditEnsureNoDuplicateGroupsExistObject = "auditEnsureNoDuplicateGroupsExist";
static const char* g_auditEnsureShadowGroupIsEmptyObject = "auditEnsureShadowGroupIsEmpty";
static const char* g_auditEnsureRootGroupExistsObject = "auditEnsureRootGroupExists";
static const char* g_auditEnsureAllAccountsHavePasswordsObject = "auditEnsureAllAccountsHavePasswords";
static const char* g_auditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject = "auditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero";
static const char* g_auditEnsureNoLegacyPlusEntriesInEtcPasswdObject = "auditEnsureNoLegacyPlusEntriesInEtcPasswd";
static const char* g_auditEnsureNoLegacyPlusEntriesInEtcShadowObject = "auditEnsureNoLegacyPlusEntriesInEtcShadow";
static const char* g_auditEnsureNoLegacyPlusEntriesInEtcGroupObject = "auditEnsureNoLegacyPlusEntriesInEtcGroup";
static const char* g_auditEnsureDefaultRootAccountGroupIsGidZeroObject = "auditEnsureDefaultRootAccountGroupIsGidZero";
static const char* g_auditEnsureRootIsOnlyUidZeroAccountObject = "auditEnsureRootIsOnlyUidZeroAccount";
static const char* g_auditEnsureAllUsersHomeDirectoriesExistObject = "auditEnsureAllUsersHomeDirectoriesExist";
static const char* g_auditEnsureUsersOwnTheirHomeDirectoriesObject = "auditEnsureUsersOwnTheirHomeDirectories";
static const char* g_auditEnsureRestrictedUserHomeDirectoriesObject = "auditEnsureRestrictedUserHomeDirectories";
static const char* g_auditEnsurePasswordHashingAlgorithmObject = "auditEnsurePasswordHashingAlgorithm";
static const char* g_auditEnsureMinDaysBetweenPasswordChangesObject = "auditEnsureMinDaysBetweenPasswordChanges";
static const char* g_auditEnsureInactivePasswordLockPeriodObject = "auditEnsureInactivePasswordLockPeriod";
static const char* g_auditMaxDaysBetweenPasswordChangesObject = "auditEnsureMaxDaysBetweenPasswordChanges";
static const char* g_auditEnsurePasswordExpirationObject = "auditEnsurePasswordExpiration";
static const char* g_auditEnsurePasswordExpirationWarningObject = "auditEnsurePasswordExpirationWarning";
static const char* g_auditEnsureSystemAccountsAreNonLoginObject = "auditEnsureSystemAccountsAreNonLogin";
static const char* g_auditEnsureAuthenticationRequiredForSingleUserModeObject = "auditEnsureAuthenticationRequiredForSingleUserMode";
static const char* g_auditEnsureDotDoesNotAppearInRootsPathObject = "auditEnsureDotDoesNotAppearInRootsPath";
static const char* g_auditEnsureRemoteLoginWarningBannerIsConfiguredObject = "auditEnsureRemoteLoginWarningBannerIsConfigured";
static const char* g_auditEnsureLocalLoginWarningBannerIsConfiguredObject = "auditEnsureLocalLoginWarningBannerIsConfigured";
static const char* g_auditEnsureSuRestrictedToRootGroupObject = "auditEnsureSuRestrictedToRootGroup";
static const char* g_auditEnsureDefaultUmaskForAllUsersObject = "auditEnsureDefaultUmaskForAllUsers";
static const char* g_auditEnsureAutomountingDisabledObject = "auditEnsureAutomountingDisabled";
static const char* g_auditEnsureKernelCompiledFromApprovedSourcesObject = "auditEnsureKernelCompiledFromApprovedSources";
static const char* g_auditEnsureDefaultDenyFirewallPolicyIsSetObject = "auditEnsureDefaultDenyFirewallPolicyIsSet";
static const char* g_auditEnsurePacketRedirectSendingIsDisabledObject = "auditEnsurePacketRedirectSendingIsDisabled";
static const char* g_auditEnsureIcmpRedirectsIsDisabledObject = "auditEnsureIcmpRedirectsIsDisabled";
static const char* g_auditEnsureSourceRoutedPacketsIsDisabledObject = "auditEnsureSourceRoutedPacketsIsDisabled";
static const char* g_auditEnsureAcceptingSourceRoutedPacketsIsDisabledObject = "auditEnsureAcceptingSourceRoutedPacketsIsDisabled";
static const char* g_auditEnsureIgnoringBogusIcmpBroadcastResponsesObject = "auditEnsureIgnoringBogusIcmpBroadcastResponses";
static const char* g_auditEnsureIgnoringIcmpEchoPingsToMulticastObject = "auditEnsureIgnoringIcmpEchoPingsToMulticast";
static const char* g_auditEnsureMartianPacketLoggingIsEnabledObject = "auditEnsureMartianPacketLoggingIsEnabled";
static const char* g_auditEnsureReversePathSourceValidationIsEnabledObject = "auditEnsureReversePathSourceValidationIsEnabled";
static const char* g_auditEnsureTcpSynCookiesAreEnabledObject = "auditEnsureTcpSynCookiesAreEnabled";
static const char* g_auditEnsureSystemNotActingAsNetworkSnifferObject = "auditEnsureSystemNotActingAsNetworkSniffer";
static const char* g_auditEnsureAllWirelessInterfacesAreDisabledObject = "auditEnsureAllWirelessInterfacesAreDisabled";
static const char* g_auditEnsureIpv6ProtocolIsEnabledObject = "auditEnsureIpv6ProtocolIsEnabled";
static const char* g_auditEnsureDccpIsDisabledObject = "auditEnsureDccpIsDisabled";
static const char* g_auditEnsureSctpIsDisabledObject = "auditEnsureSctpIsDisabled";
static const char* g_auditEnsureDisabledSupportForRdsObject = "auditEnsureDisabledSupportForRds";
static const char* g_auditEnsureTipcIsDisabledObject = "auditEnsureTipcIsDisabled";
static const char* g_auditEnsureZeroconfNetworkingIsDisabledObject = "auditEnsureZeroconfNetworkingIsDisabled";
static const char* g_auditEnsurePermissionsOnBootloaderConfigObject = "auditEnsurePermissionsOnBootloaderConfig";
static const char* g_auditEnsurePasswordReuseIsLimitedObject = "auditEnsurePasswordReuseIsLimited";
static const char* g_auditEnsureMountingOfUsbStorageDevicesIsDisabledObject = "auditEnsureMountingOfUsbStorageDevicesIsDisabled";
static const char* g_auditEnsureCoreDumpsAreRestrictedObject = "auditEnsureCoreDumpsAreRestricted";
static const char* g_auditEnsurePasswordCreationRequirementsObject = "auditEnsurePasswordCreationRequirements";
static const char* g_auditEnsureLockoutForFailedPasswordAttemptsObject = "auditEnsureLockoutForFailedPasswordAttempts";
static const char* g_auditEnsureDisabledInstallationOfCramfsFileSystemObject = "auditEnsureDisabledInstallationOfCramfsFileSystem";
static const char* g_auditEnsureDisabledInstallationOfFreevxfsFileSystemObject = "auditEnsureDisabledInstallationOfFreevxfsFileSystem";
static const char* g_auditEnsureDisabledInstallationOfHfsFileSystemObject = "auditEnsureDisabledInstallationOfHfsFileSystem";
static const char* g_auditEnsureDisabledInstallationOfHfsplusFileSystemObject = "auditEnsureDisabledInstallationOfHfsplusFileSystem";
static const char* g_auditEnsureDisabledInstallationOfJffs2FileSystemObject = "auditEnsureDisabledInstallationOfJffs2FileSystem";
static const char* g_auditEnsureVirtualMemoryRandomizationIsEnabledObject = "auditEnsureVirtualMemoryRandomizationIsEnabled";
static const char* g_auditEnsureAllBootloadersHavePasswordProtectionEnabledObject = "auditEnsureAllBootloadersHavePasswordProtectionEnabled";
static const char* g_auditEnsureLoggingIsConfiguredObject = "auditEnsureLoggingIsConfigured";
static const char* g_auditEnsureSyslogPackageIsInstalledObject = "auditEnsureSyslogPackageIsInstalled";
static const char* g_auditEnsureSystemdJournaldServicePersistsLogMessagesObject = "auditEnsureSystemdJournaldServicePersistsLogMessages";
static const char* g_auditEnsureALoggingServiceIsEnabledObject = "auditEnsureALoggingServiceIsEnabled";
static const char* g_auditEnsureFilePermissionsForAllRsyslogLogFilesObject = "auditEnsureFilePermissionsForAllRsyslogLogFiles";
static const char* g_auditEnsureLoggerConfigurationFilesAreRestrictedObject = "auditEnsureLoggerConfigurationFilesAreRestricted";
static const char* g_auditEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject = "auditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup";
static const char* g_auditEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject = "auditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser";
static const char* g_auditEnsureRsyslogNotAcceptingRemoteMessagesObject = "auditEnsureRsyslogNotAcceptingRemoteMessages";
static const char* g_auditEnsureSyslogRotaterServiceIsEnabledObject = "auditEnsureSyslogRotaterServiceIsEnabled";
static const char* g_auditEnsureTelnetServiceIsDisabledObject = "auditEnsureTelnetServiceIsDisabled";
static const char* g_auditEnsureRcprshServiceIsDisabledObject = "auditEnsureRcprshServiceIsDisabled";
static const char* g_auditEnsureTftpServiceisDisabledObject = "auditEnsureTftpServiceisDisabled";
static const char* g_auditEnsureAtCronIsRestrictedToAuthorizedUsersObject = "auditEnsureAtCronIsRestrictedToAuthorizedUsers";
static const char* g_auditEnsureSshPortIsConfiguredObject = "auditEnsureSshPortIsConfigured";
static const char* g_auditEnsureSshBestPracticeProtocolObject = "auditEnsureSshBestPracticeProtocol";
static const char* g_auditEnsureSshBestPracticeIgnoreRhostsObject = "auditEnsureSshBestPracticeIgnoreRhosts";
static const char* g_auditEnsureSshLogLevelIsSetObject = "auditEnsureSshLogLevelIsSet";
static const char* g_auditEnsureSshMaxAuthTriesIsSetObject = "auditEnsureSshMaxAuthTriesIsSet";
static const char* g_auditEnsureAllowUsersIsConfiguredObject = "auditEnsureAllowUsersIsConfigured";
static const char* g_auditEnsureDenyUsersIsConfiguredObject = "auditEnsureDenyUsersIsConfigured";
static const char* g_auditEnsureAllowGroupsIsConfiguredObject = "auditEnsureAllowGroupsIsConfigured";
static const char* g_auditEnsureDenyGroupsConfiguredObject = "auditEnsureDenyGroupsConfigured";
static const char* g_auditEnsureSshHostbasedAuthenticationIsDisabledObject = "auditEnsureSshHostbasedAuthenticationIsDisabled";
static const char* g_auditEnsureSshPermitRootLoginIsDisabledObject = "auditEnsureSshPermitRootLoginIsDisabled";
static const char* g_auditEnsureSshPermitEmptyPasswordsIsDisabledObject = "auditEnsureSshPermitEmptyPasswordsIsDisabled";
static const char* g_auditEnsureSshClientIntervalCountMaxIsConfiguredObject = "auditEnsureSshClientIntervalCountMaxIsConfigured";
static const char* g_auditEnsureSshClientAliveIntervalIsConfiguredObject = "auditEnsureSshClientAliveIntervalIsConfigured";
static const char* g_auditEnsureSshLoginGraceTimeIsSetObject = "auditEnsureSshLoginGraceTimeIsSet";
static const char* g_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject = "auditEnsureOnlyApprovedMacAlgorithmsAreUsed";
static const char* g_auditEnsureSshWarningBannerIsEnabledObject = "auditEnsureSshWarningBannerIsEnabled";
static const char* g_auditEnsureUsersCannotSetSshEnvironmentOptionsObject = "auditEnsureUsersCannotSetSshEnvironmentOptions";
static const char* g_auditEnsureAppropriateCiphersForSshObject = "auditEnsureAppropriateCiphersForSsh";
static const char* g_auditEnsureAvahiDaemonServiceIsDisabledObject = "auditEnsureAvahiDaemonServiceIsDisabled";
static const char* g_auditEnsureCupsServiceisDisabledObject = "auditEnsureCupsServiceisDisabled";
static const char* g_auditEnsurePostfixPackageIsUninstalledObject = "auditEnsurePostfixPackageIsUninstalled";
static const char* g_auditEnsurePostfixNetworkListeningIsDisabledObject = "auditEnsurePostfixNetworkListeningIsDisabled";
static const char* g_auditEnsureRpcgssdServiceIsDisabledObject = "auditEnsureRpcgssdServiceIsDisabled";
static const char* g_auditEnsureRpcidmapdServiceIsDisabledObject = "auditEnsureRpcidmapdServiceIsDisabled";
static const char* g_auditEnsurePortmapServiceIsDisabledObject = "auditEnsurePortmapServiceIsDisabled";
static const char* g_auditEnsureNetworkFileSystemServiceIsDisabledObject = "auditEnsureNetworkFileSystemServiceIsDisabled";
static const char* g_auditEnsureRpcsvcgssdServiceIsDisabledObject = "auditEnsureRpcsvcgssdServiceIsDisabled";
static const char* g_auditEnsureSnmpServerIsDisabledObject = "auditEnsureSnmpServerIsDisabled";
static const char* g_auditEnsureRsynServiceIsDisabledObject = "auditEnsureRsynServiceIsDisabled";
static const char* g_auditEnsureNisServerIsDisabledObject = "auditEnsureNisServerIsDisabled";
static const char* g_auditEnsureRshClientNotInstalledObject = "auditEnsureRshClientNotInstalled";
static const char* g_auditEnsureSmbWithSambaIsDisabledObject = "auditEnsureSmbWithSambaIsDisabled";
static const char* g_auditEnsureUsersDotFilesArentGroupOrWorldWritableObject = "auditEnsureUsersDotFilesArentGroupOrWorldWritable";
static const char* g_auditEnsureNoUsersHaveDotForwardFilesObject = "auditEnsureNoUsersHaveDotForwardFiles";
static const char* g_auditEnsureNoUsersHaveDotNetrcFilesObject = "auditEnsureNoUsersHaveDotNetrcFiles";
static const char* g_auditEnsureNoUsersHaveDotRhostsFilesObject = "auditEnsureNoUsersHaveDotRhostsFiles";
static const char* g_auditEnsureRloginServiceIsDisabledObject = "auditEnsureRloginServiceIsDisabled";
static const char* g_auditEnsureUnnecessaryAccountsAreRemovedObject = "auditEnsureUnnecessaryAccountsAreRemoved";

// Remediation
static const char* g_remediateEnsureLoggingLevelObject = "remediateEnsureLoggingLevel";
static const char* g_remediateEnsurePermissionsOnEtcIssueObject = "remediateEnsurePermissionsOnEtcIssue";
static const char* g_remediateEnsurePermissionsOnEtcIssueNetObject = "remediateEnsurePermissionsOnEtcIssueNet";
static const char* g_remediateEnsurePermissionsOnEtcHostsAllowObject = "remediateEnsurePermissionsOnEtcHostsAllow";
static const char* g_remediateEnsurePermissionsOnEtcHostsDenyObject = "remediateEnsurePermissionsOnEtcHostsDeny";
static const char* g_remediateEnsurePermissionsOnEtcSshSshdConfigObject = "remediateEnsurePermissionsOnEtcSshSshdConfig";
static const char* g_remediateEnsurePermissionsOnEtcShadowObject = "remediateEnsurePermissionsOnEtcShadow";
static const char* g_remediateEnsurePermissionsOnEtcShadowDashObject = "remediateEnsurePermissionsOnEtcShadowDash";
static const char* g_remediateEnsurePermissionsOnEtcGShadowObject = "remediateEnsurePermissionsOnEtcGShadow";
static const char* g_remediateEnsurePermissionsOnEtcGShadowDashObject = "remediateEnsurePermissionsOnEtcGShadowDash";
static const char* g_remediateEnsurePermissionsOnEtcPasswdObject = "remediateEnsurePermissionsOnEtcPasswd";
static const char* g_remediateEnsurePermissionsOnEtcPasswdDashObject = "remediateEnsurePermissionsOnEtcPasswdDash";
static const char* g_remediateEnsurePermissionsOnEtcGroupObject = "remediateEnsurePermissionsOnEtcGroup";
static const char* g_remediateEnsurePermissionsOnEtcGroupDashObject = "remediateEnsurePermissionsOnEtcGroupDash";
static const char* g_remediateEnsurePermissionsOnEtcAnacronTabObject = "remediateEnsurePermissionsOnEtcAnacronTab";
static const char* g_remediateEnsurePermissionsOnEtcCronDObject = "remediateEnsurePermissionsOnEtcCronD";
static const char* g_remediateEnsurePermissionsOnEtcCronDailyObject = "remediateEnsurePermissionsOnEtcCronDaily";
static const char* g_remediateEnsurePermissionsOnEtcCronHourlyObject = "remediateEnsurePermissionsOnEtcCronHourly";
static const char* g_remediateEnsurePermissionsOnEtcCronMonthlyObject = "remediateEnsurePermissionsOnEtcCronMonthly";
static const char* g_remediateEnsurePermissionsOnEtcCronWeeklyObject = "remediateEnsurePermissionsOnEtcCronWeekly";
static const char* g_remediateEnsurePermissionsOnEtcMotdObject = "remediateEnsurePermissionsOnEtcMotd";
static const char* g_remediateEnsureInetdNotInstalledObject = "remediateEnsureInetdNotInstalled";
static const char* g_remediateEnsureXinetdNotInstalledObject = "remediateEnsureXinetdNotInstalled";
static const char* g_remediateEnsureRshServerNotInstalledObject = "remediateEnsureRshServerNotInstalled";
static const char* g_remediateEnsureNisNotInstalledObject = "remediateEnsureNisNotInstalled";
static const char* g_remediateEnsureTftpdNotInstalledObject = "remediateEnsureTftpdNotInstalled";
static const char* g_remediateEnsureReadaheadFedoraNotInstalledObject = "remediateEnsureReadaheadFedoraNotInstalled";
static const char* g_remediateEnsureBluetoothHiddNotInstalledObject = "remediateEnsureBluetoothHiddNotInstalled";
static const char* g_remediateEnsureIsdnUtilsBaseNotInstalledObject = "remediateEnsureIsdnUtilsBaseNotInstalled";
static const char* g_remediateEnsureIsdnUtilsKdumpToolsNotInstalledObject = "remediateEnsureIsdnUtilsKdumpToolsNotInstalled";
static const char* g_remediateEnsureIscDhcpdServerNotInstalledObject = "remediateEnsureIscDhcpdServerNotInstalled";
static const char* g_remediateEnsureSendmailNotInstalledObject = "remediateEnsureSendmailNotInstalled";
static const char* g_remediateEnsureSldapdNotInstalledObject = "remediateEnsureSldapdNotInstalled";
static const char* g_remediateEnsureBind9NotInstalledObject = "remediateEnsureBind9NotInstalled";
static const char* g_remediateEnsureDovecotCoreNotInstalledObject = "remediateEnsureDovecotCoreNotInstalled";
static const char* g_remediateEnsureAuditdInstalledObject = "remediateEnsureAuditdInstalled";
static const char* g_remediateEnsurePrelinkIsDisabledObject = "remediateEnsurePrelinkIsDisabled";
static const char* g_remediateEnsureTalkClientIsNotInstalledObject = "remediateEnsureTalkClientIsNotInstalled";
static const char* g_remediateEnsureCronServiceIsEnabledObject = "remediateEnsureCronServiceIsEnabled";
static const char* g_remediateEnsureAuditdServiceIsRunningObject = "remediateEnsureAuditdServiceIsRunning";
static const char* g_remediateEnsureKernelSupportForCpuNxObject = "remediateEnsureKernelSupportForCpuNx";
static const char* g_remediateEnsureAllTelnetdPackagesUninstalledObject = "remediateEnsureAllTelnetdPackagesUninstalled";
static const char* g_remediateEnsureNodevOptionOnHomePartitionObject = "remediateEnsureNodevOptionOnHomePartition";
static const char* g_remediateEnsureNodevOptionOnTmpPartitionObject = "remediateEnsureNodevOptionOnTmpPartition";
static const char* g_remediateEnsureNodevOptionOnVarTmpPartitionObject = "remediateEnsureNodevOptionOnVarTmpPartition";
static const char* g_remediateEnsureNosuidOptionOnTmpPartitionObject = "remediateEnsureNosuidOptionOnTmpPartition";
static const char* g_remediateEnsureNosuidOptionOnVarTmpPartitionObject = "remediateEnsureNosuidOptionOnVarTmpPartition";
static const char* g_remediateEnsureNoexecOptionOnVarTmpPartitionObject = "remediateEnsureNoexecOptionOnVarTmpPartition";
static const char* g_remediateEnsureNoexecOptionOnDevShmPartitionObject = "remediateEnsureNoexecOptionOnDevShmPartition";
static const char* g_remediateEnsureNodevOptionEnabledForAllRemovableMediaObject = "remediateEnsureNodevOptionEnabledForAllRemovableMedia";
static const char* g_remediateEnsureNoexecOptionEnabledForAllRemovableMediaObject = "remediateEnsureNoexecOptionEnabledForAllRemovableMedia";
static const char* g_remediateEnsureNosuidOptionEnabledForAllRemovableMediaObject = "remediateEnsureNosuidOptionEnabledForAllRemovableMedia";
static const char* g_remediateEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject = "remediateEnsureNoexecNosuidOptionsEnabledForAllNfsMounts";
static const char* g_remediateEnsureAllEtcPasswdGroupsExistInEtcGroupObject = "remediateEnsureAllEtcPasswdGroupsExistInEtcGroup";
static const char* g_remediateEnsureNoDuplicateUidsExistObject = "remediateEnsureNoDuplicateUidsExist";
static const char* g_remediateEnsureNoDuplicateGidsExistObject = "remediateEnsureNoDuplicateGidsExist";
static const char* g_remediateEnsureNoDuplicateUserNamesExistObject = "remediateEnsureNoDuplicateUserNamesExist";
static const char* g_remediateEnsureNoDuplicateGroupsExistObject = "remediateEnsureNoDuplicateGroupsExist";
static const char* g_remediateEnsureShadowGroupIsEmptyObject = "remediateEnsureShadowGroupIsEmpty";
static const char* g_remediateEnsureRootGroupExistsObject = "remediateEnsureRootGroupExists";
static const char* g_remediateEnsureAllAccountsHavePasswordsObject = "remediateEnsureAllAccountsHavePasswords";
static const char* g_remediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject = "remediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero";
static const char* g_remediateEnsureNoLegacyPlusEntriesInEtcPasswdObject = "remediateEnsureNoLegacyPlusEntriesInEtcPasswd";
static const char* g_remediateEnsureNoLegacyPlusEntriesInEtcShadowObject = "remediateEnsureNoLegacyPlusEntriesInEtcShadow";
static const char* g_remediateEnsureNoLegacyPlusEntriesInEtcGroupObject = "remediateEnsureNoLegacyPlusEntriesInEtcGroup";
static const char* g_remediateEnsureDefaultRootAccountGroupIsGidZeroObject = "remediateEnsureDefaultRootAccountGroupIsGidZero";
static const char* g_remediateEnsureRootIsOnlyUidZeroAccountObject = "remediateEnsureRootIsOnlyUidZeroAccount";
static const char* g_remediateEnsureAllUsersHomeDirectoriesExistObject = "remediateEnsureAllUsersHomeDirectoriesExist";
static const char* g_remediateEnsureUsersOwnTheirHomeDirectoriesObject = "remediateEnsureUsersOwnTheirHomeDirectories";
static const char* g_remediateEnsureRestrictedUserHomeDirectoriesObject = "remediateEnsureRestrictedUserHomeDirectories";
static const char* g_remediateEnsurePasswordHashingAlgorithmObject = "remediateEnsurePasswordHashingAlgorithm";
static const char* g_remediateEnsureMinDaysBetweenPasswordChangesObject = "remediateEnsureMinDaysBetweenPasswordChanges";
static const char* g_remediateEnsureInactivePasswordLockPeriodObject = "remediateEnsureInactivePasswordLockPeriod";
static const char* g_remediateMaxDaysBetweenPasswordChangesObject = "remediateEnsureMaxDaysBetweenPasswordChanges";
static const char* g_remediateEnsurePasswordExpirationObject = "remediateEnsurePasswordExpiration";
static const char* g_remediateEnsurePasswordExpirationWarningObject = "remediateEnsurePasswordExpirationWarning";
static const char* g_remediateEnsureSystemAccountsAreNonLoginObject = "remediateEnsureSystemAccountsAreNonLogin";
static const char* g_remediateEnsureAuthenticationRequiredForSingleUserModeObject = "remediateEnsureAuthenticationRequiredForSingleUserMode";
static const char* g_remediateEnsureDotDoesNotAppearInRootsPathObject = "remediateEnsureDotDoesNotAppearInRootsPath";
static const char* g_remediateEnsureRemoteLoginWarningBannerIsConfiguredObject = "remediateEnsureRemoteLoginWarningBannerIsConfigured";
static const char* g_remediateEnsureLocalLoginWarningBannerIsConfiguredObject = "remediateEnsureLocalLoginWarningBannerIsConfigured";
static const char* g_remediateEnsureSuRestrictedToRootGroupObject = "remediateEnsureSuRestrictedToRootGroup";
static const char* g_remediateEnsureDefaultUmaskForAllUsersObject = "remediateEnsureDefaultUmaskForAllUsers";
static const char* g_remediateEnsureAutomountingDisabledObject = "remediateEnsureAutomountingDisabled";
static const char* g_remediateEnsureKernelCompiledFromApprovedSourcesObject = "remediateEnsureKernelCompiledFromApprovedSources";
static const char* g_remediateEnsureDefaultDenyFirewallPolicyIsSetObject = "remediateEnsureDefaultDenyFirewallPolicyIsSet";
static const char* g_remediateEnsurePacketRedirectSendingIsDisabledObject = "remediateEnsurePacketRedirectSendingIsDisabled";
static const char* g_remediateEnsureIcmpRedirectsIsDisabledObject = "remediateEnsureIcmpRedirectsIsDisabled";
static const char* g_remediateEnsureSourceRoutedPacketsIsDisabledObject = "remediateEnsureSourceRoutedPacketsIsDisabled";
static const char* g_remediateEnsureAcceptingSourceRoutedPacketsIsDisabledObject = "remediateEnsureAcceptingSourceRoutedPacketsIsDisabled";
static const char* g_remediateEnsureIgnoringBogusIcmpBroadcastResponsesObject = "remediateEnsureIgnoringBogusIcmpBroadcastResponses";
static const char* g_remediateEnsureIgnoringIcmpEchoPingsToMulticastObject = "remediateEnsureIgnoringIcmpEchoPingsToMulticast";
static const char* g_remediateEnsureMartianPacketLoggingIsEnabledObject = "remediateEnsureMartianPacketLoggingIsEnabled";
static const char* g_remediateEnsureReversePathSourceValidationIsEnabledObject = "remediateEnsureReversePathSourceValidationIsEnabled";
static const char* g_remediateEnsureTcpSynCookiesAreEnabledObject = "remediateEnsureTcpSynCookiesAreEnabled";
static const char* g_remediateEnsureSystemNotActingAsNetworkSnifferObject = "remediateEnsureSystemNotActingAsNetworkSniffer";
static const char* g_remediateEnsureAllWirelessInterfacesAreDisabledObject = "remediateEnsureAllWirelessInterfacesAreDisabled";
static const char* g_remediateEnsureIpv6ProtocolIsEnabledObject = "remediateEnsureIpv6ProtocolIsEnabled";
static const char* g_remediateEnsureDccpIsDisabledObject = "remediateEnsureDccpIsDisabled";
static const char* g_remediateEnsureSctpIsDisabledObject = "remediateEnsureSctpIsDisabled";
static const char* g_remediateEnsureDisabledSupportForRdsObject = "remediateEnsureDisabledSupportForRds";
static const char* g_remediateEnsureTipcIsDisabledObject = "remediateEnsureTipcIsDisabled";
static const char* g_remediateEnsureZeroconfNetworkingIsDisabledObject = "remediateEnsureZeroconfNetworkingIsDisabled";
static const char* g_remediateEnsurePermissionsOnBootloaderConfigObject = "remediateEnsurePermissionsOnBootloaderConfig";
static const char* g_remediateEnsurePasswordReuseIsLimitedObject = "remediateEnsurePasswordReuseIsLimited";
static const char* g_remediateEnsureMountingOfUsbStorageDevicesIsDisabledObject = "remediateEnsureMountingOfUsbStorageDevicesIsDisabled";
static const char* g_remediateEnsureCoreDumpsAreRestrictedObject = "remediateEnsureCoreDumpsAreRestricted";
static const char* g_remediateEnsurePasswordCreationRequirementsObject = "remediateEnsurePasswordCreationRequirements";
static const char* g_remediateEnsureLockoutForFailedPasswordAttemptsObject = "remediateEnsureLockoutForFailedPasswordAttempts";
static const char* g_remediateEnsureDisabledInstallationOfCramfsFileSystemObject = "remediateEnsureDisabledInstallationOfCramfsFileSystem";
static const char* g_remediateEnsureDisabledInstallationOfFreevxfsFileSystemObject = "remediateEnsureDisabledInstallationOfFreevxfsFileSystem";
static const char* g_remediateEnsureDisabledInstallationOfHfsFileSystemObject = "remediateEnsureDisabledInstallationOfHfsFileSystem";
static const char* g_remediateEnsureDisabledInstallationOfHfsplusFileSystemObject = "remediateEnsureDisabledInstallationOfHfsplusFileSystem";
static const char* g_remediateEnsureDisabledInstallationOfJffs2FileSystemObject = "remediateEnsureDisabledInstallationOfJffs2FileSystem";
static const char* g_remediateEnsureVirtualMemoryRandomizationIsEnabledObject = "remediateEnsureVirtualMemoryRandomizationIsEnabled";
static const char* g_remediateEnsureAllBootloadersHavePasswordProtectionEnabledObject = "remediateEnsureAllBootloadersHavePasswordProtectionEnabled";
static const char* g_remediateEnsureLoggingIsConfiguredObject = "remediateEnsureLoggingIsConfigured";
static const char* g_remediateEnsureSyslogPackageIsInstalledObject = "remediateEnsureSyslogPackageIsInstalled";
static const char* g_remediateEnsureSystemdJournaldServicePersistsLogMessagesObject = "remediateEnsureSystemdJournaldServicePersistsLogMessages";
static const char* g_remediateEnsureALoggingServiceIsEnabledObject = "remediateEnsureALoggingServiceIsEnabled";
static const char* g_remediateEnsureFilePermissionsForAllRsyslogLogFilesObject = "remediateEnsureFilePermissionsForAllRsyslogLogFiles";
static const char* g_remediateEnsureLoggerConfigurationFilesAreRestrictedObject = "remediateEnsureLoggerConfigurationFilesAreRestricted";
static const char* g_remediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject = "remediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroup";
static const char* g_remediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject = "remediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUser";
static const char* g_remediateEnsureRsyslogNotAcceptingRemoteMessagesObject = "remediateEnsureRsyslogNotAcceptingRemoteMessages";
static const char* g_remediateEnsureSyslogRotaterServiceIsEnabledObject = "remediateEnsureSyslogRotaterServiceIsEnabled";
static const char* g_remediateEnsureTelnetServiceIsDisabledObject = "remediateEnsureTelnetServiceIsDisabled";
static const char* g_remediateEnsureRcprshServiceIsDisabledObject = "remediateEnsureRcprshServiceIsDisabled";
static const char* g_remediateEnsureTftpServiceisDisabledObject = "remediateEnsureTftpServiceisDisabled";
static const char* g_remediateEnsureAtCronIsRestrictedToAuthorizedUsersObject = "remediateEnsureAtCronIsRestrictedToAuthorizedUsers";
static const char* g_remediateEnsureSshPortIsConfiguredObject = "remediateEnsureSshPortIsConfigured";
static const char* g_remediateEnsureSshBestPracticeProtocolObject = "remediateEnsureSshBestPracticeProtocol";
static const char* g_remediateEnsureSshBestPracticeIgnoreRhostsObject = "remediateEnsureSshBestPracticeIgnoreRhosts";
static const char* g_remediateEnsureSshLogLevelIsSetObject = "remediateEnsureSshLogLevelIsSet";
static const char* g_remediateEnsureSshMaxAuthTriesIsSetObject = "remediateEnsureSshMaxAuthTriesIsSet";
static const char* g_remediateEnsureAllowUsersIsConfiguredObject = "remediateEnsureAllowUsersIsConfigured";
static const char* g_remediateEnsureDenyUsersIsConfiguredObject = "remediateEnsureDenyUsersIsConfigured";
static const char* g_remediateEnsureAllowGroupsIsConfiguredObject = "remediateEnsureAllowGroupsIsConfigured";
static const char* g_remediateEnsureDenyGroupsConfiguredObject = "remediateEnsureDenyGroupsConfigured";
static const char* g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject = "remediateEnsureSshHostbasedAuthenticationIsDisabled";
static const char* g_remediateEnsureSshPermitRootLoginIsDisabledObject = "remediateEnsureSshPermitRootLoginIsDisabled";
static const char* g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject = "remediateEnsureSshPermitEmptyPasswordsIsDisabled";
static const char* g_remediateEnsureSshClientIntervalCountMaxIsConfiguredObject = "remediateEnsureSshClientIntervalCountMaxIsConfigured";
static const char* g_remediateEnsureSshClientAliveIntervalIsConfiguredObject = "remediateEnsureSshClientAliveIntervalIsConfigured";
static const char* g_remediateEnsureSshLoginGraceTimeIsSetObject = "remediateEnsureSshLoginGraceTimeIsSet";
static const char* g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject = "remediateEnsureOnlyApprovedMacAlgorithmsAreUsed";
static const char* g_remediateEnsureSshWarningBannerIsEnabledObject = "remediateEnsureSshWarningBannerIsEnabled";
static const char* g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject = "remediateEnsureUsersCannotSetSshEnvironmentOptions";
static const char* g_remediateEnsureAppropriateCiphersForSshObject = "remediateEnsureAppropriateCiphersForSsh";
static const char* g_remediateEnsureAvahiDaemonServiceIsDisabledObject = "remediateEnsureAvahiDaemonServiceIsDisabled";
static const char* g_remediateEnsureCupsServiceisDisabledObject = "remediateEnsureCupsServiceisDisabled";
static const char* g_remediateEnsurePostfixPackageIsUninstalledObject = "remediateEnsurePostfixPackageIsUninstalled";
static const char* g_remediateEnsurePostfixNetworkListeningIsDisabledObject = "remediateEnsurePostfixNetworkListeningIsDisabled";
static const char* g_remediateEnsureRpcgssdServiceIsDisabledObject = "remediateEnsureRpcgssdServiceIsDisabled";
static const char* g_remediateEnsureRpcidmapdServiceIsDisabledObject = "remediateEnsureRpcidmapdServiceIsDisabled";
static const char* g_remediateEnsurePortmapServiceIsDisabledObject = "remediateEnsurePortmapServiceIsDisabled";
static const char* g_remediateEnsureNetworkFileSystemServiceIsDisabledObject = "remediateEnsureNetworkFileSystemServiceIsDisabled";
static const char* g_remediateEnsureRpcsvcgssdServiceIsDisabledObject = "remediateEnsureRpcsvcgssdServiceIsDisabled";
static const char* g_remediateEnsureSnmpServerIsDisabledObject = "remediateEnsureSnmpServerIsDisabled";
static const char* g_remediateEnsureRsynServiceIsDisabledObject = "remediateEnsureRsynServiceIsDisabled";
static const char* g_remediateEnsureNisServerIsDisabledObject = "remediateEnsureNisServerIsDisabled";
static const char* g_remediateEnsureRshClientNotInstalledObject = "remediateEnsureRshClientNotInstalled";
static const char* g_remediateEnsureSmbWithSambaIsDisabledObject = "remediateEnsureSmbWithSambaIsDisabled";
static const char* g_remediateEnsureUsersDotFilesArentGroupOrWorldWritableObject = "remediateEnsureUsersDotFilesArentGroupOrWorldWritable";
static const char* g_remediateEnsureNoUsersHaveDotForwardFilesObject = "remediateEnsureNoUsersHaveDotForwardFiles";
static const char* g_remediateEnsureNoUsersHaveDotNetrcFilesObject = "remediateEnsureNoUsersHaveDotNetrcFiles";
static const char* g_remediateEnsureNoUsersHaveDotRhostsFilesObject = "remediateEnsureNoUsersHaveDotRhostsFiles";
static const char* g_remediateEnsureRloginServiceIsDisabledObject = "remediateEnsureRloginServiceIsDisabled";
static const char* g_remediateEnsureUnnecessaryAccountsAreRemovedObject = "remediateEnsureUnnecessaryAccountsAreRemoved";

// Initialization for audit before remediation
static const char* g_initEnsureLoggingLevelObject = "initEnsureLoggingLevel";
static const char* g_initEnsurePermissionsOnEtcSshSshdConfigObject = "initEnsurePermissionsOnEtcSshSshdConfig";
static const char* g_initEnsureSshPortIsConfiguredObject = "initEnsureSshPortIsConfigured";
static const char* g_initEnsureSshBestPracticeProtocolObject = "initEnsureSshBestPracticeProtocol";
static const char* g_initEnsureSshBestPracticeIgnoreRhostsObject = "initEnsureSshBestPracticeIgnoreRhosts";
static const char* g_initEnsureSshLogLevelIsSetObject = "initEnsureSshLogLevelIsSet";
static const char* g_initEnsureSshMaxAuthTriesIsSetObject = "initEnsureSshMaxAuthTriesIsSet";
static const char* g_initEnsureAllowUsersIsConfiguredObject = "initEnsureAllowUsersIsConfigured";
static const char* g_initEnsureDenyUsersIsConfiguredObject = "initEnsureDenyUsersIsConfigured";
static const char* g_initEnsureAllowGroupsIsConfiguredObject = "initEnsureAllowGroupsIsConfigured";
static const char* g_initEnsureDenyGroupsConfiguredObject = "initEnsureDenyGroupsConfigured";
static const char* g_initEnsureSshHostbasedAuthenticationIsDisabledObject = "initEnsureSshHostbasedAuthenticationIsDisabled";
static const char* g_initEnsureSshPermitRootLoginIsDisabledObject = "initEnsureSshPermitRootLoginIsDisabled";
static const char* g_initEnsureSshPermitEmptyPasswordsIsDisabledObject = "initEnsureSshPermitEmptyPasswordsIsDisabled";
static const char* g_initEnsureSshClientIntervalCountMaxIsConfiguredObject = "initEnsureSshClientIntervalCountMaxIsConfigured";
static const char* g_initEnsureSshClientAliveIntervalIsConfiguredObject = "initEnsureSshClientAliveIntervalIsConfigured";
static const char* g_initEnsureSshLoginGraceTimeIsSetObject = "initEnsureSshLoginGraceTimeIsSet";
static const char* g_initEnsureOnlyApprovedMacAlgorithmsAreUsedObject = "initEnsureOnlyApprovedMacAlgorithmsAreUsed";
static const char* g_initEnsureSshWarningBannerIsEnabledObject = "initEnsureSshWarningBannerIsEnabled";
static const char* g_initEnsureUsersCannotSetSshEnvironmentOptionsObject = "initEnsureUsersCannotSetSshEnvironmentOptions";
static const char* g_initEnsureAppropriateCiphersForSshObject = "initEnsureAppropriateCiphersForSsh";
static const char* g_initEnsurePermissionsOnEtcIssueObject = "initEnsurePermissionsOnEtcIssue";
static const char* g_initEnsurePermissionsOnEtcIssueNetObject = "initEnsurePermissionsOnEtcIssueNet";
static const char* g_initEnsurePermissionsOnEtcHostsAllowObject = "initEnsurePermissionsOnEtcHostsAllow";
static const char* g_initEnsurePermissionsOnEtcHostsDenyObject = "initEnsurePermissionsOnEtcHostsDeny";
static const char* g_initEnsurePermissionsOnEtcShadowObject = "initEnsurePermissionsOnEtcShadow";
static const char* g_initEnsurePermissionsOnEtcShadowDashObject = "initEnsurePermissionsOnEtcShadowDash";
static const char* g_initEnsurePermissionsOnEtcGShadowObject = "initEnsurePermissionsOnEtcGShadow";
static const char* g_initEnsurePermissionsOnEtcGShadowDashObject = "initEnsurePermissionsOnEtcGShadowDash";
static const char* g_initEnsurePermissionsOnEtcPasswdObject = "initEnsurePermissionsOnEtcPasswd";
static const char* g_initEnsurePermissionsOnEtcPasswdDashObject = "initEnsurePermissionsOnEtcPasswdDash";
static const char* g_initEnsurePermissionsOnEtcGroupObject = "initEnsurePermissionsOnEtcGroup";
static const char* g_initEnsurePermissionsOnEtcGroupDashObject = "initEnsurePermissionsOnEtcGroupDash";
static const char* g_initEnsurePermissionsOnEtcAnacronTabObject = "initEnsurePermissionsOnEtcAnacronTab";
static const char* g_initEnsurePermissionsOnEtcCronDObject = "initEnsurePermissionsOnEtcCronD";
static const char* g_initEnsurePermissionsOnEtcCronDailyObject = "initEnsurePermissionsOnEtcCronDaily";
static const char* g_initEnsurePermissionsOnEtcCronHourlyObject = "initEnsurePermissionsOnEtcCronHourly";
static const char* g_initEnsurePermissionsOnEtcCronMonthlyObject = "initEnsurePermissionsOnEtcCronMonthly";
static const char* g_initEnsurePermissionsOnEtcCronWeeklyObject = "initEnsurePermissionsOnEtcCronWeekly";
static const char* g_initEnsurePermissionsOnEtcMotdObject = "initEnsurePermissionsOnEtcMotd";
static const char* g_initEnsureRestrictedUserHomeDirectoriesObject = "initEnsureRestrictedUserHomeDirectories";
static const char* g_initEnsurePasswordHashingAlgorithmObject = "initEnsurePasswordHashingAlgorithm";
static const char* g_initEnsureMinDaysBetweenPasswordChangesObject = "initEnsureMinDaysBetweenPasswordChanges";
static const char* g_initEnsureInactivePasswordLockPeriodObject = "initEnsureInactivePasswordLockPeriod";
static const char* g_initEnsureMaxDaysBetweenPasswordChangesObject = "initEnsureMaxDaysBetweenPasswordChanges";
static const char* g_initEnsurePasswordExpirationObject = "initEnsurePasswordExpiration";
static const char* g_initEnsurePasswordExpirationWarningObject = "initEnsurePasswordExpirationWarning";
static const char* g_initEnsureDefaultUmaskForAllUsersObject = "initEnsureDefaultUmaskForAllUsers";
static const char* g_initEnsurePermissionsOnBootloaderConfigObject = "initEnsurePermissionsOnBootloaderConfig";
static const char* g_initEnsurePasswordReuseIsLimitedObject = "initEnsurePasswordReuseIsLimited";
static const char* g_initEnsurePasswordCreationRequirementsObject = "initEnsurePasswordCreationRequirements";
static const char* g_initEnsureFilePermissionsForAllRsyslogLogFilesObject = "initEnsureFilePermissionsForAllRsyslogLogFiles";
static const char* g_initEnsureUsersDotFilesArentGroupOrWorldWritableObject = "initEnsureUsersDotFilesArentGroupOrWorldWritable";
static const char* g_initEnsureUnnecessaryAccountsAreRemovedObject = "initEnsureUnnecessaryAccountsAreRemoved";
static const char* g_initEnsureDefaultDenyFirewallPolicyIsSetObject = "initEnsureDefaultDenyFirewallPolicyIsSet";

// Default values for checks that support configuration (and initialization)
static const char* g_defaultLoggingLevel = "Informational";
static const char* g_defaultEnsurePermissionsOnEtcIssue = "644";
static const char* g_defaultEnsurePermissionsOnEtcIssueNet = "644";
static const char* g_defaultEnsurePermissionsOnEtcHostsAllow = "644";
static const char* g_defaultEnsurePermissionsOnEtcHostsDeny = "644";
static const char* g_defaultEnsurePermissionsOnEtcShadow = "400";
static const char* g_defaultEnsurePermissionsOnEtcShadowDash = "400";
static const char* g_defaultEnsurePermissionsOnEtcGShadow = "400";
static const char* g_defaultEnsurePermissionsOnEtcGShadowDash = "400";
static const char* g_defaultEnsurePermissionsOnEtcPasswd = "644";
static const char* g_defaultEnsurePermissionsOnEtcPasswdDash = "644";
static const char* g_defaultEnsurePermissionsOnEtcGroup = "644";
static const char* g_defaultEnsurePermissionsOnEtcGroupDash = "644";
static const char* g_defaultEnsurePermissionsOnEtcAnacronTab = "600";
static const char* g_defaultEnsurePermissionsOnEtcCronD = "700";
static const char* g_defaultEnsurePermissionsOnEtcCronDaily = "700";
static const char* g_defaultEnsurePermissionsOnEtcCronHourly = "700";
static const char* g_defaultEnsurePermissionsOnEtcCronMonthly = "700";
static const char* g_defaultEnsurePermissionsOnEtcCronWeekly = "700";
static const char* g_defaultEnsurePermissionsOnEtcMotd = "644";
static const char* g_defaultEnsureRestrictedUserHomeDirectories = "700,750";
static const char* g_defaultEnsurePasswordHashingAlgorithm = "6";
static const char* g_defaultEnsureMinDaysBetweenPasswordChanges = "7";
static const char* g_defaultEnsureInactivePasswordLockPeriod = "30";
static const char* g_defaultEnsureMaxDaysBetweenPasswordChanges = "365";
static const char* g_defaultEnsurePasswordExpiration = "365";
static const char* g_defaultEnsurePasswordExpirationWarning = "7";
static const char* g_defaultEnsureDefaultUmaskForAllUsers = "077";
static const char* g_defaultEnsurePermissionsOnBootloaderConfig = "400";
static const char* g_defaultEnsurePasswordReuseIsLimited = "5";
static const char* g_defaultEnsurePasswordCreationRequirements = "1,14,4,-1,-1,-1,-1";
static const char* g_defaultEnsureFilePermissionsForAllRsyslogLogFiles = "600,640";
static const char* g_defaultEnsureUsersDotFilesArentGroupOrWorldWritable = "600,644,664,700,744";
static const char* g_defaultEnsureUnnecessaryAccountsAreRemoved = "games,test";
static const char* g_defaultEnsureDefaultDenyFirewallPolicyIsSet = "0"; //zero: audit-only, non-zero: add forced remediation

static const char* g_etcIssue = "/etc/issue";
static const char* g_etcIssueNet = "/etc/issue.net";
static const char* g_etcHostsAllow = "/etc/hosts.allow";
static const char* g_etcHostsDeny = "/etc/hosts.deny";
static const char* g_etcCronAllow = "/etc/cron.allow";
static const char* g_etcAtAllow = "/etc/at.allow";
static const char* g_etcCronDeny = "/etc/cron.deny";
static const char* g_etcAtDeny = "/etc/at.deny";
static const char* g_etcShadow = "/etc/shadow";
static const char* g_etcShadowDash = "/etc/shadow-";
static const char* g_etcGShadow = "/etc/gshadow";
static const char* g_etcGShadowDash = "/etc/gshadow-";
static const char* g_etcPasswd = "/etc/passwd";
static const char* g_etcPasswdDash = "/etc/passwd-";
static const char* g_etcPamdPasswordAuth = "/etc/pam.d/password-auth";
static const char* g_etcPamdSystemAuth = "/etc/pam.d/system-auth";
static const char* g_etcPamdLogin = "/etc/pam.d/login";
static const char* g_etcGroup = "/etc/group";
static const char* g_etcGroupDash = "/etc/group-";
static const char* g_etcAnacronTab = "/etc/anacrontab";
static const char* g_etcCronD = "/etc/cron.d";
static const char* g_etcCronDaily = "/etc/cron.daily";
static const char* g_etcCronHourly = "/etc/cron.hourly";
static const char* g_etcCronMonthly = "/etc/cron.monthly";
static const char* g_etcCronWeekly = "/etc/cron.weekly";
static const char* g_etcMotd = "/etc/motd";
static const char* g_etcEnvironment = "/etc/environment";
static const char* g_etcFstab = "/etc/fstab";
static const char* g_etcFstabCopy = "/etc/fstab.copy";
static const char* g_etcInetdConf = "/etc/inetd.conf";
static const char* g_etcModProbeD = "/etc/modprobe.d";
static const char* g_etcProfile = "/etc/profile";
static const char* g_etcRsyslogConf = "/etc/rsyslog.conf";
static const char* g_etcSyslogNgSyslogNgConf = "/etc/syslog-ng/syslog-ng.conf";
static const char* g_etcNetworkInterfaces = "/etc/network/interfaces";
static const char* g_etcSysconfigNetwork = "/etc/sysconfig/network";
static const char* g_etcSysctlConf = "/etc/sysctl.conf";
static const char* g_etcRcLocal = "/etc/rc.local";
static const char* g_etcSambaConf = "/etc/samba/smb.conf";
static const char* g_etcPostfixMainCf = "/etc/postfix/main.cf";
static const char* g_etcCronDailyLogRotate = "/etc/cron.daily/logrotate";
static const char* g_etcSecurityLimitsConf = "/etc/security/limits.conf";
static const char* g_sysCtlConf = "/etc/sysctl.d/99-sysctl.conf";

static const char* g_home = "/home";
static const char* g_devShm = "/dev/shm";
static const char* g_tmp = "/tmp";
static const char* g_varTmp = "/var/tmp";
static const char* g_varLogJournal = "/var/log/journal";
static const char* g_media = "/media/";
static const char* g_nodev = "nodev";
static const char* g_nosuid = "nosuid";
static const char* g_noexec = "noexec";
static const char* g_inetd = "inetd";
static const char* g_inetUtilsInetd = "inetutils-inetd";
static const char* g_rlogin = "rlogin";
static const char* g_xinetd = "xinetd";
static const char* g_rshServer = "rsh-server";
static const char* g_nfs = "nfs";
static const char* g_nis = "nis";
static const char* g_tftp = "tftp";
static const char* g_tftpHpa = "tftpd-hpa";
static const char* g_readAheadFedora = "readahead-fedora";
static const char* g_bluetooth = "bluetooth";
static const char* g_isdnUtilsBase = "isdnutils-base";
static const char* g_kdumpTools = "kdump-tools";
static const char* g_iscDhcpServer = "isc-dhcp-server";
static const char* g_sendmail = "sendmail";
static const char* g_slapd = "slapd";
static const char* g_bind9 = "bind9";
static const char* g_dovecotCore = "dovecot-core";
static const char* g_auoms = "auoms";
static const char* g_audit = "audit";
static const char* g_auditd = "auditd";
static const char* g_auditLibs = "audit-libs";
static const char* g_auditLibsDevel = "audit-libs-devel";
static const char* g_prelink = "prelink";
static const char* g_talk = "talk";
static const char* g_cron = "cron";
static const char* g_crond = "crond";
static const char* g_cronie = "cronie";
static const char* g_syslog = "syslog";
static const char* g_rsyslog = "rsyslog";
static const char* g_syslogNg = "syslog-ng";
static const char* g_systemd = "systemd";
static const char* g_postfix = "postfix";
static const char* g_avahiDaemon = "avahi-daemon";
static const char* g_avahiDaemonSocket = "avahi-daemon.socket";
static const char* g_cups = "cups";
static const char* g_rpcgssd = "rpcgssd";
static const char* g_rpcGssd = "rpc-gssd";
static const char* g_rpcidmapd = "rpcidmapd";
static const char* g_nfsIdmapd = "nfs-idmapd";
static const char* g_rpcbind = "rpcbind";
static const char* g_rpcbindSocket = "rpcbind.socket";
static const char* g_nfsServer = "nfs-server";
static const char* g_legacyNetworkService = "network.service";
static const char* g_snmpd = "snmpd";
static const char* g_rsync = "rsync";
static const char* g_ypserv = "ypserv";
static const char* g_rsh = "rsh";
static const char* g_rshClient = "rsh-client";
static const char* g_forward = "forward";
static const char* g_netrc = "netrc";
static const char* g_rhosts = "rhosts";
static const char* g_systemdJournald = "systemd-journald";
static const char* g_allTelnetd = "*telnetd*";
static const char* g_samba = "samba";
static const char* g_smbd = "smbd";
static const char* g_rpcSvcgssd = "rpc.svcgssd";
static const char* g_needSvcgssd = "NEED_SVCGSSD = yes";
static const char* g_inetInterfacesLocalhost = "inet_interfaces localhost";
static const char* g_autofs = "autofs";
static const char* g_ipv4ll = "ipv4ll";
static const char* g_sysCtlA = "sysctl -a";
static const char* g_fileCreateMode = "$FileCreateMode";
static const char* g_logrotate = "logrotate";
static const char* g_logrotateTimer = "logrotate.timer";
static const char* g_telnet = "telnet";
static const char* g_rcpSocket = "rcp.socket";
static const char* g_rshSocket = "rsh.socket";
static const char* g_hardCoreZero = "*\thard\tcore\t0\n";
static const char* g_fsSuidDumpable = "fs.suid_dumpable = 0";
static const char* g_bootGrubGrubCfg = "/boot/grub/grub.cfg";
static const char* g_bootGrubGrubConf = "/boot/grub/grub.conf";
static const char* g_bootGrub2GrubCfg = "/boot/grub2/grub.cfg";
static const char* g_bootGrub2GrubConf = "/boot/grub2/grub.conf";
static const char* g_minSambaProtocol = "min protocol = SMB2";
static const char* g_login = "login";

static const char* g_remediationIsNotPossible = "automatic remediation is not possible";

static const char* g_pass = SECURITY_AUDIT_PASS;
static const char* g_fail = SECURITY_AUDIT_FAIL;

static char* g_desiredLoggingLevel = NULL;
static char* g_desiredEnsurePermissionsOnEtcIssue = NULL;
static char* g_desiredEnsurePermissionsOnEtcIssueNet = NULL;
static char* g_desiredEnsurePermissionsOnEtcHostsAllow = NULL;
static char* g_desiredEnsurePermissionsOnEtcHostsDeny = NULL;
static char* g_desiredEnsurePermissionsOnEtcShadow = NULL;
static char* g_desiredEnsurePermissionsOnEtcShadowDash = NULL;
static char* g_desiredEnsurePermissionsOnEtcGShadow = NULL;
static char* g_desiredEnsurePermissionsOnEtcGShadowDash = NULL;
static char* g_desiredEnsurePermissionsOnEtcPasswd = NULL;
static char* g_desiredEnsurePermissionsOnEtcPasswdDash = NULL;
static char* g_desiredEnsurePermissionsOnEtcGroup = NULL;
static char* g_desiredEnsurePermissionsOnEtcGroupDash = NULL;
static char* g_desiredEnsurePermissionsOnEtcAnacronTab = NULL;
static char* g_desiredEnsurePermissionsOnEtcCronD = NULL;
static char* g_desiredEnsurePermissionsOnEtcCronDaily = NULL;
static char* g_desiredEnsurePermissionsOnEtcCronHourly = NULL;
static char* g_desiredEnsurePermissionsOnEtcCronMonthly = NULL;
static char* g_desiredEnsurePermissionsOnEtcCronWeekly = NULL;
static char* g_desiredEnsurePermissionsOnEtcMotd = NULL;
static char* g_desiredEnsureRestrictedUserHomeDirectories = NULL;
static char* g_desiredEnsurePasswordHashingAlgorithm = NULL;
static char* g_desiredEnsureMinDaysBetweenPasswordChanges = NULL;
static char* g_desiredEnsureInactivePasswordLockPeriod = NULL;
static char* g_desiredEnsureMaxDaysBetweenPasswordChanges = NULL;
static char* g_desiredEnsurePasswordExpiration = NULL;
static char* g_desiredEnsurePasswordExpirationWarning = NULL;
static char* g_desiredEnsureDefaultUmaskForAllUsers = NULL;
static char* g_desiredEnsurePermissionsOnBootloaderConfig = NULL;
static char* g_desiredEnsurePasswordReuseIsLimited = NULL;
static char* g_desiredEnsurePasswordCreationRequirements = NULL;
static char* g_desiredEnsureFilePermissionsForAllRsyslogLogFiles = NULL;
static char* g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable = NULL;
static char* g_desiredEnsureUnnecessaryAccountsAreRemoved = NULL;
static char* g_desiredEnsureDefaultDenyFirewallPolicyIsSet = NULL;

static const int g_shadowGid = 42;
static const int g_varLogJournalMode = 02755;

static PerfClock g_perfClock = {{0, 0}, {0, 0}};

// Expected time limits under ideal conditions
// Maximum per-rule audit time: 5 seconds
static const long g_maxAuditTime = 5000000;
// Maximum ASB rule remediation time: 5 minutes
static const long g_maxRemediateTime = 300000000;
// Maximum baseline run times: 30 minutes
static const long g_maxTotalTime = 1800000000;

static char* g_prettyName = NULL;

static bool g_auditOnly = true;

static OsConfigLogHandle g_perfLog = NULL;

OsConfigLogHandle GetPerfLog(void)
{
    return g_perfLog;
}

typedef struct BaselineRule
{
    const char* resourceId;
    const char* ruleId;
    const char* payloadKey;
} BaselineRule;

const BaselineRule g_rules[] =
{
    { "Ensure permissions on /etc/ssh/sshd_config are configured (CIS: L1 - Server - 5.2.1)", "43119747-263c-2c92-4ce5-726e63259049", "EnsurePermissionsOnEtcSshSshdConfig" },
    { "Ensure that the SSH protocol is configured (CIS: L1 - Server - 5.2.4)", "35868e8c-97eb-4981-ab79-99b25101cc86", "EnsureSshBestPracticeProtocol" },
    { "Ensure that the SSH IgnoreRhosts is configured (CIS: L1 - Server - 5.2.8)", "43945588-1bdc-495c-bac8-6a71a62d30aa", "EnsureSshBestPracticeIgnoreRhosts" },
    { "Ensure that the SSH LogLevel is configured (CIS: L1 - Server - 5.2.5)", "31f1a912-1b98-42fd-8381-1e8d1033bfd1", "EnsureSshLogLevelIsSet" },
    { "Ensure that the SSH MaxAuthTries is configured (CIS: L1 - Server - 5.2.7)", "e7708534-5d98-406f-83ae-1de835b2906e", "EnsureSshMaxAuthTriesIsSet" },
    { "Ensure that the allowed users for SSH access are configured (CIS: L1 - Server - 5.2.18)", "dc8da71d-aeba-4c03-8835-36fe158e372a", "EnsureAllowUsersIsConfigured" },
    { "Ensure that the denied users for SSH are configured (CIS: L1 - Server - 5.2.18)", "1db7d147-3a75-4637-8cdc-c119e22d0206", "EnsureDenyUsersIsConfigured" },
    { "Ensure that the allowed groups for SSH are configured (CIS: L1 - Server - 5.2.18)", "d9868b1f-0c8e-4adb-889e-14e374874ece", "EnsureAllowGroupsIsConfigured" },
    { "Ensure that the denied groups for SSH are configured (CIS: L1 - Server - 5.2.18)", "f85c04a4-427b-4ee1-b988-c3ff7b4998cc", "EnsureDenyGroupsConfigured" },
    { "Ensure that the SSH HostBasedAuthentication is configured (CIS: L1 - Server - 5.2.9)", "66511f6b-f690-43df-9654-642260699eec", "EnsureSshHostbasedAuthenticationIsDisabled" },
    { "Ensure that the SSH PermitRootLogin is configured (CIS: L1 - Server - 5.2.10)", "16511f6b-f690-43df-9654-642260699eec", "EnsureSshPermitRootLoginIsDisabled" },
    { "Ensure that the SSH PermitEmptyPasswords is configured (CIS: L1 - Server - 5.2.11)", "d50c3f39-264c-4cdc-b0ba-89de8a0f6828", "EnsureSshPermitEmptyPasswordsIsDisabled" },
    { "Ensure that the SSH ClientAliveCountMax is configured", "fd9f1554-6b72-8610-826e-78578e6f7811", "EnsureSshClientIntervalCountMaxIsConfigured" },
    { "Ensure that the SSH ClientAliveInterval is configured", "c848bba4-93b1-4756-b08a-ff94c2b4814c", "EnsureSshClientAliveIntervalIsConfigured" },
    { "Ensure that the SSH LoginGraceTime is configured (CIS: L1 - Server - 5.2.17)", "39aa5c2b-5b36-84a7-4022-570a53c86ff9", "EnsureSshLoginGraceTimeIsSet" },
    { "Ensure that only approved MAC algorithms are used (CIS: L1 - Server - 5.2.14)", "483e8b0c-7572-8467-7e0c-26e1e9ce6359", "EnsureOnlyApprovedMacAlgorithmsAreUsed" },
    { "Ensure that the SSH warning banner is configured (CIS: L1 - Server - 5.2.19)", "9e240540-5e0a-4b60-beb2-57421c65a0b9", "EnsureSshWarningBannerIsEnabled" },
    { "Ensure that the SSH PermitUserEnvironment is configured (CIS: L1 - Server - 5.2.12)", "0e665978-91f4-45af-bb7b-e4090b600c8d", "EnsureUsersCannotSetSshEnvironmentOptions" },
    { "Ensure that appropriate ciphers are used for SSH (CIS: L1 - Server - 5.2.13)", "b07d4c69-a1d9-4c22-a486-634ec3b8c380", "EnsureAppropriateCiphersForSsh" },
    { "Ensure that the SSH port is configured", "1310042d-193f-414a-a7e3-b2c4e89aa537", "EnsureSshPortIsConfigured" },
    { "Ensure nodev option set on /home partition (CIS: L1 - Server - 1.1.14)", "a87f15ed-1115-a22d-6f43-17db97c91111", "EnsureNodevOptionOnHomePartition" },
    { "Ensure nodev option set on /tmp partition (CIS: L1 - Server - 1.1.3)", "a87f15ed-1115-a22d-6f43-17db97c91161", "EnsureNodevOptionOnTmpPartition" },
    { "Ensure nodev option set on /var/tmp partition (CIS: L1 - Server - 1.1.8)", "a87f15ed-1115-a22d-6f43-17db97c91181", "EnsureNodevOptionOnVarTmpPartition" },
    { "Ensure nosuid option set on /tmp partition (CIS: L1 - Server - 1.1.4)", "a87f15ed-1115-a22d-6f43-17db97c91191", "EnsureNosuidOptionOnTmpPartition" },
    { "Ensure nosuid option set on /var/tmp partition (CIS: L1 - Server - 1.1.9)", "a87f15ed-1115-a22d-6f43-17db97c91171", "EnsureNosuidOptionOnVarTmpPartition" },
    { "Ensure noexec option set on /var/tmp partition (CIS: L1 - Server - 1.1.10)", "b97f15dd-8185-a12d-6f43-17db97c98477", "EnsureNoexecOptionOnVarTmpPartition" },
    { "Ensure noexec option set on /dev/shm partition (CIS: L1 - Server - 1.1.17)", "11e6dc3b-0659-3dc8-7d12-8eb0bb6c0890", "EnsureNoexecOptionOnDevShmPartition" },
    { "Ensure automounting is disabled (CIS: L1 - Server - 1.1.22)", "8cffbbca-3e5b-9af9-65fe-ff7b6316565d", "EnsureAutomountingDisabled" },
    { "Ensure mounting of USB storage devices is disabled (CIS: L1 - Server - 1.1.23)", "acffbbca-3e5b-9aa9-65ee-ff7b6116565f", "EnsureMountingOfUsbStorageDevicesIsDisabled" },
    { "Ensure core dumps are restricted (CIS: L1 - Server - 1.5.1)", "429123d4-7e7d-2737-6542-2e86b82c16a1", "EnsureCoreDumpsAreRestricted" },
    { "Ensure prelink is disabled (CIS: L1 - Server - 1.5.4)", "cdee3703-321a-69c4-107f-e7b350a95e5a", "EnsurePrelinkIsDisabled" },
    { "Ensure permissions on /etc/motd are configured (CIS: L1 - Server - 1.7.1.4)", "b08735e4-8565-0ab1-0fba-b15e31be98e4", "EnsurePermissionsOnEtcMotd" },
    { "Ensure permissions on /etc/issue are configured (CIS: L1 - Server - 1.7.1.5)", "69032884-324a-3289-a171-b17a541f7adb", "EnsurePermissionsOnEtcIssue" },
    { "Ensure permissions on /etc/issue.net are configured (CIS: L1 - Server - 1.7.1.6)", "9110c0c6-27f1-065e-140d-2672c1ad055b", "EnsurePermissionsOnEtcIssueNet" },
    { "Ensure the nodev option is enabled for all removable media (CIS: L1 - Server - 1.1.18)", "5c7537f2-b90b-44a4-89c9-4fca5fd79ef7", "EnsureNodevOptionEnabledForAllRemovableMedia" },
    { "Ensure the noexec option is enabled for all removable media (CIS: L1 - Server - 1.1.20)", "7976cc38-fddb-4913-9295-4fcac2e641c3", "EnsureNoexecOptionEnabledForAllRemovableMedia" },
    { "Ensure the nosuid option is enabled for all removable media (CIS: L1 - Server - 1.1.19)", "cdc390c9-fb4a-47f6-90a7-4e1bd6d0e9e6", "EnsureNosuidOptionEnabledForAllRemovableMedia" },
    { "Ensure talk client is not installed (CIS: L1 - Server - 2.3.3)", "5133422e-990f-971e-661d-ccfc913c1d2c", "EnsureTalkClientIsNotInstalled" },
    { "Ensure permissions on /etc/hosts.allow are configured (CIS: L1 - Server - 3.3.2 + 3.3.4)", "1d498679-5780-6db3-14cc-6433011e0310", "EnsurePermissionsOnEtcHostsAllow" },
    { "Ensure permissions on /etc/hosts.deny are configured (CIS: L1 - Server - 3.3.3 + 3.3.5)", "71d554b5-1436-9676-1966-939ded8d0a37", "EnsurePermissionsOnEtcHostsDeny" },
    { "Ensure default deny firewall policy is set (CIS: L1 - Server - 3.5.2.1)", "eb6c608a-2316-41bd-aa00-604249e31f27", "EnsureDefaultDenyFirewallPolicyIsSet" },
    { "Ensure the nodev/nosuid option is enabled for all NFS mounts", "7ca24433-3c08-4ff5-9fe2-d8e1830c5829", "EnsureNoexecNosuidOptionsEnabledForAllNfsMounts" },
    { "Ensure password creation requirements are configured (CIS: L1 - Server - 5.3.1)", "b042fda5-55f6-17ba-78d5-1ba33a4709f5", "EnsurePasswordCreationRequirements" },
    { "Ensure lockout for failed password attempts is configured (CIS: L1 - Server - 5.3.2)", "1895704d-a326-90ca-3795-3a1289e7482d", "EnsureLockoutForFailedPasswordAttempts" },
    { "Ensure the cramfs file system is disabled (CIS: L1 - Server - 1.1.1.1)", "9967cbaf-44be-0dd1-92ab-d4f4034b2d28", "EnsureDisabledInstallationOfCramfsFileSystem" },
    { "Ensure the freevxfs file system is disabled (CIS: L1 - Server - 1.1.1.2)", "4c066a3d-8eba-a210-3228-cff300039363", "EnsureDisabledInstallationOfFreevxfsFileSystem" },
    { "Ensure all users' home directories exist (CIS: L1 - Server - 6.2.7)", "c07e6adc-93ab-1d40-2c6d-f3f16ca9561d", "EnsureAllUsersHomeDirectoriesExist" },
    { "Ensure users own their home directories (CIS: L1 - Server - 6.2.9)", "54e255d1-038a-860b-48a2-3d1821e34661", "EnsureUsersOwnTheirHomeDirectories" },
    { "Ensure users' dot files aren't group or world writable (CIS: L1 - Server - 6.2.10)", "086cb95b-5427-3ac0-9b98-c5c5b593579a", "EnsureUsersDotFilesArentGroupOrWorldWritable" },
    { "Ensure no users have .forward files (CIS: L1 - Server - 6.2.11)", "d02297d2-9f49-5276-0a92-5ac1e42e243c", "EnsureNoUsersHaveDotForwardFiles" },
    { "Ensure no users have .netrc files (CIS: L1 - Server - 6.2.12)", "c1b7767e-6c84-6a54-10b8-b9797a8d707a", "EnsureNoUsersHaveDotNetrcFiles" },
    { "Ensure no users have .rhosts files (CIS: L1 - Server - 6.2.14)", "28884fab-89e2-2250-a057-6f35637b4848", "EnsureNoUsersHaveDotRhostsFiles" },
    { "Ensure all groups in /etc/passwd exist in /etc/group (CIS: L1 - Server - 6.2.15)", "835a98f5-46da-5332-6b45-38df223e9d9a", "EnsureAllEtcPasswdGroupsExistInEtcGroup" },
    { "Ensure no duplicate UIDs exist (CIS: L1 - Server - 6.2.16)", "df7fd9e2-0cd0-47c1-92b0-f3cab6e06869", "EnsureNoDuplicateUidsExist" },
    { "Ensure no duplicate GIDs exist (CIS: L1 - Server - 6.2.17)", "70f3d785-1410-440f-6f6d-79b051039d21", "EnsureNoDuplicateGidsExist" },
    { "Ensure no duplicate user names exist (CIS: L1 - Server - 6.2.18)", "017e0242-25b9-2c72-7ac6-d601086f5c8b", "EnsureNoDuplicateUserNamesExist" },
    { "Ensure no duplicate groups exist (CIS: L1 - Server - 6.2.19)", "bc5844b6-7e40-4f2f-7e81-1cbd18885dae", "EnsureNoDuplicateGroupsExist" },
    { "Ensure shadow group is empty (CIS: L1 - Server - 6.2.20)", "cbdc33fa-0f1c-0a90-7fe9-8ef1cf6643c2", "EnsureShadowGroupIsEmpty" },
    { "Ensure the hfs file system is disabled (CIS: L1 - Server - 1.1.1.4)", "39595d95-88a4-78e2-6e0e-fbde7fd95eed", "EnsureDisabledInstallationOfHfsFileSystem" },
    { "Ensure the hfsplus file systems is disabled (CIS: L1 - Server - 1.1.1.5)", "68fb9c92-1009-9e24-694e-3d996a5e09c5", "EnsureDisabledInstallationOfHfsplusFileSystem" },
    { "Ensure the jffs2 file systems is disabled (CIS: L1 - Server - 1.1.1.3)", "859c7aa0-6eeb-6aac-6160-2fdead2537bf", "EnsureDisabledInstallationOfJffs2FileSystem" },
    { "Ensure the kernel is compiled from approved sources", "34e19f66-2387-4cdc-8ab2-cfac8e5865f0", "EnsureKernelCompiledFromApprovedSources" },
    { "Ensure file permissions on /etc/shadow are configured (CIS: L1 - Server - 6.1.3)", "13dabe7c-02ea-09d2-1a97-42cc7ac94eaa", "EnsurePermissionsOnEtcShadow" },
    { "Ensure file permissions on /etc/shadow- are configured (CIS: L1 - Server - 6.1.7)", "1e941132-a3a7-5ccb-1817-50551b65202e", "EnsurePermissionsOnEtcShadowDash" },
    { "Ensure file permissions on /etc/gshadow are configured (CIS: L1 - Server - 6.1.5)", "0a7f2b28-8586-6cef-512e-a28f991d83cd", "EnsurePermissionsOnEtcGShadow" },
    { "Ensure file permissions on /etc/gshadow- are configured (CIS: L1 - Server - 6.1.9)", "0fe59dec-472c-4b11-a221-36053a47afb6", "EnsurePermissionsOnEtcGShadowDash" },
    { "Ensure file permissions on /etc/passwd are configured (CIS: L1 - Server - 6.1.2)", "ad534c97-1070-415c-9fc7-c92366d3fc30", "EnsurePermissionsOnEtcPasswd" },
    { "Ensure file permissions on /etc/group are configured (CIS: L1 - Server - 6.1.4)", "c41a47e9-1ba0-4e72-9f43-4659a4bfed63", "EnsurePermissionsOnEtcGroup" },
    { "Ensure file permissions on /etc/passwd- are configured (CIS: L1 - Server - 6.1.6)", "0c67cac0-1e99-8a8f-32e1-841d18b01a9a", "EnsurePermissionsOnEtcPasswdDash" },
    { "Ensure file permissions on /etc/group- are configured (CIS: L1 - Server - 6.1.8)", "865ebb92-8e64-4e3a-aa9b-0290768aa8f1", "EnsurePermissionsOnEtcGroupDash" },
    { "Ensure that access to the root account via su is restricted to the 'root' group (CIS: L1 - Server - 5.5)", "0c77cac0-1e99-8a8f-32e1-841d18b01a9a", "EnsureSuRestrictedToRootGroup" },
    { "Ensure the 'root' group exists, and contain all members who can su to root (CIS: L1 - Server - 5.6)", "8cac0c32-1add-42b9-9300-5ccb9f91aab3", "EnsureRootGroupExists" },
    { "Ensure that all user accounts that can login have a password set", "ca9d29b7-79bd-4c99-85e2-1454295c3c8e", "EnsureAllAccountsHavePasswords" },
    { "Ensure that all user accounts other than root have unique UIDs greater than zero (0)", "7de0f0e6-f97b-4e12-8f9e-c6538ca5a85b", "EnsureNonRootAccountsHaveUniqueUidsGreaterThanZero" },
    { "Ensure that randomized placement (ASLR) of virtual memory regions is enabled (CIS: L1 - Server - 1.5.3)", "d790e942-efd3-42e6-a3a5-9eb1d651a588", "EnsureVirtualMemoryRandomizationIsEnabled" },
    { "Ensure that kernel support for the XD/NX processor feature is enabled (CIS: L1 - Server - 1.5.2)", "49c89437-d116-4d84-a91d-0dd59daafa0d", "EnsureKernelSupportForCpuNx" },
    { "Ensure '.' does not appear in root's $PATH (CIS: L1 - Server - 6.2.6)", "d66f8908-7b9f-77fc-18d4-af85197e0aeb", "EnsureDotDoesNotAppearInRootsPath" },
    { "Ensure access to user home directories is restricted", "0754488a-75c7-a4e8-0fb4-9212f771623f", "EnsureRestrictedUserHomeDirectories" },
    { "Ensure the default umask for all users is configured", "0753438a-75c7-a4e8-0fb4-9213f771623f", "EnsureDefaultUmaskForAllUsers" },
    { "Ensure all bootloaders have password protection enabled (CIS: L1 - Server - 1.4.2)", "8a4f5ce8-41c4-710c-631e-fbc36a2fa53e", "EnsureAllBootloadersHavePasswordProtectionEnabled" },
    { "Ensure permissions on bootloader config are configured (CIS: L1 - Server - 1.4.1)", "091f0150-80d1-0e2d-7353-8cdb77fc6aa1", "EnsurePermissionsOnBootloaderConfig" },
    { "Ensure authentication is required for single user mode (CIS: L1 - Server - 1.4.3)", "13a48ca1-92bc-63a1-a4de-b984375fa332", "EnsureAuthenticationRequiredForSingleUserMode" },
    { "Ensure packet redirect sending is disabled (CIS: L1 - Server - 3.1.2)", "5ea9d618-1af4-4e59-65be-ffac234872e9", "EnsurePacketRedirectSendingIsDisabled" },
    { "Ensure ICMP redirects are not accepted (CIS: L1 - Server - 3.2.2)", "a492f72a-6b79-8a9d-3b4f-3fface972ab7", "EnsureIcmpRedirectsIsDisabled" },
    { "Ensure accepting source routed packets is disabled for all interfaces (CIS: L1 - Server - 3.2.1)", "4ecae4e6-a3e2-44f5-9985-ea2a21962450", "EnsureSourceRoutedPacketsIsDisabled" },
    { "Ensure the default setting for accepting source routed packets is disabled for network interfaces (CIS: L1 - Server - 3.2.1)", "63613c25-8ae1-4792-9572-02bdc941febf", "EnsureAcceptingSourceRoutedPacketsIsDisabled" },
    { "Ensure ignoring bogus ICMP responses to broadcasts is enabled (CIS: L1 - Server - 3.2.6)", "88acc143-2f76-4418-9aa9-d0d5f244ab5f", "EnsureIgnoringBogusIcmpBroadcastResponses" },
    { "Ensure ignoring ICMP echo requests (pings) sent to broadcast/multicast addresses is enabled (CIS: L1 - Server - 3.2.5)", "f5a5926d-9c64-41fa-8220-5bc0f8213550", "EnsureIgnoringIcmpEchoPingsToMulticast" },
    { "Ensure logging of martian packets (those with impossible addresses) is enabled for all interfaces", "dc1c08a3-91e8-1d60-9210-c18bdebd8778", "EnsureMartianPacketLoggingIsEnabled" },
    { "Ensure performing source validation by reverse path is enabled for all interfaces (CIS: L1 - Server - 3.2.7)", "177e6190-1026-49fb-a1f9-fd5b10302280", "EnsureReversePathSourceValidationIsEnabled" },
    { "Ensure TCP SYN cookies are enabled (CIS: L1 - Server - 3.2.8)", "db6ca14e-26c5-48cd-a6b7-fc953861043c", "EnsureTcpSynCookiesAreEnabled" },
    { "Ensure the system does not act as a network sniffer", "45766f27-5af3-453d-bade-f8195925cde1", "EnsureSystemNotActingAsNetworkSniffer" },
    { "Ensure all wireless interfaces are disabled (CIS: L1 - Server - 3.6)", "8def2d0c-303a-4c0b-858c-319f80f7c814", "EnsureAllWirelessInterfacesAreDisabled" },
    { "Ensure the IPv6 protocol is enabled", "f04b1de8-1fd3-40da-a27f-39b7ea97bf8c", "EnsureIpv6ProtocolIsEnabled" },
    { "Ensure DCCP is disabled", "93d2736e-7329-8806-3ef6-e71bb2203d11", "EnsureDccpIsDisabled" },
    { "Ensure SCTP is disabled", "78228616-15d4-33fe-0357-88e77f228f05", "EnsureSctpIsDisabled" },
    { "Ensure support for RDS is disabled", "d9ed5e76-2348-4409-94dd-c76352407fe8", "EnsureDisabledSupportForRds" },
    { "Ensure TIPC is disabled", "8ace9b14-820f-6e0d-37d8-c6df950454cd", "EnsureTipcIsDisabled" },
    { "Ensure logging is configured (CIS: L1 - Server - 4.2.1.2 + 4.2.1.3)", "15ca11bf-034a-56d6-564f-2f857d1f96c6", "EnsureLoggingIsConfigured" },
    { "Ensure the syslog, rsyslog, or syslog-ng package is installed (CIS: L1 - Server - 4.2.1.1)", "8720959b-c356-4eaa-bb4f-720fb8006183", "EnsureSyslogPackageIsInstalled" },
    { "Ensure the systemd-journald service persists log messages (CIS: L1 - Server - 4.2.2.3)", "7b3aa0e4-0464-4a0e-a265-9a585c4f266e", "EnsureSystemdJournaldServicePersistsLogMessages" },
    { "Ensure a logging service is enabled", "4c4e42e2-4cd2-3eaf-147b-ea4f61164d3e", "EnsureALoggingServiceIsEnabled" },
    { "Ensure file permissions for all rsyslog log files are configured (CIS: L1 - Server - 4.2.1.4)", "fcc86485-487a-4644-87a0-f29f1b1cd28b", "EnsureFilePermissionsForAllRsyslogLogFiles" },
    { "Ensure logger configuration files are restricted (CIS: L1 - Server - 4.2.1.4)", "afd57de2-4365-9949-79f7-f8e92c198746", "EnsureLoggerConfigurationFilesAreRestricted" },
    { "Ensure that all rsyslog log files are owned by the adm group", "c1d99621-913e-45f7-96e1-a60b1af83015", "EnsureAllRsyslogLogFilesAreOwnedByAdmGroup" },
    { "Ensure all rsyslog log files are owned by the syslog user (CIS: L1 - Server - 4.2.1.4)", "2830790c-5b3f-43cb-be6b-7572e441acc1", "EnsureAllRsyslogLogFilesAreOwnedBySyslogUser" },
    { "Ensure rsyslog does not accept remote messages", "1e9567e1-d96d-4f90-be1a-0809947e789c", "EnsureRsyslogNotAcceptingRemoteMessages" },
    { "Ensure the logrotate (syslog rotater) service is enabled (CIS: L1 - Server - 4.3)", "2d2355e7-7b07-4c0e-a395-16499c27ae94", "EnsureSyslogRotaterServiceIsEnabled" },
    { "Ensure the rlogin service is disabled", "f57ef648-bdaa-45a3-9e3a-f4649c48896f", "EnsureRloginServiceIsDisabled" },
    { "Ensure inetd is not installed (CIS: L1 - Server - 2.1.10)", "a8a37e7f-9aae-41cf-8313-42d1f69506b9", "EnsureInetdNotInstalled" },
    { "Ensure xinetd is not installed (CIS: L1 - Server - 2.1.10)", "1d9557b2-b58f-4f81-bde9-4f9b08a3b2f1", "EnsureXinetdNotInstalled" },
    { "Ensure the telnet service is disabled (CIS: L1 - Server - 2.1.8)", "0617b91c-2a28-42bd-b5b3-7562555b41ed", "EnsureTelnetServiceIsDisabled" },
    { "Ensure all telnetd packages are uninstalled", "6c716f88-a252-4fe9-9c5c-ba9236a80beb", "EnsureAllTelnetdPackagesUninstalled" },
    { "Ensure the rcp/rsh service is disabled", "dda66a42-30d1-4621-9565-f09628ac8047", "EnsureRcprshServiceIsDisabled" },
    { "Ensure the rsh-server package is uninstalled (CIS: L1 - Server - 2.1.6)", "b256491f-f804-4c44-bfa4-057dd2f44c30", "EnsureRshServerNotInstalled" },
    { "Ensure the ypbind service is disabled and the nis package is not installed", "58f5187e-88bd-4f24-8570-2c295d5c93c6", "EnsureNisNotInstalled" },
    { "Ensure the tftp service is disabled (CIS: L1 - Server - 2.1.9)", "cb086aef-fec2-467f-a03b-627c00020926", "EnsureTftpServiceisDisabled" },
    { "Ensure the tftpd package is not installed (CIS: L1 - Server - 2.1.9)", "ae9ce111-ef4d-4d34-8f76-fdc38263f153", "EnsureTftpdNotInstalled" },
    { "Ensure the readahead-fedora package is not installed", "dbae0d26-55e9-49d5-8782-86cb7412f99f", "EnsureReadaheadFedoraNotInstalled" },
    { "Ensure the bluetooth package is not installed", "9f107bb8-eaf3-445d-acbb-7ab635b442e9", "EnsureBluetoothHiddNotInstalled" },
    { "Ensure the isdn service is disabled and the isdnutils-base package is not installed", "51ebf409-911a-4d92-9d3a-1e331e7e4b27", "EnsureIsdnUtilsBaseNotInstalled" },
    { "Ensure the kdump service is disabled and the kdump-tools package is not installed", "290d7102-c4e3-4e88-863d-6ddc7e952a5a", "EnsureIsdnUtilsKdumpToolsNotInstalled" },
    { "Ensure zeroconf networking is disabled", "083550af-f4fe-4e1a-a304-dac894d58908", "EnsureZeroconfNetworkingIsDisabled" },
    { "Ensure the cron service is enabled (CIS: L1 - Server - 5.1.1)", "80302f61-d760-4165-a92b-a789e579380f", "EnsureCronServiceIsEnabled" },
    { "Ensure File permissions on /etc/anacrontab are configured", "8199ae98-8d9c-4a26-88ca-e6d9b87d3644", "EnsurePermissionsOnEtcAnacronTab" },
    { "Ensure permissions on /etc/cron.d are configured (CIS: L1 - Server - 5.1.7)", "efa30987-4c67-73f5-979f-cb50f79466de", "EnsurePermissionsOnEtcCronD" },
    { "Ensure permissions on /etc/cron.daily are configured (CIS: L1 - Server - 5.1.4)", "0cc35843-7687-60cf-5280-bb98cf9a87c2", "EnsurePermissionsOnEtcCronDaily" },
    { "Ensure permissions on /etc/cron.hourly are configured (CIS: L1 - Server - 5.1.3)", "ecdce8a2-9986-5833-8211-baf1938c1940", "EnsurePermissionsOnEtcCronHourly" },
    { "Ensure permissions on /etc/cron.monthly are configured (CIS: L1 - Server - 5.1.6)", "b5e94c1f-4d12-8bde-4c5e-98c651bd4430", "EnsurePermissionsOnEtcCronMonthly" },
    { "Ensure permissions on /etc/cron.weekly are configured (CIS: L1 - Server - 5.1.5)", "61417e01-8cc4-86ab-0e3b-867d42dea66d", "EnsurePermissionsOnEtcCronWeekly" },
    { "Ensure at/cron is restricted to authorized users (CIS: L1 - Server - 5.1.8)", "7ac5f3c8-917b-548e-3138-c6e326a362fc", "EnsureAtCronIsRestrictedToAuthorizedUsers" },
    { "Ensure the remote login warning banner is properly configured (CIS: L1 - Server - 1.7.1.3)", "d8bba1aa-69e9-2caf-1632-72938ff6759a", "EnsureRemoteLoginWarningBannerIsConfigured" },
    { "Ensure the local login warning banner is properly configured (CIS: L1 - Server - 1.7.1.2)", "14c168de-4508-a69d-0feb-2f1efe245a71", "EnsureLocalLoginWarningBannerIsConfigured" },
    { "Ensure the avahi-daemon service is disabled (CIS: L1 - Server - 2.2.3)", "c3bf78d8-43a0-4768-b790-c940621057b6", "EnsureAvahiDaemonServiceIsDisabled" },
    { "Ensure the cups service is disabled (CIS: L1 - Server - 2.2.4)", "4854666c-061b-4945-8a25-19133b8d5c7d", "EnsureCupsServiceisDisabled" },
    { "Ensure the isc-dhcp-server package is not installed (CIS: L1 - Server - 2.2.5)", "660fa012-ca99-4314-a2a8-11728020bac7", "EnsureIscDhcpdServerNotInstalled" },
    { "Ensure the sendmail package is not installed", "43356a32-24bb-401c-9746-a27b2be668fa", "EnsureSendmailNotInstalled" },
    { "Ensure the postfix package is not installed", "f56bf32f-528f-48b3-9f82-62f5ff4e9787", "EnsurePostfixPackageIsUninstalled" },
    { "Ensure that postfix network listening is disabled", "d0cc4e35-70a1-4ee5-b572-3b969201562e", "EnsurePostfixNetworkListeningIsDisabled" },
    { "Ensure the ldap service is disabled and the sldapd package is not installed (CIS: L1 - Server - 2.2.6)", "b577b358-6ec9-4ed7-b0df-259e44713b16", "EnsureSldapdNotInstalled" },
    { "Ensure the rpcgssd service is disabled", "9c11dc9f-ab7e-4c3f-923f-5a8fc4e97cb9", "EnsureRpcgssdServiceIsDisabled" },
    { "Ensure the rpcidmapd service is disabled", "b600d670-5b01-4458-9143-8aa7cd25dadc", "EnsureRpcidmapdServiceIsDisabled" },
    { "Ensure the portmap service is disabled", "f4a80328-1d67-45ed-b915-274d2e6c699e", "EnsurePortmapServiceIsDisabled" },
    { "Ensure the Network File System (NFS) service is disabled (CIS: L1 - Server - 2.2.7 - partial)", "ee372ff3-9221-498b-b467-7406bf421168", "EnsureNetworkFileSystemServiceIsDisabled" },
    { "Ensure the rpcsvcgssd service is disabled", "78963287-11b9-471b-9122-e6829e105989", "EnsureRpcsvcgssdServiceIsDisabled" },
    { "Ensure the named service is disabled and the bind9 package is not installed (CIS: L1 - Server - 2.2.8)", "361a6cb4-f761-426f-a9d0-9e82ec0b3285", "EnsureBind9NotInstalled" },
    { "Ensure the dovecot service is disabled and the dovecot-core package is not installed", "b0b6cf96-bd8a-40c5-b051-4615078a0bf0", "EnsureDovecotCoreNotInstalled" },
    { "Ensure no legacy + entries exist in /etc/passwd (CIS: L1 - Server - 6.2.2)", "cc627f0c-3b72-6cc9-36b0-d2a3957431f4", "EnsureNoLegacyPlusEntriesInEtcPasswd" },
    { "Ensure no legacy + entries exist in /etc/shadow (CIS: L1 - Server - 6.2.3)", "1a102d2e-76db-5ccf-3580-ccda064e8df3", "EnsureNoLegacyPlusEntriesInEtcShadow" },
    { "Ensure no legacy + entries exist in /etc/group (CIS: L1 - Server - 6.2.4)", "b487e075-15b4-0df1-550f-d8d5edd78eb4", "EnsureNoLegacyPlusEntriesInEtcGroup" },
    { "Ensure password expiration is configured (CIS: L1 - Server - 5.4.1.1)", "6ee7250d-142f-57db-25d0-c58207135059", "EnsurePasswordExpiration" },
    { "Ensure password expiration warning days is configured (CIS: L1 - Server - 5.4.1.3)", "b7ac978f-0963-a61f-58ae-5cf84ab76b5d", "EnsurePasswordExpirationWarning" },
    { "Ensure password reuse is limited (CIS: L1 - Server - 5.3.3)", "57bdeae2-863e-14fb-a463-d6ee64816b33", "EnsurePasswordReuseIsLimited" },
    { "Ensure the password hashing algorithm is SHA-512 (CIS: L1 - Server - 5.3.4)", "01ec5346-882b-485d-8960-01dedd608792", "EnsurePasswordHashingAlgorithm" },
    { "Ensure the minimum days between password changes is configured (CIS: L1 - Server - 5.4.1.2)", "50272a84-672d-4c11-a67e-9d058adaf67a", "EnsureMinDaysBetweenPasswordChanges" },
    { "Ensure the maximum days between password changes is configured", "7f84f3a4-360d-494c-aa1e-1e15ae9fc9ac", "EnsureMaxDaysBetweenPasswordChanges" },
    { "Ensure the inactive password lock period is configured (CIS: L1 - Server - 5.4.1.4)", "91fbaeac-f5d0-4ac9-aa1b-52215aef1ed8", "EnsureInactivePasswordLockPeriod" },
    { "Ensure all system accounts are non-login (CIS: L1 - Server - 5.4.2)", "448b668a-738c-420b-b332-51ea49922933", "EnsureSystemAccountsAreNonLogin" },
    { "Ensure default group for the root account is GID 0 (CIS: L1 - Server - 5.4.3)", "732fa92f-647e-47b8-b5a4-fdf00b02d9e2", "EnsureDefaultRootAccountGroupIsGidZero" },
    { "Ensure root is the only UID 0 account (CIS: L1 - Server - 6.2.5)", "b5845ff3-42f4-4112-b2a2-5b827232a053", "EnsureRootIsOnlyUidZeroAccount" },
    { "Ensure unnecessary accounts are removed", "627b7494-0e62-4093-9f77-db8d526d036b", "EnsureUnnecessaryAccountsAreRemoved" },
    { "Ensure the auditd package is installed", "f9fd03d2-75e4-4564-84a9-4e955f1e7c30", "EnsureAuditdInstalled" },
    { "Ensure the auditd service is running", "c146c4a4-5eb6-4205-88da-5a71a82f2d45", "EnsureAuditdServiceIsRunning" },
    { "Ensure the SNMP Server is not enabled (CIS: L1 - Server - 2.2.14)", "ca1aea32-3969-49ab-abfc-2c5796a9a8bb", "EnsureSnmpServerIsDisabled" },
    { "Ensure the rsync service is not enabled (CIS: L1 - Server - 2.2.16)", "63aba603-b1f8-40df-82c5-38915452ce23", "EnsureRsynServiceIsDisabled" },
    { "Ensure the NIS server is not enabled (CIS: L1 - Server - 2.2.17)", "b4ad3fdd-7b68-4b11-a3ed-84b37a68b995", "EnsureNisServerIsDisabled" },
    { "Ensure the rsh client is not installed (CIS: L1 - Server - 2.3.2)", "6d441f31-f888-4f4f-b1da-7cfc26263e3f", "EnsureRshClientNotInstalled" },
    { "Ensure SMB V1 with Samba is disabled (CIS: L1 - Server - 2.2.12)", "7624efb0-3026-4c72-8920-48d5be78a50e", "EnsureSmbWithSambaIsDisabled" }
};

int AsbIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, OsConfigLogHandle log)
{
    int numRules = ARRAY_SIZE(g_rules);
    int i = 0;
    int result = 0;

    if ((NULL == payloadKey) || ((NULL == resourceId) && (NULL == ruleId)))
    {
        OsConfigLogError(log, "AsbIsValidRuleIdAndName called with invalid arguments");
        return EINVAL;
    }

    for (i = 0; i < numRules; i++)
    {
        if (0 == strcmp(payloadKey, (g_rules[i]).payloadKey))
        {
            if ((NULL != resourceId) && (0 != strncmp(resourceId, g_rules[i].resourceId, strlen(g_rules[i].resourceId))))
            {
                OsConfigLogError(log, "AsbIsValidRuleIdAndName: resourceId for rule '%s' of '%s' (instead of '%s') is invalid", payloadKey, resourceId, g_rules[i].resourceId);
                result = ENOENT;
            }
            else if ((NULL != ruleId) && (0 != strncasecmp(ruleId, g_rules[i].ruleId, strlen(g_rules[i].ruleId))))
            {
                OsConfigLogError(log, "AsbIsValidRuleIdAndName: ruleId for rule '%s' of '%s' (instead of '%s') is invalid", payloadKey, ruleId, g_rules[i].ruleId);
                result = ENOENT;
            }
            else
            {
                result = 0;
            }
            break;
        }
    }

    return result;
}

static LoggingLevel GetLoggingLevelFromString(const char* value)
{
    LoggingLevel level = LoggingLevelInformational;

    if (value)
    {
        if (0 == strcmp(value, "Emergency"))
        {
            level = LoggingLevelEmergency;
        }
        else if (0 == strcmp(value, "Alert"))
        {
            level = LoggingLevelAlert;
        }
        else if (0 == strcmp(value, "Critical"))
        {
            level = LoggingLevelCritical;
        }
        else if (0 == strcmp(value, "Error"))
        {
            level = LoggingLevelError;
        }
        else if (0 == strcmp(value, "Warning"))
        {
            level = LoggingLevelWarning;
        }
        else if (0 == strcmp(value, "Notice"))
        {
            level = LoggingLevelNotice;
        }
        else if (0 == strcmp(value, "Informational"))
        {
            level = LoggingLevelInformational;
        }
        else if (0 == strcmp(value, "Debug"))
        {
            level = LoggingLevelDebug;
        }
    }

    return level;
}

void AsbInitialize(OsConfigLogHandle log)
{
    char* jsonConfiguration = NULL;
    char* kernelVersion = NULL;
    char* cpuModel = NULL;
    long totalMemory = 0;
    long freeMemory = 0;
    unsigned short freeMemoryPercentage = 0;

    g_perfLog = OpenLog(PERF_LOG_FILE, ROLLED_PERF_LOG_FILE);

    StartPerfClock(&g_perfClock, GetPerfLog());

    if (FileExists(g_configurationFile))
    {
        if (NULL != (jsonConfiguration = LoadStringFromFile(g_configurationFile, false, log)))
        {
            SetLoggingLevel(GetLoggingLevelFromJsonConfig(jsonConfiguration, log));
            SetMaxLogSize(GetMaxLogSizeFromJsonConfig(jsonConfiguration, log));
            SetMaxLogSizeDebugMultiplier(GetMaxLogSizeDebugMultiplierFromJsonConfig(jsonConfiguration, log));
            FREE_MEMORY(jsonConfiguration);
        }

        RestrictFileAccessToCurrentAccountOnly(g_configurationFile);
    }

    if (IsConsoleLoggingEnabled())
    {
        OsConfigLogWarning(log, "AsbInitialize: console logging is enabled. If the syslog rotation is not enabled this may result in a fill-up of the local storage space");
    }

    if (IsDebugLoggingEnabled())
    {
        OsConfigLogWarning(log, "AsbInitialize: debug logging is enabled and this may include private information such as unredacted usernames");
    }

    OsConfigLogInfo(log, "AsbInitialize: %s", g_asbName);

    if (NULL != (cpuModel = GetCpuModel(GetPerfLog())))
    {
        OsConfigLogInfo(log, "AsbInitialize: CPU model: %s", cpuModel);
    }

    OsConfigLogInfo(log, "AsbInitialize: CPU cores: %u", GetNumberOfCpuCores(log));

    totalMemory = GetTotalMemory(log);
    OsConfigLogInfo(log, "AsbInitialize: total memory: %lu kB", totalMemory);

    freeMemory = GetFreeMemory(log);
    freeMemoryPercentage = (freeMemory * 100) / totalMemory;
    OsConfigLogInfo(log, "AsbInitialize: free memory: %u%% (%lu kB)", freeMemoryPercentage, freeMemory);

    InitializeSshAudit(log);

    if ((NULL == (g_desiredLoggingLevel = DuplicateString(g_defaultLoggingLevel))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcIssue = DuplicateString(g_defaultEnsurePermissionsOnEtcIssue))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcIssueNet = DuplicateString(g_defaultEnsurePermissionsOnEtcIssueNet))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcHostsAllow = DuplicateString(g_defaultEnsurePermissionsOnEtcHostsAllow))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcHostsDeny = DuplicateString(g_defaultEnsurePermissionsOnEtcHostsDeny))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcShadow = DuplicateString(g_defaultEnsurePermissionsOnEtcShadow))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcShadowDash = DuplicateString(g_defaultEnsurePermissionsOnEtcShadowDash))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcGShadow = DuplicateString(g_defaultEnsurePermissionsOnEtcGShadow))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcGShadowDash = DuplicateString(g_defaultEnsurePermissionsOnEtcGShadowDash))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcPasswd = DuplicateString(g_defaultEnsurePermissionsOnEtcPasswd))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcPasswdDash = DuplicateString(g_defaultEnsurePermissionsOnEtcPasswdDash))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcGroup = DuplicateString(g_defaultEnsurePermissionsOnEtcGroup))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcGroupDash = DuplicateString(g_defaultEnsurePermissionsOnEtcGroupDash))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcAnacronTab = DuplicateString(g_defaultEnsurePermissionsOnEtcAnacronTab))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcCronD = DuplicateString(g_defaultEnsurePermissionsOnEtcCronD))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcCronDaily = DuplicateString(g_defaultEnsurePermissionsOnEtcCronDaily))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcCronHourly = DuplicateString(g_defaultEnsurePermissionsOnEtcCronHourly))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcCronMonthly = DuplicateString(g_defaultEnsurePermissionsOnEtcCronMonthly))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcCronWeekly = DuplicateString(g_defaultEnsurePermissionsOnEtcCronWeekly))) ||
        (NULL == (g_desiredEnsurePermissionsOnEtcMotd = DuplicateString(g_defaultEnsurePermissionsOnEtcMotd))) ||
        (NULL == (g_desiredEnsureRestrictedUserHomeDirectories = DuplicateString(g_defaultEnsureRestrictedUserHomeDirectories))) ||
        (NULL == (g_desiredEnsurePasswordHashingAlgorithm = DuplicateString(g_defaultEnsurePasswordHashingAlgorithm))) ||
        (NULL == (g_desiredEnsureMinDaysBetweenPasswordChanges = DuplicateString(g_defaultEnsureMinDaysBetweenPasswordChanges))) ||
        (NULL == (g_desiredEnsureInactivePasswordLockPeriod = DuplicateString(g_defaultEnsureInactivePasswordLockPeriod))) ||
        (NULL == (g_desiredEnsureMaxDaysBetweenPasswordChanges = DuplicateString(g_defaultEnsureMaxDaysBetweenPasswordChanges))) ||
        (NULL == (g_desiredEnsurePasswordExpiration = DuplicateString(g_defaultEnsurePasswordExpiration))) ||
        (NULL == (g_desiredEnsurePasswordExpirationWarning = DuplicateString(g_defaultEnsurePasswordExpirationWarning))) ||
        (NULL == (g_desiredEnsureDefaultUmaskForAllUsers = DuplicateString(g_defaultEnsureDefaultUmaskForAllUsers))) ||
        (NULL == (g_desiredEnsurePermissionsOnBootloaderConfig = DuplicateString(g_defaultEnsurePermissionsOnBootloaderConfig))) ||
        (NULL == (g_desiredEnsurePasswordReuseIsLimited = DuplicateString(g_defaultEnsurePasswordReuseIsLimited))) ||
        (NULL == (g_desiredEnsurePasswordCreationRequirements = DuplicateString(g_defaultEnsurePasswordCreationRequirements))) ||
        (NULL == (g_desiredEnsureFilePermissionsForAllRsyslogLogFiles = DuplicateString(g_defaultEnsureFilePermissionsForAllRsyslogLogFiles))) ||
        (NULL == (g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable = DuplicateString(g_defaultEnsureUsersDotFilesArentGroupOrWorldWritable))) ||
        (NULL == (g_desiredEnsureUnnecessaryAccountsAreRemoved = DuplicateString(g_defaultEnsureUnnecessaryAccountsAreRemoved))) ||
        (NULL == (g_desiredEnsureDefaultDenyFirewallPolicyIsSet = DuplicateString(g_defaultEnsureDefaultDenyFirewallPolicyIsSet))))
    {
        OsConfigLogError(log, "AsbInitialize: failed to allocate memory");
    }

    if (false == FileExists(g_etcFstabCopy))
    {
        if (false == MakeFileBackupCopy(g_etcFstab, g_etcFstabCopy, false, log))
        {
            OsConfigLogInfo(log, "AsbInitialize: cannot make a local backup copy of '%s' (%d)", g_etcFstab, errno);
        }
    }

    kernelVersion = GetOsKernelVersion(log);

    if (NULL != (g_prettyName = GetOsPrettyName(log)))
    {
        OsConfigLogInfo(log, "AsbInitialize: running on '%s' ('%s')", g_prettyName, kernelVersion);
    }
    else
    {
        OsConfigLogInfo(log, "AsbInitialize: running on an unknown Linux distribution with kernel version '%s' and without a valid PRETTY_NAME in /etc/os-release", kernelVersion);
    }

    if (IsCommodore(log))
    {
        OsConfigLogInfo(log, "AsbInitialize: running on product '%s'", PRODUCT_NAME_AZURE_COMMODORE);
    }

    if (DetectSelinux(log))
    {
        OsConfigLogInfo(log, "AsbInitialize: SELinux present");
    }

    FREE_MEMORY(kernelVersion);
    FREE_MEMORY(cpuModel);

    OsConfigLogInfo(log, "%s initialized", g_asbName);
}

void AsbShutdown(OsConfigLogHandle log)
{
    const char* auditOnly = "audit-only";
    const char* automaticRemediation = "automatic remediation";

    OsConfigLogInfo(log, "%s shutting down (%s)", g_asbName, g_auditOnly ? auditOnly : automaticRemediation);

    FREE_MEMORY(g_desiredLoggingLevel);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcIssue);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcIssueNet);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcHostsAllow);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcHostsDeny);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcShadow);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcShadowDash);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcGShadow);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcGShadowDash);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcPasswd);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcPasswdDash);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcGroup);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcGroupDash);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcAnacronTab);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcCronD);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcCronDaily);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcCronHourly);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcCronMonthly);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcCronWeekly);
    FREE_MEMORY(g_desiredEnsurePermissionsOnEtcMotd);
    FREE_MEMORY(g_desiredEnsureRestrictedUserHomeDirectories);
    FREE_MEMORY(g_desiredEnsurePasswordHashingAlgorithm);
    FREE_MEMORY(g_desiredEnsureMinDaysBetweenPasswordChanges);
    FREE_MEMORY(g_desiredEnsureInactivePasswordLockPeriod);
    FREE_MEMORY(g_desiredEnsureMaxDaysBetweenPasswordChanges);
    FREE_MEMORY(g_desiredEnsurePasswordExpiration);
    FREE_MEMORY(g_desiredEnsurePasswordExpirationWarning);
    FREE_MEMORY(g_desiredEnsureDefaultUmaskForAllUsers);
    FREE_MEMORY(g_desiredEnsurePermissionsOnBootloaderConfig);
    FREE_MEMORY(g_desiredEnsurePasswordReuseIsLimited);
    FREE_MEMORY(g_desiredEnsurePasswordCreationRequirements);
    FREE_MEMORY(g_desiredEnsureFilePermissionsForAllRsyslogLogFiles);
    FREE_MEMORY(g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable);
    FREE_MEMORY(g_desiredEnsureUnnecessaryAccountsAreRemoved);
    FREE_MEMORY(g_desiredEnsureDefaultDenyFirewallPolicyIsSet);

    SshAuditCleanup(log);

    PackageUtilsCleanup();

    if (0 == StopPerfClock(&g_perfClock, GetPerfLog()))
    {
        LogPerfClock(&g_perfClock, g_asbName, NULL, 0, g_maxTotalTime, GetPerfLog());

        // For telemetry:
        OsConfigLogCritical(log, "TargetName: '%s', BaselineName: '%s', Mode: '%s', Seconds: %.02f",
            g_prettyName, g_asbName, g_auditOnly ? auditOnly : automaticRemediation, GetPerfClockTime(&g_perfClock, log) / 1000000.0);
    }

    FREE_MEMORY(g_prettyName);

    CloseLog(&g_perfLog);

    // When done, allow others access to read the performance log
    SetFileAccess(PERF_LOG_FILE, 0, 0, 0644, NULL);
    SetFileAccess(ROLLED_PERF_LOG_FILE, 0, 0, 0644, NULL);
}

static char* AuditEnsureLoggingLevel(OsConfigLogHandle log)
{
    char* reason = NULL;
    LoggingLevel existingLevel = GetLoggingLevel();
    LoggingLevel desiredLevel = GetLoggingLevelFromString(g_desiredLoggingLevel ? g_desiredLoggingLevel : g_defaultLoggingLevel);

    // We need to configure the desired logging level even in audit-only mode
    SetLoggingLevelPersistently(desiredLevel, log);

// We need to avoid the warning treated as error for 'reason' always being non-NULL in this case
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
    if (existingLevel != desiredLevel)
    {
        OsConfigLogInfo(log, "Locally configured logging level for Azure OSConfig is %d instead of %d", existingLevel, desiredLevel);
        OsConfigCaptureReason(&reason, "Locally configured logging level for Azure OSConfig is %d instead of %d", existingLevel, desiredLevel);
    }
    else
    {
        OsConfigLogInfo(log, "Locally configured logging level for Azure OSConfig is %d", desiredLevel);
        OsConfigCaptureSuccessReason(&reason, "Locally configured logging level for Azure OSConfig is %d", desiredLevel);
    }
#pragma GCC diagnostic pop

    return reason;
}

static char* AuditEnsurePermissionsOnEtcIssue(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcIssue, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcIssue ?
        g_desiredEnsurePermissionsOnEtcIssue : g_defaultEnsurePermissionsOnEtcIssue, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcIssueNet(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcIssueNet, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcIssueNet ?
        g_desiredEnsurePermissionsOnEtcIssueNet : g_defaultEnsurePermissionsOnEtcIssueNet, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcHostsAllow(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcHostsAllow, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcHostsAllow ?
        g_desiredEnsurePermissionsOnEtcHostsAllow : g_defaultEnsurePermissionsOnEtcHostsAllow, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcHostsDeny(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcHostsDeny, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcHostsDeny ?
        g_desiredEnsurePermissionsOnEtcHostsDeny : g_defaultEnsurePermissionsOnEtcHostsDeny, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcSshSshdConfig(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsurePermissionsOnEtcSshSshdConfigObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcShadow(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcShadow, 0, g_shadowGid, strtol(g_desiredEnsurePermissionsOnEtcShadow ?
        g_desiredEnsurePermissionsOnEtcShadow : g_defaultEnsurePermissionsOnEtcShadow, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcShadowDash(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcShadowDash, 0, g_shadowGid, strtol(g_desiredEnsurePermissionsOnEtcShadowDash ?
        g_desiredEnsurePermissionsOnEtcShadowDash : g_defaultEnsurePermissionsOnEtcShadowDash, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGShadow(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGShadow, 0, g_shadowGid, strtol(g_desiredEnsurePermissionsOnEtcGShadow ?
        g_desiredEnsurePermissionsOnEtcGShadow : g_defaultEnsurePermissionsOnEtcGShadow, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGShadowDash(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGShadowDash, 0, g_shadowGid, strtol(g_desiredEnsurePermissionsOnEtcGShadowDash ?
        g_desiredEnsurePermissionsOnEtcGShadowDash : g_defaultEnsurePermissionsOnEtcGShadowDash, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcPasswd(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcPasswd, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcPasswd ?
        g_desiredEnsurePermissionsOnEtcPasswd : g_defaultEnsurePermissionsOnEtcPasswd, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcPasswdDash(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcPasswdDash, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcPasswdDash ?
        g_desiredEnsurePermissionsOnEtcPasswdDash : g_defaultEnsurePermissionsOnEtcPasswdDash, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGroup(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGroup, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcGroup ?
        g_desiredEnsurePermissionsOnEtcGroup : g_defaultEnsurePermissionsOnEtcGroup, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGroupDash(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGroupDash, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcGroupDash ?
        g_desiredEnsurePermissionsOnEtcGroupDash : g_defaultEnsurePermissionsOnEtcGroupDash, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcAnacronTab(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcAnacronTab, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcAnacronTab ?
        g_desiredEnsurePermissionsOnEtcAnacronTab : g_defaultEnsurePermissionsOnEtcAnacronTab, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronD(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronD, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcCronD ?
        g_desiredEnsurePermissionsOnEtcCronD : g_defaultEnsurePermissionsOnEtcCronD, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronDaily(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronDaily, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcCronDaily ?
        g_desiredEnsurePermissionsOnEtcCronDaily : g_defaultEnsurePermissionsOnEtcCronDaily, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronHourly(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronHourly, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcCronHourly ?
        g_desiredEnsurePermissionsOnEtcCronHourly : g_defaultEnsurePermissionsOnEtcCronHourly, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronMonthly(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronMonthly, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcCronMonthly ?
        g_desiredEnsurePermissionsOnEtcCronMonthly : g_defaultEnsurePermissionsOnEtcCronMonthly, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronWeekly(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronWeekly, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcCronWeekly ?
        g_desiredEnsurePermissionsOnEtcCronWeekly : g_defaultEnsurePermissionsOnEtcCronWeekly, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcMotd(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcMotd, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcMotd ?
        g_desiredEnsurePermissionsOnEtcMotd : g_defaultEnsurePermissionsOnEtcMotd, NULL, 8), &reason, log);
    return reason;
}

static char* AuditEnsureKernelSupportForCpuNx(OsConfigLogHandle log)
{
    char* reason = NULL;
    if (false == CheckCpuFlagSupported("nx", &reason, log))
    {
        FREE_MEMORY(reason);
        reason = DuplicateString("A CPU that supports the NX (no-execute) bit technology is necessary. Automatic remediation is not possible");
    }
    return reason;
}

static char* AuditEnsureNodevOptionOnHomePartition(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_home, NULL, g_nodev, &reason, log);
    return reason;
}

static char* AuditEnsureNodevOptionOnTmpPartition(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_tmp, NULL, g_nodev, &reason, log);
    return reason;
}

static char* AuditEnsureNodevOptionOnVarTmpPartition(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_nodev, &reason, log);
    return reason;
}

static char* AuditEnsureNosuidOptionOnTmpPartition(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_tmp, NULL, g_nosuid, &reason, log);
    return reason;
}

static char* AuditEnsureNosuidOptionOnVarTmpPartition(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_nosuid, &reason, log);
    return reason;
}

static char* AuditEnsureNoexecOptionOnVarTmpPartition(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_noexec, &reason, log);
    return reason;
}

static char* AuditEnsureNoexecOptionOnDevShmPartition(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_devShm, NULL, g_noexec, &reason, log);
    return reason;
}

static char* AuditEnsureNodevOptionEnabledForAllRemovableMedia(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_nodev, &reason, log);
    return reason;
}

static char* AuditEnsureNoexecOptionEnabledForAllRemovableMedia(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_noexec, &reason, log);
    return reason;
}

static char* AuditEnsureNosuidOptionEnabledForAllRemovableMedia(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_nosuid, &reason, log);
    return reason;
}

static char* AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckFileSystemMountingOption(g_etcFstab, NULL, g_nfs, g_noexec, &reason, log));
    CheckFileSystemMountingOption(g_etcFstab, NULL, g_nfs, g_nosuid, &reason, log);
    return reason;
}

static char* AuditEnsureInetdNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckPackageNotInstalled(g_inetd, &reason, log));
    CheckPackageNotInstalled(g_inetUtilsInetd, &reason, log);
    return reason;
}

static char* AuditEnsureXinetdNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_xinetd, &reason, log);
    return reason;
}

static char* AuditEnsureAllTelnetdPackagesUninstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_allTelnetd, &reason, log);
    return reason;
}

static char* AuditEnsureRshServerNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_rshServer, &reason, log);
    return reason;
}

static char* AuditEnsureNisNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_nis, &reason, log);
    return reason;
}

static char* AuditEnsureTftpdNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_tftpHpa, &reason, log);
    return reason;
}

static char* AuditEnsureReadaheadFedoraNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_readAheadFedora, &reason, log);
    return reason;
}

static char* AuditEnsureBluetoothHiddNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckPackageNotInstalled(g_bluetooth, &reason, log));
    CheckDaemonNotActive(g_bluetooth, &reason, log);
    return reason;
}

static char* AuditEnsureIsdnUtilsBaseNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_isdnUtilsBase, &reason, log);
    return reason;
}

static char* AuditEnsureIsdnUtilsKdumpToolsNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_kdumpTools, &reason, log);
    return reason;
}

static char* AuditEnsureIscDhcpdServerNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_iscDhcpServer, &reason, log);
    return reason;
}

static char* AuditEnsureSendmailNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_sendmail, &reason, log);
    return reason;
}

static char* AuditEnsureSldapdNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_slapd, &reason, log);
    return reason;
}

static char* AuditEnsureBind9NotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_bind9, &reason, log);
    return reason;
}

static char* AuditEnsureDovecotCoreNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_dovecotCore, &reason, log);
    return reason;
}

static char* AuditEnsureAuditdInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_ZERO(CheckPackageInstalled(g_audit, &reason, log));
    RETURN_REASON_IF_ZERO(CheckPackageInstalled(g_auditd, &reason, log));
    RETURN_REASON_IF_ZERO(CheckPackageInstalled(g_auditLibs, &reason, log));
    RETURN_REASON_IF_ZERO(CheckPackageInstalled(g_auditLibsDevel, &reason, log));
    return reason;
}

static char* AuditEnsureAllEtcPasswdGroupsExistInEtcGroup(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckAllEtcPasswdGroupsExistInEtcGroup(&reason, log);
    return reason;
}

static char* AuditEnsureNoDuplicateUidsExist(OsConfigLogHandle log)
{
    char* reason = NULL;
    char* updatedReason = NULL;
    if (0 != CheckNoDuplicateUidsExist(&reason, log))
    {
        if (NULL != (updatedReason = FormatAllocateString("%s, %s", reason, g_remediationIsNotPossible)))
        {
            FREE_MEMORY(reason);
            reason = updatedReason;
        }
    }
    return reason;
}

static char* AuditEnsureNoDuplicateGidsExist(OsConfigLogHandle log)
{
    char* reason = NULL;
    char* updatedReason = NULL;
    if (0 != CheckNoDuplicateGidsExist(&reason, log))
    {
        if (NULL != (updatedReason = FormatAllocateString("%s, %s", reason, g_remediationIsNotPossible)))
        {
            FREE_MEMORY(reason);
            reason = updatedReason;
        }
    }
    return reason;
}

static char* AuditEnsureNoDuplicateUserNamesExist(OsConfigLogHandle log)
{
    char* reason = NULL;
    char* updatedReason = NULL;
    if (0 != CheckNoDuplicateUserNamesExist(&reason, log))
    {
        if (NULL != (updatedReason = FormatAllocateString("%s, %s", reason, g_remediationIsNotPossible)))
        {
            FREE_MEMORY(reason);
            reason = updatedReason;
        }
    }
    return reason;
}

static char* AuditEnsureNoDuplicateGroupsExist(OsConfigLogHandle log)
{
    char* reason = NULL;
    char* updatedReason = NULL;
    if (0 != CheckNoDuplicateGroupNamesExist(&reason, log))
    {
        if (NULL != (updatedReason = FormatAllocateString("%s, %s", reason, g_remediationIsNotPossible)))
        {
            FREE_MEMORY(reason);
            reason = updatedReason;
        }
    }
    return reason;
}

static char* AuditEnsureShadowGroupIsEmpty(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckShadowGroupIsEmpty(&reason, log);
    return reason;
}

static char* AuditEnsureRootGroupExists(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckRootGroupExists(&reason, log);
    return reason;
}

static char* AuditEnsureAllAccountsHavePasswords(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckAllUsersHavePasswordsSet(&reason, log);
    return reason;
}

static char* AuditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckRootIsOnlyUidZeroAccount(&reason, log);
    return reason;
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcPasswd(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckNoLegacyPlusEntriesInFile(g_etcPasswd, &reason, log);
    return reason;
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcShadow(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckNoLegacyPlusEntriesInFile(g_etcShadow, &reason, log);
    return reason;
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcGroup(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckNoLegacyPlusEntriesInFile(g_etcGroup, &reason, log);
    return reason;
}

static char* AuditEnsureDefaultRootAccountGroupIsGidZero(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckDefaultRootAccountGroupIsGidZero(&reason, log);
    return reason;
}

static char* AuditEnsureRootIsOnlyUidZeroAccount(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckRootGroupExists(&reason, log));
    CheckRootIsOnlyUidZeroAccount(&reason, log);
    return reason;
}

static char* AuditEnsureAllUsersHomeDirectoriesExist(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckAllUsersHomeDirectoriesExist(&reason, log);
    return reason;
}

static char* AuditEnsureUsersOwnTheirHomeDirectories(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckUsersOwnTheirHomeDirectories(&reason, log);
    return reason;
}

static char* AuditEnsureRestrictedUserHomeDirectories(OsConfigLogHandle log)
{
    int* modes = NULL;
    int numberOfModes = 0;
    char* reason = NULL;

    if (0 == ConvertStringToIntegers(g_desiredEnsureRestrictedUserHomeDirectories ?
        g_desiredEnsureRestrictedUserHomeDirectories : g_defaultEnsureRestrictedUserHomeDirectories, ',', &modes, &numberOfModes, 8, log))
    {
        CheckRestrictedUserHomeDirectories((unsigned int*)modes, (unsigned int)numberOfModes, &reason, log);
    }
    else
    {
        reason = FormatAllocateString("Failed to parse '%s'", g_desiredEnsureRestrictedUserHomeDirectories ?
            g_desiredEnsureRestrictedUserHomeDirectories : g_defaultEnsureRestrictedUserHomeDirectories);
    }

    FREE_MEMORY(modes);
    return reason;
}

static char* AuditEnsurePasswordHashingAlgorithm(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPasswordHashingAlgorithm((unsigned int)atoi(g_desiredEnsurePasswordHashingAlgorithm ?
        g_desiredEnsurePasswordHashingAlgorithm : g_defaultEnsurePasswordHashingAlgorithm), &reason, log);
    return reason;
}

static char* AuditEnsureMinDaysBetweenPasswordChanges(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckMinDaysBetweenPasswordChanges(atoi(g_desiredEnsureMinDaysBetweenPasswordChanges ?
        g_desiredEnsureMinDaysBetweenPasswordChanges : g_defaultEnsureMinDaysBetweenPasswordChanges), &reason, log);
    return reason;
}

static char* AuditEnsureInactivePasswordLockPeriod(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckLockoutAfterInactivityLessThan(atoi(g_desiredEnsureInactivePasswordLockPeriod ?
        g_desiredEnsureInactivePasswordLockPeriod : g_defaultEnsureInactivePasswordLockPeriod), &reason, log));
    CheckUsersRecordedPasswordChangeDates(&reason, log);
    return reason;
}

static char* AuditEnsureMaxDaysBetweenPasswordChanges(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckMaxDaysBetweenPasswordChanges(atoi(g_desiredEnsureMaxDaysBetweenPasswordChanges ?
        g_desiredEnsureMaxDaysBetweenPasswordChanges : g_defaultEnsureMaxDaysBetweenPasswordChanges), &reason, log);
    return reason;
}

static char* AuditEnsurePasswordExpiration(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPasswordExpirationLessThan(atol(g_desiredEnsurePasswordExpiration ?
        g_desiredEnsurePasswordExpiration : g_defaultEnsurePasswordExpiration), &reason, log);
    return reason;
}

static char* AuditEnsurePasswordExpirationWarning(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPasswordExpirationWarning(atol(g_desiredEnsurePasswordExpirationWarning ?
        g_desiredEnsurePasswordExpirationWarning : g_defaultEnsurePasswordExpirationWarning), &reason, log);
    return reason;
}

static char* AuditEnsureSystemAccountsAreNonLogin(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckSystemAccountsAreNonLogin(&reason, log);
    return reason;
}

static char* AuditEnsureAuthenticationRequiredForSingleUserMode(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckRootPasswordForSingleUserMode(&reason, log);
    return reason;
}

static char* AuditEnsurePrelinkIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_prelink, &reason, log);
    return reason;
}

static char* AuditEnsureTalkClientIsNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_talk, &reason, log);
    return reason;
}

static char* AuditEnsureDotDoesNotAppearInRootsPath(OsConfigLogHandle log)
{
    const char* path = "PATH";
    const char* dot = ".";
    const char comment = '#';
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckTextNotFoundInEnvironmentVariable(path, dot, false, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckMarkedTextNotFoundInFile("/etc/sudoers", "secure_path", dot, comment, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckMarkedTextNotFoundInFile(g_etcEnvironment, path, dot, comment, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckMarkedTextNotFoundInFile(g_etcProfile, path, dot, comment, &reason, log));
    CheckMarkedTextNotFoundInFile("/root/.profile", path, dot, comment, &reason, log);
    return reason;
}

static char* AuditEnsureCronServiceIsEnabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_ZERO(((0 == CheckPackageInstalled(g_cron, &reason, log)) && CheckDaemonActive(g_cron, &reason, log)) ? 0 : ENOENT);
    RETURN_REASON_IF_ZERO(((0 == CheckPackageInstalled(g_cronie, &reason, log)) && CheckDaemonActive(g_crond, &reason, log)) ? 0 : ENOENT);
    return reason;
}

static char* AuditEnsureRemoteLoginWarningBannerIsConfigured(OsConfigLogHandle log)
{
    char* reason = NULL;
    if (0 == CheckFileExists(g_etcIssueNet, &reason, log))
    {
        RETURN_REASON_IF_NOT_ZERO(CheckTextIsNotFoundInFile(g_etcIssueNet, "\\m", &reason, log));
        RETURN_REASON_IF_NOT_ZERO(CheckTextIsNotFoundInFile(g_etcIssueNet, "\\r", &reason, log));
        RETURN_REASON_IF_NOT_ZERO(CheckTextIsNotFoundInFile(g_etcIssueNet, "\\s", &reason, log));
        CheckTextIsNotFoundInFile(g_etcIssueNet, "\\v", &reason, log);
    }
    else if (IsCurrentOs(PRETTY_NAME_SLES_15, log))
    {
        FREE_MEMORY(reason);
        reason = FormatAllocateString("%s'%s' does not exist in '%s'", g_pass, g_etcIssueNet, PRETTY_NAME_SLES_15);
    }
    return reason;
}

static char* AuditEnsureLocalLoginWarningBannerIsConfigured(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckTextIsNotFoundInFile(g_etcIssue, "\\m", &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckTextIsNotFoundInFile(g_etcIssue, "\\r", &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckTextIsNotFoundInFile(g_etcIssue, "\\s", &reason, log));
    CheckTextIsNotFoundInFile(g_etcIssue, "\\v", &reason, log);
    return reason;
}

static char* AuditEnsureAuditdServiceIsRunning(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckDaemonActive(g_auditd, &reason, log);
    CheckDaemonNotActive(g_auoms, &reason, log);
    return reason;
}

static char* AuditEnsureSuRestrictedToRootGroup(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextIsFoundInFile("/etc/pam.d/su", "use_uid", &reason, log);
    return reason;
}

static char* AuditEnsureDefaultUmaskForAllUsers(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckLoginUmask(g_desiredEnsureDefaultUmaskForAllUsers ?
        g_desiredEnsureDefaultUmaskForAllUsers : g_defaultEnsureDefaultUmaskForAllUsers, &reason, log);
    return reason;
}

static char* AuditEnsureAutomountingDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_autofs, &reason, log);
    return reason;
}

static char* AuditEnsureKernelCompiledFromApprovedSources(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckOsAndKernelMatchDistro(&reason, log);
    return reason;
}

static char* AuditEnsureDefaultDenyFirewallPolicyIsSet(OsConfigLogHandle log)
{
    const char* readIpTables = "iptables -S";
    char* reason = NULL;
    int forceDrop = atoi(g_desiredEnsureDefaultDenyFirewallPolicyIsSet ?
        g_desiredEnsureDefaultDenyFirewallPolicyIsSet : g_defaultEnsureDefaultDenyFirewallPolicyIsSet);

    if ((0 != CheckTextFoundInCommandOutput(readIpTables, "-P INPUT DROP", &reason, log)) ||
        (0 != CheckTextFoundInCommandOutput(readIpTables, "-P FORWARD DROP", &reason, log)) ||
        (0 != CheckTextFoundInCommandOutput(readIpTables, "-P OUTPUT DROP", &reason, log)))
    {
        FREE_MEMORY(reason);
        reason = FormatAllocateString("Ensure that all necessary communication channels have explicit "
            "ACCEPT firewall policies set and then manually set the default firewall policy for "
            "INPUT, FORWARD and OUTPUT to DROP%s", forceDrop ? "." : ". Automatic remediation is not possible");
    }

    return reason;
}

static char* AuditEnsurePacketRedirectSendingIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv4.conf.all.send_redirects = 0", &reason, log));
    CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv4.conf.default.send_redirects = 0", &reason, log);
    return reason;
}

static char* AuditEnsureIcmpRedirectsIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv4.conf.default.accept_redirects = 0", &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv6.conf.default.accept_redirects = 0", &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv4.conf.all.accept_redirects = 0", &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv6.conf.all.accept_redirects = 0", &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv4.conf.default.secure_redirects = 0", &reason, log));
    CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv4.conf.all.secure_redirects = 0", &reason, log);
    return reason;
}

static char* AuditEnsureSourceRoutedPacketsIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/all/accept_source_route", '#', "0", &reason, log));
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv6/conf/all/accept_source_route", '#', "0", &reason, log);
    return reason;
}

static char* AuditEnsureAcceptingSourceRoutedPacketsIsDisabled(OsConfigLogHandle log)
{
    char* reason = 0;
    RETURN_REASON_IF_NOT_ZERO(CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/default/accept_source_route", '#', "0", &reason, log));
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv6/conf/default/accept_source_route", '#', "0", &reason, log);
    return reason;
}

static char* AuditEnsureIgnoringBogusIcmpBroadcastResponses(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/icmp_ignore_bogus_error_responses", '#', "1", &reason, log);
    return reason;
}

static char* AuditEnsureIgnoringIcmpEchoPingsToMulticast(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/icmp_echo_ignore_broadcasts", '#', "1", &reason, log);
    return reason;
}

static char* AuditEnsureMartianPacketLoggingIsEnabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv4.conf.all.log_martians = 1", &reason, log));
    CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv4.conf.default.log_martians = 1", &reason, log);
    return reason;
}

static char* AuditEnsureReversePathSourceValidationIsEnabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/all/rp_filter", '#', "1", &reason, log));
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/default/rp_filter", '#', "1", &reason, log);
    return reason;
}

static char* AuditEnsureTcpSynCookiesAreEnabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/tcp_syncookies", '#', "1", &reason, log);
    return reason;
}

static char* AuditEnsureSystemNotActingAsNetworkSniffer(OsConfigLogHandle log)
{
    const char* command = "ip address";
    const char* text = "PROMISC";
    char* reason = NULL;
    RETURN_REASON_IF_ZERO(((0 == CheckLineNotFoundOrCommentedOut(g_etcNetworkInterfaces, '#', text, &reason, log)) &&
        (0 == CheckLineNotFoundOrCommentedOut(g_etcRcLocal, '#', text, &reason, log))) ? 0 : ENOENT);
    CheckTextNotFoundInCommandOutput(command, text, &reason, log);
    return reason;
}

static char* AuditEnsureAllWirelessInterfacesAreDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckAllWirelessInterfacesAreDisabled(&reason, log);
    return reason;
}

static char* AuditEnsureIpv6ProtocolIsEnabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv6.conf.all.disable_ipv6 = 0", &reason, log));
    CheckTextFoundInCommandOutput(g_sysCtlA, "net.ipv6.conf.default.disable_ipv6 = 0", &reason, log);
    return reason;
}

static char* AuditEnsureDccpIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextFoundInFolder(g_etcModProbeD, "install dccp /bin/true", &reason, log);
    return reason;
}

static char* AuditEnsureSctpIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextFoundInFolder(g_etcModProbeD, "install sctp /bin/true", &reason, log);
    return reason;
}

static char* AuditEnsureDisabledSupportForRds(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextFoundInFolder(g_etcModProbeD, "install rds /bin/true", &reason, log);
    return reason;
}

static char* AuditEnsureTipcIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextFoundInFolder(g_etcModProbeD, "install tipc /bin/true", &reason, log);
    return reason;
}

static char* AuditEnsureZeroconfNetworkingIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckDaemonNotActive(g_avahiDaemon, &reason, log) ? 0 : ENOENT);
    RETURN_REASON_IF_NOT_ZERO(CheckLineNotFoundOrCommentedOut(g_etcNetworkInterfaces, '#', g_ipv4ll, &reason, log));
    if (FileExists(g_etcSysconfigNetwork) && IsAFile(g_etcSysconfigNetwork, log))
    {
        // NOZEROCONF is only processed when legacy network-scripts are in use.
        // If network.service is not active, then we should return early.
        RETURN_REASON_IF_NOT_ZERO(CheckDaemonNotActive(g_legacyNetworkService, &reason, log));
        CheckLineFoundNotCommentedOut(g_etcSysconfigNetwork, '#', "NOZEROCONF=yes", &reason, log);
    }
    return reason;
}

static char* AuditEnsurePermissionsOnBootloaderConfig(OsConfigLogHandle log)
{
    const char* value = g_desiredEnsurePermissionsOnBootloaderConfig ?
        g_desiredEnsurePermissionsOnBootloaderConfig : g_defaultEnsurePermissionsOnBootloaderConfig;
    unsigned int mode = strtol(value, NULL, 8);
    char* reason = NULL;

    RETURN_REASON_IF_NOT_ZERO(CheckFileAccess(g_bootGrubGrubCfg, 0, 0, mode, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckFileAccess(g_bootGrubGrubConf, 0, 0, mode, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckFileAccess(g_bootGrub2GrubCfg, 0, 0, mode, &reason, log));
    CheckFileAccess(g_bootGrub2GrubConf, 0, 0, mode, &reason, log);

    return reason;
}

static char* AuditEnsurePasswordReuseIsLimited(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckEnsurePasswordReuseIsLimited(atoi(g_desiredEnsurePasswordReuseIsLimited ?
        g_desiredEnsurePasswordReuseIsLimited : g_defaultEnsurePasswordReuseIsLimited), &reason, log);
    return reason;
}

static char* AuditEnsureMountingOfUsbStorageDevicesIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextFoundInFolder(g_etcModProbeD, "install usb-storage /bin/true", &reason, log);
    return reason;
}

static char* AuditEnsureCoreDumpsAreRestricted(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckCoreDumpsHardLimitIsDisabledForAllUsers(&reason, log));
    CheckLineFoundNotCommentedOut(g_sysCtlConf, '#', g_fsSuidDumpable, &reason, log);
    return reason;
}

static char* AuditEnsurePasswordCreationRequirements(OsConfigLogHandle log)
{
    int* values = NULL;
    int numberOfValues = 0;
    char* reason = NULL;

    if ((0 == ConvertStringToIntegers(g_desiredEnsurePasswordCreationRequirements ? g_desiredEnsurePasswordCreationRequirements :
        g_defaultEnsurePasswordCreationRequirements, ',', &values, &numberOfValues, 10, log)) && (7 == numberOfValues))
    {
        CheckPasswordCreationRequirements(values[0], values[1], values[2], values[3], values[4], values[5], values[6], &reason, log);
    }
    else
    {
        reason = FormatAllocateString("Failed to parse '%s'. There must be 7 numbers, comma separated, in this order: retry, minlen, minclass, dcredit, ucredit, ocredit, lcredit",
            g_desiredEnsurePasswordCreationRequirements ? g_desiredEnsurePasswordCreationRequirements : g_defaultEnsurePasswordCreationRequirements);
    }

    FREE_MEMORY(values);
    return reason;
}

static char* AuditEnsureLockoutForFailedPasswordAttempts(OsConfigLogHandle log)
{
    const char* pamFailLockSo = "pam_faillock.so";
    const char* pamTally2So = "pam_tally2.so";
    const char* pamTallySo = "pam_tally.so";
    char* reason = NULL;
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdSystemAuth, pamFailLockSo, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdPasswordAuth, pamFailLockSo, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdLogin, pamFailLockSo, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdSystemAuth, pamTally2So, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdPasswordAuth, pamTally2So, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdLogin, pamTally2So, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdSystemAuth, pamTallySo, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdPasswordAuth, pamTallySo, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdLogin, pamTallySo, '#', &reason, log));
    FREE_MEMORY(reason);
    reason = DuplicateString("Neither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules exist for this distribution. "
        "Manually set lockout for failed password attempts following specific instructions for this distrubution. Automatic remediation is not possible");
    return reason;
}

static char* AuditEnsureDisabledInstallationOfCramfsFileSystem(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextFoundInFolder(g_etcModProbeD, "install cramfs", &reason, log);
    return reason;
}

static char* AuditEnsureDisabledInstallationOfFreevxfsFileSystem(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextFoundInFolder(g_etcModProbeD, "install freevxfs", &reason, log);
    return reason;
}

static char* AuditEnsureDisabledInstallationOfHfsFileSystem(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextFoundInFolder(g_etcModProbeD, "install hfs", &reason, log);
    return reason;
}

static char* AuditEnsureDisabledInstallationOfHfsplusFileSystem(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextFoundInFolder(g_etcModProbeD, "install hfsplus", &reason, log);
    return reason;
}

static char* AuditEnsureDisabledInstallationOfJffs2FileSystem(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckTextFoundInFolder(g_etcModProbeD, "install jffs2", &reason, log);
    return reason;
}

static char* AuditEnsureVirtualMemoryRandomizationIsEnabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    if (0 == CheckSmallFileContainsText("/proc/sys/kernel/randomize_va_space", "2", &reason, log))
    {
        return reason;
    }
    CheckSmallFileContainsText("/proc/sys/kernel/randomize_va_space", "1", &reason, log);
    return reason;
}

static char* AuditEnsureAllBootloadersHavePasswordProtectionEnabled(OsConfigLogHandle log)
{
    const char* password = "password";
    char* reason = NULL;
    RETURN_REASON_IF_ZERO(CheckLineFoundNotCommentedOut("/boot/grub/grub.conf", '#', password, &reason, log));
    RETURN_REASON_IF_ZERO(CheckLineFoundNotCommentedOut("/boot/grub2/grub.conf", '#', password, &reason, log));
    RETURN_REASON_IF_ZERO(CheckLineFoundNotCommentedOut("/boot/grub/grub.cfg", '#', password, &reason, log));
    FREE_MEMORY(reason);
    reason = DuplicateString("Manually set a boot loader password for GRUB. Automatic remediation is not possible");
    return reason;
}

static char* AuditEnsureLoggingIsConfigured(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckPackageInstalled(g_systemd, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckDaemonActive(g_systemdJournald, &reason, log) ? 0 : ENOENT);
    RETURN_REASON_IF_ZERO(((0 == CheckPackageInstalled(g_rsyslog, &reason, log)) && CheckDaemonActive(g_rsyslog, &reason, log)) ? 0 : ENOENT);
    RETURN_REASON_IF_ZERO(((0 == CheckPackageInstalled(g_syslog, &reason, log)) && CheckDaemonActive(g_syslog, &reason, log)) ? 0 : ENOENT);
    RETURN_REASON_IF_ZERO(((0 == CheckPackageInstalled(g_syslogNg, &reason, log)) && CheckDaemonActive(g_syslogNg, &reason, log)) ? 0 : ENOENT);
    return reason;
}

static char* AuditEnsureSyslogPackageIsInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    if (0 == CheckPackageInstalled(g_systemd, &reason, log))
    {
        RETURN_REASON_IF_ZERO(CheckPackageInstalled(g_syslog, &reason, log));
        RETURN_REASON_IF_ZERO(CheckPackageInstalled(g_rsyslog, &reason, log));
    }
    CheckPackageInstalled(g_syslogNg, &reason, log);
    return reason;
}

static char* AuditEnsureSystemdJournaldServicePersistsLogMessages(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckPackageInstalled(g_systemd, &reason, log));
    CheckDirectoryAccess(g_varLogJournal, 0, -1, g_varLogJournalMode, &reason, log);
    return reason;
}

static char* AuditEnsureALoggingServiceIsEnabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    if (0 == CheckPackageNotInstalled(g_systemd, &reason, log))
    {
        RETURN_REASON_IF_ZERO(((0 == CheckPackageNotInstalled(g_syslogNg, &reason, log)) && CheckDaemonActive(g_rsyslog, &reason, log)) ? 0 : ENOENT);
        RETURN_REASON_IF_ZERO(((0 == CheckPackageNotInstalled(g_rsyslog, &reason, log)) && CheckDaemonActive(g_syslogNg, &reason, log)) ? 0 : ENOENT);
    }
    CheckDaemonActive(g_systemdJournald, &reason, log);
    return reason;
}

static char* AuditEnsureFilePermissionsForAllRsyslogLogFiles(OsConfigLogHandle log)
{
    int* modes = NULL;
    int numberOfModes = 0;
    char* reason = NULL;

    if ((0 == ConvertStringToIntegers(g_desiredEnsureFilePermissionsForAllRsyslogLogFiles ? g_desiredEnsureFilePermissionsForAllRsyslogLogFiles :
        g_defaultEnsureFilePermissionsForAllRsyslogLogFiles, ',', &modes, &numberOfModes, 8, log)) && (numberOfModes > 0))
    {
        CheckIntegerOptionFromFileEqualWithAny(g_etcRsyslogConf, g_fileCreateMode, ' ', modes, numberOfModes, &reason, log);
    }
    else
    {
        reason = FormatAllocateString("Failed to parse '%s'", g_desiredEnsureFilePermissionsForAllRsyslogLogFiles ?
            g_desiredEnsureFilePermissionsForAllRsyslogLogFiles : g_defaultEnsureFilePermissionsForAllRsyslogLogFiles);
    }

    FREE_MEMORY(modes);
    return reason;
}

static char* AuditEnsureLoggerConfigurationFilesAreRestricted(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckFileAccess(g_etcRsyslogConf, 0, 0, 0640, &reason, log));
    CheckFileAccess(g_etcSyslogNgSyslogNgConf, 0, 0, 0640, &reason, log);
    return reason;
}

static char* AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(OsConfigLogHandle log)
{
    const char* fileGroup = "$FileGroup adm";
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckTextIsFoundInFile(g_etcRsyslogConf, fileGroup, &reason, log));
    CheckLineFoundNotCommentedOut(g_etcRsyslogConf, '#', fileGroup, &reason, log);
    return reason;
}

static char* AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(OsConfigLogHandle log)
{
    const char* fileOwner = "$FileOwner syslog";
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckTextIsFoundInFile(g_etcRsyslogConf, fileOwner, &reason, log));
    CheckLineFoundNotCommentedOut(g_etcRsyslogConf, '#', fileOwner, &reason, log);
    return reason;
}

static char* AuditEnsureRsyslogNotAcceptingRemoteMessages(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "$ModLoad imudp", &reason, log));
    CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "$ModLoad imtcp", &reason, log);
    return reason;
}

static char* AuditEnsureSyslogRotaterServiceIsEnabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckPackageInstalled(g_logrotate, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckFileAccess(g_etcCronDailyLogRotate, 0, 0, 0755, &reason, log));
    if ((false == IsRedHatBased(log)) && (false == IsCurrentOs(PRETTY_NAME_UBUNTU_16_04, log)) && (false == IsCurrentOs(PRETTY_NAME_UBUNTU_18_04, log)))
    {
        CheckDaemonActive(g_logrotateTimer, &reason, log);
    }
    return reason;
}

static char* AuditEnsureTelnetServiceIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckDaemonNotActive(g_telnet, &reason, log) ? 0 : ENOENT);
    CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', g_telnet, &reason, log);
    return reason;
}

static char* AuditEnsureRcprshServiceIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckDaemonNotActive(g_rcpSocket, &reason, log) ? 0 : ENOENT);
    CheckDaemonNotActive(g_rshSocket, &reason, log);
    return reason;
}

static char* AuditEnsureTftpServiceisDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckDaemonNotActive(g_tftpHpa, &reason, log) ? 0 : ENOENT);
    CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', g_tftp, &reason, log);
    return reason;
}

static char* AuditEnsureAtCronIsRestrictedToAuthorizedUsers(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckFileNotFound(g_etcCronDeny, &reason, log) ? 0 : ENOENT);
    RETURN_REASON_IF_NOT_ZERO(CheckFileNotFound(g_etcAtDeny, &reason, log) ? 0 : ENOENT);
    RETURN_REASON_IF_NOT_ZERO(CheckFileExists(g_etcCronAllow, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckFileExists(g_etcAtAllow, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckFileAccess(g_etcCronAllow, 0, 0, 0600, &reason, log));
    CheckFileAccess(g_etcAtAllow, 0, 0, 0600, &reason, log);
    return reason;
}

static char* AuditEnsureSshPortIsConfigured(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshPortIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshBestPracticeProtocol(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshBestPracticeProtocolObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshBestPracticeIgnoreRhosts(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshBestPracticeIgnoreRhostsObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshLogLevelIsSet(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshLogLevelIsSetObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshMaxAuthTriesIsSet(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshMaxAuthTriesIsSetObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureAllowUsersIsConfigured(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureAllowUsersIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureDenyUsersIsConfigured(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureDenyUsersIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureAllowGroupsIsConfigured(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureAllowGroupsIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureDenyGroupsConfigured(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureDenyGroupsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshHostbasedAuthenticationIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshHostbasedAuthenticationIsDisabledObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshPermitRootLoginIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshPermitRootLoginIsDisabledObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshPermitEmptyPasswordsIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshPermitEmptyPasswordsIsDisabledObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshClientIntervalCountMaxIsConfigured(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshClientIntervalCountMaxIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshClientAliveIntervalIsConfigured(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshClientAliveIntervalIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshLoginGraceTimeIsSet(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshLoginGraceTimeIsSetObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureOnlyApprovedMacAlgorithmsAreUsed(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshWarningBannerIsEnabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshWarningBannerIsEnabledObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureUsersCannotSetSshEnvironmentOptions(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureUsersCannotSetSshEnvironmentOptionsObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureAppropriateCiphersForSsh(OsConfigLogHandle log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureAppropriateCiphersForSshObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureAvahiDaemonServiceIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_avahiDaemon, &reason, log);
    return reason;
}

static char* AuditEnsureCupsServiceisDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_cups, &reason, log);
    return reason;
}

static char* AuditEnsurePostfixPackageIsUninstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_postfix, &reason, log);
    return reason;
}

static char* AuditEnsurePostfixNetworkListeningIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    if ((0 != CheckPackageNotInstalled(g_postfix, &reason, log)) && (0 == CheckFileExists(g_etcPostfixMainCf, &reason, log)))
    {
        CheckTextIsFoundInFile(g_etcPostfixMainCf, g_inetInterfacesLocalhost, &reason, log);
    }
    return reason;
}

static char* AuditEnsureRpcgssdServiceIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckDaemonNotActive(g_rpcgssd, &reason, log) ? 0 : ENOENT);
    CheckDaemonNotActive(g_rpcGssd, &reason, log);
    return reason;
}

static char* AuditEnsureRpcidmapdServiceIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckDaemonNotActive(g_rpcidmapd, &reason, log) ? 0 : ENOENT);
    CheckDaemonNotActive(g_nfsIdmapd, &reason, log);
    return reason;
}

static char* AuditEnsurePortmapServiceIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckDaemonNotActive(g_rpcbindSocket, &reason, log) ? 0 : ENOENT);
    CheckDaemonNotActive(g_rpcbind, &reason, log);
    return reason;
}

static char* AuditEnsureNetworkFileSystemServiceIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_nfsServer, &reason, log);
    return reason;
}

static char* AuditEnsureRpcsvcgssdServiceIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', g_needSvcgssd, &reason, log));
    CheckDaemonNotActive(g_rpcSvcgssd, &reason, log);
    return reason;
}

static char* AuditEnsureSnmpServerIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_snmpd, &reason, log);
    return reason;
}

static char* AuditEnsureRsynServiceIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_rsync, &reason, log);
    return reason;
}

static char* AuditEnsureNisServerIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_ypserv, &reason, log);
    return reason;
}

static char* AuditEnsureRshClientNotInstalled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckPackageNotInstalled(g_rsh, &reason, log));
    CheckPackageNotInstalled(g_rshClient, &reason, log);
    return reason;
}

static char* AuditEnsureSmbWithSambaIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    if (IsDaemonActive(g_smbd, log))
    {
        RETURN_REASON_IF_NOT_ZERO(CheckLineFoundNotCommentedOut(g_etcSambaConf, '#', g_minSambaProtocol, &reason, log));
        CheckLineFoundNotCommentedOut(g_etcSambaConf, ';', g_minSambaProtocol, &reason, log);
    }
    else
    {
        RETURN_REASON_IF_NOT_ZERO(CheckFileNotFound(g_etcSambaConf, &reason, log));
        CheckPackageNotInstalled(g_samba, &reason, log);
    }
    return reason;
}

static char* AuditEnsureUsersDotFilesArentGroupOrWorldWritable(OsConfigLogHandle log)
{
    int* modes = NULL;
    int numberOfModes = 0;
    char* reason = NULL;

    if ((0 == ConvertStringToIntegers(g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable ? g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable :
        g_defaultEnsureUsersDotFilesArentGroupOrWorldWritable, ',', &modes, &numberOfModes, 8, log)) && (numberOfModes >= 2))
    {
        CheckUsersRestrictedDotFiles((unsigned int*)modes, (unsigned int)numberOfModes, &reason, log);
    }
    else
    {
        reason = FormatAllocateString("Failed to parse '%s'. There must be at least two access numbers, comma separated",
            g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable ? g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable :
            g_defaultEnsureUsersDotFilesArentGroupOrWorldWritable);
    }

    FREE_MEMORY(modes);
    return reason;
}

static char* AuditEnsureNoUsersHaveDotForwardFiles(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckOrEnsureUsersDontHaveDotFiles(g_forward, false, &reason, log);
    return reason;
}

static char* AuditEnsureNoUsersHaveDotNetrcFiles(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckOrEnsureUsersDontHaveDotFiles(g_netrc, false, &reason, log);
    return reason;
}

static char* AuditEnsureNoUsersHaveDotRhostsFiles(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckOrEnsureUsersDontHaveDotFiles(g_rhosts, false, &reason, log);
    return reason;
}

static char* AuditEnsureRloginServiceIsDisabled(OsConfigLogHandle log)
{
    char* reason = NULL;
    RETURN_REASON_IF_NOT_ZERO(CheckDaemonNotActive(g_rlogin, &reason, log) ? 0 : ENOENT);
    RETURN_REASON_IF_NOT_ZERO(CheckPackageNotInstalled(g_rlogin, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckPackageNotInstalled(g_inetd, &reason, log));
    RETURN_REASON_IF_NOT_ZERO(CheckPackageNotInstalled(g_inetUtilsInetd, &reason, log));
    CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', g_login, &reason, log);
    return reason;
}

static char* AuditEnsureUnnecessaryAccountsAreRemoved(OsConfigLogHandle log)
{
    char* reason = NULL;
    CheckUserAccountsNotFound(g_desiredEnsureUnnecessaryAccountsAreRemoved ?
        g_desiredEnsureUnnecessaryAccountsAreRemoved : g_defaultEnsureUnnecessaryAccountsAreRemoved, &reason, log);
    return reason;
}

static int InitEnsurePermissionsOnEtcSshSshdConfig(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsurePermissionsOnEtcSshSshdConfigObject, value, log);
}

static int InitEnsureSshPortIsConfigured(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshPortIsConfiguredObject, value, log);
}

static int InitEnsureSshBestPracticeProtocol(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshBestPracticeProtocolObject, value, log);
}

static int InitEnsureSshBestPracticeIgnoreRhosts(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshBestPracticeIgnoreRhostsObject, value, log);
}

static int InitEnsureSshLogLevelIsSet(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshLogLevelIsSetObject, value, log);
}

static int InitEnsureSshMaxAuthTriesIsSet(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshMaxAuthTriesIsSetObject, value, log);
}

static int InitEnsureAllowUsersIsConfigured(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureAllowUsersIsConfiguredObject, value, log);
}

static int InitEnsureDenyUsersIsConfigured(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureDenyUsersIsConfiguredObject, value, log);
}

static int InitEnsureAllowGroupsIsConfigured(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureAllowGroupsIsConfiguredObject, value, log);
}

static int InitEnsureDenyGroupsConfigured(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureDenyGroupsConfiguredObject, value, log);
}

static int InitEnsureSshHostbasedAuthenticationIsDisabled(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshHostbasedAuthenticationIsDisabledObject, value, log);
}

static int InitEnsureSshPermitRootLoginIsDisabled(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshPermitRootLoginIsDisabledObject, value, log);
}

static int InitEnsureSshPermitEmptyPasswordsIsDisabled(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshPermitEmptyPasswordsIsDisabledObject, value, log);
}

static int InitEnsureSshClientIntervalCountMaxIsConfigured(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshClientIntervalCountMaxIsConfiguredObject, value, log);
}

static int InitEnsureSshClientAliveIntervalIsConfigured(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshClientAliveIntervalIsConfiguredObject, value, log);
}

static int InitEnsureSshLoginGraceTimeIsSet(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshLoginGraceTimeIsSetObject, value, log);
}

static int InitEnsureOnlyApprovedMacAlgorithmsAreUsed(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureOnlyApprovedMacAlgorithmsAreUsedObject, value, log);
}

static int InitEnsureSshWarningBannerIsEnabled(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureSshWarningBannerIsEnabledObject, value, log);
}

static int InitEnsureUsersCannotSetSshEnvironmentOptions(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureUsersCannotSetSshEnvironmentOptionsObject, value, log);
}

static int InitEnsureAppropriateCiphersForSsh(char* value, OsConfigLogHandle log)
{
    return InitializeSshAuditCheck(g_initEnsureAppropriateCiphersForSshObject, value, log);
}

static int ReplaceString(char** target, char* source, const char* defaultValue)
{
    bool isValidValue = ((NULL == source) || (0 == source[0])) ? false : true;
    int status = 0;

    if (NULL == target)
    {
        return EINVAL;
    }

    FREE_MEMORY(*target);
    status = (NULL != (*target = DuplicateString(isValidValue ? source : defaultValue))) ? 0 : ENOMEM;

    return status;
}

static int InitEnsureLoggingLevel(char* value)
{
    return ReplaceString(&g_desiredLoggingLevel, value, g_defaultLoggingLevel);
}

static int InitEnsurePermissionsOnEtcIssue(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcIssue, value, g_defaultEnsurePermissionsOnEtcIssue);
}

static int InitEnsurePermissionsOnEtcIssueNet(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcIssueNet, value, g_defaultEnsurePermissionsOnEtcIssueNet);
}

static int InitEnsurePermissionsOnEtcHostsAllow(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcHostsAllow, value, g_defaultEnsurePermissionsOnEtcHostsAllow);
}

static int InitEnsurePermissionsOnEtcHostsDeny(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcHostsDeny, value, g_defaultEnsurePermissionsOnEtcHostsDeny);
}

static int InitEnsurePermissionsOnEtcShadow(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcShadow, value, g_defaultEnsurePermissionsOnEtcShadow);
}

static int InitEnsurePermissionsOnEtcShadowDash(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcShadowDash, value, g_defaultEnsurePermissionsOnEtcShadowDash);
}

static int InitEnsurePermissionsOnEtcGShadow(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcGShadow, value, g_defaultEnsurePermissionsOnEtcGShadow);
}

static int InitEnsurePermissionsOnEtcGShadowDash(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcGShadowDash, value, g_defaultEnsurePermissionsOnEtcGShadowDash);
}

static int InitEnsurePermissionsOnEtcPasswd(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcPasswd, value, g_defaultEnsurePermissionsOnEtcPasswd);
}

static int InitEnsurePermissionsOnEtcPasswdDash(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcPasswdDash, value, g_defaultEnsurePermissionsOnEtcPasswdDash);
}

static int InitEnsurePermissionsOnEtcGroup(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcGroup, value, g_defaultEnsurePermissionsOnEtcGroup);
}

static int InitEnsurePermissionsOnEtcGroupDash(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcGroupDash, value, g_defaultEnsurePermissionsOnEtcGroupDash);
}

static int InitEnsurePermissionsOnEtcAnacronTab(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcAnacronTab, value, g_defaultEnsurePermissionsOnEtcAnacronTab);
}

static int InitEnsurePermissionsOnEtcCronD(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcCronD, value, g_defaultEnsurePermissionsOnEtcCronD);
}

static int InitEnsurePermissionsOnEtcCronDaily(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcCronDaily, value, g_defaultEnsurePermissionsOnEtcCronDaily);
}

static int InitEnsurePermissionsOnEtcCronHourly(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcCronHourly, value, g_defaultEnsurePermissionsOnEtcCronHourly);
}

static int InitEnsurePermissionsOnEtcCronMonthly(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcCronMonthly, value, g_defaultEnsurePermissionsOnEtcCronMonthly);
}

static int InitEnsurePermissionsOnEtcCronWeekly(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcCronWeekly, value, g_defaultEnsurePermissionsOnEtcCronWeekly);
}

static int InitEnsurePermissionsOnEtcMotd(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnEtcMotd, value, g_defaultEnsurePermissionsOnEtcMotd);
}

static int InitEnsureRestrictedUserHomeDirectories(char* value)
{
    return ReplaceString(&g_desiredEnsureRestrictedUserHomeDirectories, value, g_defaultEnsureRestrictedUserHomeDirectories);
}

static int InitEnsurePasswordHashingAlgorithm(char* value)
{
    return ReplaceString(&g_desiredEnsurePasswordHashingAlgorithm, value, g_defaultEnsurePasswordHashingAlgorithm);
}

static int InitEnsureMinDaysBetweenPasswordChanges(char* value)
{
    return ReplaceString(&g_desiredEnsureMinDaysBetweenPasswordChanges, value, g_defaultEnsureMinDaysBetweenPasswordChanges);
}

static int InitEnsureInactivePasswordLockPeriod(char* value)
{
    return ReplaceString(&g_desiredEnsureInactivePasswordLockPeriod, value, g_defaultEnsureInactivePasswordLockPeriod);
}

static int InitEnsureMaxDaysBetweenPasswordChanges(char* value)
{
    return ReplaceString(&g_desiredEnsureMaxDaysBetweenPasswordChanges, value, g_defaultEnsureMaxDaysBetweenPasswordChanges);
}

static int InitEnsurePasswordExpiration(char* value)
{
    return ReplaceString(&g_desiredEnsurePasswordExpiration, value, g_defaultEnsurePasswordExpiration);
}

static int InitEnsurePasswordExpirationWarning(char* value)
{
    return ReplaceString(&g_desiredEnsurePasswordExpirationWarning, value, g_defaultEnsurePasswordExpirationWarning);
}

static int InitEnsureDefaultUmaskForAllUsers(char* value)
{
    return ReplaceString(&g_desiredEnsureDefaultUmaskForAllUsers, value, g_defaultEnsureDefaultUmaskForAllUsers);
}

static int InitEnsurePermissionsOnBootloaderConfig(char* value)
{
    return ReplaceString(&g_desiredEnsurePermissionsOnBootloaderConfig, value, g_defaultEnsurePermissionsOnBootloaderConfig);
}

static int InitEnsurePasswordReuseIsLimited(char* value)
{
    return ReplaceString(&g_desiredEnsurePasswordReuseIsLimited, value, g_defaultEnsurePasswordReuseIsLimited);
}

static int InitEnsurePasswordCreationRequirements(char* value)
{
    return ReplaceString(&g_desiredEnsurePasswordCreationRequirements, value, g_defaultEnsurePasswordCreationRequirements);
}

static int InitEnsureFilePermissionsForAllRsyslogLogFiles(char* value)
{
    return ReplaceString(&g_desiredEnsureFilePermissionsForAllRsyslogLogFiles, value, g_defaultEnsureFilePermissionsForAllRsyslogLogFiles);
}

static int InitEnsureUsersDotFilesArentGroupOrWorldWritable(char* value)
{
    return ReplaceString(&g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable, value, g_defaultEnsureUsersDotFilesArentGroupOrWorldWritable);
}

static int InitEnsureUnnecessaryAccountsAreRemoved(char* value)
{
    return ReplaceString(&g_desiredEnsureUnnecessaryAccountsAreRemoved, value, g_defaultEnsureUnnecessaryAccountsAreRemoved);
}

static int InitEnsureDefaultDenyFirewallPolicyIsSet(char* value)
{
    return ReplaceString(&g_desiredEnsureDefaultDenyFirewallPolicyIsSet, value, g_defaultEnsureDefaultDenyFirewallPolicyIsSet);
}

static int RemediateEnsureLoggingLevel(char* value, OsConfigLogHandle log)
{
    InitEnsureLoggingLevel(value);
    SetLoggingLevelPersistently(GetLoggingLevelFromString(g_desiredLoggingLevel), log);
    return 0;
};

static int RemediateEnsurePermissionsOnEtcIssue(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcIssue(value);
    return SetFileAccess(g_etcIssue, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcIssue, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcIssueNet(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcIssueNet(value);
    return SetFileAccess(g_etcIssueNet, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcIssueNet, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcHostsAllow(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcHostsAllow(value);
    return SetFileAccess(g_etcHostsAllow, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcHostsAllow, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcHostsDeny(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcHostsDeny(value);
    return SetFileAccess(g_etcHostsDeny, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcHostsDeny, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcSshSshdConfig(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsurePermissionsOnEtcSshSshdConfigObject, value, NULL, log);
};

static int RemediateEnsurePermissionsOnEtcShadow(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcShadow(value);
    return SetFileAccess(g_etcShadow, 0, g_shadowGid, strtol(g_desiredEnsurePermissionsOnEtcShadow, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcShadowDash(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcShadowDash(value);
    return SetFileAccess(g_etcShadowDash, 0, g_shadowGid, strtol(g_desiredEnsurePermissionsOnEtcShadowDash, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcGShadow(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcGShadow(value);
    return SetFileAccess(g_etcGShadow, 0, g_shadowGid, strtol(g_desiredEnsurePermissionsOnEtcGShadow, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcGShadowDash(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcGShadowDash(value);
    return SetFileAccess(g_etcGShadowDash, 0, g_shadowGid, strtol(g_desiredEnsurePermissionsOnEtcGShadowDash, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcPasswd(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcPasswd(value);
    return SetFileAccess(g_etcPasswd, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcPasswd, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcPasswdDash(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcPasswdDash(value);
    return SetFileAccess(g_etcPasswdDash, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcPasswdDash, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcGroup(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcGroup(value);
    return SetFileAccess(g_etcGroup, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcGroup, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcGroupDash(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcGroupDash(value);
    return SetFileAccess(g_etcGroupDash, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcGroupDash, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcAnacronTab(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcAnacronTab(value);
    return SetFileAccess(g_etcAnacronTab, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcAnacronTab, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcCronD(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcCronD(value);
    return SetFileAccess(g_etcCronD, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcCronD, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcCronDaily(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcCronDaily(value);
    return SetFileAccess(g_etcCronDaily, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcCronDaily, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcCronHourly(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcCronHourly(value);
    return SetFileAccess(g_etcCronHourly, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcCronHourly, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcCronMonthly(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcCronMonthly(value);
    return SetFileAccess(g_etcCronMonthly, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcCronMonthly, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcCronWeekly(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcCronWeekly(value);
    return SetFileAccess(g_etcCronWeekly, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcCronWeekly, NULL, 8), log);
};

static int RemediateEnsurePermissionsOnEtcMotd(char* value, OsConfigLogHandle log)
{
    InitEnsurePermissionsOnEtcMotd(value);
    return SetFileAccess(g_etcMotd, 0, 0, strtol(g_desiredEnsurePermissionsOnEtcMotd, NULL, 8), log);
};

static int RemediateEnsureInetdNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == UninstallPackage(g_inetd, log)) &&
        (0 == UninstallPackage(g_inetUtilsInetd, log))) ? 0 : ENOENT;
}

static int RemediateEnsureXinetdNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_xinetd, log);
}

static int RemediateEnsureRshServerNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_rshServer, log);
}

static int RemediateEnsureNisNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_nis, log);
}

static int RemediateEnsureTftpdNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_tftpHpa, log);
}

static int RemediateEnsureReadaheadFedoraNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_readAheadFedora, log);
}

static int RemediateEnsureBluetoothHiddNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_bluetooth, log);
    return UninstallPackage(g_bluetooth, log);
}

static int RemediateEnsureIsdnUtilsBaseNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_isdnUtilsBase, log);
}

static int RemediateEnsureIsdnUtilsKdumpToolsNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_kdumpTools, log);
}

static int RemediateEnsureIscDhcpdServerNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_iscDhcpServer, log);
}

static int RemediateEnsureSendmailNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_sendmail, log);
}

static int RemediateEnsureSldapdNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_slapd, log);
}

static int RemediateEnsureBind9NotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_bind9, log);
}

static int RemediateEnsureDovecotCoreNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_dovecotCore, log);
}

static int RemediateEnsureAuditdInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == InstallPackage(g_audit, log)) || (0 == InstallPackage(g_auditd, log)) ||
        (0 == InstallPackage(g_auditLibs, log)) || (0 == InstallPackage(g_auditLibsDevel, log))) ? 0 : ENOENT;
}

static int RemediateEnsurePrelinkIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_prelink, log);
}

static int RemediateEnsureTalkClientIsNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_talk, log);
}

static int RemediateEnsureCronServiceIsEnabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);

    return (((0 == InstallPackage(g_cron, log)) && EnableAndStartDaemon(g_cron, log)) ||
        (((0 == InstallPackage(g_cronie, log)) && EnableAndStartDaemon(g_crond, log)))) ? 0 : ENOENT;
}

static int RemediateEnsureAuditdServiceIsRunning(char* value, OsConfigLogHandle log)
{
    int status = 0;
    UNUSED(value);

    // The auoms service is part of Microsoft's Operations Management Suite (OMS) and is used for collecting audit events.
    // Conflicts between auoms and auditd can arise because both services attempt to manage and collect audit events.
    // One of the recommended mitigation strategies is Single Service Usage: use either auoms or auditd, but not both.
    // To mitigate this conflict we try to stop and disable auoms when present and we are asked to enable and start auditd:
    if (IsDaemonActive(g_auoms, log))
    {
        StopAndDisableDaemon(g_auoms, log);
    }

    if (IsDaemonActive(g_auoms, log))
    {
        OsConfigLogWarning(log, "RemediateEnsureAuditdServiceIsRunning: '%s' is active and collides with '%s', %s",
            g_auoms, g_auditd, g_remediationIsNotPossible);
    }

    if ((0 != InstallPackage(g_audit, log)) && (0 != InstallPackage(g_auditd, log)) &&
        (0 != InstallPackage(g_auditLibs, log)) && (0 != InstallPackage(g_auditLibsDevel, log)))
    {
        status = ENOENT;
    }
    else if ((false == CheckDaemonActive(g_auditd, NULL, log)) && (false == EnableAndStartDaemon(g_auditd, log)))
    {
        ExecuteCommand(NULL, "restorecon -r -v /var/log/audit", false, false, 0, 0, NULL, NULL, log);
        EnableAndStartDaemon(g_auditd, log);
        status = CheckDaemonActive(g_auditd, NULL, log) ? 0 : ENOENT;
    }

    return status;
}

static int RemediateEnsureKernelSupportForCpuNx(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    OsConfigLogInfo(log, "A CPU that supports the NX (no-execute) bit technology is necessary, %s", g_remediationIsNotPossible);
    return 0;
}

static int RemediateEnsureNodevOptionOnHomePartition(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetFileSystemMountingOption(g_home, NULL, g_nodev, log);
}

static int RemediateEnsureNodevOptionOnTmpPartition(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetFileSystemMountingOption(g_tmp, NULL, g_nodev, log);
}

static int RemediateEnsureNodevOptionOnVarTmpPartition(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetFileSystemMountingOption(g_varTmp, NULL, g_nodev, log);
}

static int RemediateEnsureNosuidOptionOnTmpPartition(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetFileSystemMountingOption(g_tmp, NULL, g_nosuid, log);
}

static int RemediateEnsureNosuidOptionOnVarTmpPartition(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetFileSystemMountingOption(g_varTmp, NULL, g_nosuid, log);
}

static int RemediateEnsureNoexecOptionOnVarTmpPartition(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetFileSystemMountingOption(g_varTmp, NULL, g_noexec, log);
}

static int RemediateEnsureNoexecOptionOnDevShmPartition(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetFileSystemMountingOption(g_devShm, NULL, g_noexec, log);
}

static int RemediateEnsureNodevOptionEnabledForAllRemovableMedia(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetFileSystemMountingOption(g_media, NULL, g_nodev, log);
}

static int RemediateEnsureNoexecOptionEnabledForAllRemovableMedia(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetFileSystemMountingOption(g_media, NULL, g_noexec, log);
}

static int RemediateEnsureNosuidOptionEnabledForAllRemovableMedia(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetFileSystemMountingOption(g_media, NULL, g_nosuid, log);
}

static int RemediateEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == SetFileSystemMountingOption(g_nfs, NULL, g_nosuid, log)) &&
        (0 == SetFileSystemMountingOption(g_nfs, NULL, g_noexec, log))) ? 0 : ENOENT;
}

static int RemediateEnsureAllTelnetdPackagesUninstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_allTelnetd, log);
}

static int RemediateEnsureAllEtcPasswdGroupsExistInEtcGroup(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetAllEtcPasswdGroupsToExistInEtcGroup(log);
}

static int RemediateEnsureNoDuplicateUidsExist(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    OsConfigLogInfo(log, "Any duplicate UIDs must be manually removed, %s", g_remediationIsNotPossible);
    return 0;
}

static int RemediateEnsureNoDuplicateGidsExist(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    OsConfigLogInfo(log, "Any duplicate GIDs must be manually removed, %s", g_remediationIsNotPossible);
    return 0;
}

static int RemediateEnsureNoDuplicateUserNamesExist(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    OsConfigLogInfo(log, "Any duplicate usernames must be manually removed, %s", g_remediationIsNotPossible);
    return 0;
}

static int RemediateEnsureNoDuplicateGroupsExist(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    OsConfigLogInfo(log, "Any duplicate groups must be manually removed, %s", g_remediationIsNotPossible);
    return 0;
}

static int RemediateEnsureShadowGroupIsEmpty(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetShadowGroupEmpty(log);
}

static int RemediateEnsureRootGroupExists(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return RepairRootGroup(log);
}

static int RemediateEnsureAllAccountsHavePasswords(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    // We cannot automatically add passwords for user accounts that can login and do not have passwords set.
    // If we try for example to run a command such as usermod, the command line can reveal that password
    // in clear before it gets encrypted and saved. Thus we simply remove such accounts:
    return RemoveUsersWithoutPasswords(log);
}

static int RemediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetRootIsOnlyUidZeroAccount(log);
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcPasswd(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ReplaceMarkedLinesInFile(g_etcPasswd, "+", NULL, '#', true, log);
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcShadow(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ReplaceMarkedLinesInFile(g_etcShadow, "+", NULL, '#', true, log);
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcGroup(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ReplaceMarkedLinesInFile(g_etcGroup, "+", NULL, '#', true, log);
}

static int RemediateEnsureDefaultRootAccountGroupIsGidZero(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetDefaultRootAccountGroupIsGidZero(log);
}

static int RemediateEnsureRootIsOnlyUidZeroAccount(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetRootIsOnlyUidZeroAccount(log);
}

static int RemediateEnsureAllUsersHomeDirectoriesExist(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetUserHomeDirectories(log);
}

static int RemediateEnsureUsersOwnTheirHomeDirectories(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetUserHomeDirectories(log);
}

static int RemediateEnsureRestrictedUserHomeDirectories(char* value, OsConfigLogHandle log)
{
    int* modes = NULL;
    int numberOfModes = 0;
    int status = 0;

    InitEnsureRestrictedUserHomeDirectories(value);

    if ((0 == (status = ConvertStringToIntegers(g_desiredEnsureRestrictedUserHomeDirectories, ',', &modes, &numberOfModes, 8, log))) && (numberOfModes > 1))
    {
        status = SetRestrictedUserHomeDirectories((unsigned int*)modes, (unsigned int)numberOfModes, modes[0], modes[numberOfModes - 1], log);
    }

    FREE_MEMORY(modes);
    return status;
}

static int RemediateEnsurePasswordHashingAlgorithm(char* value, OsConfigLogHandle log)
{
    InitEnsurePasswordHashingAlgorithm(value);
    return SetPasswordHashingAlgorithm((unsigned int)atoi(g_desiredEnsurePasswordHashingAlgorithm), log);
}

static int RemediateEnsureMinDaysBetweenPasswordChanges(char* value, OsConfigLogHandle log)
{
    InitEnsureMinDaysBetweenPasswordChanges(value);
    return SetMinDaysBetweenPasswordChanges(atol(g_desiredEnsureMinDaysBetweenPasswordChanges), log);
}

static int RemediateEnsureInactivePasswordLockPeriod(char* value, OsConfigLogHandle log)
{
    InitEnsureInactivePasswordLockPeriod(value);
    return SetLockoutAfterInactivityLessThan(atol(g_desiredEnsureInactivePasswordLockPeriod), log);
}

static int RemediateEnsureMaxDaysBetweenPasswordChanges(char* value, OsConfigLogHandle log)
{
    InitEnsureMaxDaysBetweenPasswordChanges(value);
    return SetMaxDaysBetweenPasswordChanges(atol(g_desiredEnsureMaxDaysBetweenPasswordChanges), log);
}

static int RemediateEnsurePasswordExpiration(char* value, OsConfigLogHandle log)
{
    InitEnsurePasswordExpiration(value);
    return ((0 == EnsureUsersHaveDatesOfLastPasswordChanges(log)) &&
        (0 == SetMaxDaysBetweenPasswordChanges(atol(g_desiredEnsurePasswordExpiration), log)) &&
        (0 == CheckPasswordExpirationLessThan(atol(g_desiredEnsurePasswordExpiration), NULL, log))) ? 0 : ENOENT;
}

static int RemediateEnsurePasswordExpirationWarning(char* value, OsConfigLogHandle log)
{
    InitEnsurePasswordExpirationWarning(value);
    return SetPasswordExpirationWarning(atol(g_desiredEnsurePasswordExpirationWarning), log);
}

static int RemediateEnsureSystemAccountsAreNonLogin(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetSystemAccountsNonLogin(log);
}

static int RemediateEnsureAuthenticationRequiredForSingleUserMode(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    OsConfigLogInfo(log, "For single user mode the root user account must have a password set. "
        "Manually set a password for root user account if necessary, %s", g_remediationIsNotPossible);
    return 0;
}

static int RemediateEnsureDotDoesNotAppearInRootsPath(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return RemoveDotsFromPath(log);
}

static int RemediateEnsureRemoteLoginWarningBannerIsConfigured(char* value, OsConfigLogHandle log)
{
    const char* escapes = "mrsv";
    unsigned int numEscapes = 4;
    int status = 0;
    UNUSED(value);
    if (0 == CheckFileExists(g_etcIssueNet, NULL, log))
    {
        status = RemoveEscapeSequencesFromFile(g_etcIssueNet, escapes, numEscapes, ' ', log);
    }
    return status;
}

static int RemediateEnsureLocalLoginWarningBannerIsConfigured(char* value, OsConfigLogHandle log)
{
    const char* escapes = "mrsv";
    unsigned int numEscapes = 4;
    UNUSED(value);
    return RemoveEscapeSequencesFromFile(g_etcIssue, escapes, numEscapes, ' ', log);
}

static int RemediateEnsureSuRestrictedToRootGroup(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return RestrictSuToRootGroup(log);
}

static int RemediateEnsureDefaultUmaskForAllUsers(char* value, OsConfigLogHandle log)
{
    const char* umask = "UMASK";
    InitEnsureDefaultUmaskForAllUsers(value);
    return SetEtcLoginDefValue(umask, g_desiredEnsureDefaultUmaskForAllUsers, log);
}

static int RemediateEnsureAutomountingDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_autofs, log);
    return CheckDaemonNotActive(g_autofs, NULL, log) ? 0 : ENOENT;
}

static int RemediateEnsureKernelCompiledFromApprovedSources(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    OsConfigLogInfo(log, "Automatic remediation is not possible");
    return 0;
}

static int RemediateEnsureDefaultDenyFirewallPolicyIsSet(char* value, OsConfigLogHandle log)
{
    int status = 0;
    UNUSED(value);
    InitEnsureDefaultDenyFirewallPolicyIsSet(value);
    if (atoi(g_desiredEnsureDefaultDenyFirewallPolicyIsSet))
    {
        status = SetDefaultDenyFirewallPolicy(log);
    }
    else
    {
        OsConfigLogInfo(log, "Automatic remediation is not possible. Manually ensure that "
            "all necessary communication channels have explicit ACCEPT firewall policies set "
            "and then set the default firewall policy for INPUT, FORWARD and OUTPUT to DROP");
    }
    return status;
}

static int RemediateEnsurePacketRedirectSendingIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == ExecuteCommand(NULL, "sysctl -w net.ipv4.conf.all.send_redirects=0", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ExecuteCommand(NULL, "sysctl -w net.ipv4.conf.default.send_redirects=0", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv4.conf.all.send_redirects", "net.ipv4.conf.all.send_redirects = 0\n", '#', true, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv4.conf.default.send_redirects", "net.ipv4.conf.default.send_redirects = 0\n", '#', true, log))) ? 0 : ENOENT;
}

static int RemediateEnsureIcmpRedirectsIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == ExecuteCommand(NULL, "sysctl -w net.ipv4.conf.default.accept_redirects=0", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ExecuteCommand(NULL, "sysctl -w net.ipv6.conf.default.accept_redirects=0", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ExecuteCommand(NULL, "sysctl -w net.ipv4.conf.all.accept_redirects=0", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ExecuteCommand(NULL, "sysctl -w net.ipv6.conf.all.accept_redirects=0", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ExecuteCommand(NULL, "sysctl -w net.ipv4.conf.default.secure_redirects=0", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ExecuteCommand(NULL, "sysctl -w net.ipv4.conf.all.secure_redirects=0", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv4.conf.default.accept_redirects", "net.ipv4.conf.default.accept_redirects = 0\n", '#', true, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv6.conf.default.accept_redirects", "net.ipv6.conf.default.accept_redirects = 0\n", '#', true, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv4.conf.all.accept_redirects", "net.ipv4.conf.all.accept_redirects = 0\n", '#', true, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv6.conf.all.accept_redirects", "net.ipv6.conf.all.accept_redirects = 0\n", '#', true, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv4.conf.default.secure_redirects", "net.ipv4.conf.default.secure_redirects = 0\n", true, '#', log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv4.conf.all.secure_redirects", "net.ipv4.conf.all.secure_redirects = 0\n", '#', true, log))) ? 0 : ENOENT;
}

static int RemediateEnsureSourceRoutedPacketsIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return (SavePayloadToFile("/proc/sys/net/ipv4/conf/all/accept_source_route", "0", 1, log) &&
        SavePayloadToFile("/proc/sys/net/ipv6/conf/all/accept_source_route", "0", 1, log)) ? 0 : ENOENT;
}

static int RemediateEnsureAcceptingSourceRoutedPacketsIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return (SavePayloadToFile("/proc/sys/net/ipv4/conf/default/accept_source_route", "0", 1, log) &&
        SavePayloadToFile("/proc/sys/net/ipv6/conf/default/accept_source_route", "0", 1, log)) ? 0 : ENOENT;
}

static int RemediateEnsureIgnoringBogusIcmpBroadcastResponses(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SavePayloadToFile("/proc/sys/net/ipv4/icmp_ignore_bogus_error_responses", "1", 1, log) ? 0 : ENOENT;
}

static int RemediateEnsureIgnoringIcmpEchoPingsToMulticast(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SavePayloadToFile("/proc/sys/net/ipv4/icmp_echo_ignore_broadcasts", "1", 1, log) ? 0 : ENOENT;
}

static int RemediateEnsureMartianPacketLoggingIsEnabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == ExecuteCommand(NULL, "sysctl -w net.ipv4.conf.all.log_martians=1", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ExecuteCommand(NULL, "sysctl -w net.ipv4.conf.default.log_martians=1", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv4.conf.all.log_martians", "net.ipv4.conf.all.log_martians = 1\n", '#', true, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv4.conf.default.log_martians", "net.ipv4.conf.default.log_martians = 1\n", '#', true, log))) ? 0 : ENOENT;
}

static int RemediateEnsureReversePathSourceValidationIsEnabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return (SavePayloadToFile("/proc/sys/net/ipv4/conf/all/rp_filter", "1", 1, log) &&
        SavePayloadToFile("/proc/sys/net/ipv4/conf/default/rp_filter", "1", 1, log)) ? 0 : ENOENT;
}

static int RemediateEnsureTcpSynCookiesAreEnabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SavePayloadToFile("/proc/sys/net/ipv4/tcp_syncookies", "1", 1, log) ? 0 : ENOENT;
}

static int RemediateEnsureSystemNotActingAsNetworkSniffer(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == ReplaceMarkedLinesInFile(g_etcNetworkInterfaces, "PROMISC", NULL, '#', true, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcRcLocal, "PROMISC", NULL, '#', true, log))) ? 0 : ENOENT;
}

static int RemediateEnsureAllWirelessInterfacesAreDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return DisableAllWirelessInterfaces(log);
}

static int RemediateEnsureIpv6ProtocolIsEnabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == ExecuteCommand(NULL, "sysctl -w net.ipv6.conf.default.disable_ipv6=0", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ExecuteCommand(NULL, "sysctl -w net.ipv6.conf.all.disable_ipv6=0", true, false, 0, 0, NULL, NULL, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv6.conf.default.disable_ipv6", "net.ipv6.conf.default.disable_ipv6 = 0\n", '#', true, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcSysctlConf, "net.ipv6.conf.all.disable_ipv6", "net.ipv6.conf.all.disable_ipv6 = 0\n", '#', true, log))) ? 0 : ENOENT;
}

static int RemediateEnsureDccpIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    const char* fileName = "/etc/modprobe.d/dccp.conf";
    const char* payload = "install dccp /bin/true";
    return SecureSaveToFile(fileName, payload, strlen(payload), log) ? 0 : ENOENT;
}

static int RemediateEnsureSctpIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    const char* fileName = "/etc/modprobe.d/sctp.conf";
    const char* payload = "install sctp /bin/true";
    return SecureSaveToFile(fileName, payload, strlen(payload), log) ? 0 : ENOENT;
}

static int RemediateEnsureDisabledSupportForRds(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    const char* fileName = "/etc/modprobe.d/rds.conf";
    const char* payload = "install rds /bin/true";
    return SecureSaveToFile(fileName, payload, strlen(payload), log) ? 0 : ENOENT;
}

static int RemediateEnsureTipcIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    const char* fileName = "/etc/modprobe.d/tipc.conf";
    const char* payload = "install tipc /bin/true";
    return SecureSaveToFile(fileName, payload, strlen(payload), log) ? 0 : ENOENT;
}

static int RemediateEnsureZeroconfNetworkingIsDisabled(char* value, OsConfigLogHandle log)
{
    int status = 0;
    UNUSED(value);
    // avahi-daemon may be retriggered by avahi-daemon.socket:
    StopAndDisableDaemon(g_avahiDaemonSocket, log);
    StopAndDisableDaemon(g_avahiDaemon, log);
    if (0 == (status = (CheckDaemonNotActive(g_avahiDaemon, NULL, log) ? 0 : ENOENT)))
    {
        if (0 == (status = ReplaceMarkedLinesInFile(g_etcNetworkInterfaces, g_ipv4ll, NULL, '#', true, log)))
        {
            if (FileExists(g_etcSysconfigNetwork) && IsAFile(g_etcSysconfigNetwork, log) && IsDaemonActive(g_legacyNetworkService, log))
            {
                // cloud-init regenerates the config file on every boot, and discards any changes before its header,
                // so we need to add the NOZEROCONF line to the top of the file.
                status = ReplaceMarkedLinesInFilePrepend(g_etcSysconfigNetwork, "NOZEROCONF", "NOZEROCONF=yes\n", '#', true, log);
            }
        }
    }
    return status;
}

static int RemediateEnsurePermissionsOnBootloaderConfig(char* value, OsConfigLogHandle log)
{
    unsigned int mode = 0;
    InitEnsurePermissionsOnBootloaderConfig(value);
    mode = strtol(g_desiredEnsurePermissionsOnBootloaderConfig, NULL, 8);

    return ((FileExists(g_bootGrubGrubCfg) && (0 == SetFileAccess(g_bootGrubGrubCfg, 0, 0, mode, log))) ||
        (FileExists(g_bootGrubGrubConf) && (0 == SetFileAccess(g_bootGrubGrubConf, 0, 0, mode, log))) ||
        (FileExists(g_bootGrub2GrubCfg) && (0 == SetFileAccess(g_bootGrub2GrubCfg, 0, 0, mode, log))) ||
        (FileExists(g_bootGrub2GrubConf) && (0 == SetFileAccess(g_bootGrub2GrubConf, 0, 0, mode, log)))) ? 0 : ENOENT;
}

static int RemediateEnsurePasswordReuseIsLimited(char* value, OsConfigLogHandle log)
{
    InitEnsurePasswordReuseIsLimited(value);
    return SetEnsurePasswordReuseIsLimited(atoi(g_desiredEnsurePasswordReuseIsLimited), log);
}

static int RemediateEnsureMountingOfUsbStorageDevicesIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    const char* fileName = "/etc/modprobe.d/usb-storage.conf";
    const char* payload = "install usb-storage /bin/true";
    return SecureSaveToFile(fileName, payload, strlen(payload), log) ? 0 : ENOENT;
}

static int RemediateEnsureCoreDumpsAreRestricted(char* value, OsConfigLogHandle log)
{
    int status = 0;
    UNUSED(value);

    if ((0 == CheckCoreDumpsHardLimitIsDisabledForAllUsers(NULL, log)) &&
        (0 == CheckLineFoundNotCommentedOut(g_sysCtlConf, '#', g_fsSuidDumpable, NULL, log)))
    {
        return status;
    }

    if (0 == (status = ReplaceMarkedLinesInFile(g_etcSecurityLimitsConf, "hard core", g_hardCoreZero, '#', true, log)))
    {
        if (false == FileExists(g_sysCtlConf))
        {
            status = SavePayloadToFile(g_sysCtlConf, g_fsSuidDumpable, strlen(g_fsSuidDumpable), log) ? 0 : ENOENT;
        }
        else
        {
            status = ReplaceMarkedLinesInFile(g_sysCtlConf, "fs.suid_dumpable", g_fsSuidDumpable, '#', true, log);
        }
    }

    return status;
}

static int RemediateEnsurePasswordCreationRequirements(char* value, OsConfigLogHandle log)
{
    int* values = NULL;
    int numberOfValues = 0;
    int status = 0;

    InitEnsurePasswordCreationRequirements(value);

    if ((0 == ConvertStringToIntegers(g_desiredEnsurePasswordCreationRequirements, ',', &values, &numberOfValues, 10, log)) && (7 == numberOfValues))
    {
        status = SetPasswordCreationRequirements(values[0], values[1], values[2], values[3], values[4], values[5], values[6], log);
    }
    else
    {
        OsConfigLogError(log, "RemediateEnsurePasswordCreationRequirements: failed to parse '%s'. There must be 7 numbers, comma separated, "
            "in this order: retry, minlen, minclass, dcredit, ucredit, ocredit, lcredit", g_desiredEnsurePasswordCreationRequirements);
    }

    FREE_MEMORY(values);
    return status;
}

static int RemediateEnsureLockoutForFailedPasswordAttempts(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetLockoutForFailedPasswordAttempts(log);
}

static int RemediateEnsureDisabledInstallationOfCramfsFileSystem(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    const char* fileName = "/etc/modprobe.d/cramfs.conf";
    const char* payload = "install cramfs /bin/true";
    return SecureSaveToFile(fileName, payload, strlen(payload), log) ? 0 : ENOENT;
}

static int RemediateEnsureDisabledInstallationOfFreevxfsFileSystem(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    const char* fileName = "/etc/modprobe.d/freevxfs.conf";
    const char* payload = "install freevxfs /bin/true";
    return SecureSaveToFile(fileName, payload, strlen(payload), log) ? 0 : ENOENT;
}

static int RemediateEnsureDisabledInstallationOfHfsFileSystem(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    const char* fileName = "/etc/modprobe.d/hfs.conf";
    const char* payload = "install hfs /bin/true";
    return SecureSaveToFile(fileName, payload, strlen(payload), log) ? 0 : ENOENT;
}

static int RemediateEnsureDisabledInstallationOfHfsplusFileSystem(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    const char* fileName = "/etc/modprobe.d/hfsplus.conf";
    const char* payload = "install hfsplus /bin/true";
    return SecureSaveToFile(fileName, payload, strlen(payload), log) ? 0 : ENOENT;
}

static int RemediateEnsureDisabledInstallationOfJffs2FileSystem(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    const char* fileName = "/etc/modprobe.d/jffs2.conf";
    const char* payload = "install jffs2 /bin/true";
    return SecureSaveToFile(fileName, payload, strlen(payload), log) ? 0 : ENOENT;
}

static int RemediateEnsureVirtualMemoryRandomizationIsEnabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return EnableVirtualMemoryRandomization(log);
    return 0;
}

static int RemediateEnsureAllBootloadersHavePasswordProtectionEnabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    OsConfigLogInfo(log, "Manually set a boot loader password for GRUB, %s", g_remediationIsNotPossible);
    return 0;
}

static int RemediateEnsureLoggingIsConfigured(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return (((0 == InstallPackage(g_systemd, log) &&
        ((0 == InstallPackage(g_rsyslog, log)) || (0 == InstallPackage(g_syslog, log)))) || (0 == InstallPackage(g_syslogNg, log))) &&
        (((0 == CheckPackageInstalled(g_systemd, NULL, log)) && EnableAndStartDaemon(g_systemdJournald, log))) &&
        ((((0 == CheckPackageInstalled(g_rsyslog, NULL, log)) && EnableAndStartDaemon(g_rsyslog, log))) ||
        (((0 == CheckPackageInstalled(g_syslog, NULL, log)) && EnableAndStartDaemon(g_syslog, log))) ||
        (((0 == CheckPackageInstalled(g_syslogNg, NULL, log)) && EnableAndStartDaemon(g_syslogNg, log))))) ? 0 : ENOENT;
}

static int RemediateEnsureSyslogPackageIsInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == InstallPackage(g_systemd, log) &&
        ((0 == InstallPackage(g_rsyslog, log)) || (0 == InstallPackage(g_syslog, log)))) ||
        ((0 == InstallPackage(g_syslogNg, log)))) ? 0 : ENOENT;
}

static int RemediateEnsureSystemdJournaldServicePersistsLogMessages(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == InstallPackage(g_systemd, log)) &&
        (0 == SetDirectoryAccess(g_varLogJournal, 0, -1, g_varLogJournalMode, log))) ? 0 : ENOENT;
}

static int RemediateEnsureALoggingServiceIsEnabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((((0 == InstallPackage(g_systemd, log)) && EnableAndStartDaemon(g_systemdJournald, log)) &&
        (((0 == InstallPackage(g_rsyslog, log)) && EnableAndStartDaemon(g_rsyslog, log)) ||
        (((0 == InstallPackage(g_syslog, log) && EnableAndStartDaemon(g_syslog, log)))))) ||
        (((0 == InstallPackage(g_syslogNg, log)) && EnableAndStartDaemon(g_syslogNg, log)))) ? 0 : ENOENT;
}

static int RemediateEnsureFilePermissionsForAllRsyslogLogFiles(char* value, OsConfigLogHandle log)
{
    const char* formatTemplate = "0%03d";
    int* modes = NULL;
    int numberOfModes = 0;
    char* formattedMode = NULL;
    int status = 0;

    InitEnsureFilePermissionsForAllRsyslogLogFiles(value);

    if (0 == (status = ConvertStringToIntegers(g_desiredEnsureFilePermissionsForAllRsyslogLogFiles, ',', &modes, &numberOfModes, 8, log)))
    {
        if (numberOfModes > 0)
        {
            if (NULL != (formattedMode = FormatAllocateString(formatTemplate, modes[numberOfModes - 1])))
            {
                status = SetEtcConfValue(g_etcRsyslogConf, g_fileCreateMode, formattedMode, log);
                FREE_MEMORY(formattedMode);
            }
            else
            {
                OsConfigLogError(log, "RemediateEnsureFilePermissionsForAllRsyslogLogFiles: out of memory");
                status = ENOMEM;
            }
        }
        else
        {
            OsConfigLogError(log, "RemediateEnsureFilePermissionsForAllRsyslogLogFiles: failed to parse desired value '%s'",
                g_desiredEnsureFilePermissionsForAllRsyslogLogFiles);
            status = ENOENT;
        }
    }

    FREE_MEMORY(modes);
    return status;
}

static int RemediateEnsureLoggerConfigurationFilesAreRestricted(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == SetFileAccess(g_etcSyslogNgSyslogNgConf, 0, 0, 0640, log)) &&
        (0 == SetFileAccess(g_etcRsyslogConf, 0, 0, 0640, log))) ? 0 : ENOENT;
}

static int RemediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetEtcConfValue(g_etcRsyslogConf, "$FileGroup", "adm", log);
}

static int RemediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return SetEtcConfValue(g_etcRsyslogConf, "$FileOwner", "syslog", log);
}

static int RemediateEnsureRsyslogNotAcceptingRemoteMessages(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == ReplaceMarkedLinesInFile(g_etcRsyslogConf, "$ModLoad imudp", NULL, '#', true, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcRsyslogConf, "$ModLoad imtcp", NULL, '#', true, log))) ? 0 : ENOENT;
}

static int RemediateEnsureSyslogRotaterServiceIsEnabled(char* value, OsConfigLogHandle log)
{
    int status = ENOENT;
    UNUSED(value);
    if ((0 == InstallPackage(g_logrotate, log)) && (0 == SetFileAccess(g_etcCronDailyLogRotate, 0, 0, 0755, log)))
    {
        status = 0;
        if ((false == IsRedHatBased(log)) && (false == IsCurrentOs(PRETTY_NAME_UBUNTU_16_04, log)) && (false == IsCurrentOs(PRETTY_NAME_UBUNTU_18_04, log)))
        {
            status = EnableAndStartDaemon(g_logrotateTimer, log) ? 0 : ENOENT;
        }
    }
    return status;
}

static int RemediateEnsureTelnetServiceIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_telnet, log);
    return (CheckDaemonNotActive(g_telnet, NULL, log) &&
        (0 == ReplaceMarkedLinesInFile(g_etcInetdConf, g_telnet, NULL, '#', true, log))) ? 0 : ENOENT;
}

static int RemediateEnsureRcprshServiceIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rcpSocket, log);
    StopAndDisableDaemon(g_rshSocket, log);
    return (CheckDaemonNotActive(g_rcpSocket, NULL, log) && CheckDaemonNotActive(g_rshSocket, NULL, log)) ? 0 : ENOENT;
}

static int RemediateEnsureTftpServiceisDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_tftpHpa, log);
    return (CheckDaemonNotActive(g_tftpHpa, NULL, log) &&
        (0 == ReplaceMarkedLinesInFile(g_etcInetdConf, g_tftp, NULL, '#', true, log))) ? 0 : ENOENT;
}

static int RemediateEnsureAtCronIsRestrictedToAuthorizedUsers(char* value, OsConfigLogHandle log)
{
    const char* payload = "root\n";
    UNUSED(value);
    remove(g_etcCronDeny);
    remove(g_etcAtDeny);
    return (SecureSaveToFile(g_etcCronAllow, payload, strlen(payload), log) &&
        SecureSaveToFile(g_etcAtAllow, payload, strlen(payload), log) &&
        (0 != CheckFileExists(g_etcCronDeny, NULL, log)) &&
        (0 != CheckFileExists(g_etcAtDeny, NULL, log)) &&
        (0 == SetFileAccess(g_etcCronAllow, 0, 0, 0600, log)) &&
        (0 == SetFileAccess(g_etcAtAllow, 0, 0, 0600, log))) ? 0 : ENOENT;
}

static int RemediateEnsureSshPortIsConfigured(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshPortIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureSshBestPracticeProtocol(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshBestPracticeProtocolObject, value, NULL, log);
}

static int RemediateEnsureSshBestPracticeIgnoreRhosts(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshBestPracticeIgnoreRhostsObject, value, NULL, log);
}

static int RemediateEnsureSshLogLevelIsSet(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshLogLevelIsSetObject, value, NULL, log);
}

static int RemediateEnsureSshMaxAuthTriesIsSet(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshMaxAuthTriesIsSetObject, value, NULL, log);
}

static int RemediateEnsureAllowUsersIsConfigured(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureAllowUsersIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureDenyUsersIsConfigured(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureDenyUsersIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureAllowGroupsIsConfigured(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureAllowGroupsIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureDenyGroupsConfigured(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureDenyGroupsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureSshHostbasedAuthenticationIsDisabled(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject, value, NULL, log);
}

static int RemediateEnsureSshPermitRootLoginIsDisabled(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshPermitRootLoginIsDisabledObject, value, NULL, log);
}

static int RemediateEnsureSshPermitEmptyPasswordsIsDisabled(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject, value, NULL, log);
}

static int RemediateEnsureSshClientIntervalCountMaxIsConfigured(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshClientIntervalCountMaxIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureSshClientAliveIntervalIsConfigured(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshClientAliveIntervalIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureSshLoginGraceTimeIsSet(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshLoginGraceTimeIsSetObject, value, NULL, log);
}

static int RemediateEnsureOnlyApprovedMacAlgorithmsAreUsed(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject, value, NULL, log);
}

static int RemediateEnsureSshWarningBannerIsEnabled(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshWarningBannerIsEnabledObject, value, NULL, log);
}

static int RemediateEnsureUsersCannotSetSshEnvironmentOptions(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject, value, NULL, log);
}

static int RemediateEnsureAppropriateCiphersForSsh(char* value, OsConfigLogHandle log)
{
    return ProcessSshAuditCheck(g_remediateEnsureAppropriateCiphersForSshObject, value, NULL, log);
}

static int RemediateEnsureAvahiDaemonServiceIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_avahiDaemonSocket, log);
    StopAndDisableDaemon(g_avahiDaemon, log);
    return CheckDaemonNotActive(g_avahiDaemon, NULL, log) ? 0 : ENOENT;
}

static int RemediateEnsureCupsServiceisDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_cups, log);
    UninstallPackage(g_cups, log);
    return CheckDaemonNotActive(g_cups, NULL, log) ? 0 : ENOENT;
}

static int RemediateEnsurePostfixPackageIsUninstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return UninstallPackage(g_postfix, log);
}

static int RemediateEnsurePostfixNetworkListeningIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    int result = 0;
    if (0 == IsPackageInstalled(g_postfix, log))
    {
        result = DisablePostfixNetworkListening(log);
    }
    return result;
}

static int CheckAndFreeReason(char *reason)
{
    int result = (0 == strncmp(g_pass, reason, strlen(g_pass))) ? 0 : ENOENT;
    FREE_MEMORY(reason);
    return result;
}

static int RemediateEnsureRpcgssdServiceIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcgssd, log);
    StopAndDisableDaemon(g_rpcGssd, log);
    return CheckAndFreeReason(AuditEnsureRpcgssdServiceIsDisabled(log));
}

static int RemediateEnsureRpcidmapdServiceIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcidmapd, log);
    StopAndDisableDaemon(g_nfsIdmapd, log);
    return CheckAndFreeReason(AuditEnsureRpcidmapdServiceIsDisabled(log));
}

static int RemediateEnsurePortmapServiceIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    if (CheckDaemonActive(g_rpcbind, NULL, log))
    {
        RestartDaemon(g_rpcbind, log);
        StopDaemon(g_rpcbind, log);
        DisableDaemon(g_rpcbind, log);
        MaskDaemon(g_rpcbind, log);
    }
    if (CheckDaemonActive(g_rpcbindSocket, NULL, log))
    {
        RestartDaemon(g_rpcbindSocket, log);
        StopDaemon(g_rpcbindSocket, log);
        DisableDaemon(g_rpcbindSocket, log);
        MaskDaemon(g_rpcbindSocket, log);
    }
    return (CheckDaemonNotActive(g_rpcbindSocket, NULL, log) && CheckDaemonNotActive(g_rpcbind, NULL, log)) ? 0 : ENOENT;
}

static int RemediateEnsureNetworkFileSystemServiceIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_nfsServer, log);
    return CheckAndFreeReason(AuditEnsureNetworkFileSystemServiceIsDisabled(log));
}

static int RemediateEnsureRpcsvcgssdServiceIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    int status = 0;
    StopAndDisableDaemon(g_rpcSvcgssd, log);
    if (FileExists(g_etcInetdConf))
    {
        status = ReplaceMarkedLinesInFile(g_etcInetdConf, g_needSvcgssd, NULL, '#', true, log);
    }
    return ((0 == status) && (false == IsDaemonActive(g_rpcSvcgssd, log))) ? 0 : ENOENT;
}

static int RemediateEnsureSnmpServerIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_snmpd, log);
    return CheckAndFreeReason(AuditEnsureSnmpServerIsDisabled(log));
}

static int RemediateEnsureRsynServiceIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rsync, log);
    return CheckAndFreeReason(AuditEnsureRsynServiceIsDisabled(log));
}

static int RemediateEnsureNisServerIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_ypserv, log);
    return CheckAndFreeReason(AuditEnsureNisServerIsDisabled(log));
}

static int RemediateEnsureRshClientNotInstalled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return ((0 == UninstallPackage(g_rsh, log)) &&
        (0 == UninstallPackage(g_rshClient, log))) ? 0 : ENOENT;
}

static int RemediateEnsureSmbWithSambaIsDisabled(char* value, OsConfigLogHandle log)
{
    const char* command = "sed -i '/^\\[global\\]/a min protocol = SMB2' /etc/samba/smb.conf";
    const char* smb1 = "SMB1";
    int status = 0;

    UNUSED(value);

    if (IsDaemonActive(g_smbd, log))
    {
        if (0 == (status = ReplaceMarkedLinesInFile(g_etcSambaConf, smb1, NULL, '#', true, log)))
        {
            if (0 != FindTextInFile(g_etcSambaConf, g_minSambaProtocol, log))
            {
                status = ExecuteCommand(NULL, command, true, false, 0, 0, NULL, NULL, log);
            }
        }
    }
    else
    {
        UninstallPackage(g_samba, log);
        remove(g_etcSambaConf);
        status = CheckPackageNotInstalled(g_samba, NULL, log);
    }

    return status;
}

static int RemediateEnsureUsersDotFilesArentGroupOrWorldWritable(char* value, OsConfigLogHandle log)
{
    int* modes = NULL;
    int numberOfModes = 0;
    int status = 0;

    InitEnsureUsersDotFilesArentGroupOrWorldWritable(value);

    if ((0 == (status = ConvertStringToIntegers(g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable, ',', &modes, &numberOfModes, 8, log))) && (numberOfModes > 0))
    {
        status = SetUsersRestrictedDotFiles((unsigned int*)modes, (unsigned int)numberOfModes, modes[numberOfModes - 1], log);
    }

    FREE_MEMORY(modes);
    return status;
}

static int RemediateEnsureNoUsersHaveDotForwardFiles(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return CheckOrEnsureUsersDontHaveDotFiles(g_forward, true, NULL, log);
}

static int RemediateEnsureNoUsersHaveDotNetrcFiles(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return CheckOrEnsureUsersDontHaveDotFiles(g_netrc, true, NULL, log);
}

static int RemediateEnsureNoUsersHaveDotRhostsFiles(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    return CheckOrEnsureUsersDontHaveDotFiles(g_rhosts, true, NULL, log);
}

static int RemediateEnsureRloginServiceIsDisabled(char* value, OsConfigLogHandle log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rlogin, log);
    UninstallPackage(g_rlogin, log);
    UninstallPackage(g_inetd, log);
    UninstallPackage(g_inetUtilsInetd, log);
    return ((0 == CheckPackageNotInstalled(g_rlogin, NULL, log)) &&
        (0 == CheckPackageNotInstalled(g_inetd, NULL, log)) &&
        (0 == CheckPackageNotInstalled(g_inetUtilsInetd, NULL, log)) &&
        (0 == ReplaceMarkedLinesInFile(g_etcInetdConf, g_login, NULL, '#', true, log))) ? 0 : ENOENT;
}

static int RemediateEnsureUnnecessaryAccountsAreRemoved(char* value, OsConfigLogHandle log)
{
    InitEnsureUnnecessaryAccountsAreRemoved(value);
    return RemoveUserAccounts(g_desiredEnsureUnnecessaryAccountsAreRemoved, log);
}

int AsbMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OsConfigLogHandle log)
{
    JSON_Value* jsonValue = NULL;
    char* serializedValue = NULL;
    PerfClock perfClock = {{0, 0}, {0, 0}};
    int status = 0;
    char* result = NULL;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(log, "AsbMmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    StartPerfClock(&perfClock, GetPerfLog());

    if (0 != strcmp(componentName, g_securityBaselineComponentName))
    {
        OsConfigLogError(log, "AsbMmiGet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }
    else
    {
        if (0 == strcmp(objectName, g_auditEnsureLoggingLevelObject))
        {
            result = AuditEnsureLoggingLevel(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcIssueObject))
        {
            result = AuditEnsurePermissionsOnEtcIssue(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcIssueNetObject))
        {
            result = AuditEnsurePermissionsOnEtcIssueNet(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcHostsAllowObject))
        {
            result = AuditEnsurePermissionsOnEtcHostsAllow(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcHostsDenyObject))
        {
            result = AuditEnsurePermissionsOnEtcHostsDeny(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcSshSshdConfigObject))
        {
            result = AuditEnsurePermissionsOnEtcSshSshdConfig(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcShadowObject))
        {
            result = AuditEnsurePermissionsOnEtcShadow(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcShadowDashObject))
        {
            result = AuditEnsurePermissionsOnEtcShadowDash(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGShadowObject))
        {
            result = AuditEnsurePermissionsOnEtcGShadow(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGShadowDashObject))
        {
            result = AuditEnsurePermissionsOnEtcGShadowDash(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcPasswdObject))
        {
            result = AuditEnsurePermissionsOnEtcPasswd(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcPasswdDashObject))
        {
            result = AuditEnsurePermissionsOnEtcPasswdDash(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGroupObject))
        {
            result = AuditEnsurePermissionsOnEtcGroup(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGroupDashObject))
        {
            result = AuditEnsurePermissionsOnEtcGroupDash(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcAnacronTabObject))
        {
            result = AuditEnsurePermissionsOnEtcAnacronTab(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronDObject))
        {
            result = AuditEnsurePermissionsOnEtcCronD(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronDailyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronDaily(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronHourlyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronHourly(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronMonthlyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronMonthly(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronWeeklyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronWeekly(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcMotdObject))
        {
            result = AuditEnsurePermissionsOnEtcMotd(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureKernelSupportForCpuNxObject))
        {
            result = AuditEnsureKernelSupportForCpuNx(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionOnHomePartitionObject))
        {
            result = AuditEnsureNodevOptionOnHomePartition(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionOnTmpPartitionObject))
        {
            result = AuditEnsureNodevOptionOnTmpPartition(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionOnVarTmpPartitionObject))
        {
            result = AuditEnsureNodevOptionOnVarTmpPartition(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNosuidOptionOnTmpPartitionObject))
        {
            result = AuditEnsureNosuidOptionOnTmpPartition(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNosuidOptionOnVarTmpPartitionObject))
        {
            result = AuditEnsureNosuidOptionOnVarTmpPartition(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecOptionOnVarTmpPartitionObject))
        {
            result = AuditEnsureNoexecOptionOnVarTmpPartition(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecOptionOnDevShmPartitionObject))
        {
            result = AuditEnsureNoexecOptionOnDevShmPartition(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionEnabledForAllRemovableMediaObject))
        {
            result = AuditEnsureNodevOptionEnabledForAllRemovableMedia(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecOptionEnabledForAllRemovableMediaObject))
        {
            result = AuditEnsureNoexecOptionEnabledForAllRemovableMedia(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNosuidOptionEnabledForAllRemovableMediaObject))
        {
            result = AuditEnsureNosuidOptionEnabledForAllRemovableMedia(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject))
        {
            result = AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureInetdNotInstalledObject))
        {
            result = AuditEnsureInetdNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureXinetdNotInstalledObject))
        {
            result = AuditEnsureXinetdNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllTelnetdPackagesUninstalledObject))
        {
            result = AuditEnsureAllTelnetdPackagesUninstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRshServerNotInstalledObject))
        {
            result = AuditEnsureRshServerNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNisNotInstalledObject))
        {
            result = AuditEnsureNisNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureTftpdNotInstalledObject))
        {
            result = AuditEnsureTftpdNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureReadaheadFedoraNotInstalledObject))
        {
            result = AuditEnsureReadaheadFedoraNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureBluetoothHiddNotInstalledObject))
        {
            result = AuditEnsureBluetoothHiddNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureIsdnUtilsBaseNotInstalledObject))
        {
            result = AuditEnsureIsdnUtilsBaseNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureIsdnUtilsKdumpToolsNotInstalledObject))
        {
            result = AuditEnsureIsdnUtilsKdumpToolsNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureIscDhcpdServerNotInstalledObject))
        {
            result = AuditEnsureIscDhcpdServerNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSendmailNotInstalledObject))
        {
            result = AuditEnsureSendmailNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSldapdNotInstalledObject))
        {
            result = AuditEnsureSldapdNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureBind9NotInstalledObject))
        {
            result = AuditEnsureBind9NotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDovecotCoreNotInstalledObject))
        {
            result = AuditEnsureDovecotCoreNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAuditdInstalledObject))
        {
            result = AuditEnsureAuditdInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllEtcPasswdGroupsExistInEtcGroupObject))
        {
            result = AuditEnsureAllEtcPasswdGroupsExistInEtcGroup(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateUidsExistObject))
        {
            result = AuditEnsureNoDuplicateUidsExist(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateGidsExistObject))
        {
            result = AuditEnsureNoDuplicateGidsExist(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateUserNamesExistObject))
        {
            result = AuditEnsureNoDuplicateUserNamesExist(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateGroupsExistObject))
        {
            result = AuditEnsureNoDuplicateGroupsExist(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureShadowGroupIsEmptyObject))
        {
            result = AuditEnsureShadowGroupIsEmpty(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRootGroupExistsObject))
        {
            result = AuditEnsureRootGroupExists(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllAccountsHavePasswordsObject))
        {
            result = AuditEnsureAllAccountsHavePasswords(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject))
        {
            result = AuditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoLegacyPlusEntriesInEtcPasswdObject))
        {
            result = AuditEnsureNoLegacyPlusEntriesInEtcPasswd(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoLegacyPlusEntriesInEtcShadowObject))
        {
            result = AuditEnsureNoLegacyPlusEntriesInEtcShadow(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoLegacyPlusEntriesInEtcGroupObject))
        {
            result = AuditEnsureNoLegacyPlusEntriesInEtcGroup(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDefaultRootAccountGroupIsGidZeroObject))
        {
            result = AuditEnsureDefaultRootAccountGroupIsGidZero(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRootIsOnlyUidZeroAccountObject))
        {
            result = AuditEnsureRootIsOnlyUidZeroAccount(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllUsersHomeDirectoriesExistObject))
        {
            result = AuditEnsureAllUsersHomeDirectoriesExist(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureUsersOwnTheirHomeDirectoriesObject))
        {
            result = AuditEnsureUsersOwnTheirHomeDirectories(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRestrictedUserHomeDirectoriesObject))
        {
            result = AuditEnsureRestrictedUserHomeDirectories(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordHashingAlgorithmObject))
        {
            result = AuditEnsurePasswordHashingAlgorithm(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureMinDaysBetweenPasswordChangesObject))
        {
            result = AuditEnsureMinDaysBetweenPasswordChanges(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureInactivePasswordLockPeriodObject))
        {
            result = AuditEnsureInactivePasswordLockPeriod(log);
        }
        else if (0 == strcmp(objectName, g_auditMaxDaysBetweenPasswordChangesObject))
        {
            result = AuditEnsureMaxDaysBetweenPasswordChanges(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordExpirationObject))
        {
            result = AuditEnsurePasswordExpiration(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordExpirationWarningObject))
        {
            result = AuditEnsurePasswordExpirationWarning(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSystemAccountsAreNonLoginObject))
        {
            result = AuditEnsureSystemAccountsAreNonLogin(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAuthenticationRequiredForSingleUserModeObject))
        {
            result = AuditEnsureAuthenticationRequiredForSingleUserMode(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePrelinkIsDisabledObject))
        {
            result = AuditEnsurePrelinkIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureTalkClientIsNotInstalledObject))
        {
            result = AuditEnsureTalkClientIsNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDotDoesNotAppearInRootsPathObject))
        {
            result = AuditEnsureDotDoesNotAppearInRootsPath(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureCronServiceIsEnabledObject))
        {
            result = AuditEnsureCronServiceIsEnabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRemoteLoginWarningBannerIsConfiguredObject))
        {
            result = AuditEnsureRemoteLoginWarningBannerIsConfigured(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureLocalLoginWarningBannerIsConfiguredObject))
        {
            result = AuditEnsureLocalLoginWarningBannerIsConfigured(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAuditdServiceIsRunningObject))
        {
            result = AuditEnsureAuditdServiceIsRunning(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSuRestrictedToRootGroupObject))
        {
            result = AuditEnsureSuRestrictedToRootGroup(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDefaultUmaskForAllUsersObject))
        {
            result = AuditEnsureDefaultUmaskForAllUsers(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAutomountingDisabledObject))
        {
            result = AuditEnsureAutomountingDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureKernelCompiledFromApprovedSourcesObject))
        {
            result = AuditEnsureKernelCompiledFromApprovedSources(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDefaultDenyFirewallPolicyIsSetObject))
        {
            result = AuditEnsureDefaultDenyFirewallPolicyIsSet(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePacketRedirectSendingIsDisabledObject))
        {
            result = AuditEnsurePacketRedirectSendingIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureIcmpRedirectsIsDisabledObject))
        {
            result = AuditEnsureIcmpRedirectsIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSourceRoutedPacketsIsDisabledObject))
        {
            result = AuditEnsureSourceRoutedPacketsIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAcceptingSourceRoutedPacketsIsDisabledObject))
        {
            result = AuditEnsureAcceptingSourceRoutedPacketsIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureIgnoringBogusIcmpBroadcastResponsesObject))
        {
            result = AuditEnsureIgnoringBogusIcmpBroadcastResponses(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureIgnoringIcmpEchoPingsToMulticastObject))
        {
            result = AuditEnsureIgnoringIcmpEchoPingsToMulticast(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureMartianPacketLoggingIsEnabledObject))
        {
            result = AuditEnsureMartianPacketLoggingIsEnabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureReversePathSourceValidationIsEnabledObject))
        {
            result = AuditEnsureReversePathSourceValidationIsEnabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureTcpSynCookiesAreEnabledObject))
        {
            result = AuditEnsureTcpSynCookiesAreEnabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSystemNotActingAsNetworkSnifferObject))
        {
            result = AuditEnsureSystemNotActingAsNetworkSniffer(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllWirelessInterfacesAreDisabledObject))
        {
            result = AuditEnsureAllWirelessInterfacesAreDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureIpv6ProtocolIsEnabledObject))
        {
            result = AuditEnsureIpv6ProtocolIsEnabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDccpIsDisabledObject))
        {
            result = AuditEnsureDccpIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSctpIsDisabledObject))
        {
            result = AuditEnsureSctpIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledSupportForRdsObject))
        {
            result = AuditEnsureDisabledSupportForRds(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureTipcIsDisabledObject))
        {
            result = AuditEnsureTipcIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureZeroconfNetworkingIsDisabledObject))
        {
            result = AuditEnsureZeroconfNetworkingIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnBootloaderConfigObject))
        {
            result = AuditEnsurePermissionsOnBootloaderConfig(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordReuseIsLimitedObject))
        {
            result = AuditEnsurePasswordReuseIsLimited(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureMountingOfUsbStorageDevicesIsDisabledObject))
        {
            result = AuditEnsureMountingOfUsbStorageDevicesIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureCoreDumpsAreRestrictedObject))
        {
            result = AuditEnsureCoreDumpsAreRestricted(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordCreationRequirementsObject))
        {
            result = AuditEnsurePasswordCreationRequirements(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureLockoutForFailedPasswordAttemptsObject))
        {
            result = AuditEnsureLockoutForFailedPasswordAttempts(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfCramfsFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfCramfsFileSystem(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfFreevxfsFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfFreevxfsFileSystem(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfHfsFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfHfsFileSystem(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfHfsplusFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfHfsplusFileSystem(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfJffs2FileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfJffs2FileSystem(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureVirtualMemoryRandomizationIsEnabledObject))
        {
            result = AuditEnsureVirtualMemoryRandomizationIsEnabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllBootloadersHavePasswordProtectionEnabledObject))
        {
            result = AuditEnsureAllBootloadersHavePasswordProtectionEnabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureLoggingIsConfiguredObject))
        {
            result = AuditEnsureLoggingIsConfigured(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSyslogPackageIsInstalledObject))
        {
            result = AuditEnsureSyslogPackageIsInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSystemdJournaldServicePersistsLogMessagesObject))
        {
            result = AuditEnsureSystemdJournaldServicePersistsLogMessages(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureALoggingServiceIsEnabledObject))
        {
            result = AuditEnsureALoggingServiceIsEnabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureFilePermissionsForAllRsyslogLogFilesObject))
        {
            result = AuditEnsureFilePermissionsForAllRsyslogLogFiles(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureLoggerConfigurationFilesAreRestrictedObject))
        {
            result = AuditEnsureLoggerConfigurationFilesAreRestricted(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject))
        {
            result = AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject))
        {
            result = AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRsyslogNotAcceptingRemoteMessagesObject))
        {
            result = AuditEnsureRsyslogNotAcceptingRemoteMessages(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSyslogRotaterServiceIsEnabledObject))
        {
            result = AuditEnsureSyslogRotaterServiceIsEnabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureTelnetServiceIsDisabledObject))
        {
            result = AuditEnsureTelnetServiceIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRcprshServiceIsDisabledObject))
        {
            result = AuditEnsureRcprshServiceIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureTftpServiceisDisabledObject))
        {
            result = AuditEnsureTftpServiceisDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAtCronIsRestrictedToAuthorizedUsersObject))
        {
            result = AuditEnsureAtCronIsRestrictedToAuthorizedUsers(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshPortIsConfiguredObject))
        {
            result = AuditEnsureSshPortIsConfigured(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshBestPracticeProtocolObject))
        {
            result = AuditEnsureSshBestPracticeProtocol(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshBestPracticeIgnoreRhostsObject))
        {
            result = AuditEnsureSshBestPracticeIgnoreRhosts(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshLogLevelIsSetObject))
        {
            result = AuditEnsureSshLogLevelIsSet(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshMaxAuthTriesIsSetObject))
        {
            result = AuditEnsureSshMaxAuthTriesIsSet(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllowUsersIsConfiguredObject))
        {
            result = AuditEnsureAllowUsersIsConfigured(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDenyUsersIsConfiguredObject))
        {
            result = AuditEnsureDenyUsersIsConfigured(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllowGroupsIsConfiguredObject))
        {
            result = AuditEnsureAllowGroupsIsConfigured(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureDenyGroupsConfiguredObject))
        {
            result = AuditEnsureDenyGroupsConfigured(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshHostbasedAuthenticationIsDisabledObject))
        {
            result = AuditEnsureSshHostbasedAuthenticationIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshPermitRootLoginIsDisabledObject))
        {
            result = AuditEnsureSshPermitRootLoginIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshPermitEmptyPasswordsIsDisabledObject))
        {
            result = AuditEnsureSshPermitEmptyPasswordsIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshClientIntervalCountMaxIsConfiguredObject))
        {
            result = AuditEnsureSshClientIntervalCountMaxIsConfigured(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshClientAliveIntervalIsConfiguredObject))
        {
            result = AuditEnsureSshClientAliveIntervalIsConfigured(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshLoginGraceTimeIsSetObject))
        {
            result = AuditEnsureSshLoginGraceTimeIsSet(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject))
        {
            result = AuditEnsureOnlyApprovedMacAlgorithmsAreUsed(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshWarningBannerIsEnabledObject))
        {
            result = AuditEnsureSshWarningBannerIsEnabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureUsersCannotSetSshEnvironmentOptionsObject))
        {
            result = AuditEnsureUsersCannotSetSshEnvironmentOptions(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAppropriateCiphersForSshObject))
        {
            result = AuditEnsureAppropriateCiphersForSsh(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureAvahiDaemonServiceIsDisabledObject))
        {
            result = AuditEnsureAvahiDaemonServiceIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureCupsServiceisDisabledObject))
        {
            result = AuditEnsureCupsServiceisDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePostfixPackageIsUninstalledObject))
        {
            result = AuditEnsurePostfixPackageIsUninstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePostfixNetworkListeningIsDisabledObject))
        {
            result = AuditEnsurePostfixNetworkListeningIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRpcgssdServiceIsDisabledObject))
        {
            result = AuditEnsureRpcgssdServiceIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRpcidmapdServiceIsDisabledObject))
        {
            result = AuditEnsureRpcidmapdServiceIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsurePortmapServiceIsDisabledObject))
        {
            result = AuditEnsurePortmapServiceIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNetworkFileSystemServiceIsDisabledObject))
        {
            result = AuditEnsureNetworkFileSystemServiceIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRpcsvcgssdServiceIsDisabledObject))
        {
            result = AuditEnsureRpcsvcgssdServiceIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSnmpServerIsDisabledObject))
        {
            result = AuditEnsureSnmpServerIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRsynServiceIsDisabledObject))
        {
            result = AuditEnsureRsynServiceIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNisServerIsDisabledObject))
        {
            result = AuditEnsureNisServerIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRshClientNotInstalledObject))
        {
            result = AuditEnsureRshClientNotInstalled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureSmbWithSambaIsDisabledObject))
        {
            result = AuditEnsureSmbWithSambaIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureUsersDotFilesArentGroupOrWorldWritableObject))
        {
            result = AuditEnsureUsersDotFilesArentGroupOrWorldWritable(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoUsersHaveDotForwardFilesObject))
        {
            result = AuditEnsureNoUsersHaveDotForwardFiles(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoUsersHaveDotNetrcFilesObject))
        {
            result = AuditEnsureNoUsersHaveDotNetrcFiles(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoUsersHaveDotRhostsFilesObject))
        {
            result = AuditEnsureNoUsersHaveDotRhostsFiles(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureRloginServiceIsDisabledObject))
        {
            result = AuditEnsureRloginServiceIsDisabled(log);
        }
        else if (0 == strcmp(objectName, g_auditEnsureUnnecessaryAccountsAreRemovedObject))
        {
            result = AuditEnsureUnnecessaryAccountsAreRemoved(log);
        }
        else
        {
            OsConfigLogError(log, "AsbMmiGet called for an unsupported object (%s)", objectName);
            status = EINVAL;
        }
    }

    if (0 == status)
    {
        if ((NULL == result) || (NULL == (jsonValue = json_value_init_string(result))))
        {
            OsConfigLogError(log, "AsbMmiGet(%s, %s): audit failure without a valid reason", componentName, objectName);
            FREE_MEMORY(result);
            if (NULL == (result = DuplicateString(g_fail)))
            {
                OsConfigLogError(log, "AsbMmiGet: DuplicateString failed");
                status = ENOMEM;
            }
        }

        if (NULL != result)
        {
            if ((NULL == jsonValue) && (NULL == (jsonValue = json_value_init_string(result))))
            {
                OsConfigLogError(log, "AsbMmiGet(%s, %s): json_value_init_string(%s) failed", componentName, objectName, result);
                status = ENOMEM;
            }
            else if (NULL == (serializedValue = json_serialize_to_string(jsonValue)))
            {
                OsConfigLogError(log, "AsbMmiGet(%s, %s): json_serialize_to_string(%s) failed", componentName, objectName, result);
                status = ENOMEM;
            }
            else
            {
                *payloadSizeBytes = (int)strlen(serializedValue);

                if ((maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > maxPayloadSizeBytes))
                {
                    OsConfigLogInfo(log, "MmiGet(%s, %s) insufficient max size (%d bytes) vs actual size (%d bytes), report will be truncated",
                        componentName, objectName, maxPayloadSizeBytes, *payloadSizeBytes);

                    *payloadSizeBytes = maxPayloadSizeBytes;
                }

                if (NULL != (*payload = (char*)malloc(*payloadSizeBytes + 1)))
                {
                    memset(*payload, 0, *payloadSizeBytes + 1);
                    memcpy(*payload, serializedValue, *payloadSizeBytes);
                }
                else
                {
                    OsConfigLogError(log, "AsbMmiGet: failed to allocate %d bytes", *payloadSizeBytes + 1);
                    *payloadSizeBytes = 0;
                    status = ENOMEM;
                }
            }
        }
    }

    OsConfigLogInfo(log, "AsbMmiGet(%s, %s, %.*s, %d) returning %d", componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);

    if (NULL != serializedValue)
    {
        json_free_serialized_string(serializedValue);
    }

    if (NULL != jsonValue)
    {
        json_value_free(jsonValue);
    }

    FREE_MEMORY(result);

    if (0 == StopPerfClock(&perfClock, GetPerfLog()))
    {
        LogPerfClock(&perfClock, componentName, objectName, status, g_maxAuditTime, GetPerfLog());

        // For telemetry:
        OsConfigLogCritical(log, "TargetName: '%s', ComponentName: '%s', 'ObjectName:'%s', ObjectResult:'%s (%d)', Reason: '%.*s', Microseconds: %ld",
            g_prettyName, componentName, objectName, strerror(status), status, *payloadSizeBytes, *payload, GetPerfClockTime(&perfClock, log));
    }

    return status;
}

int AsbMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log)
{
    const char* init = "init";
    JSON_Value* jsonValue = NULL;
    char* jsonString = NULL;
    char* payloadString = NULL;
    PerfClock perfClock = {{0, 0}, {0, 0}};
    int status = 0;

    // No payload is accepted for now, this may change once the complete Azure Security Baseline is implemented
    if ((NULL == componentName) || (NULL == objectName))
    {
        OsConfigLogError(log, "AsbMmiSet(%s, %s, %s, %d) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        return EINVAL;
    }

    StartPerfClock(&perfClock, GetPerfLog());

    if (0 != strcmp(componentName, g_securityBaselineComponentName))
    {
        OsConfigLogError(log, "AsbMmiSet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }

    if ((0 == status) && (NULL != payload) && (0 < payloadSizeBytes))
    {
        if (NULL != (payloadString = malloc(payloadSizeBytes + 1)))
        {
            memset(payloadString, 0, payloadSizeBytes + 1);
            memcpy(payloadString, payload, payloadSizeBytes);

            if (NULL != (jsonValue = json_parse_string(payloadString)))
            {
                if (NULL == (jsonString = (char*)json_value_get_string(jsonValue)))
                {
                    status = EINVAL;
                    OsConfigLogError(log, "AsbMmiSet: json_value_get_string(%s) failed", payloadString);
                }
            }
            else
            {
                status = EINVAL;
                OsConfigLogError(log, "AsbMmiSet: json_parse_string(%s) failed", payloadString);
            }
        }
        else
        {
            status = ENOMEM;
            OsConfigLogError(log, "AsbMmiSet: failed to allocate %d bytes of memory", payloadSizeBytes + 1);
        }
    }

    if (0 == status)
    {
        if (0 == strcmp(objectName, g_remediateEnsureLoggingLevelObject))
        {
            status = RemediateEnsureLoggingLevel(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcIssueObject))
        {
            status = RemediateEnsurePermissionsOnEtcIssue(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcIssueNetObject))
        {
            status = RemediateEnsurePermissionsOnEtcIssueNet(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcHostsAllowObject))
        {
            status = RemediateEnsurePermissionsOnEtcHostsAllow(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcHostsDenyObject))
        {
            status = RemediateEnsurePermissionsOnEtcHostsDeny(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcSshSshdConfigObject))
        {
            status = RemediateEnsurePermissionsOnEtcSshSshdConfig(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcShadowObject))
        {
            status = RemediateEnsurePermissionsOnEtcShadow(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcShadowDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcShadowDash(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGShadowObject))
        {
            status = RemediateEnsurePermissionsOnEtcGShadow(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGShadowDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcGShadowDash(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcPasswdObject))
        {
            status = RemediateEnsurePermissionsOnEtcPasswd(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcPasswdDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcPasswdDash(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGroupObject))
        {
            status = RemediateEnsurePermissionsOnEtcGroup(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGroupDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcGroupDash(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcAnacronTabObject))
        {
            status = RemediateEnsurePermissionsOnEtcAnacronTab(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronDObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronD(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronDailyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronDaily(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronHourlyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronHourly(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronMonthlyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronMonthly(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronWeeklyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronWeekly(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcMotdObject))
        {
            status = RemediateEnsurePermissionsOnEtcMotd(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureInetdNotInstalledObject))
        {
            status = RemediateEnsureInetdNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureXinetdNotInstalledObject))
        {
            status = RemediateEnsureXinetdNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRshServerNotInstalledObject))
        {
            status = RemediateEnsureRshServerNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNisNotInstalledObject))
        {
            status = RemediateEnsureNisNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTftpdNotInstalledObject))
        {
            status = RemediateEnsureTftpdNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureReadaheadFedoraNotInstalledObject))
        {
            status = RemediateEnsureReadaheadFedoraNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureBluetoothHiddNotInstalledObject))
        {
            status = RemediateEnsureBluetoothHiddNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIsdnUtilsBaseNotInstalledObject))
        {
            status = RemediateEnsureIsdnUtilsBaseNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIsdnUtilsKdumpToolsNotInstalledObject))
        {
            status = RemediateEnsureIsdnUtilsKdumpToolsNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIscDhcpdServerNotInstalledObject))
        {
            status = RemediateEnsureIscDhcpdServerNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSendmailNotInstalledObject))
        {
            status = RemediateEnsureSendmailNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSldapdNotInstalledObject))
        {
            status = RemediateEnsureSldapdNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureBind9NotInstalledObject))
        {
            status = RemediateEnsureBind9NotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDovecotCoreNotInstalledObject))
        {
            status = RemediateEnsureDovecotCoreNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuditdInstalledObject))
        {
            status = RemediateEnsureAuditdInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePrelinkIsDisabledObject))
        {
            status = RemediateEnsurePrelinkIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTalkClientIsNotInstalledObject))
        {
            status = RemediateEnsureTalkClientIsNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureCronServiceIsEnabledObject))
        {
            status = RemediateEnsureCronServiceIsEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuditdServiceIsRunningObject))
        {
            status = RemediateEnsureAuditdServiceIsRunning(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureKernelSupportForCpuNxObject))
        {
            status = RemediateEnsureKernelSupportForCpuNx(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionOnHomePartitionObject))
        {
            status = RemediateEnsureNodevOptionOnHomePartition(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionOnTmpPartitionObject))
        {
            status = RemediateEnsureNodevOptionOnTmpPartition(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionOnVarTmpPartitionObject))
        {
            status = RemediateEnsureNodevOptionOnVarTmpPartition(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNosuidOptionOnTmpPartitionObject))
        {
            status = RemediateEnsureNosuidOptionOnTmpPartition(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNosuidOptionOnVarTmpPartitionObject))
        {
            status = RemediateEnsureNosuidOptionOnVarTmpPartition(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecOptionOnVarTmpPartitionObject))
        {
            status = RemediateEnsureNoexecOptionOnVarTmpPartition(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecOptionOnDevShmPartitionObject))
        {
            status = RemediateEnsureNoexecOptionOnDevShmPartition(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionEnabledForAllRemovableMediaObject))
        {
            status = RemediateEnsureNodevOptionEnabledForAllRemovableMedia(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecOptionEnabledForAllRemovableMediaObject))
        {
            status = RemediateEnsureNoexecOptionEnabledForAllRemovableMedia(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNosuidOptionEnabledForAllRemovableMediaObject))
        {
            status = RemediateEnsureNosuidOptionEnabledForAllRemovableMedia(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject))
        {
            status = RemediateEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllTelnetdPackagesUninstalledObject))
        {
            status = RemediateEnsureAllTelnetdPackagesUninstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllEtcPasswdGroupsExistInEtcGroupObject))
        {
            status = RemediateEnsureAllEtcPasswdGroupsExistInEtcGroup(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateUidsExistObject))
        {
            status = RemediateEnsureNoDuplicateUidsExist(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateGidsExistObject))
        {
            status = RemediateEnsureNoDuplicateGidsExist(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateUserNamesExistObject))
        {
            status = RemediateEnsureNoDuplicateUserNamesExist(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateGroupsExistObject))
        {
            status = RemediateEnsureNoDuplicateGroupsExist(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureShadowGroupIsEmptyObject))
        {
            status = RemediateEnsureShadowGroupIsEmpty(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRootGroupExistsObject))
        {
            status = RemediateEnsureRootGroupExists(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllAccountsHavePasswordsObject))
        {
            status = RemediateEnsureAllAccountsHavePasswords(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject))
        {
            status = RemediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoLegacyPlusEntriesInEtcPasswdObject))
        {
            status = RemediateEnsureNoLegacyPlusEntriesInEtcPasswd(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoLegacyPlusEntriesInEtcShadowObject))
        {
            status = RemediateEnsureNoLegacyPlusEntriesInEtcShadow(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoLegacyPlusEntriesInEtcGroupObject))
        {
            status = RemediateEnsureNoLegacyPlusEntriesInEtcGroup(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDefaultRootAccountGroupIsGidZeroObject))
        {
            status = RemediateEnsureDefaultRootAccountGroupIsGidZero(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRootIsOnlyUidZeroAccountObject))
        {
            status = RemediateEnsureRootIsOnlyUidZeroAccount(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllUsersHomeDirectoriesExistObject))
        {
            status = RemediateEnsureAllUsersHomeDirectoriesExist(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUsersOwnTheirHomeDirectoriesObject))
        {
            status = RemediateEnsureUsersOwnTheirHomeDirectories(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRestrictedUserHomeDirectoriesObject))
        {
            status = RemediateEnsureRestrictedUserHomeDirectories(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordHashingAlgorithmObject))
        {
            status = RemediateEnsurePasswordHashingAlgorithm(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureMinDaysBetweenPasswordChangesObject))
        {
            status = RemediateEnsureMinDaysBetweenPasswordChanges(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureInactivePasswordLockPeriodObject))
        {
            status = RemediateEnsureInactivePasswordLockPeriod(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateMaxDaysBetweenPasswordChangesObject))
        {
            status = RemediateEnsureMaxDaysBetweenPasswordChanges(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordExpirationObject))
        {
            status = RemediateEnsurePasswordExpiration(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordExpirationWarningObject))
        {
            status = RemediateEnsurePasswordExpirationWarning(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSystemAccountsAreNonLoginObject))
        {
            status = RemediateEnsureSystemAccountsAreNonLogin(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuthenticationRequiredForSingleUserModeObject))
        {
            status = RemediateEnsureAuthenticationRequiredForSingleUserMode(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDotDoesNotAppearInRootsPathObject))
        {
            status = RemediateEnsureDotDoesNotAppearInRootsPath(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRemoteLoginWarningBannerIsConfiguredObject))
        {
            status = RemediateEnsureRemoteLoginWarningBannerIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLocalLoginWarningBannerIsConfiguredObject))
        {
            status = RemediateEnsureLocalLoginWarningBannerIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuditdServiceIsRunningObject))
        {
            status = RemediateEnsureAuditdServiceIsRunning(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSuRestrictedToRootGroupObject))
        {
            status = RemediateEnsureSuRestrictedToRootGroup(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDefaultUmaskForAllUsersObject))
        {
            status = RemediateEnsureDefaultUmaskForAllUsers(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAutomountingDisabledObject))
        {
            status = RemediateEnsureAutomountingDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureKernelCompiledFromApprovedSourcesObject))
        {
            status = RemediateEnsureKernelCompiledFromApprovedSources(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDefaultDenyFirewallPolicyIsSetObject))
        {
            status = RemediateEnsureDefaultDenyFirewallPolicyIsSet(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePacketRedirectSendingIsDisabledObject))
        {
            status = RemediateEnsurePacketRedirectSendingIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIcmpRedirectsIsDisabledObject))
        {
            status = RemediateEnsureIcmpRedirectsIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSourceRoutedPacketsIsDisabledObject))
        {
            status = RemediateEnsureSourceRoutedPacketsIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAcceptingSourceRoutedPacketsIsDisabledObject))
        {
            status = RemediateEnsureAcceptingSourceRoutedPacketsIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIgnoringBogusIcmpBroadcastResponsesObject))
        {
            status = RemediateEnsureIgnoringBogusIcmpBroadcastResponses(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIgnoringIcmpEchoPingsToMulticastObject))
        {
            status = RemediateEnsureIgnoringIcmpEchoPingsToMulticast(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureMartianPacketLoggingIsEnabledObject))
        {
            status = RemediateEnsureMartianPacketLoggingIsEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureReversePathSourceValidationIsEnabledObject))
        {
            status = RemediateEnsureReversePathSourceValidationIsEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTcpSynCookiesAreEnabledObject))
        {
            status = RemediateEnsureTcpSynCookiesAreEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSystemNotActingAsNetworkSnifferObject))
        {
            status = RemediateEnsureSystemNotActingAsNetworkSniffer(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllWirelessInterfacesAreDisabledObject))
        {
            status = RemediateEnsureAllWirelessInterfacesAreDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIpv6ProtocolIsEnabledObject))
        {
            status = RemediateEnsureIpv6ProtocolIsEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDccpIsDisabledObject))
        {
            status = RemediateEnsureDccpIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSctpIsDisabledObject))
        {
            status = RemediateEnsureSctpIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledSupportForRdsObject))
        {
            status = RemediateEnsureDisabledSupportForRds(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTipcIsDisabledObject))
        {
            status = RemediateEnsureTipcIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureZeroconfNetworkingIsDisabledObject))
        {
            status = RemediateEnsureZeroconfNetworkingIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnBootloaderConfigObject))
        {
            status = RemediateEnsurePermissionsOnBootloaderConfig(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordReuseIsLimitedObject))
        {
            status = RemediateEnsurePasswordReuseIsLimited(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureMountingOfUsbStorageDevicesIsDisabledObject))
        {
            status = RemediateEnsureMountingOfUsbStorageDevicesIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureCoreDumpsAreRestrictedObject))
        {
            status = RemediateEnsureCoreDumpsAreRestricted(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordCreationRequirementsObject))
        {
            status = RemediateEnsurePasswordCreationRequirements(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLockoutForFailedPasswordAttemptsObject))
        {
            status = RemediateEnsureLockoutForFailedPasswordAttempts(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfCramfsFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfCramfsFileSystem(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfFreevxfsFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfFreevxfsFileSystem(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfHfsFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfHfsFileSystem(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfHfsplusFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfHfsplusFileSystem(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfJffs2FileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfJffs2FileSystem(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureVirtualMemoryRandomizationIsEnabledObject))
        {
            status = RemediateEnsureVirtualMemoryRandomizationIsEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllBootloadersHavePasswordProtectionEnabledObject))
        {
            status = RemediateEnsureAllBootloadersHavePasswordProtectionEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLoggingIsConfiguredObject))
        {
            status = RemediateEnsureLoggingIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSyslogPackageIsInstalledObject))
        {
            status = RemediateEnsureSyslogPackageIsInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSystemdJournaldServicePersistsLogMessagesObject))
        {
            status = RemediateEnsureSystemdJournaldServicePersistsLogMessages(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureALoggingServiceIsEnabledObject))
        {
            status = RemediateEnsureALoggingServiceIsEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureFilePermissionsForAllRsyslogLogFilesObject))
        {
            status = RemediateEnsureFilePermissionsForAllRsyslogLogFiles(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLoggerConfigurationFilesAreRestrictedObject))
        {
            status = RemediateEnsureLoggerConfigurationFilesAreRestricted(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject))
        {
            status = RemediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject))
        {
            status = RemediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRsyslogNotAcceptingRemoteMessagesObject))
        {
            status = RemediateEnsureRsyslogNotAcceptingRemoteMessages(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSyslogRotaterServiceIsEnabledObject))
        {
            status = RemediateEnsureSyslogRotaterServiceIsEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTelnetServiceIsDisabledObject))
        {
            status = RemediateEnsureTelnetServiceIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRcprshServiceIsDisabledObject))
        {
            status = RemediateEnsureRcprshServiceIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTftpServiceisDisabledObject))
        {
            status = RemediateEnsureTftpServiceisDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAtCronIsRestrictedToAuthorizedUsersObject))
        {
            status = RemediateEnsureAtCronIsRestrictedToAuthorizedUsers(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshPortIsConfiguredObject))
        {
            status = RemediateEnsureSshPortIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshBestPracticeProtocolObject))
        {
            status = RemediateEnsureSshBestPracticeProtocol(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshBestPracticeIgnoreRhostsObject))
        {
            status = RemediateEnsureSshBestPracticeIgnoreRhosts(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshLogLevelIsSetObject))
        {
            status = RemediateEnsureSshLogLevelIsSet(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshMaxAuthTriesIsSetObject))
        {
            status = RemediateEnsureSshMaxAuthTriesIsSet(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllowUsersIsConfiguredObject))
        {
            status = RemediateEnsureAllowUsersIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDenyUsersIsConfiguredObject))
        {
            status = RemediateEnsureDenyUsersIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllowGroupsIsConfiguredObject))
        {
            status = RemediateEnsureAllowGroupsIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDenyGroupsConfiguredObject))
        {
            status = RemediateEnsureDenyGroupsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject))
        {
            status = RemediateEnsureSshHostbasedAuthenticationIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshPermitRootLoginIsDisabledObject))
        {
            status = RemediateEnsureSshPermitRootLoginIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject))
        {
            status = RemediateEnsureSshPermitEmptyPasswordsIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshClientIntervalCountMaxIsConfiguredObject))
        {
            status = RemediateEnsureSshClientIntervalCountMaxIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshClientAliveIntervalIsConfiguredObject))
        {
            status = RemediateEnsureSshClientAliveIntervalIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshLoginGraceTimeIsSetObject))
        {
            status = RemediateEnsureSshLoginGraceTimeIsSet(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject))
        {
            status = RemediateEnsureOnlyApprovedMacAlgorithmsAreUsed(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshWarningBannerIsEnabledObject))
        {
            status = RemediateEnsureSshWarningBannerIsEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject))
        {
            status = RemediateEnsureUsersCannotSetSshEnvironmentOptions(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAppropriateCiphersForSshObject))
        {
            status = RemediateEnsureAppropriateCiphersForSsh(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAvahiDaemonServiceIsDisabledObject))
        {
            status = RemediateEnsureAvahiDaemonServiceIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureCupsServiceisDisabledObject))
        {
            status = RemediateEnsureCupsServiceisDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePostfixPackageIsUninstalledObject))
        {
            status = RemediateEnsurePostfixPackageIsUninstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePostfixNetworkListeningIsDisabledObject))
        {
            status = RemediateEnsurePostfixNetworkListeningIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRpcgssdServiceIsDisabledObject))
        {
            status = RemediateEnsureRpcgssdServiceIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRpcidmapdServiceIsDisabledObject))
        {
            status = RemediateEnsureRpcidmapdServiceIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePortmapServiceIsDisabledObject))
        {
            status = RemediateEnsurePortmapServiceIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNetworkFileSystemServiceIsDisabledObject))
        {
            status = RemediateEnsureNetworkFileSystemServiceIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRpcsvcgssdServiceIsDisabledObject))
        {
            status = RemediateEnsureRpcsvcgssdServiceIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSnmpServerIsDisabledObject))
        {
            status = RemediateEnsureSnmpServerIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRsynServiceIsDisabledObject))
        {
            status = RemediateEnsureRsynServiceIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNisServerIsDisabledObject))
        {
            status = RemediateEnsureNisServerIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRshClientNotInstalledObject))
        {
            status = RemediateEnsureRshClientNotInstalled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSmbWithSambaIsDisabledObject))
        {
            status = RemediateEnsureSmbWithSambaIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUsersDotFilesArentGroupOrWorldWritableObject))
        {
            status = RemediateEnsureUsersDotFilesArentGroupOrWorldWritable(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoUsersHaveDotForwardFilesObject))
        {
            status = RemediateEnsureNoUsersHaveDotForwardFiles(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoUsersHaveDotNetrcFilesObject))
        {
            status = RemediateEnsureNoUsersHaveDotNetrcFiles(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoUsersHaveDotRhostsFilesObject))
        {
            status = RemediateEnsureNoUsersHaveDotRhostsFiles(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRloginServiceIsDisabledObject))
        {
            status = RemediateEnsureRloginServiceIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUnnecessaryAccountsAreRemovedObject))
        {
            status = RemediateEnsureUnnecessaryAccountsAreRemoved(jsonString, log);
        }
        // Initialization for audit before remediation
        else if (0 == strcmp(objectName, g_initEnsureLoggingLevelObject))
        {
            status = InitEnsureLoggingLevel(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcSshSshdConfigObject))
        {
            status = InitEnsurePermissionsOnEtcSshSshdConfig(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshPortIsConfiguredObject))
        {
            status = InitEnsureSshPortIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshBestPracticeProtocolObject))
        {
            status = InitEnsureSshBestPracticeProtocol(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshBestPracticeIgnoreRhostsObject))
        {
            status = InitEnsureSshBestPracticeIgnoreRhosts(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshLogLevelIsSetObject))
        {
            status = InitEnsureSshLogLevelIsSet(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshMaxAuthTriesIsSetObject))
        {
            status = InitEnsureSshMaxAuthTriesIsSet(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureAllowUsersIsConfiguredObject))
        {
            status = InitEnsureAllowUsersIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureDenyUsersIsConfiguredObject))
        {
            status = InitEnsureDenyUsersIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureAllowGroupsIsConfiguredObject))
        {
            status = InitEnsureAllowGroupsIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureDenyGroupsConfiguredObject))
        {
            status = InitEnsureDenyGroupsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshHostbasedAuthenticationIsDisabledObject))
        {
            status = InitEnsureSshHostbasedAuthenticationIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshPermitRootLoginIsDisabledObject))
        {
            status = InitEnsureSshPermitRootLoginIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshPermitEmptyPasswordsIsDisabledObject))
        {
            status = InitEnsureSshPermitEmptyPasswordsIsDisabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshClientIntervalCountMaxIsConfiguredObject))
        {
            status = InitEnsureSshClientIntervalCountMaxIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshClientAliveIntervalIsConfiguredObject))
        {
            status = InitEnsureSshClientAliveIntervalIsConfigured(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshLoginGraceTimeIsSetObject))
        {
            status = InitEnsureSshLoginGraceTimeIsSet(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureOnlyApprovedMacAlgorithmsAreUsedObject))
        {
            status = InitEnsureOnlyApprovedMacAlgorithmsAreUsed(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshWarningBannerIsEnabledObject))
        {
            status = InitEnsureSshWarningBannerIsEnabled(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureUsersCannotSetSshEnvironmentOptionsObject))
        {
            status = InitEnsureUsersCannotSetSshEnvironmentOptions(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsureAppropriateCiphersForSshObject))
        {
            status = InitEnsureAppropriateCiphersForSsh(jsonString, log);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcIssueObject))
        {
            status = InitEnsurePermissionsOnEtcIssue(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcIssueNetObject))
        {
            status = InitEnsurePermissionsOnEtcIssueNet(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcHostsAllowObject))
        {
            status = InitEnsurePermissionsOnEtcHostsAllow(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcHostsDenyObject))
        {
            status = InitEnsurePermissionsOnEtcHostsDeny(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcShadowObject))
        {
            status = InitEnsurePermissionsOnEtcShadow(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcShadowDashObject))
        {
            status = InitEnsurePermissionsOnEtcShadowDash(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcGShadowObject))
        {
            status = InitEnsurePermissionsOnEtcGShadow(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcGShadowDashObject))
        {
            status = InitEnsurePermissionsOnEtcGShadowDash(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcPasswdObject))
        {
            status = InitEnsurePermissionsOnEtcPasswd(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcPasswdDashObject))
        {
            status = InitEnsurePermissionsOnEtcPasswdDash(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcGroupObject))
        {
            status = InitEnsurePermissionsOnEtcGroup(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcGroupDashObject))
        {
            status = InitEnsurePermissionsOnEtcGroupDash(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcAnacronTabObject))
        {
            status = InitEnsurePermissionsOnEtcAnacronTab(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcCronDObject))
        {
            status = InitEnsurePermissionsOnEtcCronD(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcCronDailyObject))
        {
            status = InitEnsurePermissionsOnEtcCronDaily(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcCronHourlyObject))
        {
            status = InitEnsurePermissionsOnEtcCronHourly(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcCronMonthlyObject))
        {
            status = InitEnsurePermissionsOnEtcCronMonthly(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcCronWeeklyObject))
        {
            status = InitEnsurePermissionsOnEtcCronWeekly(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcMotdObject))
        {
            status = InitEnsurePermissionsOnEtcMotd(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureRestrictedUserHomeDirectoriesObject))
        {
            status = InitEnsureRestrictedUserHomeDirectories(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePasswordHashingAlgorithmObject))
        {
            status = InitEnsurePasswordHashingAlgorithm(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureMinDaysBetweenPasswordChangesObject))
        {
            status = InitEnsureMinDaysBetweenPasswordChanges(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureInactivePasswordLockPeriodObject))
        {
            status = InitEnsureInactivePasswordLockPeriod(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureMaxDaysBetweenPasswordChangesObject))
        {
            status = InitEnsureMaxDaysBetweenPasswordChanges(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePasswordExpirationObject))
        {
            status = InitEnsurePasswordExpiration(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePasswordExpirationWarningObject))
        {
            status = InitEnsurePasswordExpirationWarning(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureDefaultUmaskForAllUsersObject))
        {
            status = InitEnsureDefaultUmaskForAllUsers(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnBootloaderConfigObject))
        {
            status = InitEnsurePermissionsOnBootloaderConfig(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePasswordReuseIsLimitedObject))
        {
            status = InitEnsurePasswordReuseIsLimited(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsurePasswordCreationRequirementsObject))
        {
            status = InitEnsurePasswordCreationRequirements(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureFilePermissionsForAllRsyslogLogFilesObject))
        {
            status = InitEnsureFilePermissionsForAllRsyslogLogFiles(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureUsersDotFilesArentGroupOrWorldWritableObject))
        {
            status = InitEnsureUsersDotFilesArentGroupOrWorldWritable(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureUnnecessaryAccountsAreRemovedObject))
        {
            status = InitEnsureUnnecessaryAccountsAreRemoved(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureDefaultDenyFirewallPolicyIsSetObject))
        {
            status = InitEnsureDefaultDenyFirewallPolicyIsSet(jsonString);
        }
        else
        {
            OsConfigLogError(log, "AsbMmiSet called for an unsupported object name: %s", objectName);
            status = EINVAL;
        }
    }

    OsConfigLogInfo(log, "AsbMmiSet(%s, %s, %.*s, %d) returning %d", componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);

    if (NULL != jsonValue)
    {
        json_value_free(jsonValue);
    }

    FREE_MEMORY(payloadString);

    if (0 == StopPerfClock(&perfClock, GetPerfLog()))
    {
        // Ignore the successful init* objects and focus on remediate* ones
        if (0 != strncmp(objectName, init, strlen(init)))
        {
            g_auditOnly = false;

            LogPerfClock(&perfClock, componentName, objectName, status, g_maxRemediateTime, GetPerfLog());

            // For telemetry:
            OsConfigLogCritical(log, "TargetName: '%s', ComponentName: '%s', 'ObjectName:'%s', ObjectResult:'%s (%d)', Microseconds: %ld",
                g_prettyName, componentName, objectName, strerror(status), status, GetPerfClockTime(&perfClock, log));

        }
    }

    return status;
}
