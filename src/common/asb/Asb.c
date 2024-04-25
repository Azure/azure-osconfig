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
#include <Asb.h>

static const char* g_asbName = "Azure Security Baseline for Linux";

static const char* g_securityBaselineComponentName = "SecurityBaseline";

// Audit
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

// Initialization for audit before remediation ----------- TODO: add these to MIM, to Test Recipe, to unit-tests, etc!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
/****************************************************************************************************
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
static const char* g_initEnsureInetdNotInstalledObject = "initEnsureInetdNotInstalled";
static const char* g_initEnsureXinetdNotInstalledObject = "initEnsureXinetdNotInstalled";
static const char* g_initEnsureRshServerNotInstalledObject = "initEnsureRshServerNotInstalled";
static const char* g_initEnsureNisNotInstalledObject = "initEnsureNisNotInstalled";
static const char* g_initEnsureTftpdNotInstalledObject = "initEnsureTftpdNotInstalled";
static const char* g_initEnsureReadaheadFedoraNotInstalledObject = "initEnsureReadaheadFedoraNotInstalled";
static const char* g_initEnsureBluetoothHiddNotInstalledObject = "initEnsureBluetoothHiddNotInstalled";
static const char* g_initEnsureIsdnUtilsBaseNotInstalledObject = "initEnsureIsdnUtilsBaseNotInstalled";
static const char* g_initEnsureIsdnUtilsKdumpToolsNotInstalledObject = "initEnsureIsdnUtilsKdumpToolsNotInstalled";
static const char* g_initEnsureIscDhcpdServerNotInstalledObject = "initEnsureIscDhcpdServerNotInstalled";
static const char* g_initEnsureSendmailNotInstalledObject = "initEnsureSendmailNotInstalled";
static const char* g_initEnsureSldapdNotInstalledObject = "initEnsureSldapdNotInstalled";
static const char* g_initEnsureBind9NotInstalledObject = "initEnsureBind9NotInstalled";
static const char* g_initEnsureDovecotCoreNotInstalledObject = "initEnsureDovecotCoreNotInstalled";
static const char* g_initEnsureAuditdInstalledObject = "initEnsureAuditdInstalled";
static const char* g_initEnsurePrelinkIsDisabledObject = "initEnsurePrelinkIsDisabled";
static const char* g_initEnsureTalkClientIsNotInstalledObject = "initEnsureTalkClientIsNotInstalled";
static const char* g_initEnsureCronServiceIsEnabledObject = "initEnsureCronServiceIsEnabled";
static const char* g_initEnsureAuditdServiceIsRunningObject = "initEnsureAuditdServiceIsRunning";
static const char* g_initEnsureKernelSupportForCpuNxObject = "initEnsureKernelSupportForCpuNx";
static const char* g_initEnsureAllTelnetdPackagesUninstalledObject = "initEnsureAllTelnetdPackagesUninstalled";
static const char* g_initEnsureNodevOptionOnHomePartitionObject = "initEnsureNodevOptionOnHomePartition";
static const char* g_initEnsureNodevOptionOnTmpPartitionObject = "initEnsureNodevOptionOnTmpPartition";
static const char* g_initEnsureNodevOptionOnVarTmpPartitionObject = "initEnsureNodevOptionOnVarTmpPartition";
static const char* g_initEnsureNosuidOptionOnTmpPartitionObject = "initEnsureNosuidOptionOnTmpPartition";
static const char* g_initEnsureNosuidOptionOnVarTmpPartitionObject = "initEnsureNosuidOptionOnVarTmpPartition";
static const char* g_initEnsureNoexecOptionOnVarTmpPartitionObject = "initEnsureNoexecOptionOnVarTmpPartition";
static const char* g_initEnsureNoexecOptionOnDevShmPartitionObject = "initEnsureNoexecOptionOnDevShmPartition";
static const char* g_initEnsureNodevOptionEnabledForAllRemovableMediaObject = "initEnsureNodevOptionEnabledForAllRemovableMedia";
static const char* g_initEnsureNoexecOptionEnabledForAllRemovableMediaObject = "initEnsureNoexecOptionEnabledForAllRemovableMedia";
static const char* g_initEnsureNosuidOptionEnabledForAllRemovableMediaObject = "initEnsureNosuidOptionEnabledForAllRemovableMedia";
static const char* g_initEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject = "initEnsureNoexecNosuidOptionsEnabledForAllNfsMounts";
static const char* g_initEnsureAllEtcPasswdGroupsExistInEtcGroupObject = "initEnsureAllEtcPasswdGroupsExistInEtcGroup";
static const char* g_initEnsureNoDuplicateUidsExistObject = "initEnsureNoDuplicateUidsExist";
static const char* g_initEnsureNoDuplicateGidsExistObject = "initEnsureNoDuplicateGidsExist";
static const char* g_initEnsureNoDuplicateUserNamesExistObject = "initEnsureNoDuplicateUserNamesExist";
static const char* g_initEnsureNoDuplicateGroupsExistObject = "initEnsureNoDuplicateGroupsExist";
static const char* g_initEnsureShadowGroupIsEmptyObject = "initEnsureShadowGroupIsEmpty";
static const char* g_initEnsureRootGroupExistsObject = "initEnsureRootGroupExists";
static const char* g_initEnsureAllAccountsHavePasswordsObject = "initEnsureAllAccountsHavePasswords";
static const char* g_initEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject = "initEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero";
static const char* g_initEnsureNoLegacyPlusEntriesInEtcPasswdObject = "initEnsureNoLegacyPlusEntriesInEtcPasswd";
static const char* g_initEnsureNoLegacyPlusEntriesInEtcShadowObject = "initEnsureNoLegacyPlusEntriesInEtcShadow";
static const char* g_initEnsureNoLegacyPlusEntriesInEtcGroupObject = "initEnsureNoLegacyPlusEntriesInEtcGroup";
static const char* g_initEnsureDefaultRootAccountGroupIsGidZeroObject = "initEnsureDefaultRootAccountGroupIsGidZero";
static const char* g_initEnsureRootIsOnlyUidZeroAccountObject = "initEnsureRootIsOnlyUidZeroAccount";
static const char* g_initEnsureAllUsersHomeDirectoriesExistObject = "initEnsureAllUsersHomeDirectoriesExist";
static const char* g_initEnsureUsersOwnTheirHomeDirectoriesObject = "initEnsureUsersOwnTheirHomeDirectories";
static const char* g_initEnsureRestrictedUserHomeDirectoriesObject = "initEnsureRestrictedUserHomeDirectories";
static const char* g_initEnsurePasswordHashingAlgorithmObject = "initEnsurePasswordHashingAlgorithm";
static const char* g_initEnsureMinDaysBetweenPasswordChangesObject = "initEnsureMinDaysBetweenPasswordChanges";
static const char* g_initEnsureInactivePasswordLockPeriodObject = "initEnsureInactivePasswordLockPeriod";
static const char* g_initMaxDaysBetweenPasswordChangesObject = "initEnsureMaxDaysBetweenPasswordChanges";
static const char* g_initEnsurePasswordExpirationObject = "initEnsurePasswordExpiration";
static const char* g_initEnsurePasswordExpirationWarningObject = "initEnsurePasswordExpirationWarning";
static const char* g_initEnsureSystemAccountsAreNonLoginObject = "initEnsureSystemAccountsAreNonLogin";
static const char* g_initEnsureAuthenticationRequiredForSingleUserModeObject = "initEnsureAuthenticationRequiredForSingleUserMode";
static const char* g_initEnsureDotDoesNotAppearInRootsPathObject = "initEnsureDotDoesNotAppearInRootsPath";
static const char* g_initEnsureRemoteLoginWarningBannerIsConfiguredObject = "initEnsureRemoteLoginWarningBannerIsConfigured";
static const char* g_initEnsureLocalLoginWarningBannerIsConfiguredObject = "initEnsureLocalLoginWarningBannerIsConfigured";
static const char* g_initEnsureSuRestrictedToRootGroupObject = "initEnsureSuRestrictedToRootGroup";
static const char* g_initEnsureDefaultUmaskForAllUsersObject = "initEnsureDefaultUmaskForAllUsers";
static const char* g_initEnsureAutomountingDisabledObject = "initEnsureAutomountingDisabled";
static const char* g_initEnsureKernelCompiledFromApprovedSourcesObject = "initEnsureKernelCompiledFromApprovedSources";
static const char* g_initEnsureDefaultDenyFirewallPolicyIsSetObject = "initEnsureDefaultDenyFirewallPolicyIsSet";
static const char* g_initEnsurePacketRedirectSendingIsDisabledObject = "initEnsurePacketRedirectSendingIsDisabled";
static const char* g_initEnsureIcmpRedirectsIsDisabledObject = "initEnsureIcmpRedirectsIsDisabled";
static const char* g_initEnsureSourceRoutedPacketsIsDisabledObject = "initEnsureSourceRoutedPacketsIsDisabled";
static const char* g_initEnsureAcceptingSourceRoutedPacketsIsDisabledObject = "initEnsureAcceptingSourceRoutedPacketsIsDisabled";
static const char* g_initEnsureIgnoringBogusIcmpBroadcastResponsesObject = "initEnsureIgnoringBogusIcmpBroadcastResponses";
static const char* g_initEnsureIgnoringIcmpEchoPingsToMulticastObject = "initEnsureIgnoringIcmpEchoPingsToMulticast";
static const char* g_initEnsureMartianPacketLoggingIsEnabledObject = "initEnsureMartianPacketLoggingIsEnabled";
static const char* g_initEnsureReversePathSourceValidationIsEnabledObject = "initEnsureReversePathSourceValidationIsEnabled";
static const char* g_initEnsureTcpSynCookiesAreEnabledObject = "initEnsureTcpSynCookiesAreEnabled";
static const char* g_initEnsureSystemNotActingAsNetworkSnifferObject = "initEnsureSystemNotActingAsNetworkSniffer";
static const char* g_initEnsureAllWirelessInterfacesAreDisabledObject = "initEnsureAllWirelessInterfacesAreDisabled";
static const char* g_initEnsureIpv6ProtocolIsEnabledObject = "initEnsureIpv6ProtocolIsEnabled";
static const char* g_initEnsureDccpIsDisabledObject = "initEnsureDccpIsDisabled";
static const char* g_initEnsureSctpIsDisabledObject = "initEnsureSctpIsDisabled";
static const char* g_initEnsureDisabledSupportForRdsObject = "initEnsureDisabledSupportForRds";
static const char* g_initEnsureTipcIsDisabledObject = "initEnsureTipcIsDisabled";
static const char* g_initEnsureZeroconfNetworkingIsDisabledObject = "initEnsureZeroconfNetworkingIsDisabled";
static const char* g_initEnsurePermissionsOnBootloaderConfigObject = "initEnsurePermissionsOnBootloaderConfig";
static const char* g_initEnsurePasswordReuseIsLimitedObject = "initEnsurePasswordReuseIsLimited";
static const char* g_initEnsureMountingOfUsbStorageDevicesIsDisabledObject = "initEnsureMountingOfUsbStorageDevicesIsDisabled";
static const char* g_initEnsureCoreDumpsAreRestrictedObject = "initEnsureCoreDumpsAreRestricted";
static const char* g_initEnsurePasswordCreationRequirementsObject = "initEnsurePasswordCreationRequirements";
static const char* g_initEnsureLockoutForFailedPasswordAttemptsObject = "initEnsureLockoutForFailedPasswordAttempts";
static const char* g_initEnsureDisabledInstallationOfCramfsFileSystemObject = "initEnsureDisabledInstallationOfCramfsFileSystem";
static const char* g_initEnsureDisabledInstallationOfFreevxfsFileSystemObject = "initEnsureDisabledInstallationOfFreevxfsFileSystem";
static const char* g_initEnsureDisabledInstallationOfHfsFileSystemObject = "initEnsureDisabledInstallationOfHfsFileSystem";
static const char* g_initEnsureDisabledInstallationOfHfsplusFileSystemObject = "initEnsureDisabledInstallationOfHfsplusFileSystem";
static const char* g_initEnsureDisabledInstallationOfJffs2FileSystemObject = "initEnsureDisabledInstallationOfJffs2FileSystem";
static const char* g_initEnsureVirtualMemoryRandomizationIsEnabledObject = "initEnsureVirtualMemoryRandomizationIsEnabled";
static const char* g_initEnsureAllBootloadersHavePasswordProtectionEnabledObject = "initEnsureAllBootloadersHavePasswordProtectionEnabled";
static const char* g_initEnsureLoggingIsConfiguredObject = "initEnsureLoggingIsConfigured";
static const char* g_initEnsureSyslogPackageIsInstalledObject = "initEnsureSyslogPackageIsInstalled";
static const char* g_initEnsureSystemdJournaldServicePersistsLogMessagesObject = "initEnsureSystemdJournaldServicePersistsLogMessages";
static const char* g_initEnsureALoggingServiceIsEnabledObject = "initEnsureALoggingServiceIsEnabled";
static const char* g_initEnsureFilePermissionsForAllRsyslogLogFilesObject = "initEnsureFilePermissionsForAllRsyslogLogFiles";
static const char* g_initEnsureLoggerConfigurationFilesAreRestrictedObject = "initEnsureLoggerConfigurationFilesAreRestricted";
static const char* g_initEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject = "initEnsureAllRsyslogLogFilesAreOwnedByAdmGroup";
static const char* g_initEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject = "initEnsureAllRsyslogLogFilesAreOwnedBySyslogUser";
static const char* g_initEnsureRsyslogNotAcceptingRemoteMessagesObject = "initEnsureRsyslogNotAcceptingRemoteMessages";
static const char* g_initEnsureSyslogRotaterServiceIsEnabledObject = "initEnsureSyslogRotaterServiceIsEnabled";
static const char* g_initEnsureTelnetServiceIsDisabledObject = "initEnsureTelnetServiceIsDisabled";
static const char* g_initEnsureRcprshServiceIsDisabledObject = "initEnsureRcprshServiceIsDisabled";
static const char* g_initEnsureTftpServiceisDisabledObject = "initEnsureTftpServiceisDisabled";
static const char* g_initEnsureAtCronIsRestrictedToAuthorizedUsersObject = "initEnsureAtCronIsRestrictedToAuthorizedUsers";
static const char* g_initEnsureAvahiDaemonServiceIsDisabledObject = "initEnsureAvahiDaemonServiceIsDisabled";
static const char* g_initEnsureCupsServiceisDisabledObject = "initEnsureCupsServiceisDisabled";
static const char* g_initEnsurePostfixPackageIsUninstalledObject = "initEnsurePostfixPackageIsUninstalled";
static const char* g_initEnsurePostfixNetworkListeningIsDisabledObject = "initEnsurePostfixNetworkListeningIsDisabled";
static const char* g_initEnsureRpcgssdServiceIsDisabledObject = "initEnsureRpcgssdServiceIsDisabled";
static const char* g_initEnsureRpcidmapdServiceIsDisabledObject = "initEnsureRpcidmapdServiceIsDisabled";
static const char* g_initEnsurePortmapServiceIsDisabledObject = "initEnsurePortmapServiceIsDisabled";
static const char* g_initEnsureNetworkFileSystemServiceIsDisabledObject = "initEnsureNetworkFileSystemServiceIsDisabled";
static const char* g_initEnsureRpcsvcgssdServiceIsDisabledObject = "initEnsureRpcsvcgssdServiceIsDisabled";
static const char* g_initEnsureSnmpServerIsDisabledObject = "initEnsureSnmpServerIsDisabled";
static const char* g_initEnsureRsynServiceIsDisabledObject = "initEnsureRsynServiceIsDisabled";
static const char* g_initEnsureNisServerIsDisabledObject = "initEnsureNisServerIsDisabled";
static const char* g_initEnsureRshClientNotInstalledObject = "initEnsureRshClientNotInstalled";
static const char* g_initEnsureSmbWithSambaIsDisabledObject = "initEnsureSmbWithSambaIsDisabled";
static const char* g_initEnsureUsersDotFilesArentGroupOrWorldWritableObject = "initEnsureUsersDotFilesArentGroupOrWorldWritable";
static const char* g_initEnsureNoUsersHaveDotForwardFilesObject = "initEnsureNoUsersHaveDotForwardFiles";
static const char* g_initEnsureNoUsersHaveDotNetrcFilesObject = "initEnsureNoUsersHaveDotNetrcFiles";
static const char* g_initEnsureNoUsersHaveDotRhostsFilesObject = "initEnsureNoUsersHaveDotRhostsFiles";
static const char* g_initEnsureRloginServiceIsDisabledObject = "initEnsureRloginServiceIsDisabled";
static const char* g_initEnsureUnnecessaryAccountsAreRemovedObject = "initEnsureUnnecessaryAccountsAreRemoved";
****************************************************************************************************************/

static const char* g_etcIssue = "/etc/issue";
static const char* g_etcIssueNet = "/etc/issue.net";
static const char* g_etcHostsAllow = "/etc/hosts.allow";
static const char* g_etcHostsDeny = "/etc/hosts.deny";
static const char* g_etcShadow = "/etc/shadow";
static const char* g_etcShadowDash = "/etc/shadow-";
static const char* g_etcGShadow = "/etc/gshadow";
static const char* g_etcGShadowDash = "/etc/gshadow-";
static const char* g_etcPasswd = "/etc/passwd";
static const char* g_etcPasswdDash = "/etc/passwd-";
static const char* g_etcPamdCommonPassword = "/etc/pam.d/common-password";
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
static const char* g_etcMtab = "/etc/mtab";
static const char* g_etcInetdConf = "/etc/inetd.conf";
static const char* g_etcModProbeD = "/etc/modprobe.d";
static const char* g_etcProfile = "/etc/profile";
static const char* g_etcRsyslogConf = "/etc/rsyslog.conf";
static const char* g_etcSyslogNgSyslogNgConf = "/etc/syslog-ng/syslog-ng.conf";

static const char* g_tmp = "/tmp";
static const char* g_varTmp = "/var/tmp";
static const char* g_media = "/media/";
static const char* g_nodev = "nodev";
static const char* g_nosuid = "nosuid";
static const char* g_noexec = "noexec";
static const char* g_inetd = "inetd";
static const char* g_inetUtilsInetd = "inetutils-inetd";
static const char* g_xinetd = "xinetd";
static const char* g_rshServer = "rsh-server";
static const char* g_nis = "nis";
static const char* g_tftpd = "tftpd-hpa";
static const char* g_readAheadFedora = "readahead-fedora";
static const char* g_bluetooth = "bluetooth";
static const char* g_isdnUtilsBase = "isdnutils-base";
static const char* g_kdumpTools = "kdump-tools";
static const char* g_iscDhcpServer = "isc-dhcp-server";
static const char* g_sendmail = "sendmail";
static const char* g_slapd = "slapd";
static const char* g_bind9 = "bind9";
static const char* g_dovecotCore = "dovecot-core";
static const char* g_auditd = "auditd";
static const char* g_prelink = "prelink";
static const char* g_talk = "talk";
static const char* g_cron = "cron";
static const char* g_syslog = "syslog";
static const char* g_rsyslog = "rsyslog";
static const char* g_syslogNg = "syslog-ng";
static const char* g_systemd = "systemd";
static const char* g_postfix = "postfix";
static const char* g_avahiDaemon = "avahi-daemon";
static const char* g_cups = "cups";
static const char* g_rpcgssd = "rpcgssd";
static const char* g_rpcGssd = "rpc-gssd";
static const char* g_rpcidmapd = "rpcidmapd";
static const char* g_nfsIdmapd = "nfs-idmapd";
static const char* g_rpcbind = "rpcbind";
static const char* g_rpcbindService = "rpcbind.service";
static const char* g_rpcbindSocket = "rpcbind.socket";
static const char* g_nfsServer = "nfs-server";
static const char* g_snmpd = "snmpd";
static const char* g_rsync = "rsync";
static const char* g_ypserv = "ypserv";
static const char* g_rsh = "rsh";
static const char* g_rshClient = "rsh-client";
static const char* g_forward = "forward";
static const char* g_netrc = "netrc";
static const char* g_rhosts = "rhosts";
static const char* g_systemdJournald = "systemd-journald";

static long g_minDaysBetweenPasswordChanges = 7;
static long g_maxDaysBetweenPasswordChanges = 365;
static long g_passwordExpirationWarning = 7;
static long g_passwordExpiration = 365;
static long g_maxInactiveDays = 30;

static const char* g_pass = SECURITY_AUDIT_PASS;
static const char* g_fail = SECURITY_AUDIT_FAIL;

void AsbInitialize(void* log)
{
    InitializeSshAudit(log);
    OsConfigLogInfo(log, "%s initialized", g_asbName);
}

void AsbShutdown(void* log)
{
    OsConfigLogInfo(log, "%s shutting down", g_asbName);
    SshAuditCleanup(log);
}

static char* AuditEnsurePermissionsOnEtcIssue(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcIssue, 0, 0, 644, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcIssueNet(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcIssueNet, 0, 0, 644, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcHostsAllow(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcHostsAllow, 0, 0, 644, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcHostsDeny(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcHostsDeny, 0, 0, 644, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcSshSshdConfig(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsurePermissionsOnEtcSshSshdConfigObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcShadow(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcShadow, 0, 42, 400, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcShadowDash(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcShadowDash, 0, 42, 400, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGShadow(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGShadow, 0, 42, 400, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGShadowDash(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGShadowDash, 0, 42, 400, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcPasswd(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcPasswd, 0, 0, 644, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcPasswdDash(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcPasswdDash, 0, 0, 600, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGroup(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGroup, 0, 0, 644, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGroupDash(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGroupDash, 0, 0, 644, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcAnacronTab(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcAnacronTab, 0, 0, 600, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronD(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronD, 0, 0, 700, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronDaily(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronDaily, 0, 0, 700, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronHourly(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronHourly, 0, 0, 700, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronMonthly(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronMonthly, 0, 0, 700, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronWeekly(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronWeekly, 0, 0, 700, &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcMotd(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcMotd, 0, 0, 644, &reason, log);
    return reason;
}

static char* AuditEnsureKernelSupportForCpuNx(void* log)
{
    char* reason = NULL;
    CheckCpuFlagSupported("nx", &reason, log);
    return reason;
}

static char* AuditEnsureNodevOptionOnHomePartition(void* log)
{
    const char* home = "/home";
    char* reason = NULL;
    if (0 != CheckFileSystemMountingOption(g_etcFstab, home, NULL, g_nodev, &reason, log))
    {
        CheckFileSystemMountingOption(g_etcMtab, home, NULL, g_nodev, &reason, log); 
    }
    return reason;
}

static char* AuditEnsureNodevOptionOnTmpPartition(void* log)
{
    char* reason = NULL;
    if (0 != CheckFileSystemMountingOption(g_etcFstab, g_tmp, NULL, g_nodev, &reason, log))
    {
        CheckFileSystemMountingOption(g_etcMtab, g_tmp, NULL, g_nodev, &reason, log);
    }
    return reason;
}

static char* AuditEnsureNodevOptionOnVarTmpPartition(void* log)
{
    char* reason = NULL;
    if (0 != CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_nodev, &reason, log))
    {
        CheckFileSystemMountingOption(g_etcMtab, g_varTmp, NULL, g_nodev, &reason, log);
    }
    return reason;
}

static char* AuditEnsureNosuidOptionOnTmpPartition(void* log)
{
    char* reason = NULL;
    if (0 != CheckFileSystemMountingOption(g_etcFstab, g_tmp, NULL, g_nosuid, &reason, log))
    {
        CheckFileSystemMountingOption(g_etcMtab, g_tmp, NULL, g_nosuid, &reason, log);
    }
    return reason;
}

static char* AuditEnsureNosuidOptionOnVarTmpPartition(void* log)
{
    char* reason = NULL;
    if (0 != CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_nosuid, &reason, log))
    {
        CheckFileSystemMountingOption(g_etcMtab, g_varTmp, NULL, g_nosuid, &reason, log);
    }
    return reason;
}

static char* AuditEnsureNoexecOptionOnVarTmpPartition(void* log)
{
    char* reason = NULL;
    if (0 != CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_noexec, &reason, log))
    {
        CheckFileSystemMountingOption(g_etcMtab, g_varTmp, NULL, g_noexec, &reason, log);
    }
    return reason;
}

static char* AuditEnsureNoexecOptionOnDevShmPartition(void* log)
{
    const char* devShm = "/dev/shm";
    char* reason = NULL;
    if (0 != CheckFileSystemMountingOption(g_etcFstab, devShm, NULL, g_noexec, &reason, log))
    {
        CheckFileSystemMountingOption(g_etcMtab, devShm, NULL, g_noexec, &reason, log);
    }
    return reason;
}

static char* AuditEnsureNodevOptionEnabledForAllRemovableMedia(void* log)
{
    char* reason = NULL;
    if (0 != CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_nodev, &reason, log))
    {
        CheckFileSystemMountingOption(g_etcMtab, g_media, NULL, g_nodev, &reason, log);
    }
    return reason;
}

static char* AuditEnsureNoexecOptionEnabledForAllRemovableMedia(void* log)
{
    char* reason = NULL;
    if (0 != CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_noexec, &reason, log))
    {
        CheckFileSystemMountingOption(g_etcMtab, g_media, NULL, g_noexec, &reason, log);
    }
    return reason;
}

static char* AuditEnsureNosuidOptionEnabledForAllRemovableMedia(void* log)
{
    char* reason = NULL;
    if (0 != CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_nosuid, &reason, log))
    {
        CheckFileSystemMountingOption(g_etcMtab, g_media, NULL, g_nosuid, &reason, log);
    }
    return reason;
}

static char* AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(void* log)
{
    const char* nfs = "nfs";
    char* reason = NULL;
    if ((0 != CheckFileSystemMountingOption(g_etcFstab, NULL, nfs, g_noexec, &reason, log)) ||
        (0 != CheckFileSystemMountingOption(g_etcFstab, NULL, nfs, g_nosuid, &reason, log)))
    {
        CheckFileSystemMountingOption(g_etcMtab, NULL, nfs, g_noexec, &reason, log);
        CheckFileSystemMountingOption(g_etcMtab, NULL, nfs, g_nosuid, &reason, log);
    }
    return reason;
}

static char* AuditEnsureInetdNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_inetd, &reason, log);
    CheckPackageNotInstalled(g_inetUtilsInetd, &reason, log);
    return reason;
}

static char* AuditEnsureXinetdNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_xinetd, &reason, log);
    return reason;
}

static char* AuditEnsureAllTelnetdPackagesUninstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled("*telnetd*", &reason, log);
    return reason;
}

static char* AuditEnsureRshServerNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_rshServer, &reason, log);
    return reason;
}

static char* AuditEnsureNisNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_nis, &reason, log);
    return reason;
}

static char* AuditEnsureTftpdNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_tftpd, &reason, log);
    return reason;
}

static char* AuditEnsureReadaheadFedoraNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_readAheadFedora, &reason, log);
    return reason;
}

static char* AuditEnsureBluetoothHiddNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_bluetooth, &reason, log);
    CheckDaemonNotActive(g_bluetooth, &reason, log);
    return reason;
}

static char* AuditEnsureIsdnUtilsBaseNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_isdnUtilsBase, &reason, log);
    return reason;
}

static char* AuditEnsureIsdnUtilsKdumpToolsNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_kdumpTools, &reason, log);
    return reason;
}

static char* AuditEnsureIscDhcpdServerNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_iscDhcpServer, &reason, log);
    return reason;
}

static char* AuditEnsureSendmailNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_sendmail, &reason, log);
    return reason;
}

static char* AuditEnsureSldapdNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_slapd, &reason, log);
    return reason;
}

static char* AuditEnsureBind9NotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageInstalled(g_bind9, &reason, log);
    return reason;
}

static char* AuditEnsureDovecotCoreNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_dovecotCore, &reason, log);
    return reason;
}

static char* AuditEnsureAuditdInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageInstalled(g_auditd, &reason, log);
    return reason;
}

static char* AuditEnsureAllEtcPasswdGroupsExistInEtcGroup(void* log)
{
    char* reason = NULL;
    CheckAllEtcPasswdGroupsExistInEtcGroup(&reason, log);
    return reason;
}

static char* AuditEnsureNoDuplicateUidsExist(void* log)
{
    char* reason = NULL;
    CheckNoDuplicateUidsExist(&reason, log);
    return reason;
}

static char* AuditEnsureNoDuplicateGidsExist(void* log)
{
    char* reason = NULL;
    CheckNoDuplicateGidsExist(&reason, log);
    return reason;
}

static char* AuditEnsureNoDuplicateUserNamesExist(void* log)
{
    char* reason = NULL;
    CheckNoDuplicateUserNamesExist(&reason, log);
    return reason;
}

static char* AuditEnsureNoDuplicateGroupsExist(void* log)
{
    char* reason = NULL;
    CheckNoDuplicateGroupsExist(&reason, log);
    return reason;
}

static char* AuditEnsureShadowGroupIsEmpty(void* log)
{
    char* reason = NULL;
    CheckShadowGroupIsEmpty(&reason, log);
    return reason;
}

static char* AuditEnsureRootGroupExists(void* log)
{
    char* reason = NULL;
    CheckRootGroupExists(&reason, log);
    return reason;
}

static char* AuditEnsureAllAccountsHavePasswords(void* log)
{
    char* reason = NULL;
    CheckAllUsersHavePasswordsSet(&reason, log);
    return reason;
}

static char* AuditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(void* log)
{
    char* reason = NULL;
    CheckRootIsOnlyUidZeroAccount(&reason, log);
    return reason;
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcPasswd(void* log)
{
    char* reason = NULL;
    CheckNoLegacyPlusEntriesInFile("etc/passwd", &reason, log);
    return reason;
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcShadow(void* log)
{
    char* reason = NULL;
    CheckNoLegacyPlusEntriesInFile("etc/shadow", &reason, log);
    return reason;
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcGroup(void* log)
{
    char* reason = NULL;
    CheckNoLegacyPlusEntriesInFile("etc/group", &reason, log);
    return reason;
}

static char* AuditEnsureDefaultRootAccountGroupIsGidZero(void* log)
{
    char* reason = NULL;
    CheckDefaultRootAccountGroupIsGidZero(&reason, log);
    return reason;
}

static char* AuditEnsureRootIsOnlyUidZeroAccount(void* log)
{
    char* reason = NULL;
    CheckRootGroupExists(&reason, log);
    CheckRootIsOnlyUidZeroAccount(&reason, log);
    return reason;
}

static char* AuditEnsureAllUsersHomeDirectoriesExist(void* log)
{
    char* reason = NULL;
    CheckAllUsersHomeDirectoriesExist(&reason, log);
    return reason;
}

static char* AuditEnsureUsersOwnTheirHomeDirectories(void* log)
{
    char* reason = NULL;
    CheckUsersOwnTheirHomeDirectories(&reason, log);
    return reason;
}

static char* AuditEnsureRestrictedUserHomeDirectories(void* log)
{
    unsigned int modes[] = {700, 750};
    char* reason = NULL;
    CheckRestrictedUserHomeDirectories(modes, ARRAY_SIZE(modes), &reason, log);
    return reason;
}

static char* AuditEnsurePasswordHashingAlgorithm(void* log)
{
    char* reason = NULL;
    CheckPasswordHashingAlgorithm(sha512, &reason, log);
    return reason;
}

static char* AuditEnsureMinDaysBetweenPasswordChanges(void* log)
{
    char* reason = NULL;
    CheckMinDaysBetweenPasswordChanges(g_minDaysBetweenPasswordChanges, &reason, log);
    return reason;
}

static char* AuditEnsureInactivePasswordLockPeriod(void* log)
{
    char* reason = NULL;
    CheckLockoutAfterInactivityLessThan(g_maxInactiveDays, &reason, log);
    CheckUsersRecordedPasswordChangeDates(&reason, log);
    return reason;
}

static char* AuditEnsureMaxDaysBetweenPasswordChanges(void* log)
{
    char* reason = NULL;
    CheckMaxDaysBetweenPasswordChanges(g_maxDaysBetweenPasswordChanges, &reason, log);
    return reason;
}

static char* AuditEnsurePasswordExpiration(void* log)
{
    char* reason = NULL;
    CheckPasswordExpirationLessThan(g_passwordExpiration, &reason, log);
    return reason;
}

static char* AuditEnsurePasswordExpirationWarning(void* log)
{
    char* reason = NULL;
    CheckPasswordExpirationWarning(g_passwordExpirationWarning, &reason, log);
    return reason;
}

static char* AuditEnsureSystemAccountsAreNonLogin(void* log)
{
    char* reason = NULL;
    CheckSystemAccountsAreNonLogin(&reason, log);
    return reason;
}

static char* AuditEnsureAuthenticationRequiredForSingleUserMode(void* log)
{
    char* reason = NULL;
    CheckRootPasswordForSingleUserMode(&reason, log);
    return reason;
}

static char* AuditEnsurePrelinkIsDisabled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_prelink, &reason, log);
    return reason;
}

static char* AuditEnsureTalkClientIsNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_talk, &reason, log);
    return reason;
}

static char* AuditEnsureDotDoesNotAppearInRootsPath(void* log)
{
    const char* path = "PATH";
    const char* dot = ".";
    char* reason = NULL;
    CheckTextNotFoundInEnvironmentVariable(path, dot, false, &reason, log);
    CheckMarkedTextNotFoundInFile("/etc/sudoers", "secure_path", dot, &reason, log);
    CheckMarkedTextNotFoundInFile(g_etcEnvironment, path, dot, &reason, log);
    CheckMarkedTextNotFoundInFile(g_etcProfile, path, dot, &reason, log);
    CheckMarkedTextNotFoundInFile("/root/.profile", path, dot, &reason, log);
    return reason;
}

static char* AuditEnsureCronServiceIsEnabled(void* log)
{
    char* reason = NULL;
    CheckPackageInstalled(g_cron, &reason, log);
    CheckDaemonActive(g_cron, &reason, log);
    return reason;
}

static char* AuditEnsureRemoteLoginWarningBannerIsConfigured(void* log)
{
    char* reason = NULL;
    CheckTextIsNotFoundInFile(g_etcIssueNet, "\\m", &reason, log);
    CheckTextIsNotFoundInFile(g_etcIssueNet, "\\r", &reason, log);
    CheckTextIsNotFoundInFile(g_etcIssueNet, "\\s", &reason, log);
    CheckTextIsNotFoundInFile(g_etcIssueNet, "\\v", &reason, log);
    return reason;
}

static char* AuditEnsureLocalLoginWarningBannerIsConfigured(void* log)
{
    char* reason = NULL;
    CheckTextIsNotFoundInFile(g_etcIssue, "\\m", &reason, log);
    CheckTextIsNotFoundInFile(g_etcIssue, "\\r", &reason, log);
    CheckTextIsNotFoundInFile(g_etcIssue, "\\s", &reason, log);
    CheckTextIsNotFoundInFile(g_etcIssue, "\\v", &reason, log);
    return reason;
}

static char* AuditEnsureAuditdServiceIsRunning(void* log)
{
    char* reason = NULL;
    CheckDaemonActive(g_auditd, &reason, log);
    return reason;
}

static char* AuditEnsureSuRestrictedToRootGroup(void* log)
{
    char* reason = NULL;
    CheckTextIsFoundInFile("/etc/pam.d/su", "use_uid", &reason, log);
    return reason;
}

static char* AuditEnsureDefaultUmaskForAllUsers(void* log)
{
    char* reason = NULL;
    CheckLoginUmask("077", &reason, log);
    return reason;
}

static char* AuditEnsureAutomountingDisabled(void* log)
{
    const char* autofs = "autofs";
    char* reason = NULL;
    CheckPackageInstalled(autofs, &reason, log);
    CheckDaemonNotActive(autofs, &reason, log);
    return reason;
}

static char* AuditEnsureKernelCompiledFromApprovedSources(void* log)
{
    char* reason = NULL;
    CheckOsAndKernelMatchDistro(&reason, log);
    return reason;
}

static char* AuditEnsureDefaultDenyFirewallPolicyIsSet(void* log)
{
    const char* readIpTables = "iptables -S";
    char* reason = NULL;
    CheckTextFoundInCommandOutput(readIpTables, "-P INPUT DROP", &reason, log);
    CheckTextFoundInCommandOutput(readIpTables, "-P FORWARD DROP", &reason, log);
    CheckTextFoundInCommandOutput(readIpTables, "-P OUTPUT DROP", &reason, log);
    return reason;
}

static char* AuditEnsurePacketRedirectSendingIsDisabled(void* log)
{
    const char* command = "sysctl -a";
    char* reason = NULL;
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.all.send_redirects = 0", &reason, log);
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.default.send_redirects = 0", &reason, log);
    return reason;
}

static char* AuditEnsureIcmpRedirectsIsDisabled(void* log)
{
    const char* command = "sysctl -a";
    char* reason = NULL;
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.default.accept_redirects = 0", &reason, log);
    CheckTextFoundInCommandOutput(command, "net.ipv6.conf.default.accept_redirects = 0", &reason, log);
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.all.accept_redirects = 0", &reason, log);
    CheckTextFoundInCommandOutput(command, "net.ipv6.conf.all.accept_redirects = 0", &reason, log);
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.default.secure_redirects = 0", &reason, log);
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.all.secure_redirects = 0", &reason, log);
    return reason;
}

static char* AuditEnsureSourceRoutedPacketsIsDisabled(void* log)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/all/accept_source_route", '#', "0", &reason, log);
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv6/conf/all/accept_source_route", '#', "0", &reason, log);
    return reason;
}

static char* AuditEnsureAcceptingSourceRoutedPacketsIsDisabled(void* log)
{
    char* reason = 0;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/all/accept_source_route", '#', "0", &reason, log);
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv6/conf/default/accept_source_route", '#', "0", &reason, log);
    return reason;
}

static char* AuditEnsureIgnoringBogusIcmpBroadcastResponses(void* log)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/icmp_ignore_bogus_error_responses", '#', "1", &reason, log);
    return reason;
}

static char* AuditEnsureIgnoringIcmpEchoPingsToMulticast(void* log)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/icmp_echo_ignore_broadcasts", '#', "1", &reason, log);
    return reason;
}

static char* AuditEnsureMartianPacketLoggingIsEnabled(void* log)
{
    const char* command = "sysctl -a";
    char* reason = NULL;
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.all.log_martians = 1", &reason, log);
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.default.log_martians = 1", &reason, log);
    return reason;
}

static char* AuditEnsureReversePathSourceValidationIsEnabled(void* log)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/all/rp_filter", '#', "1", &reason, log);
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/default/rp_filter", '#', "1", &reason, log);
    return reason;
}

static char* AuditEnsureTcpSynCookiesAreEnabled(void* log)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/tcp_syncookies", '#', "1", &reason, log);
    return reason;
}

static char* AuditEnsureSystemNotActingAsNetworkSniffer(void* log)
{
    const char* command = "/sbin/ip addr list";
    const char* text = "PROMISC";
    char* reason = NULL;
    CheckTextNotFoundInCommandOutput(command, text, &reason, log);
    CheckLineNotFoundOrCommentedOut("/etc/network/interfaces", '#', text, &reason, log);
    CheckLineNotFoundOrCommentedOut("/etc/rc.local", '#', text, &reason, log);
    return reason;
}

static char* AuditEnsureAllWirelessInterfacesAreDisabled(void* log)
{
    char* reason = NULL;
    if (0 == CheckTextNotFoundInCommandOutput("/sbin/iwconfig 2>&1 | /bin/egrep -v 'no wireless extensions|not found'", "Frequency", &reason, log))
    {
        OsConfigResetReason(&reason);
        OsConfigCaptureSuccessReason(&reason, "No active wireless interfaces are present");
    }
    else
    {
        OsConfigResetReason(&reason);
        OsConfigCaptureReason(&reason, "At least one active wireless interface is present");
    }
    return reason;
}

static char* AuditEnsureIpv6ProtocolIsEnabled(void* log)
{
    char* reason = NULL;
    CheckTextFoundInCommandOutput("cat /sys/module/ipv6/parameters/disable", "0", &reason, log);
    return reason;
}

static char* AuditEnsureDccpIsDisabled(void* log)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install dccp /bin/true", &reason, log);
    return reason;
}

static char* AuditEnsureSctpIsDisabled(void* log)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install sctp /bin/true", &reason, log);
    return reason;
}

static char* AuditEnsureDisabledSupportForRds(void* log)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install rds /bin/true", &reason, log);
    return reason;
}

static char* AuditEnsureTipcIsDisabled(void* log)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install tipc /bin/true", &reason, log);
    return reason;
}

static char* AuditEnsureZeroconfNetworkingIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_avahiDaemon, &reason, log);
    CheckLineNotFoundOrCommentedOut("/etc/network/interfaces", '#', "ipv4ll", &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnBootloaderConfig(void* log)
{
    char* reason = NULL;
    CheckFileAccess("/boot/grub/grub.cfg", 0, 0, 400, &reason, log);
    CheckFileAccess("/boot/grub/grub.conf", 0, 0, 400, &reason, log);
    CheckFileAccess("/boot/grub2/grub.cfg", 0, 0, 400, &reason, log);
    return reason;
}

static char* AuditEnsurePasswordReuseIsLimited(void* log)
{
    const char* etcPamdSystemAuth = "/etc/pam.d/system-auth";
    char* reason = NULL;
    if (0 == CheckIntegerOptionFromFileLessOrEqualWith(g_etcPamdCommonPassword, "remember", '=', 5, &reason, log))
    {
        return reason;
    }
    CheckIntegerOptionFromFileLessOrEqualWith(etcPamdSystemAuth, "remember", '=', 5, &reason, log);
    return reason;
}

static char* AuditEnsureMountingOfUsbStorageDevicesIsDisabled(void* log)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install usb-storage /bin/true", &reason, log);
    return reason;
}

static char* AuditEnsureCoreDumpsAreRestricted(void* log)
{
    const char* fsSuidDumpable = "fs.suid_dumpable = 0";
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/etc/security/limits.conf", '#', "hard core 0", &reason, log);
    CheckTextFoundInFolder("/etc/security/limits.d", fsSuidDumpable, &reason, log);
    CheckTextFoundInCommandOutput("sysctl -a", fsSuidDumpable, &reason, log);
    return reason;
}

static char* AuditEnsurePasswordCreationRequirements(void* log)
{
    char* reason = NULL;
    CheckPasswordCreationRequirements(14, 4, -1, -1, -1, -1, &reason, log);
    return reason;
}

static char* AuditEnsureLockoutForFailedPasswordAttempts(void* log)
{
    const char* passwordAuth = "/etc/pam.d/password-auth";
    const char* commonAuth = "/etc/pam.d/common-auth";
    char* reason = NULL;
    if (0 == CheckLockoutForFailedPasswordAttempts(passwordAuth, &reason, log))
    {
        return reason;
    }
    OsConfigResetReason(&reason);
    CheckLockoutForFailedPasswordAttempts(commonAuth, &reason, log);
    return reason;
}

static char* AuditEnsureDisabledInstallationOfCramfsFileSystem(void* log)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install cramfs", &reason, log);
    return reason;
}

static char* AuditEnsureDisabledInstallationOfFreevxfsFileSystem(void* log)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install freevxfs", &reason, log);
    return reason;
}

static char* AuditEnsureDisabledInstallationOfHfsFileSystem(void* log)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install hfs", &reason, log);
    return reason;
}

static char* AuditEnsureDisabledInstallationOfHfsplusFileSystem(void* log)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install hfsplus", &reason, log);
    return reason;
}

static char* AuditEnsureDisabledInstallationOfJffs2FileSystem(void* log)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install jffs2", &reason, log);
    return reason;
}

static char* AuditEnsureVirtualMemoryRandomizationIsEnabled(void* log)
{
    char* reason = NULL;
    if (0 == CheckFileContents("/proc/sys/kernel/randomize_va_space", "2", &reason, log))
    {
        return reason;
    }
    OsConfigResetReason(&reason);
    if (0 != CheckFileContents("/proc/sys/kernel/randomize_va_space", "1", &reason, log))
    {
        OsConfigCaptureReason(&reason, "neither 2");
    }
    return reason;
}

static char* AuditEnsureAllBootloadersHavePasswordProtectionEnabled(void* log)
{
    const char* password = "password";
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/boot/grub/grub.cfg", '#', password, &reason, log);
    CheckLineFoundNotCommentedOut("/boot/grub/grub.conf", '#', password, &reason, log);
    CheckLineFoundNotCommentedOut("/boot/grub2/grub.conf", '#', password, &reason, log);
    return reason;
}

static char* AuditEnsureLoggingIsConfigured(void* log)
{
    char* reason = NULL;
    CheckFileExists("/var/log/syslog", &reason, log);
    return reason;
}

static char* AuditEnsureSyslogPackageIsInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageInstalled(g_syslog, &reason, log);
    CheckPackageInstalled(g_rsyslog, &reason, log);
    CheckPackageInstalled(g_syslogNg, &reason, log);
    return reason;
}

static char* AuditEnsureSystemdJournaldServicePersistsLogMessages(void* log)
{
    char* reason = NULL;
    CheckPackageInstalled(g_systemd, &reason, log);
    CheckDirectoryAccess("/var/log/journal", 0, -1, 2775, false, &reason, log);
    return reason;
}

static char* AuditEnsureALoggingServiceIsEnabled(void* log)
{
    char* reason = NULL;
    if ((0 == CheckPackageNotInstalled(g_syslogNg, &reason, log)) && 
        (0 == CheckPackageNotInstalled(g_systemd, &reason, log)) && 
        CheckDaemonActive(g_rsyslog, &reason, log))
    {
        return reason;
    }
    OsConfigResetReason(&reason);
    if ((0 == CheckPackageNotInstalled(g_rsyslog, &reason, log)) && 
        (0 == CheckPackageNotInstalled(g_systemd, &reason, log)) && 
        CheckDaemonActive(g_syslogNg, &reason, log)) 
    {
        return reason;
    }
    OsConfigResetReason(&reason);
    CheckPackageInstalled(g_systemd, &reason, log);
    CheckDaemonActive(g_systemdJournald, &reason, log);
    return reason;
}

static char* AuditEnsureFilePermissionsForAllRsyslogLogFiles(void* log)
{
    const char* fileCreateMode = "$FileCreateMode";
    char* reason = NULL;
    int modes[] = {600, 640};
    CheckIntegerOptionFromFileEqualWithAny(g_etcRsyslogConf, fileCreateMode, ' ', modes, ARRAY_SIZE(modes), &reason, log);
    if (0 == FileExists(g_etcSyslogNgSyslogNgConf))
    {
        CheckIntegerOptionFromFileEqualWithAny(g_etcSyslogNgSyslogNgConf, fileCreateMode, ' ', modes, ARRAY_SIZE(modes), &reason, log);
    }
    return reason;
}

static char* AuditEnsureLoggerConfigurationFilesAreRestricted(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcRsyslogConf, 0, 0, 640, &reason, log);
    CheckFileAccess(g_etcSyslogNgSyslogNgConf, 0, 0, 640, &reason, log);
    return reason;
}

static char* AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(void* log)
{
    char* reason = NULL;
    CheckTextIsFoundInFile(g_etcRsyslogConf, "FileGroup adm", &reason, log);
    CheckLineFoundNotCommentedOut(g_etcRsyslogConf, '#', "FileGroup adm", &reason, log);
    return reason;
}

static char* AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(void* log)
{
    char* reason = NULL;
    CheckTextIsFoundInFile(g_etcRsyslogConf, "FileOwner syslog", &reason, log);
    CheckLineFoundNotCommentedOut(g_etcRsyslogConf, '#', "FileOwner syslog", &reason, log);
    return reason;
}

static char* AuditEnsureRsyslogNotAcceptingRemoteMessages(void* log)
{
    char* reason = NULL;
    CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "ModLoad imudp", &reason, log);
    CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "ModLoad imtcp", &reason, log);
    return reason;
}

static char* AuditEnsureSyslogRotaterServiceIsEnabled(void* log)
{
    char* reason = NULL;
    CheckPackageInstalled("logrotate", &reason, log);
    CheckFileAccess("/etc/cron.daily/logrotate", 0, 0, 755, &reason, log);
    return reason;
}

static char* AuditEnsureTelnetServiceIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive("telnet.socket", &reason, log);
    CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "telnet", &reason, log);
    return reason;
}

static char* AuditEnsureRcprshServiceIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive("rcp.socket", &reason, log);
    CheckDaemonNotActive("rsh.socket", &reason, log);
    return reason;
}

static char* AuditEnsureTftpServiceisDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive("tftpd-hpa", &reason, log);
    CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "tftp", &reason, log);
    return reason;
}

static char* AuditEnsureAtCronIsRestrictedToAuthorizedUsers(void* log)
{
    const char* etcCronAllow = "/etc/cron.allow";
    const char* etcAtAllow = "/etc/at.allow";
    char* reason = NULL;
    CheckFileNotFound("/etc/cron.deny", &reason, log);
    CheckFileNotFound("/etc/at.deny", &reason, log);
    CheckFileExists(etcCronAllow, &reason, log);
    CheckFileExists(etcAtAllow, &reason, log);
    CheckFileAccess(etcCronAllow, 0, 0, 600, &reason, log);
    CheckFileAccess(etcAtAllow, 0, 0, 600, &reason, log);
    return reason;
}

static char* AuditEnsureSshPortIsConfigured(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshPortIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshBestPracticeProtocol(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshBestPracticeProtocolObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshBestPracticeIgnoreRhosts(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshBestPracticeIgnoreRhostsObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshLogLevelIsSet(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshLogLevelIsSetObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshMaxAuthTriesIsSet(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshMaxAuthTriesIsSetObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureAllowUsersIsConfigured(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureAllowUsersIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureDenyUsersIsConfigured(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureDenyUsersIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureAllowGroupsIsConfigured(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureAllowGroupsIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureDenyGroupsConfigured(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureDenyGroupsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshHostbasedAuthenticationIsDisabled(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshHostbasedAuthenticationIsDisabledObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshPermitRootLoginIsDisabled(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshPermitRootLoginIsDisabledObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshPermitEmptyPasswordsIsDisabled(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshPermitEmptyPasswordsIsDisabledObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshClientIntervalCountMaxIsConfigured(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshClientIntervalCountMaxIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshClientAliveIntervalIsConfigured(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshClientAliveIntervalIsConfiguredObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshLoginGraceTimeIsSet(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshLoginGraceTimeIsSetObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureOnlyApprovedMacAlgorithmsAreUsed(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureSshWarningBannerIsEnabled(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshWarningBannerIsEnabledObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureUsersCannotSetSshEnvironmentOptions(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureUsersCannotSetSshEnvironmentOptionsObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureAppropriateCiphersForSsh(void* log)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureAppropriateCiphersForSshObject, NULL, &reason, log);
    return reason;
}

static char* AuditEnsureAvahiDaemonServiceIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_avahiDaemon, &reason, log);
    return reason;
}

static char* AuditEnsureCupsServiceisDisabled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_cups, &reason, log);
    CheckDaemonNotActive(g_cups, &reason, log);
    return reason;
}

static char* AuditEnsurePostfixPackageIsUninstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_postfix, &reason, log);
    return reason;
}

static char* AuditEnsurePostfixNetworkListeningIsDisabled(void* log)
{
    char* reason = NULL;
    if (0 == CheckFileExists("/etc/postfix/main.cf", &reason, log))
    {
        CheckTextIsFoundInFile("/etc/postfix/main.cf", "inet_interfaces localhost", &reason, log);
    }
    return reason;
}

static char* AuditEnsureRpcgssdServiceIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_rpcgssd, &reason, log);
    CheckDaemonNotActive(g_rpcGssd, &reason, log);
    return reason;
}

static char* AuditEnsureRpcidmapdServiceIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_rpcidmapd, &reason, log);
    CheckDaemonNotActive(g_nfsIdmapd, &reason, log);
    return reason;
}

static char* AuditEnsurePortmapServiceIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_rpcbind, &reason, log);
    CheckDaemonNotActive(g_rpcbindService, &reason, log);
    CheckDaemonNotActive(g_rpcbindSocket, &reason, log);
    return reason;
}

static char* AuditEnsureNetworkFileSystemServiceIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_nfsServer, &reason, log);
    return reason;
}

static char* AuditEnsureRpcsvcgssdServiceIsDisabled(void* log)
{
    char* reason = NULL;
    CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "NEED_SVCGSSD = yes", &reason, log);
    CheckDaemonNotActive("rpc.svcgssd", &reason, log);
    return reason;
}

static char* AuditEnsureSnmpServerIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_snmpd, &reason, log);
    return reason;
}

static char* AuditEnsureRsynServiceIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_rsync, &reason, log);
    return reason;
}

static char* AuditEnsureNisServerIsDisabled(void* log)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_ypserv, &reason, log);
    return reason;
}

static char* AuditEnsureRshClientNotInstalled(void* log)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_rsh, &reason, log);
    CheckPackageNotInstalled(g_rshClient, &reason, log);
    return reason;
}

static char* AuditEnsureSmbWithSambaIsDisabled(void* log)
{
    const char* etcSambaConf = "/etc/samba/smb.conf";
    const char* minProtocol = "min protocol = SMB2";
    char* reason = NULL;
    if (0 != CheckPackageNotInstalled("samba", &reason, log)) 
    {
        CheckLineNotFoundOrCommentedOut(etcSambaConf, '#', minProtocol, &reason, log);
        CheckLineNotFoundOrCommentedOut(etcSambaConf, ';', minProtocol, &reason, log);
    }
    return reason;
}

static char* AuditEnsureUsersDotFilesArentGroupOrWorldWritable(void* log)
{
    unsigned int modes[] = {600, 644, 664, 700, 744};
    char* reason = NULL;
    CheckUsersRestrictedDotFiles(modes, ARRAY_SIZE(modes), &reason, log);
    return reason;
}

static char* AuditEnsureNoUsersHaveDotForwardFiles(void* log)
{
    char* reason = NULL;
    CheckOrEnsureUsersDontHaveDotFiles(g_forward, false, &reason, log);
    return reason;
}

static char* AuditEnsureNoUsersHaveDotNetrcFiles(void* log)
{
    char* reason = NULL;
    CheckOrEnsureUsersDontHaveDotFiles(g_netrc, false, &reason, log);
    return reason;
}

static char* AuditEnsureNoUsersHaveDotRhostsFiles(void* log)
{
    char* reason = NULL;
    CheckOrEnsureUsersDontHaveDotFiles(g_rhosts, false, &reason, log);
    return reason;
}

static char* AuditEnsureRloginServiceIsDisabled(void* log)
{
    const char* rlogin = "rlogin";
    char* reason = NULL;
    CheckDaemonNotActive(rlogin, &reason, log);
    CheckPackageNotInstalled(rlogin, &reason, log);
    CheckPackageNotInstalled(g_inetd, &reason, log);
    CheckPackageNotInstalled(g_inetUtilsInetd, &reason, log);
    CheckTextIsNotFoundInFile(g_etcInetdConf, "login", &reason, log);
    return reason;
}

static char* AuditEnsureUnnecessaryAccountsAreRemoved(void* log)
{
    const char* names[] = {"games"};
    char* reason = NULL;
    CheckUserAccountsNotFound(names, ARRAY_SIZE(names), &reason, log);
    return reason;
}

static int RemediateEnsurePermissionsOnEtcIssue(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcIssue, 0, 0, 644, log);
};

static int RemediateEnsurePermissionsOnEtcIssueNet(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcIssueNet, 0, 0, 644, log);
};

static int RemediateEnsurePermissionsOnEtcHostsAllow(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcHostsAllow, 0, 0, 644, log);
};

static int RemediateEnsurePermissionsOnEtcHostsDeny(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcHostsDeny, 0, 0, 644, log);
};

static int RemediateEnsurePermissionsOnEtcSshSshdConfig(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsurePermissionsOnEtcSshSshdConfigObject, value, NULL, log);
};

static int RemediateEnsurePermissionsOnEtcShadow(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcShadow, 0, 42, 400, log);
};

static int RemediateEnsurePermissionsOnEtcShadowDash(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcShadowDash, 0, 42, 400, log);
};

static int RemediateEnsurePermissionsOnEtcGShadow(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcGShadow, 0, 42, 400, log);
};

static int RemediateEnsurePermissionsOnEtcGShadowDash(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcGShadowDash, 0, 42, 400, log);
};

static int RemediateEnsurePermissionsOnEtcPasswd(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcPasswd, 0, 0, 644, log);
};

static int RemediateEnsurePermissionsOnEtcPasswdDash(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcPasswdDash, 0, 0, 600, log);
};

static int RemediateEnsurePermissionsOnEtcGroup(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcGroup, 0, 0, 644, log);
};

static int RemediateEnsurePermissionsOnEtcGroupDash(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcGroupDash, 0, 0, 644, log);
};

static int RemediateEnsurePermissionsOnEtcAnacronTab(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcAnacronTab, 0, 0, 600, log);
};

static int RemediateEnsurePermissionsOnEtcCronD(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcCronD, 0, 0, 700, log);
};

static int RemediateEnsurePermissionsOnEtcCronDaily(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcCronDaily, 0, 0, 700, log);
};

static int RemediateEnsurePermissionsOnEtcCronHourly(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcCronHourly, 0, 0, 700, log);
};

static int RemediateEnsurePermissionsOnEtcCronMonthly(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcCronMonthly, 0, 0, 700, log);
};

static int RemediateEnsurePermissionsOnEtcCronWeekly(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcCronWeekly, 0, 0, 700, log);
};

static int RemediateEnsurePermissionsOnEtcMotd(char* value, void* log)
{
    UNUSED(value);
    return SetFileAccess(g_etcMotd, 0, 0, 644, log);
};

static int RemediateEnsureInetdNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return ((0 == UninstallPackage(g_inetd, log)) &&
        (0 == UninstallPackage(g_inetUtilsInetd, log))) ? 0 : ENOENT;
}

static int RemediateEnsureXinetdNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_xinetd, log);
}

static int RemediateEnsureRshServerNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_rshServer, log);
}

static int RemediateEnsureNisNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_nis, log);
}

static int RemediateEnsureTftpdNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_tftpd, log);
}

static int RemediateEnsureReadaheadFedoraNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_readAheadFedora, log);
}

static int RemediateEnsureBluetoothHiddNotInstalled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_bluetooth, log);
    return UninstallPackage(g_bluetooth, log);
}

static int RemediateEnsureIsdnUtilsBaseNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_isdnUtilsBase, log);
}

static int RemediateEnsureIsdnUtilsKdumpToolsNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_kdumpTools, log);
}

static int RemediateEnsureIscDhcpdServerNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_iscDhcpServer, log);
}

static int RemediateEnsureSendmailNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_sendmail, log);
}

static int RemediateEnsureSldapdNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_slapd, log);
}

static int RemediateEnsureBind9NotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_bind9, log);
}

static int RemediateEnsureDovecotCoreNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_dovecotCore, log);
}

static int RemediateEnsureAuditdInstalled(char* value, void* log)
{
    UNUSED(value);
    return InstallPackage(g_auditd, log);
}

static int RemediateEnsurePrelinkIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_prelink, log);
}

static int RemediateEnsureTalkClientIsNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_talk, log);
}

static int RemediateEnsureCronServiceIsEnabled(char* value, void* log)
{
    UNUSED(value);
    return (0 == InstallPackage(g_cron, log) &&
        EnableAndStartDaemon(g_cron, log)) ? 0 : ENOENT;
}

static int RemediateEnsureAuditdServiceIsRunning(char* value, void* log)
{
    UNUSED(value);
    return (0 == InstallPackage(g_auditd, log) &&
        EnableAndStartDaemon(g_auditd, log)) ? 0 : ENOENT;
}

static int RemediateEnsureKernelSupportForCpuNx(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnHomePartition(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnTmpPartition(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnVarTmpPartition(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionOnTmpPartition(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionOnVarTmpPartition(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionOnVarTmpPartition(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionOnDevShmPartition(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionEnabledForAllRemovableMedia(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionEnabledForAllRemovableMedia(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionEnabledForAllRemovableMedia(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllTelnetdPackagesUninstalled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllEtcPasswdGroupsExistInEtcGroup(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateUidsExist(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateGidsExist(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateUserNamesExist(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateGroupsExist(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureShadowGroupIsEmpty(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRootGroupExists(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllAccountsHavePasswords(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcPasswd(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcShadow(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcGroup(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultRootAccountGroupIsGidZero(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRootIsOnlyUidZeroAccount(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllUsersHomeDirectoriesExist(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUsersOwnTheirHomeDirectories(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRestrictedUserHomeDirectories(char* value, void* log)
{
    unsigned int modes[] = {700, 750};
    UNUSED(value);
    return SetRestrictedUserHomeDirectories(modes, ARRAY_SIZE(modes), 700, 750, log);
}

static int RemediateEnsurePasswordHashingAlgorithm(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMinDaysBetweenPasswordChanges(char* value, void* log)
{
    UNUSED(value);
    return SetMinDaysBetweenPasswordChanges(g_minDaysBetweenPasswordChanges, log);
}

static int RemediateEnsureInactivePasswordLockPeriod(char* value, void* log)
{
    UNUSED(value);
    return SetLockoutAfterInactivityLessThan(g_maxInactiveDays, log);
}

static int RemediateEnsureMaxDaysBetweenPasswordChanges(char* value, void* log)
{
    UNUSED(value);
    return SetMaxDaysBetweenPasswordChanges(g_maxDaysBetweenPasswordChanges, log);
}

static int RemediateEnsurePasswordExpiration(char* value, void* log)
{
    UNUSED(value);
    return ((0 == SetMinDaysBetweenPasswordChanges(g_minDaysBetweenPasswordChanges, log)) &&
        (0 == SetMaxDaysBetweenPasswordChanges(g_maxDaysBetweenPasswordChanges, log)) &&
        (0 == CheckPasswordExpirationLessThan(g_passwordExpiration, NULL, log))) ? 0 : ENOENT;
}

static int RemediateEnsurePasswordExpirationWarning(char* value, void* log)
{
    UNUSED(value);
    return SetPasswordExpirationWarning(g_passwordExpirationWarning, log);
}

static int RemediateEnsureSystemAccountsAreNonLogin(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAuthenticationRequiredForSingleUserMode(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDotDoesNotAppearInRootsPath(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRemoteLoginWarningBannerIsConfigured(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLocalLoginWarningBannerIsConfigured(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSuRestrictedToRootGroup(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultUmaskForAllUsers(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAutomountingDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureKernelCompiledFromApprovedSources(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultDenyFirewallPolicyIsSet(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePacketRedirectSendingIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIcmpRedirectsIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSourceRoutedPacketsIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAcceptingSourceRoutedPacketsIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIgnoringBogusIcmpBroadcastResponses(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIgnoringIcmpEchoPingsToMulticast(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMartianPacketLoggingIsEnabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureReversePathSourceValidationIsEnabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTcpSynCookiesAreEnabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSystemNotActingAsNetworkSniffer(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllWirelessInterfacesAreDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIpv6ProtocolIsEnabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDccpIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSctpIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledSupportForRds(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTipcIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureZeroconfNetworkingIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePermissionsOnBootloaderConfig(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordReuseIsLimited(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMountingOfUsbStorageDevicesIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureCoreDumpsAreRestricted(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordCreationRequirements(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLockoutForFailedPasswordAttempts(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfCramfsFileSystem(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfFreevxfsFileSystem(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfHfsFileSystem(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfHfsplusFileSystem(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfJffs2FileSystem(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureVirtualMemoryRandomizationIsEnabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllBootloadersHavePasswordProtectionEnabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLoggingIsConfigured(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSyslogPackageIsInstalled(char* value, void* log)
{
    UNUSED(value);
    return ((0 == InstallPackage(g_systemd, log) && 
        ((0 == InstallPackage(g_rsyslog, log)) || (0 == InstallPackage(g_syslog, log)))) ||
        ((0 == InstallPackage(g_syslogNg, log)))) ? 0 : ENOENT;
}

static int RemediateEnsureSystemdJournaldServicePersistsLogMessages(char* value, void* log)
{
    UNUSED(value);
    return ((0 == InstallPackage(g_systemd, log)) &&
        (0 == SetDirectoryAccess("/var/log/journal", 0, -1, 2775, log))) ? 0 : ENOENT;
}

static int RemediateEnsureALoggingServiceIsEnabled(char* value, void* log)
{
    UNUSED(value);
    return ((((0 == InstallPackage(g_systemd, log)) && EnableAndStartDaemon(g_systemdJournald, log)) &&
        (((0 == InstallPackage(g_rsyslog, log)) && EnableAndStartDaemon(g_rsyslog, log)) || 
        (((0 == InstallPackage(g_syslog, log) && EnableAndStartDaemon(g_syslog, log)))))) ||
        (((0 == InstallPackage(g_syslogNg, log)) && EnableAndStartDaemon(g_syslogNg, log)))) ? 0 : ENOENT;
}

static int RemediateEnsureFilePermissionsForAllRsyslogLogFiles(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLoggerConfigurationFilesAreRestricted(char* value, void* log)
{
    UNUSED(value);
    return ((0 == SetFileAccess(g_etcSyslogNgSyslogNgConf, 0, 0, 640, log)) &&
        (0 == SetFileAccess(g_etcRsyslogConf, 0, 0, 640, log))) ? 0 : ENOENT;
}

static int RemediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRsyslogNotAcceptingRemoteMessages(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSyslogRotaterServiceIsEnabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTelnetServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRcprshServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTftpServiceisDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAtCronIsRestrictedToAuthorizedUsers(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshPortIsConfigured(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshPortIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureSshBestPracticeProtocol(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshBestPracticeProtocolObject, value, NULL, log);
}

static int RemediateEnsureSshBestPracticeIgnoreRhosts(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshBestPracticeIgnoreRhostsObject, value, NULL, log);
}

static int RemediateEnsureSshLogLevelIsSet(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshLogLevelIsSetObject, value, NULL, log);
}

static int RemediateEnsureSshMaxAuthTriesIsSet(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshMaxAuthTriesIsSetObject, value, NULL, log);
}

static int RemediateEnsureAllowUsersIsConfigured(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureAllowUsersIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureDenyUsersIsConfigured(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureDenyUsersIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureAllowGroupsIsConfigured(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureAllowGroupsIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureDenyGroupsConfigured(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureDenyGroupsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureSshHostbasedAuthenticationIsDisabled(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject, value, NULL, log);
}

static int RemediateEnsureSshPermitRootLoginIsDisabled(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshPermitRootLoginIsDisabledObject, value, NULL, log);
}

static int RemediateEnsureSshPermitEmptyPasswordsIsDisabled(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject, value, NULL, log);
}

static int RemediateEnsureSshClientIntervalCountMaxIsConfigured(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshClientIntervalCountMaxIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureSshClientAliveIntervalIsConfigured(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshClientAliveIntervalIsConfiguredObject, value, NULL, log);
}

static int RemediateEnsureSshLoginGraceTimeIsSet(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshLoginGraceTimeIsSetObject, value, NULL, log);
}

static int RemediateEnsureOnlyApprovedMacAlgorithmsAreUsed(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject, value, NULL, log);
}

static int RemediateEnsureSshWarningBannerIsEnabled(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshWarningBannerIsEnabledObject, value, NULL, log);
}

static int RemediateEnsureUsersCannotSetSshEnvironmentOptions(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject, value, NULL, log);
}

static int RemediateEnsureAppropriateCiphersForSsh(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsureAppropriateCiphersForSshObject, value, NULL, log);
}

static int RemediateEnsureAvahiDaemonServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_avahiDaemon, log);
    return (0 == strncmp(g_pass, AuditEnsureAvahiDaemonServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureCupsServiceisDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_cups, log);
    return UninstallPackage(g_cups, log);
}

static int RemediateEnsurePostfixPackageIsUninstalled(char* value, void* log)
{
    UNUSED(value);
    return UninstallPackage(g_postfix, log);
}

static int RemediateEnsurePostfixNetworkListeningIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRpcgssdServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcgssd, log);
    StopAndDisableDaemon(g_rpcGssd, log);
    return (0 == strncmp(g_pass, AuditEnsureRpcgssdServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRpcidmapdServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcidmapd, log);
    StopAndDisableDaemon(g_nfsIdmapd, log);
    return (0 == strncmp(g_pass, AuditEnsureRpcidmapdServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsurePortmapServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcbind, log);
    StopAndDisableDaemon(g_rpcbindService, log);
    StopAndDisableDaemon(g_rpcbindSocket, log);
    return (0 == strncmp(g_pass, AuditEnsurePortmapServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureNetworkFileSystemServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_nfsServer, log);
    return (0 == strncmp(g_pass, AuditEnsureNetworkFileSystemServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRpcsvcgssdServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns    
}

static int RemediateEnsureSnmpServerIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_snmpd, log);
    return (0 == strncmp(g_pass, AuditEnsureSnmpServerIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRsynServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rsync, log);
    return (0 == strncmp(g_pass, AuditEnsureRsynServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureNisServerIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_ypserv, log);
    return (0 == strncmp(g_pass, AuditEnsureNisServerIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRshClientNotInstalled(char* value, void* log)
{
    UNUSED(value);
    return ((0 == UninstallPackage(g_rsh, log)) && 
        (0 == UninstallPackage(g_rshClient, log))) ? 0 : ENOENT;
}

static int RemediateEnsureSmbWithSambaIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUsersDotFilesArentGroupOrWorldWritable(char* value, void* log)
{
    unsigned int modes[] = {600, 644, 664, 700, 744};
    UNUSED(value);
    return SetUsersRestrictedDotFiles(modes, ARRAY_SIZE(modes), 744, log);
}

static int RemediateEnsureNoUsersHaveDotForwardFiles(char* value, void* log)
{
    UNUSED(value);
    return CheckOrEnsureUsersDontHaveDotFiles(g_forward, true, NULL, log);
}

static int RemediateEnsureNoUsersHaveDotNetrcFiles(char* value, void* log)
{
    UNUSED(value);
    return CheckOrEnsureUsersDontHaveDotFiles(g_netrc, true, NULL, log);
}

static int RemediateEnsureNoUsersHaveDotRhostsFiles(char* value, void* log)
{
    UNUSED(value);
    return CheckOrEnsureUsersDontHaveDotFiles(g_rhosts, true, NULL, log);
}

static int RemediateEnsureRloginServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUnnecessaryAccountsAreRemoved(char* value, void* log)
{
    const char* names[] = {"games"};
    UNUSED(value);
    return RemoveUserAccounts(names, ARRAY_SIZE(names), log);
}

static int InitEnsurePermissionsOnEtcSshSshdConfig(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsurePermissionsOnEtcSshSshdConfigObject, value, log);
}

static int InitEnsureSshPortIsConfigured(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshPortIsConfiguredObject, value, log);
}

static int InitEnsureSshBestPracticeProtocol(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshBestPracticeProtocolObject, value, log);
}

static int InitEnsureSshBestPracticeIgnoreRhosts(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshBestPracticeIgnoreRhostsObject, value, log);
}

static int InitEnsureSshLogLevelIsSet(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshLogLevelIsSetObject, value, log);
}

static int InitEnsureSshMaxAuthTriesIsSet(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshMaxAuthTriesIsSetObject, value, log);
}

static int InitEnsureAllowUsersIsConfigured(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureAllowUsersIsConfiguredObject, value, log);
}

static int InitEnsureDenyUsersIsConfigured(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureDenyUsersIsConfiguredObject, value, log);
}

static int InitEnsureAllowGroupsIsConfigured(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureAllowGroupsIsConfiguredObject, value, log);
}

static int InitEnsureDenyGroupsConfigured(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureDenyGroupsConfiguredObject, value, log);
}

static int InitEnsureSshHostbasedAuthenticationIsDisabled(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshHostbasedAuthenticationIsDisabledObject, value, log);
}

static int InitEnsureSshPermitRootLoginIsDisabled(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshPermitRootLoginIsDisabledObject, value, log);
}

static int InitEnsureSshPermitEmptyPasswordsIsDisabled(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshPermitEmptyPasswordsIsDisabledObject, value, log);
}

static int InitEnsureSshClientIntervalCountMaxIsConfigured(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshClientIntervalCountMaxIsConfiguredObject, value, log);
}

static int InitEnsureSshClientAliveIntervalIsConfigured(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshClientAliveIntervalIsConfiguredObject, value, log);
}

static int InitEnsureSshLoginGraceTimeIsSet(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshLoginGraceTimeIsSetObject, value, log);
}

static int InitEnsureOnlyApprovedMacAlgorithmsAreUsed(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureOnlyApprovedMacAlgorithmsAreUsedObject, value, log);
}

static int InitEnsureSshWarningBannerIsEnabled(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureSshWarningBannerIsEnabledObject, value, log);
}

static int InitEnsureUsersCannotSetSshEnvironmentOptions(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureUsersCannotSetSshEnvironmentOptionsObject, value, log);
}

static int InitEnsureAppropriateCiphersForSsh(char* value, void* log)
{
    return InitializeSshAuditCheck(g_initEnsureAppropriateCiphersForSshObject, value, log);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
static int InitEnsurePermissionsOnEtcIssue(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcIssueObject, value, log); }
static int InitEnsurePermissionsOnEtcIssueNet(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcIssueNetObject, value, log); }
static int InitEnsurePermissionsOnEtcHostsAllow(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcHostsAllowObject, value, log); }
static int InitEnsurePermissionsOnEtcHostsDeny(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcHostsDenyObject, value, log); }
static int InitEnsurePermissionsOnEtcShadow(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcShadowObject, value, log); }
static int InitEnsurePermissionsOnEtcShadowDash(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcShadowDashObject, value, log); }
static int InitEnsurePermissionsOnEtcGShadow(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcGShadowObject, value, log); }
static int InitEnsurePermissionsOnEtcGShadowDash(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcGShadowDashObject, value, log); }
static int InitEnsurePermissionsOnEtcPasswd(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcPasswdObject, value, log); }
static int InitEnsurePermissionsOnEtcPasswdDash(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcPasswdDashObject, value, log); }
static int InitEnsurePermissionsOnEtcGroup(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcGroupObject, value, log); }
static int InitEnsurePermissionsOnEtcGroupDash(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcGroupDashObject, value, log); }
static int InitEnsurePermissionsOnEtcAnacronTab(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcAnacronTabObject, value, log); }
static int InitEnsurePermissionsOnEtcCronD(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcCronDObject, value, log); }
static int InitEnsurePermissionsOnEtcCronDaily(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcCronDailyObject, value, log); }
static int InitEnsurePermissionsOnEtcCronHourly(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcCronHourlyObject, value, log); }
static int InitEnsurePermissionsOnEtcCronMonthly(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcCronMonthlyObject, value, log); }
static int InitEnsurePermissionsOnEtcCronWeekly(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcCronWeeklyObject, value, log); }
static int InitEnsurePermissionsOnEtcMotd(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnEtcMotdObject, value, log); }
static int InitEnsureInetdNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureInetdNotInstalledObject, value, log); }
static int InitEnsureXinetdNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureXinetdNotInstalledObject, value, log); }
static int InitEnsureRshServerNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRshServerNotInstalledObject, value, log); }
static int InitEnsureNisNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNisNotInstalledObject, value, log); }
static int InitEnsureTftpdNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureTftpdNotInstalledObject, value, log); }
static int InitEnsureReadaheadFedoraNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureReadaheadFedoraNotInstalledObject, value, log); }
static int InitEnsureBluetoothHiddNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureBluetoothHiddNotInstalledObject, value, log); }
static int InitEnsureIsdnUtilsBaseNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureIsdnUtilsBaseNotInstalledObject, value, log); }
static int InitEnsureIsdnUtilsKdumpToolsNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureIsdnUtilsKdumpToolsNotInstalledObject, value, log); }
static int InitEnsureIscDhcpdServerNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureIscDhcpdServerNotInstalledObject, value, log); }
static int InitEnsureSendmailNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSendmailNotInstalledObject, value, log); }
static int InitEnsureSldapdNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSldapdNotInstalledObject, value, log); }
static int InitEnsureBind9NotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureBind9NotInstalledObject, value, log); }
static int InitEnsureDovecotCoreNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDovecotCoreNotInstalledObject, value, log); }
static int InitEnsureAuditdInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAuditdInstalledObject, value, log); }
static int InitEnsurePrelinkIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePrelinkIsDisabledObject, value, log); }
static int InitEnsureTalkClientIsNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureTalkClientIsNotInstalledObject, value, log); }
static int InitEnsureCronServiceIsEnabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureCronServiceIsEnabledObject, value, log); }
static int InitEnsureAuditdServiceIsRunning(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAuditdServiceIsRunningObject, value, log); }
static int InitEnsureKernelSupportForCpuNx(char* value, void* log) { return InitializeAsbCheck(g_initEnsureKernelSupportForCpuNxObject, value, log); }
static int InitEnsureAllTelnetdPackagesUninstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAllTelnetdPackagesUninstalledObject, value, log); }
static int InitEnsureNodevOptionOnHomePartition(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNodevOptionOnHomePartitionObject, value, log); }
static int InitEnsureNodevOptionOnTmpPartition(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNodevOptionOnTmpPartitionObject, value, log); }
static int InitEnsureNodevOptionOnVarTmpPartition(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNodevOptionOnVarTmpPartitionObject, value, log); }
static int InitEnsureNosuidOptionOnTmpPartition(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNosuidOptionOnTmpPartitionObject, value, log); }
static int InitEnsureNosuidOptionOnVarTmpPartition(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNosuidOptionOnVarTmpPartitionObject, value, log); }
static int InitEnsureNoexecOptionOnVarTmpPartition(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoexecOptionOnVarTmpPartitionObject, value, log); }
static int InitEnsureNoexecOptionOnDevShmPartition(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoexecOptionOnDevShmPartitionObject, value, log); }
static int InitEnsureNodevOptionEnabledForAllRemovableMedia(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNodevOptionEnabledForAllRemovableMediaObject, value, log); }
static int InitEnsureNoexecOptionEnabledForAllRemovableMedia(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoexecOptionEnabledForAllRemovableMediaObject, value, log); }
static int InitEnsureNosuidOptionEnabledForAllRemovableMedia(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNosuidOptionEnabledForAllRemovableMediaObject, value, log); }
static int InitEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject, value, log); }
static int InitEnsureAllEtcPasswdGroupsExistInEtcGroup(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAllEtcPasswdGroupsExistInEtcGroupObject, value, log); }
static int InitEnsureNoDuplicateUidsExist(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoDuplicateUidsExistObject, value, log); }
static int InitEnsureNoDuplicateGidsExist(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoDuplicateGidsExistObject, value, log); }
static int InitEnsureNoDuplicateUserNamesExist(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoDuplicateUserNamesExistObject, value, log); }
static int InitEnsureNoDuplicateGroupsExist(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoDuplicateGroupsExistObject, value, log); }
static int InitEnsureShadowGroupIsEmpty(char* value, void* log) { return InitializeAsbCheck(g_initEnsureShadowGroupIsEmptyObject, value, log); }
static int InitEnsureRootGroupExists(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRootGroupExistsObject, value, log); }
static int InitEnsureAllAccountsHavePasswords(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAllAccountsHavePasswordsObject, value, log); }
static int InitEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject, value, log); }
static int InitEnsureNoLegacyPlusEntriesInEtcPasswd(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoLegacyPlusEntriesInEtcPasswdObject, value, log); }
static int InitEnsureNoLegacyPlusEntriesInEtcShadow(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoLegacyPlusEntriesInEtcShadowObject, value, log); }
static int InitEnsureNoLegacyPlusEntriesInEtcGroup(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoLegacyPlusEntriesInEtcGroupObject, value, log); }
static int InitEnsureDefaultRootAccountGroupIsGidZero(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDefaultRootAccountGroupIsGidZeroObject, value, log); }
static int InitEnsureRootIsOnlyUidZeroAccount(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRootIsOnlyUidZeroAccountObject, value, log); }
static int InitEnsureAllUsersHomeDirectoriesExist(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAllUsersHomeDirectoriesExistObject, value, log); }
static int InitEnsureUsersOwnTheirHomeDirectories(char* value, void* log) { return InitializeAsbCheck(g_initEnsureUsersOwnTheirHomeDirectoriesObject, value, log); }
static int InitEnsureRestrictedUserHomeDirectories(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRestrictedUserHomeDirectoriesObject, value, log); }
static int InitEnsurePasswordHashingAlgorithm(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePasswordHashingAlgorithmObject, value, log); }
static int InitEnsureMinDaysBetweenPasswordChanges(char* value, void* log) { return InitializeAsbCheck(g_initEnsureMinDaysBetweenPasswordChangesObject, value, log); }
static int InitEnsureInactivePasswordLockPeriod(char* value, void* log) { return InitializeAsbCheck(g_initEnsureInactivePasswordLockPeriodObject, value, log); }
static int InitMaxDaysBetweenPasswordChanges(char* value, void* log) { return InitializeAsbCheck(g_initEnsureMaxDaysBetweenPasswordChangesObject, value, log); }
static int InitEnsurePasswordExpiration(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePasswordExpirationObject, value, log); }
static int InitEnsurePasswordExpirationWarning(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePasswordExpirationWarningObject, value, log); }
static int InitEnsureSystemAccountsAreNonLogin(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSystemAccountsAreNonLoginObject, value, log); }
static int InitEnsureAuthenticationRequiredForSingleUserMode(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAuthenticationRequiredForSingleUserModeObject, value, log); }
static int InitEnsureDotDoesNotAppearInRootsPath(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDotDoesNotAppearInRootsPathObject, value, log); }
static int InitEnsureRemoteLoginWarningBannerIsConfigured(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRemoteLoginWarningBannerIsConfiguredObject, value, log); }
static int InitEnsureLocalLoginWarningBannerIsConfigured(char* value, void* log) { return InitializeAsbCheck(g_initEnsureLocalLoginWarningBannerIsConfiguredObject, value, log); }
static int InitEnsureSuRestrictedToRootGroup(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSuRestrictedToRootGroupObject, value, log); }
static int InitEnsureDefaultUmaskForAllUsers(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDefaultUmaskForAllUsersObject, value, log); }
static int InitEnsureAutomountingDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAutomountingDisabledObject, value, log); }
static int InitEnsureKernelCompiledFromApprovedSources(char* value, void* log) { return InitializeAsbCheck(g_initEnsureKernelCompiledFromApprovedSourcesObject, value, log); }
static int InitEnsureDefaultDenyFirewallPolicyIsSet(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDefaultDenyFirewallPolicyIsSetObject, value, log); }
static int InitEnsurePacketRedirectSendingIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePacketRedirectSendingIsDisabledObject, value, log); }
static int InitEnsureIcmpRedirectsIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureIcmpRedirectsIsDisabledObject, value, log); }
static int InitEnsureSourceRoutedPacketsIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSourceRoutedPacketsIsDisabledObject, value, log); }
static int InitEnsureAcceptingSourceRoutedPacketsIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAcceptingSourceRoutedPacketsIsDisabledObject, value, log); }
static int InitEnsureIgnoringBogusIcmpBroadcastResponses(char* value, void* log) { return InitializeAsbCheck(g_initEnsureIgnoringBogusIcmpBroadcastResponsesObject, value, log); }
static int InitEnsureIgnoringIcmpEchoPingsToMulticast(char* value, void* log) { return InitializeAsbCheck(g_initEnsureIgnoringIcmpEchoPingsToMulticastObject, value, log); }
static int InitEnsureMartianPacketLoggingIsEnabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureMartianPacketLoggingIsEnabledObject, value, log); }
static int InitEnsureReversePathSourceValidationIsEnabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureReversePathSourceValidationIsEnabledObject, value, log); }
static int InitEnsureTcpSynCookiesAreEnabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureTcpSynCookiesAreEnabledObject, value, log); }
static int InitEnsureSystemNotActingAsNetworkSniffer(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSystemNotActingAsNetworkSnifferObject, value, log); }
static int InitEnsureAllWirelessInterfacesAreDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAllWirelessInterfacesAreDisabledObject, value, log); }
static int InitEnsureIpv6ProtocolIsEnabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureIpv6ProtocolIsEnabledObject, value, log); }
static int InitEnsureDccpIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDccpIsDisabledObject, value, log); }
static int InitEnsureSctpIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSctpIsDisabledObject, value, log); }
static int InitEnsureDisabledSupportForRds(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDisabledSupportForRdsObject, value, log); }
static int InitEnsureTipcIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureTipcIsDisabledObject, value, log); }
static int InitEnsureZeroconfNetworkingIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureZeroconfNetworkingIsDisabledObject, value, log); }
static int InitEnsurePermissionsOnBootloaderConfig(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePermissionsOnBootloaderConfigObject, value, log); }
static int InitEnsurePasswordReuseIsLimited(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePasswordReuseIsLimitedObject, value, log); }
static int InitEnsureMountingOfUsbStorageDevicesIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureMountingOfUsbStorageDevicesIsDisabledObject, value, log); }
static int InitEnsureCoreDumpsAreRestricted(char* value, void* log) { return InitializeAsbCheck(g_initEnsureCoreDumpsAreRestrictedObject, value, log); }
static int InitEnsurePasswordCreationRequirements(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePasswordCreationRequirementsObject, value, log); }
static int InitEnsureLockoutForFailedPasswordAttempts(char* value, void* log) { return InitializeAsbCheck(g_initEnsureLockoutForFailedPasswordAttemptsObject, value, log); }
static int InitEnsureDisabledInstallationOfCramfsFileSystem(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDisabledInstallationOfCramfsFileSystemObject, value, log); }
static int InitEnsureDisabledInstallationOfFreevxfsFileSystem(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDisabledInstallationOfFreevxfsFileSystemObject, value, log); }
static int InitEnsureDisabledInstallationOfHfsFileSystem(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDisabledInstallationOfHfsFileSystemObject, value, log); }
static int InitEnsureDisabledInstallationOfHfsplusFileSystem(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDisabledInstallationOfHfsplusFileSystemObject, value, log); }
static int InitEnsureDisabledInstallationOfJffs2FileSystem(char* value, void* log) { return InitializeAsbCheck(g_initEnsureDisabledInstallationOfJffs2FileSystemObject, value, log); }
static int InitEnsureVirtualMemoryRandomizationIsEnabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureVirtualMemoryRandomizationIsEnabledObject, value, log); }
static int InitEnsureAllBootloadersHavePasswordProtectionEnabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAllBootloadersHavePasswordProtectionEnabledObject, value, log); }
static int InitEnsureLoggingIsConfigured(char* value, void* log) { return InitializeAsbCheck(g_initEnsureLoggingIsConfiguredObject, value, log); }
static int InitEnsureSyslogPackageIsInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSyslogPackageIsInstalledObject, value, log); }
static int InitEnsureSystemdJournaldServicePersistsLogMessages(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSystemdJournaldServicePersistsLogMessagesObject, value, log); }
static int InitEnsureALoggingServiceIsEnabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureALoggingServiceIsEnabledObject, value, log); }
static int InitEnsureFilePermissionsForAllRsyslogLogFiles(char* value, void* log) { return InitializeAsbCheck(g_initEnsureFilePermissionsForAllRsyslogLogFilesObject, value, log); }
static int InitEnsureLoggerConfigurationFilesAreRestricted(char* value, void* log) { return InitializeAsbCheck(g_initEnsureLoggerConfigurationFilesAreRestrictedObject, value, log); }
static int InitEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject, value, log); }
static int InitEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject, value, log); }
static int InitEnsureRsyslogNotAcceptingRemoteMessages(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRsyslogNotAcceptingRemoteMessagesObject, value, log); }
static int InitEnsureSyslogRotaterServiceIsEnabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSyslogRotaterServiceIsEnabledObject, value, log); }
static int InitEnsureTelnetServiceIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureTelnetServiceIsDisabledObject, value, log); }
static int InitEnsureRcprshServiceIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRcprshServiceIsDisabledObject, value, log); }
static int InitEnsureTftpServiceisDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureTftpServiceisDisabledObject, value, log); }
static int InitEnsureAtCronIsRestrictedToAuthorizedUsers(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAtCronIsRestrictedToAuthorizedUsersObject, value, log); }
static int InitEnsureAvahiDaemonServiceIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureAvahiDaemonServiceIsDisabledObject, value, log); }
static int InitEnsureCupsServiceisDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureCupsServiceisDisabledObject, value, log); }
static int InitEnsurePostfixPackageIsUninstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePostfixPackageIsUninstalledObject, value, log); }
static int InitEnsurePostfixNetworkListeningIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePostfixNetworkListeningIsDisabledObject, value, log); }
static int InitEnsureRpcgssdServiceIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRpcgssdServiceIsDisabledObject, value, log); }
static int InitEnsureRpcidmapdServiceIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRpcidmapdServiceIsDisabledObject, value, log); }
static int InitEnsurePortmapServiceIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsurePortmapServiceIsDisabledObject, value, log); }
static int InitEnsureNetworkFileSystemServiceIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNetworkFileSystemServiceIsDisabledObject, value, log); }
static int InitEnsureRpcsvcgssdServiceIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRpcsvcgssdServiceIsDisabledObject, value, log); }
static int InitEnsureSnmpServerIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSnmpServerIsDisabledObject, value, log); }
static int InitEnsureRsynServiceIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRsynServiceIsDisabledObject, value, log); }
static int InitEnsureNisServerIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNisServerIsDisabledObject, value, log); }
static int InitEnsureRshClientNotInstalled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRshClientNotInstalledObject, value, log); }
static int InitEnsureSmbWithSambaIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureSmbWithSambaIsDisabledObject, value, log); }
static int InitEnsureUsersDotFilesArentGroupOrWorldWritable(char* value, void* log) { return InitializeAsbCheck(g_initEnsureUsersDotFilesArentGroupOrWorldWritableObject, value, log); }
static int InitEnsureNoUsersHaveDotForwardFiles(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoUsersHaveDotForwardFilesObject, value, log); }
static int InitEnsureNoUsersHaveDotNetrcFiles(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoUsersHaveDotNetrcFilesObject, value, log); }
static int InitEnsureNoUsersHaveDotRhostsFiles(char* value, void* log) { return InitializeAsbCheck(g_initEnsureNoUsersHaveDotRhostsFilesObject, value, log); }
static int InitEnsureRloginServiceIsDisabled(char* value, void* log) { return InitializeAsbCheck(g_initEnsureRloginServiceIsDisabledObject, value, log); }
static int InitEnsureUnnecessaryAccountsAreRemoved(char* value, void* log) { return InitializeAsbCheck(g_initEnsureUnnecessaryAccountsAreRemovedObject, value, log); }
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int AsbMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, void* log)
{
    JSON_Value* jsonValue = NULL;
    char* serializedValue = NULL;
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

    if (0 != strcmp(componentName, g_securityBaselineComponentName))
    {
        OsConfigLogError(log, "AsbMmiGet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }
    else
    {
        if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcIssueObject))
        {
            result = AuditEnsurePermissionsOnEtcIssue();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcIssueNetObject))
        {
            result = AuditEnsurePermissionsOnEtcIssueNet();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcHostsAllowObject))
        {
            result = AuditEnsurePermissionsOnEtcHostsAllow();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcHostsDenyObject))
        {
            result = AuditEnsurePermissionsOnEtcHostsDeny();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcSshSshdConfigObject))
        {
            result = AuditEnsurePermissionsOnEtcSshSshdConfig();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcShadowObject))
        {
            result = AuditEnsurePermissionsOnEtcShadow();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcShadowDashObject))
        {
            result = AuditEnsurePermissionsOnEtcShadowDash();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGShadowObject))
        {
            result = AuditEnsurePermissionsOnEtcGShadow();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGShadowDashObject))
        {
            result = AuditEnsurePermissionsOnEtcGShadowDash();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcPasswdObject))
        {
            result = AuditEnsurePermissionsOnEtcPasswd();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcPasswdDashObject))
        {
            result = AuditEnsurePermissionsOnEtcPasswdDash();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGroupObject))
        {
            result = AuditEnsurePermissionsOnEtcGroup();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGroupDashObject))
        {
            result = AuditEnsurePermissionsOnEtcGroupDash();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcAnacronTabObject))
        {
            result = AuditEnsurePermissionsOnEtcAnacronTab();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronDObject))
        {
            result = AuditEnsurePermissionsOnEtcCronD();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronDailyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronDaily();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronHourlyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronHourly();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronMonthlyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronMonthly();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronWeeklyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronWeekly();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcMotdObject))
        {
            result = AuditEnsurePermissionsOnEtcMotd();
        }
        else if (0 == strcmp(objectName, g_auditEnsureKernelSupportForCpuNxObject))
        {
            result = AuditEnsureKernelSupportForCpuNx();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionOnHomePartitionObject))
        {
            result = AuditEnsureNodevOptionOnHomePartition();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionOnTmpPartitionObject))
        {
            result = AuditEnsureNodevOptionOnTmpPartition();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionOnVarTmpPartitionObject))
        {
            result = AuditEnsureNodevOptionOnVarTmpPartition();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNosuidOptionOnTmpPartitionObject))
        {
            result = AuditEnsureNosuidOptionOnTmpPartition();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNosuidOptionOnVarTmpPartitionObject))
        {
            result = AuditEnsureNosuidOptionOnVarTmpPartition();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecOptionOnVarTmpPartitionObject))
        {
            result = AuditEnsureNoexecOptionOnVarTmpPartition();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecOptionOnDevShmPartitionObject))
        {
            result = AuditEnsureNoexecOptionOnDevShmPartition();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionEnabledForAllRemovableMediaObject))
        {
            result = AuditEnsureNodevOptionEnabledForAllRemovableMedia();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecOptionEnabledForAllRemovableMediaObject))
        {
            result = AuditEnsureNoexecOptionEnabledForAllRemovableMedia();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNosuidOptionEnabledForAllRemovableMediaObject))
        {
            result = AuditEnsureNosuidOptionEnabledForAllRemovableMedia();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject))
        {
            result = AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts();
        }
        else if (0 == strcmp(objectName, g_auditEnsureInetdNotInstalledObject))
        {
            result = AuditEnsureInetdNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureXinetdNotInstalledObject))
        {
            result = AuditEnsureXinetdNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllTelnetdPackagesUninstalledObject))
        {
            result = AuditEnsureAllTelnetdPackagesUninstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRshServerNotInstalledObject))
        {
            result = AuditEnsureRshServerNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNisNotInstalledObject))
        {
            result = AuditEnsureNisNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureTftpdNotInstalledObject))
        {
            result = AuditEnsureTftpdNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureReadaheadFedoraNotInstalledObject))
        {
            result = AuditEnsureReadaheadFedoraNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureBluetoothHiddNotInstalledObject))
        {
            result = AuditEnsureBluetoothHiddNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureIsdnUtilsBaseNotInstalledObject))
        {
            result = AuditEnsureIsdnUtilsBaseNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureIsdnUtilsKdumpToolsNotInstalledObject))
        {
            result = AuditEnsureIsdnUtilsKdumpToolsNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureIscDhcpdServerNotInstalledObject))
        {
            result = AuditEnsureIscDhcpdServerNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSendmailNotInstalledObject))
        {
            result = AuditEnsureSendmailNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSldapdNotInstalledObject))
        {
            result = AuditEnsureSldapdNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureBind9NotInstalledObject))
        {
            result = AuditEnsureBind9NotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDovecotCoreNotInstalledObject))
        {
            result = AuditEnsureDovecotCoreNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAuditdInstalledObject))
        {
            result = AuditEnsureAuditdInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllEtcPasswdGroupsExistInEtcGroupObject))
        {
            result = AuditEnsureAllEtcPasswdGroupsExistInEtcGroup();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateUidsExistObject))
        {
            result = AuditEnsureNoDuplicateUidsExist();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateGidsExistObject))
        {
            result = AuditEnsureNoDuplicateGidsExist();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateUserNamesExistObject))
        {
            result = AuditEnsureNoDuplicateUserNamesExist();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateGroupsExistObject))
        {
            result = AuditEnsureNoDuplicateGroupsExist();
        }
        else if (0 == strcmp(objectName, g_auditEnsureShadowGroupIsEmptyObject))
        {
            result = AuditEnsureShadowGroupIsEmpty();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRootGroupExistsObject))
        {
            result = AuditEnsureRootGroupExists();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllAccountsHavePasswordsObject))
        {
            result = AuditEnsureAllAccountsHavePasswords();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject))
        {
            result = AuditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoLegacyPlusEntriesInEtcPasswdObject))
        {
            result = AuditEnsureNoLegacyPlusEntriesInEtcPasswd();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoLegacyPlusEntriesInEtcShadowObject))
        {
            result = AuditEnsureNoLegacyPlusEntriesInEtcShadow();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoLegacyPlusEntriesInEtcGroupObject))
        {
            result = AuditEnsureNoLegacyPlusEntriesInEtcGroup();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDefaultRootAccountGroupIsGidZeroObject))
        {
            result = AuditEnsureDefaultRootAccountGroupIsGidZero();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRootIsOnlyUidZeroAccountObject))
        {
            result = AuditEnsureRootIsOnlyUidZeroAccount();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllUsersHomeDirectoriesExistObject))
        {
            result = AuditEnsureAllUsersHomeDirectoriesExist();
        }
        else if (0 == strcmp(objectName, g_auditEnsureUsersOwnTheirHomeDirectoriesObject))
        {
            result = AuditEnsureUsersOwnTheirHomeDirectories();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRestrictedUserHomeDirectoriesObject))
        {
            result = AuditEnsureRestrictedUserHomeDirectories();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordHashingAlgorithmObject))
        {
            result = AuditEnsurePasswordHashingAlgorithm();
        }
        else if (0 == strcmp(objectName, g_auditEnsureMinDaysBetweenPasswordChangesObject))
        {
            result = AuditEnsureMinDaysBetweenPasswordChanges();
        }
        else if (0 == strcmp(objectName, g_auditEnsureInactivePasswordLockPeriodObject))
        {
            result = AuditEnsureInactivePasswordLockPeriod();
        }
        else if (0 == strcmp(objectName, g_auditMaxDaysBetweenPasswordChangesObject))
        {
            result = AuditEnsureMaxDaysBetweenPasswordChanges();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordExpirationObject))
        {
            result = AuditEnsurePasswordExpiration();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordExpirationWarningObject))
        {
            result = AuditEnsurePasswordExpirationWarning();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSystemAccountsAreNonLoginObject))
        {
            result = AuditEnsureSystemAccountsAreNonLogin();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAuthenticationRequiredForSingleUserModeObject))
        {
            result = AuditEnsureAuthenticationRequiredForSingleUserMode();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePrelinkIsDisabledObject))
        {
            result = AuditEnsurePrelinkIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureTalkClientIsNotInstalledObject))
        {
            result = AuditEnsureTalkClientIsNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDotDoesNotAppearInRootsPathObject))
        {
            result = AuditEnsureDotDoesNotAppearInRootsPath();
        }
        else if (0 == strcmp(objectName, g_auditEnsureCronServiceIsEnabledObject))
        {
            result = AuditEnsureCronServiceIsEnabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRemoteLoginWarningBannerIsConfiguredObject))
        {
            result = AuditEnsureRemoteLoginWarningBannerIsConfigured();
        }
        else if (0 == strcmp(objectName, g_auditEnsureLocalLoginWarningBannerIsConfiguredObject))
        {
            result = AuditEnsureLocalLoginWarningBannerIsConfigured();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAuditdServiceIsRunningObject))
        {
            result = AuditEnsureAuditdServiceIsRunning();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSuRestrictedToRootGroupObject))
        {
            result = AuditEnsureSuRestrictedToRootGroup();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDefaultUmaskForAllUsersObject))
        {
            result = AuditEnsureDefaultUmaskForAllUsers();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAutomountingDisabledObject))
        {
            result = AuditEnsureAutomountingDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureKernelCompiledFromApprovedSourcesObject))
        {
            result = AuditEnsureKernelCompiledFromApprovedSources();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDefaultDenyFirewallPolicyIsSetObject))
        {
            result = AuditEnsureDefaultDenyFirewallPolicyIsSet();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePacketRedirectSendingIsDisabledObject))
        {
            result = AuditEnsurePacketRedirectSendingIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureIcmpRedirectsIsDisabledObject))
        {
            result = AuditEnsureIcmpRedirectsIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSourceRoutedPacketsIsDisabledObject))
        {
            result = AuditEnsureSourceRoutedPacketsIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAcceptingSourceRoutedPacketsIsDisabledObject))
        {
            result = AuditEnsureAcceptingSourceRoutedPacketsIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureIgnoringBogusIcmpBroadcastResponsesObject))
        {
            result = AuditEnsureIgnoringBogusIcmpBroadcastResponses();
        }
        else if (0 == strcmp(objectName, g_auditEnsureIgnoringIcmpEchoPingsToMulticastObject))
        {
            result = AuditEnsureIgnoringIcmpEchoPingsToMulticast();
        }
        else if (0 == strcmp(objectName, g_auditEnsureMartianPacketLoggingIsEnabledObject))
        {
            result = AuditEnsureMartianPacketLoggingIsEnabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureReversePathSourceValidationIsEnabledObject))
        {
            result = AuditEnsureReversePathSourceValidationIsEnabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureTcpSynCookiesAreEnabledObject))
        {
            result = AuditEnsureTcpSynCookiesAreEnabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSystemNotActingAsNetworkSnifferObject))
        {
            result = AuditEnsureSystemNotActingAsNetworkSniffer();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllWirelessInterfacesAreDisabledObject))
        {
            result = AuditEnsureAllWirelessInterfacesAreDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureIpv6ProtocolIsEnabledObject))
        {
            result = AuditEnsureIpv6ProtocolIsEnabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDccpIsDisabledObject))
        {
            result = AuditEnsureDccpIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSctpIsDisabledObject))
        {
            result = AuditEnsureSctpIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledSupportForRdsObject))
        {
            result = AuditEnsureDisabledSupportForRds();
        }
        else if (0 == strcmp(objectName, g_auditEnsureTipcIsDisabledObject))
        {
            result = AuditEnsureTipcIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureZeroconfNetworkingIsDisabledObject))
        {
            result = AuditEnsureZeroconfNetworkingIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnBootloaderConfigObject))
        {
            result = AuditEnsurePermissionsOnBootloaderConfig();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordReuseIsLimitedObject))
        {
            result = AuditEnsurePasswordReuseIsLimited();
        }
        else if (0 == strcmp(objectName, g_auditEnsureMountingOfUsbStorageDevicesIsDisabledObject))
        {
            result = AuditEnsureMountingOfUsbStorageDevicesIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureCoreDumpsAreRestrictedObject))
        {
            result = AuditEnsureCoreDumpsAreRestricted();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordCreationRequirementsObject))
        {
            result = AuditEnsurePasswordCreationRequirements();
        }
        else if (0 == strcmp(objectName, g_auditEnsureLockoutForFailedPasswordAttemptsObject))
        {
            result = AuditEnsureLockoutForFailedPasswordAttempts();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfCramfsFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfCramfsFileSystem();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfFreevxfsFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfFreevxfsFileSystem();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfHfsFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfHfsFileSystem();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfHfsplusFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfHfsplusFileSystem();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfJffs2FileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfJffs2FileSystem();
        }
        else if (0 == strcmp(objectName, g_auditEnsureVirtualMemoryRandomizationIsEnabledObject))
        {
            result = AuditEnsureVirtualMemoryRandomizationIsEnabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllBootloadersHavePasswordProtectionEnabledObject))
        {
            result = AuditEnsureAllBootloadersHavePasswordProtectionEnabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureLoggingIsConfiguredObject))
        {
            result = AuditEnsureLoggingIsConfigured();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSyslogPackageIsInstalledObject))
        {
            result = AuditEnsureSyslogPackageIsInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSystemdJournaldServicePersistsLogMessagesObject))
        {
            result = AuditEnsureSystemdJournaldServicePersistsLogMessages();
        }
        else if (0 == strcmp(objectName, g_auditEnsureALoggingServiceIsEnabledObject))
        {
            result = AuditEnsureALoggingServiceIsEnabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureFilePermissionsForAllRsyslogLogFilesObject))
        {
            result = AuditEnsureFilePermissionsForAllRsyslogLogFiles();
        }
        else if (0 == strcmp(objectName, g_auditEnsureLoggerConfigurationFilesAreRestrictedObject))
        {
            result = AuditEnsureLoggerConfigurationFilesAreRestricted();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject))
        {
            result = AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject))
        {
            result = AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRsyslogNotAcceptingRemoteMessagesObject))
        {
            result = AuditEnsureRsyslogNotAcceptingRemoteMessages();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSyslogRotaterServiceIsEnabledObject))
        {
            result = AuditEnsureSyslogRotaterServiceIsEnabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureTelnetServiceIsDisabledObject))
        {
            result = AuditEnsureTelnetServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRcprshServiceIsDisabledObject))
        {
            result = AuditEnsureRcprshServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureTftpServiceisDisabledObject))
        {
            result = AuditEnsureTftpServiceisDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAtCronIsRestrictedToAuthorizedUsersObject))
        {
            result = AuditEnsureAtCronIsRestrictedToAuthorizedUsers();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshPortIsConfiguredObject))
        {
            result = AuditEnsureSshPortIsConfigured();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshBestPracticeProtocolObject))
        {
            result = AuditEnsureSshBestPracticeProtocol();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshBestPracticeIgnoreRhostsObject))
        {
            result = AuditEnsureSshBestPracticeIgnoreRhosts();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshLogLevelIsSetObject))
        {
            result = AuditEnsureSshLogLevelIsSet();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshMaxAuthTriesIsSetObject))
        {
            result = AuditEnsureSshMaxAuthTriesIsSet();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllowUsersIsConfiguredObject))
        {
            result = AuditEnsureAllowUsersIsConfigured();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDenyUsersIsConfiguredObject))
        {
            result = AuditEnsureDenyUsersIsConfigured();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllowGroupsIsConfiguredObject))
        {
            result = AuditEnsureAllowGroupsIsConfigured();
        }
        else if (0 == strcmp(objectName, g_auditEnsureDenyGroupsConfiguredObject))
        {
            result = AuditEnsureDenyGroupsConfigured();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshHostbasedAuthenticationIsDisabledObject))
        {
            result = AuditEnsureSshHostbasedAuthenticationIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshPermitRootLoginIsDisabledObject))
        {
            result = AuditEnsureSshPermitRootLoginIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshPermitEmptyPasswordsIsDisabledObject))
        {
            result = AuditEnsureSshPermitEmptyPasswordsIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshClientIntervalCountMaxIsConfiguredObject))
        {
            result = AuditEnsureSshClientIntervalCountMaxIsConfigured();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshClientAliveIntervalIsConfiguredObject))
        {
            result = AuditEnsureSshClientAliveIntervalIsConfigured();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshLoginGraceTimeIsSetObject))
        {
            result = AuditEnsureSshLoginGraceTimeIsSet();
        }
        else if (0 == strcmp(objectName, g_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject))
        {
            result = AuditEnsureOnlyApprovedMacAlgorithmsAreUsed();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshWarningBannerIsEnabledObject))
        {
            result = AuditEnsureSshWarningBannerIsEnabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureUsersCannotSetSshEnvironmentOptionsObject))
        {
            result = AuditEnsureUsersCannotSetSshEnvironmentOptions();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAppropriateCiphersForSshObject))
        {
            result = AuditEnsureAppropriateCiphersForSsh();
        }
        else if (0 == strcmp(objectName, g_auditEnsureAvahiDaemonServiceIsDisabledObject))
        {
            result = AuditEnsureAvahiDaemonServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureCupsServiceisDisabledObject))
        {
            result = AuditEnsureCupsServiceisDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePostfixPackageIsUninstalledObject))
        {
            result = AuditEnsurePostfixPackageIsUninstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePostfixNetworkListeningIsDisabledObject))
        {
            result = AuditEnsurePostfixNetworkListeningIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRpcgssdServiceIsDisabledObject))
        {
            result = AuditEnsureRpcgssdServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRpcidmapdServiceIsDisabledObject))
        {
            result = AuditEnsureRpcidmapdServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePortmapServiceIsDisabledObject))
        {
            result = AuditEnsurePortmapServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNetworkFileSystemServiceIsDisabledObject))
        {
            result = AuditEnsureNetworkFileSystemServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRpcsvcgssdServiceIsDisabledObject))
        {
            result = AuditEnsureRpcsvcgssdServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSnmpServerIsDisabledObject))
        {
            result = AuditEnsureSnmpServerIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRsynServiceIsDisabledObject))
        {
            result = AuditEnsureRsynServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNisServerIsDisabledObject))
        {
            result = AuditEnsureNisServerIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRshClientNotInstalledObject))
        {
            result = AuditEnsureRshClientNotInstalled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSmbWithSambaIsDisabledObject))
        {
            result = AuditEnsureSmbWithSambaIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureUsersDotFilesArentGroupOrWorldWritableObject))
        {
            result = AuditEnsureUsersDotFilesArentGroupOrWorldWritable();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoUsersHaveDotForwardFilesObject))
        {
            result = AuditEnsureNoUsersHaveDotForwardFiles();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoUsersHaveDotNetrcFilesObject))
        {
            result = AuditEnsureNoUsersHaveDotNetrcFiles();
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoUsersHaveDotRhostsFilesObject))
        {
            result = AuditEnsureNoUsersHaveDotRhostsFiles();
        }
        else if (0 == strcmp(objectName, g_auditEnsureRloginServiceIsDisabledObject))
        {
            result = AuditEnsureRloginServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_auditEnsureUnnecessaryAccountsAreRemovedObject))
        {
            result = AuditEnsureUnnecessaryAccountsAreRemoved();
        }
        else
        {
            OsConfigLogError(log, "AsbMmiGet called for an unsupported object (%s)", objectName);
            status = EINVAL;
        }
    }

    if (0 == status)
    {
        if (NULL == result)
        {
            OsConfigLogError(log, "AsbMmiGet(%s, %s): audit failure without a reason", componentName, objectName);
            result = DuplicateString(g_fail);

            if (NULL == result)
            {
                OsConfigLogError(log, "AsbMmiGet: DuplicateString failed");
                status = ENOMEM;
            }
        }
        
        if (NULL != result)
        {
            if (NULL == (jsonValue = json_value_init_string(result)))
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
                if ((maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > maxPayloadSizeBytes))
                {
                    OsConfigLogError(log, "MmiGet(%s, %s) insufficient max size (%d bytes) vs actual size (%d bytes), report will be truncated",
                        componentName, objectName, maxPayloadSizeBytes, *payloadSizeBytes);

                    *payloadSizeBytes = maxPayloadSizeBytes;
                }
                
                *payloadSizeBytes = (int)strlen(serializedValue);

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

    return status;
}

int AsbMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, void* log)
{
    JSON_Value* jsonValue = NULL;
    char* jsonString = NULL;
    char* payloadString = NULL;
    int status = 0;

    // No payload is accepted for now, this may change once the complete Security Baseline is implemented
    if ((NULL == componentName) || (NULL == objectName))
    {
        OsConfigLogError(log, "AsbMmiSet(%s, %s, %s, %d) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        return EINVAL;
    }

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
        if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcIssueObject))
        {
            status = RemediateEnsurePermissionsOnEtcIssue(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcIssueNetObject))
        {
            status = RemediateEnsurePermissionsOnEtcIssueNet(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcHostsAllowObject))
        {
            status = RemediateEnsurePermissionsOnEtcHostsAllow(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcHostsDenyObject))
        {
            status = RemediateEnsurePermissionsOnEtcHostsDeny(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcSshSshdConfigObject))
        {
            status = RemediateEnsurePermissionsOnEtcSshSshdConfig(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcShadowObject))
        {
            status = RemediateEnsurePermissionsOnEtcShadow(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcShadowDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcShadowDash(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGShadowObject))
        {
            status = RemediateEnsurePermissionsOnEtcGShadow(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGShadowDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcGShadowDash(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcPasswdObject))
        {
            status = RemediateEnsurePermissionsOnEtcPasswd(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcPasswdDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcPasswdDash(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGroupObject))
        {
            status = RemediateEnsurePermissionsOnEtcGroup(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGroupDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcGroupDash(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcAnacronTabObject))
        {
            status = RemediateEnsurePermissionsOnEtcAnacronTab(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronDObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronD(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronDailyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronDaily(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronHourlyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronHourly(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronMonthlyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronMonthly(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronWeeklyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronWeekly(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcMotdObject))
        {
            status = RemediateEnsurePermissionsOnEtcMotd(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureInetdNotInstalledObject))
        {
            status = RemediateEnsureInetdNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureXinetdNotInstalledObject))
        {
            status = RemediateEnsureXinetdNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRshServerNotInstalledObject))
        {
            status = RemediateEnsureRshServerNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNisNotInstalledObject))
        {
            status = RemediateEnsureNisNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTftpdNotInstalledObject))
        {
            status = RemediateEnsureTftpdNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureReadaheadFedoraNotInstalledObject))
        {
            status = RemediateEnsureReadaheadFedoraNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureBluetoothHiddNotInstalledObject))
        {
            status = RemediateEnsureBluetoothHiddNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIsdnUtilsBaseNotInstalledObject))
        {
            status = RemediateEnsureIsdnUtilsBaseNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIsdnUtilsKdumpToolsNotInstalledObject))
        {
            status = RemediateEnsureIsdnUtilsKdumpToolsNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIscDhcpdServerNotInstalledObject))
        {
            status = RemediateEnsureIscDhcpdServerNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSendmailNotInstalledObject))
        {
            status = RemediateEnsureSendmailNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSldapdNotInstalledObject))
        {
            status = RemediateEnsureSldapdNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureBind9NotInstalledObject))
        {
            status = RemediateEnsureBind9NotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDovecotCoreNotInstalledObject))
        {
            status = RemediateEnsureDovecotCoreNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuditdInstalledObject))
        {
            status = RemediateEnsureAuditdInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePrelinkIsDisabledObject))
        {
            status = RemediateEnsurePrelinkIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTalkClientIsNotInstalledObject))
        {
            status = RemediateEnsureTalkClientIsNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureCronServiceIsEnabledObject))
        {
            status = RemediateEnsureCronServiceIsEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuditdServiceIsRunningObject))
        {
            status = RemediateEnsureAuditdServiceIsRunning(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureKernelSupportForCpuNxObject))
        {
            status = RemediateEnsureKernelSupportForCpuNx(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionOnHomePartitionObject))
        {
            status = RemediateEnsureNodevOptionOnHomePartition(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionOnTmpPartitionObject))
        {
            status = RemediateEnsureNodevOptionOnTmpPartition(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionOnVarTmpPartitionObject))
        {
            status = RemediateEnsureNodevOptionOnVarTmpPartition(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNosuidOptionOnTmpPartitionObject))
        {
            status = RemediateEnsureNosuidOptionOnTmpPartition(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNosuidOptionOnVarTmpPartitionObject))
        {
            status = RemediateEnsureNosuidOptionOnVarTmpPartition(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecOptionOnVarTmpPartitionObject))
        {
            status = RemediateEnsureNoexecOptionOnVarTmpPartition(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecOptionOnDevShmPartitionObject))
        {
            status = RemediateEnsureNoexecOptionOnDevShmPartition(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionEnabledForAllRemovableMediaObject))
        {
            status = RemediateEnsureNodevOptionEnabledForAllRemovableMedia(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecOptionEnabledForAllRemovableMediaObject))
        {
            status = RemediateEnsureNoexecOptionEnabledForAllRemovableMedia(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNosuidOptionEnabledForAllRemovableMediaObject))
        {
            status = RemediateEnsureNosuidOptionEnabledForAllRemovableMedia(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject))
        {
            status = RemediateEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllTelnetdPackagesUninstalledObject))
        {
            status = RemediateEnsureAllTelnetdPackagesUninstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllEtcPasswdGroupsExistInEtcGroupObject))
        {
            status = RemediateEnsureAllEtcPasswdGroupsExistInEtcGroup(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateUidsExistObject))
        {
            status = RemediateEnsureNoDuplicateUidsExist(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateGidsExistObject))
        {
            status = RemediateEnsureNoDuplicateGidsExist(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateUserNamesExistObject))
        {
            status = RemediateEnsureNoDuplicateUserNamesExist(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateGroupsExistObject))
        {
            status = RemediateEnsureNoDuplicateGroupsExist(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureShadowGroupIsEmptyObject))
        {
            status = RemediateEnsureShadowGroupIsEmpty(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRootGroupExistsObject))
        {
            status = RemediateEnsureRootGroupExists(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllAccountsHavePasswordsObject))
        {
            status = RemediateEnsureAllAccountsHavePasswords(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject))
        {
            status = RemediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoLegacyPlusEntriesInEtcPasswdObject))
        {
            status = RemediateEnsureNoLegacyPlusEntriesInEtcPasswd(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoLegacyPlusEntriesInEtcShadowObject))
        {
            status = RemediateEnsureNoLegacyPlusEntriesInEtcShadow(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoLegacyPlusEntriesInEtcGroupObject))
        {
            status = RemediateEnsureNoLegacyPlusEntriesInEtcGroup(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDefaultRootAccountGroupIsGidZeroObject))
        {
            status = RemediateEnsureDefaultRootAccountGroupIsGidZero(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRootIsOnlyUidZeroAccountObject))
        {
            status = RemediateEnsureRootIsOnlyUidZeroAccount(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllUsersHomeDirectoriesExistObject))
        {
            status = RemediateEnsureAllUsersHomeDirectoriesExist(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUsersOwnTheirHomeDirectoriesObject))
        {
            status = RemediateEnsureUsersOwnTheirHomeDirectories(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRestrictedUserHomeDirectoriesObject))
        {
            status = RemediateEnsureRestrictedUserHomeDirectories(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordHashingAlgorithmObject))
        {
            status = RemediateEnsurePasswordHashingAlgorithm(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureMinDaysBetweenPasswordChangesObject))
        {
            status = RemediateEnsureMinDaysBetweenPasswordChanges(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureInactivePasswordLockPeriodObject))
        {
            status = RemediateEnsureInactivePasswordLockPeriod(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateMaxDaysBetweenPasswordChangesObject))
        {
            status = RemediateEnsureMaxDaysBetweenPasswordChanges(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordExpirationObject))
        {
            status = RemediateEnsurePasswordExpiration(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordExpirationWarningObject))
        {
            status = RemediateEnsurePasswordExpirationWarning(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSystemAccountsAreNonLoginObject))
        {
            status = RemediateEnsureSystemAccountsAreNonLogin(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuthenticationRequiredForSingleUserModeObject))
        {
            status = RemediateEnsureAuthenticationRequiredForSingleUserMode(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDotDoesNotAppearInRootsPathObject))
        {
            status = RemediateEnsureDotDoesNotAppearInRootsPath(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRemoteLoginWarningBannerIsConfiguredObject))
        {
            status = RemediateEnsureRemoteLoginWarningBannerIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLocalLoginWarningBannerIsConfiguredObject))
        {
            status = RemediateEnsureLocalLoginWarningBannerIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuditdServiceIsRunningObject))
        {
            status = RemediateEnsureAuditdServiceIsRunning(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSuRestrictedToRootGroupObject))
        {
            status = RemediateEnsureSuRestrictedToRootGroup(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDefaultUmaskForAllUsersObject))
        {
            status = RemediateEnsureDefaultUmaskForAllUsers(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAutomountingDisabledObject))
        {
            status = RemediateEnsureAutomountingDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureKernelCompiledFromApprovedSourcesObject))
        {
            status = RemediateEnsureKernelCompiledFromApprovedSources(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDefaultDenyFirewallPolicyIsSetObject))
        {
            status = RemediateEnsureDefaultDenyFirewallPolicyIsSet(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePacketRedirectSendingIsDisabledObject))
        {
            status = RemediateEnsurePacketRedirectSendingIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIcmpRedirectsIsDisabledObject))
        {
            status = RemediateEnsureIcmpRedirectsIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSourceRoutedPacketsIsDisabledObject))
        {
            status = RemediateEnsureSourceRoutedPacketsIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAcceptingSourceRoutedPacketsIsDisabledObject))
        {
            status = RemediateEnsureAcceptingSourceRoutedPacketsIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIgnoringBogusIcmpBroadcastResponsesObject))
        {
            status = RemediateEnsureIgnoringBogusIcmpBroadcastResponses(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIgnoringIcmpEchoPingsToMulticastObject))
        {
            status = RemediateEnsureIgnoringIcmpEchoPingsToMulticast(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureMartianPacketLoggingIsEnabledObject))
        {
            status = RemediateEnsureMartianPacketLoggingIsEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureReversePathSourceValidationIsEnabledObject))
        {
            status = RemediateEnsureReversePathSourceValidationIsEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTcpSynCookiesAreEnabledObject))
        {
            status = RemediateEnsureTcpSynCookiesAreEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSystemNotActingAsNetworkSnifferObject))
        {
            status = RemediateEnsureSystemNotActingAsNetworkSniffer(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllWirelessInterfacesAreDisabledObject))
        {
            status = RemediateEnsureAllWirelessInterfacesAreDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIpv6ProtocolIsEnabledObject))
        {
            status = RemediateEnsureIpv6ProtocolIsEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDccpIsDisabledObject))
        {
            status = RemediateEnsureDccpIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSctpIsDisabledObject))
        {
            status = RemediateEnsureSctpIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledSupportForRdsObject))
        {
            status = RemediateEnsureDisabledSupportForRds(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTipcIsDisabledObject))
        {
            status = RemediateEnsureTipcIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureZeroconfNetworkingIsDisabledObject))
        {
            status = RemediateEnsureZeroconfNetworkingIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnBootloaderConfigObject))
        {
            status = RemediateEnsurePermissionsOnBootloaderConfig(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordReuseIsLimitedObject))
        {
            status = RemediateEnsurePasswordReuseIsLimited(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureMountingOfUsbStorageDevicesIsDisabledObject))
        {
            status = RemediateEnsureMountingOfUsbStorageDevicesIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureCoreDumpsAreRestrictedObject))
        {
            status = RemediateEnsureCoreDumpsAreRestricted(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordCreationRequirementsObject))
        {
            status = RemediateEnsurePasswordCreationRequirements(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLockoutForFailedPasswordAttemptsObject))
        {
            status = RemediateEnsureLockoutForFailedPasswordAttempts(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfCramfsFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfCramfsFileSystem(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfFreevxfsFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfFreevxfsFileSystem(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfHfsFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfHfsFileSystem(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfHfsplusFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfHfsplusFileSystem(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfJffs2FileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfJffs2FileSystem(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureVirtualMemoryRandomizationIsEnabledObject))
        {
            status = RemediateEnsureVirtualMemoryRandomizationIsEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllBootloadersHavePasswordProtectionEnabledObject))
        {
            status = RemediateEnsureAllBootloadersHavePasswordProtectionEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLoggingIsConfiguredObject))
        {
            status = RemediateEnsureLoggingIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSyslogPackageIsInstalledObject))
        {
            status = RemediateEnsureSyslogPackageIsInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSystemdJournaldServicePersistsLogMessagesObject))
        {
            status = RemediateEnsureSystemdJournaldServicePersistsLogMessages(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureALoggingServiceIsEnabledObject))
        {
            status = RemediateEnsureALoggingServiceIsEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureFilePermissionsForAllRsyslogLogFilesObject))
        {
            status = RemediateEnsureFilePermissionsForAllRsyslogLogFiles(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLoggerConfigurationFilesAreRestrictedObject))
        {
            status = RemediateEnsureLoggerConfigurationFilesAreRestricted(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject))
        {
            status = RemediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject))
        {
            status = RemediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRsyslogNotAcceptingRemoteMessagesObject))
        {
            status = RemediateEnsureRsyslogNotAcceptingRemoteMessages(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSyslogRotaterServiceIsEnabledObject))
        {
            status = RemediateEnsureSyslogRotaterServiceIsEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTelnetServiceIsDisabledObject))
        {
            status = RemediateEnsureTelnetServiceIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRcprshServiceIsDisabledObject))
        {
            status = RemediateEnsureRcprshServiceIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTftpServiceisDisabledObject))
        {
            status = RemediateEnsureTftpServiceisDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAtCronIsRestrictedToAuthorizedUsersObject))
        {
            status = RemediateEnsureAtCronIsRestrictedToAuthorizedUsers(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshPortIsConfiguredObject))
        {
            status = RemediateEnsureSshPortIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshBestPracticeProtocolObject))
        {
            status = RemediateEnsureSshBestPracticeProtocol(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshBestPracticeIgnoreRhostsObject))
        {
            status = RemediateEnsureSshBestPracticeIgnoreRhosts(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshLogLevelIsSetObject))
        {
            status = RemediateEnsureSshLogLevelIsSet(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshMaxAuthTriesIsSetObject))
        {
            status = RemediateEnsureSshMaxAuthTriesIsSet(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllowUsersIsConfiguredObject))
        {
            status = RemediateEnsureAllowUsersIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDenyUsersIsConfiguredObject))
        {
            status = RemediateEnsureDenyUsersIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllowGroupsIsConfiguredObject))
        {
            status = RemediateEnsureAllowGroupsIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDenyGroupsConfiguredObject))
        {
            status = RemediateEnsureDenyGroupsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject))
        {
            status = RemediateEnsureSshHostbasedAuthenticationIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshPermitRootLoginIsDisabledObject))
        {
            status = RemediateEnsureSshPermitRootLoginIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject))
        {
            status = RemediateEnsureSshPermitEmptyPasswordsIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshClientIntervalCountMaxIsConfiguredObject))
        {
            status = RemediateEnsureSshClientIntervalCountMaxIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshClientAliveIntervalIsConfiguredObject))
        {
            status = RemediateEnsureSshClientAliveIntervalIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshLoginGraceTimeIsSetObject))
        {
            status = RemediateEnsureSshLoginGraceTimeIsSet(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject))
        {
            status = RemediateEnsureOnlyApprovedMacAlgorithmsAreUsed(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshWarningBannerIsEnabledObject))
        {
            status = RemediateEnsureSshWarningBannerIsEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject))
        {
            status = RemediateEnsureUsersCannotSetSshEnvironmentOptions(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAppropriateCiphersForSshObject))
        {
            status = RemediateEnsureAppropriateCiphersForSsh(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAvahiDaemonServiceIsDisabledObject))
        {
            status = RemediateEnsureAvahiDaemonServiceIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureCupsServiceisDisabledObject))
        {
            status = RemediateEnsureCupsServiceisDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePostfixPackageIsUninstalledObject))
        {
            status = RemediateEnsurePostfixPackageIsUninstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePostfixNetworkListeningIsDisabledObject))
        {
            status = RemediateEnsurePostfixNetworkListeningIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRpcgssdServiceIsDisabledObject))
        {
            status = RemediateEnsureRpcgssdServiceIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRpcidmapdServiceIsDisabledObject))
        {
            status = RemediateEnsureRpcidmapdServiceIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePortmapServiceIsDisabledObject))
        {
            status = RemediateEnsurePortmapServiceIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNetworkFileSystemServiceIsDisabledObject))
        {
            status = RemediateEnsureNetworkFileSystemServiceIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRpcsvcgssdServiceIsDisabledObject))
        {
            status = RemediateEnsureRpcsvcgssdServiceIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSnmpServerIsDisabledObject))
        {
            status = RemediateEnsureSnmpServerIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRsynServiceIsDisabledObject))
        {
            status = RemediateEnsureRsynServiceIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNisServerIsDisabledObject))
        {
            status = RemediateEnsureNisServerIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRshClientNotInstalledObject))
        {
            status = RemediateEnsureRshClientNotInstalled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSmbWithSambaIsDisabledObject))
        {
            status = RemediateEnsureSmbWithSambaIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUsersDotFilesArentGroupOrWorldWritableObject))
        {
            status = RemediateEnsureUsersDotFilesArentGroupOrWorldWritable(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoUsersHaveDotForwardFilesObject))
        {
            status = RemediateEnsureNoUsersHaveDotForwardFiles(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoUsersHaveDotNetrcFilesObject))
        {
            status = RemediateEnsureNoUsersHaveDotNetrcFiles(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoUsersHaveDotRhostsFilesObject))
        {
            status = RemediateEnsureNoUsersHaveDotRhostsFiles(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRloginServiceIsDisabledObject))
        {
            status = RemediateEnsureRloginServiceIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUnnecessaryAccountsAreRemovedObject))
        {
            status = RemediateEnsureUnnecessaryAccountsAreRemoved(jsonString);
        }
        // Initialization for audit before remediation
        else if (0 == strcmp(objectName, g_initEnsurePermissionsOnEtcSshSshdConfigObject))
        {
            status = InitEnsurePermissionsOnEtcSshSshdConfig(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshPortIsConfiguredObject))
        {
            status = InitEnsureSshPortIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshBestPracticeProtocolObject))
        {
            status = InitEnsureSshBestPracticeProtocol(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshBestPracticeIgnoreRhostsObject))
        {
            status = InitEnsureSshBestPracticeIgnoreRhosts(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshLogLevelIsSetObject))
        {
            status = InitEnsureSshLogLevelIsSet(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshMaxAuthTriesIsSetObject))
        {
            status = InitEnsureSshMaxAuthTriesIsSet(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureAllowUsersIsConfiguredObject))
        {
            status = InitEnsureAllowUsersIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureDenyUsersIsConfiguredObject))
        {
            status = InitEnsureDenyUsersIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureAllowGroupsIsConfiguredObject))
        {
            status = InitEnsureAllowGroupsIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureDenyGroupsConfiguredObject))
        {
            status = InitEnsureDenyGroupsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshHostbasedAuthenticationIsDisabledObject))
        {
            status = InitEnsureSshHostbasedAuthenticationIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshPermitRootLoginIsDisabledObject))
        {
            status = InitEnsureSshPermitRootLoginIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshPermitEmptyPasswordsIsDisabledObject))
        {
            status = InitEnsureSshPermitEmptyPasswordsIsDisabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshClientIntervalCountMaxIsConfiguredObject))
        {
            status = InitEnsureSshClientIntervalCountMaxIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshClientAliveIntervalIsConfiguredObject))
        {
            status = InitEnsureSshClientAliveIntervalIsConfigured(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshLoginGraceTimeIsSetObject))
        {
            status = InitEnsureSshLoginGraceTimeIsSet(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureOnlyApprovedMacAlgorithmsAreUsedObject))
        {
            status = InitEnsureOnlyApprovedMacAlgorithmsAreUsed(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureSshWarningBannerIsEnabledObject))
        {
            status = InitEnsureSshWarningBannerIsEnabled(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureUsersCannotSetSshEnvironmentOptionsObject))
        {
            status = InitEnsureUsersCannotSetSshEnvironmentOptions(jsonString);
        }
        else if (0 == strcmp(objectName, g_initEnsureAppropriateCiphersForSshObject))
        {
            status = InitEnsureAppropriateCiphersForSsh(jsonString);
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

    return status;
}