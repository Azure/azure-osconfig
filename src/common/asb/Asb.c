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

// Initialization for audit before remediation 
// TODO: For those kept, then we will add them to SecurityBaseline MIM, to its Test Recipe, to unit-tests, etc
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

// Default values for checks that support configuration (and initialization)
static const char* g_defaultEnsurePermissionsOnEtcIssue = "644";
static const char* g_defaultEnsurePermissionsOnEtcIssueNet = "644";
static const char* g_defaultEnsurePermissionsOnEtcHostsAllow = "644";
static const char* g_defaultEnsurePermissionsOnEtcHostsDeny = "644";
static const char* g_defaultEnsurePermissionsOnEtcShadow = "400";
static const char* g_defaultEnsurePermissionsOnEtcShadowDash = "400";
static const char* g_defaultEnsurePermissionsOnEtcGShadow = "400";
static const char* g_defaultEnsurePermissionsOnEtcGShadowDash = "400";
static const char* g_defaultEnsurePermissionsOnEtcPasswd = "644";
static const char* g_defaultEnsurePermissionsOnEtcPasswdDash = "600";
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
static const char* g_defaultEnsurePasswordHashingAlgorithm = "sha512";
static const char* g_defaultEnsureMinDaysBetweenPasswordChanges = "7";
static const char* g_defaultEnsureInactivePasswordLockPeriod = "30";
static const char* g_defaultEnsureMaxDaysBetweenPasswordChanges = "365";
static const char* g_defaultEnsurePasswordExpiration = "365";
static const char* g_defaultEnsurePasswordExpirationWarning = "7";
static const char* g_defaultEnsureDefaultUmaskForAllUsers = "777";
static const char* g_defaultEnsurePermissionsOnBootloaderConfig = "400";
static const char* g_defaultEnsurePasswordReuseIsLimited = "5";
static const char* g_defaultEnsurePasswordCreationRequirements = "14,4,-1,-1,-1,-1";
static const char* g_defaultEnsureFilePermissionsForAllRsyslogLogFiles = "600,640";
static const char* g_defaultEnsureUsersDotFilesArentGroupOrWorldWritable = "600,644,664,700,744";
static const char* g_defaultEnsureUnnecessaryAccountsAreRemoved = "games,test";

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

static const char* g_pass = SECURITY_AUDIT_PASS;
static const char* g_fail = SECURITY_AUDIT_FAIL;

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

void AsbInitialize(void* log)
{
    InitializeSshAudit(log);

    if ((NULL == (g_desiredEnsurePermissionsOnEtcIssue = DuplicateString(g_defaultEnsurePermissionsOnEtcIssue))) ||
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
        (NULL == (g_desiredEnsureUnnecessaryAccountsAreRemoved = DuplicateString(g_defaultEnsureUnnecessaryAccountsAreRemoved))))
    {
        OsConfigLogError(log, "AsbInitialize: failed to allocate memory");
    }
    
    OsConfigLogInfo(log, "%s initialized", g_asbName);
}

void AsbShutdown(void* log)
{
    OsConfigLogInfo(log, "%s shutting down", g_asbName);
        
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

    SshAuditCleanup(log);
}

static char* AuditEnsurePermissionsOnEtcIssue(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcIssue, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcIssue ? 
        g_desiredEnsurePermissionsOnEtcIssue : g_defaultEnsurePermissionsOnEtcIssue), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcIssueNet(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcIssueNet, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcIssueNet ? 
        g_desiredEnsurePermissionsOnEtcIssueNet : g_defaultEnsurePermissionsOnEtcIssueNet), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcHostsAllow(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcHostsAllow, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcHostsAllow ? 
        g_desiredEnsurePermissionsOnEtcHostsAllow : g_defaultEnsurePermissionsOnEtcHostsAllow), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcHostsDeny(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcHostsDeny, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcHostsDeny ? 
        g_desiredEnsurePermissionsOnEtcHostsDeny : g_defaultEnsurePermissionsOnEtcHostsDeny), &reason, log);
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
    CheckFileAccess(g_etcShadow, 0, 42, atoi(g_desiredEnsurePermissionsOnEtcShadow ? 
        g_desiredEnsurePermissionsOnEtcShadow : g_defaultEnsurePermissionsOnEtcShadow), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcShadowDash(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcShadowDash, 0, 42, atoi(g_desiredEnsurePermissionsOnEtcShadowDash ? 
        g_desiredEnsurePermissionsOnEtcShadowDash : g_defaultEnsurePermissionsOnEtcShadowDash), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGShadow(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGShadow, 0, 42, atoi(g_desiredEnsurePermissionsOnEtcGShadow ? 
        g_desiredEnsurePermissionsOnEtcGShadow : g_defaultEnsurePermissionsOnEtcGShadow), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGShadowDash(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGShadowDash, 0, 42, atoi(g_desiredEnsurePermissionsOnEtcGShadowDash ? 
        g_desiredEnsurePermissionsOnEtcGShadowDash : g_defaultEnsurePermissionsOnEtcGShadowDash), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcPasswd(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcPasswd, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcPasswd ? 
        g_desiredEnsurePermissionsOnEtcPasswd : g_defaultEnsurePermissionsOnEtcPasswd), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcPasswdDash(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcPasswdDash, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcPasswdDash ? 
        g_desiredEnsurePermissionsOnEtcPasswdDash : g_defaultEnsurePermissionsOnEtcPasswdDash), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGroup(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGroup, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcGroup ? 
        g_desiredEnsurePermissionsOnEtcGroup : g_defaultEnsurePermissionsOnEtcGroup), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcGroupDash(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGroupDash, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcGroupDash ? 
        g_desiredEnsurePermissionsOnEtcGroupDash : g_defaultEnsurePermissionsOnEtcGroupDash), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcAnacronTab(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcAnacronTab, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcAnacronTab ? 
        g_desiredEnsurePermissionsOnEtcAnacronTab : g_defaultEnsurePermissionsOnEtcAnacronTab), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronD(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronD, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcCronD ? 
        g_desiredEnsurePermissionsOnEtcCronD : g_defaultEnsurePermissionsOnEtcCronD), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronDaily(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronDaily, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcCronDaily ? 
        g_desiredEnsurePermissionsOnEtcCronDaily : g_defaultEnsurePermissionsOnEtcCronDaily), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronHourly(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronHourly, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcCronHourly ? 
        g_desiredEnsurePermissionsOnEtcCronHourly : g_defaultEnsurePermissionsOnEtcCronHourly), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronMonthly(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronMonthly, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcCronMonthly ? 
        g_desiredEnsurePermissionsOnEtcCronMonthly : g_defaultEnsurePermissionsOnEtcCronMonthly), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcCronWeekly(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronWeekly, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcCronWeekly ? 
        g_desiredEnsurePermissionsOnEtcCronWeekly : g_defaultEnsurePermissionsOnEtcCronWeekly), &reason, log);
    return reason;
}

static char* AuditEnsurePermissionsOnEtcMotd(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcMotd, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcMotd ? 
        g_desiredEnsurePermissionsOnEtcMotd : g_defaultEnsurePermissionsOnEtcMotd), &reason, log);
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
    int* modes = NULL;
    int numberOfModes = 0;
    char* reason = NULL;

    if (0 == ConvertStringToIntegers(g_desiredEnsureRestrictedUserHomeDirectories ? 
        g_desiredEnsureRestrictedUserHomeDirectories : g_defaultEnsureRestrictedUserHomeDirectories, ',', &modes, &numberOfModes, log))
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

static char* AuditEnsurePasswordHashingAlgorithm(void* log)
{
    char* reason = NULL;
    CheckPasswordHashingAlgorithm(atoi(g_desiredEnsurePasswordHashingAlgorithm ? 
        g_desiredEnsurePasswordHashingAlgorithm : g_defaultEnsurePasswordHashingAlgorithm), &reason, log);
    return reason;
}

static char* AuditEnsureMinDaysBetweenPasswordChanges(void* log)
{
    char* reason = NULL;
    CheckMinDaysBetweenPasswordChanges(atoi(g_desiredEnsureMinDaysBetweenPasswordChanges ? 
        g_desiredEnsureMinDaysBetweenPasswordChanges : g_defaultEnsureMinDaysBetweenPasswordChanges), &reason, log);
    return reason;
}

static char* AuditEnsureInactivePasswordLockPeriod(void* log)
{
    char* reason = NULL;
    CheckLockoutAfterInactivityLessThan(atoi(g_desiredEnsureInactivePasswordLockPeriod ? 
        g_desiredEnsureInactivePasswordLockPeriod : g_defaultEnsureInactivePasswordLockPeriod), &reason, log);
    CheckUsersRecordedPasswordChangeDates(&reason, log);
    return reason;
}

static char* AuditEnsureMaxDaysBetweenPasswordChanges(void* log)
{
    char* reason = NULL;
    CheckMaxDaysBetweenPasswordChanges(atoi(g_desiredEnsureMaxDaysBetweenPasswordChanges ? 
        g_desiredEnsureMaxDaysBetweenPasswordChanges : g_defaultEnsureMaxDaysBetweenPasswordChanges), &reason, log);
    return reason;
}

static char* AuditEnsurePasswordExpiration(void* log)
{
    char* reason = NULL;
    CheckPasswordExpirationLessThan(atoi(g_desiredEnsurePasswordExpiration ? 
        g_desiredEnsurePasswordExpiration : g_defaultEnsurePasswordExpiration), &reason, log);
    return reason;
}

static char* AuditEnsurePasswordExpirationWarning(void* log)
{
    char* reason = NULL;
    CheckPasswordExpirationWarning(atoi(g_desiredEnsurePasswordExpirationWarning ? 
        g_desiredEnsurePasswordExpirationWarning : g_defaultEnsurePasswordExpirationWarning), &reason, log);
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
    CheckLoginUmask(g_desiredEnsureDefaultUmaskForAllUsers ? 
        g_desiredEnsureDefaultUmaskForAllUsers : g_defaultEnsureDefaultUmaskForAllUsers, &reason, log);
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
        FREE_MEMORY(reason);
        reason = FormatAllocateString("%sNo active wireless interfaces are present", g_pass);
    }
    else
    {
        FREE_MEMORY(reason);
        reason = DuplicateString("At least one active wireless interface is present");
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
    const char* value = g_desiredEnsurePermissionsOnBootloaderConfig ? 
        g_desiredEnsurePermissionsOnBootloaderConfig : g_defaultEnsurePermissionsOnBootloaderConfig;
    char* reason = NULL;
    CheckFileAccess("/boot/grub/grub.cfg", 0, 0, (unsigned int)atoi(value), &reason, log);
    CheckFileAccess("/boot/grub/grub.conf", 0, 0, (unsigned int)atoi(value), &reason, log);
    CheckFileAccess("/boot/grub2/grub.cfg", 0, 0, (unsigned int)atoi(value), &reason, log);
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
    int* values = NULL;
    int numberOfValues = 0;
    char* reason = NULL;

    if ((0 == ConvertStringToIntegers(g_desiredEnsurePasswordCreationRequirements ? g_desiredEnsurePasswordCreationRequirements : 
        g_defaultEnsurePasswordCreationRequirements, ',', &values, &numberOfValues, log)) && (6 == numberOfValues))
    {
        CheckPasswordCreationRequirements(values[0], values[1], values[2], values[3], values[4], values[5], &reason, log);
    }
    else
    {
        reason = FormatAllocateString("Failed to parse '%s'. There must be 6 numbers, comma separated, in this order: minlen, minclass, dcredit, ucredit, ocredit, lcredit", 
            g_desiredEnsurePasswordCreationRequirements ? g_desiredEnsurePasswordCreationRequirements : g_defaultEnsurePasswordCreationRequirements);
    }

    FREE_MEMORY(values);
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
    FREE_MEMORY(reason);
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
    CheckFileContents("/proc/sys/kernel/randomize_va_space", "1", &reason, log);
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
    FREE_MEMORY(reason);
    if ((0 == CheckPackageNotInstalled(g_rsyslog, &reason, log)) && 
        (0 == CheckPackageNotInstalled(g_systemd, &reason, log)) && 
        CheckDaemonActive(g_syslogNg, &reason, log)) 
    {
        return reason;
    }
    FREE_MEMORY(reason);
    CheckPackageInstalled(g_systemd, &reason, log);
    CheckDaemonActive(g_systemdJournald, &reason, log);
    return reason;
}

static char* AuditEnsureFilePermissionsForAllRsyslogLogFiles(void* log)
{
    const char* fileCreateMode = "$FileCreateMode";
    int* modes = NULL;
    int numberOfModes = 0;
    char* reason = NULL;

    if (0 == ConvertStringToIntegers(g_desiredEnsureFilePermissionsForAllRsyslogLogFiles ?
        g_desiredEnsureFilePermissionsForAllRsyslogLogFiles : g_defaultEnsureFilePermissionsForAllRsyslogLogFiles, ',', &modes, &numberOfModes, log))
    {
        CheckIntegerOptionFromFileEqualWithAny(g_etcRsyslogConf, fileCreateMode, ' ', modes, numberOfModes, &reason, log);
        if (0 == FileExists(g_etcSyslogNgSyslogNgConf))
        {
            CheckIntegerOptionFromFileEqualWithAny(g_etcSyslogNgSyslogNgConf, fileCreateMode, ' ', modes, numberOfModes, &reason, log);
        }
    }
    else
    {
        reason = FormatAllocateString("Failed to parse '%s'", g_desiredEnsureFilePermissionsForAllRsyslogLogFiles ?
            g_desiredEnsureFilePermissionsForAllRsyslogLogFiles : g_defaultEnsureFilePermissionsForAllRsyslogLogFiles);
    }

    FREE_MEMORY(modes);
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
    int* modes = NULL;
    int numberOfModes = 0;
    char* reason = NULL;

    if (0 == ConvertStringToIntegers(g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable ?
        g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable : g_defaultEnsureUsersDotFilesArentGroupOrWorldWritable, ',', &modes, &numberOfModes, log))
    {
        CheckUsersRestrictedDotFiles((unsigned int*)modes, (unsigned int)numberOfModes, &reason, log);
    }
    else
    {
        reason = FormatAllocateString("Failed to parse '%s'", g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable ?
            g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable : g_defaultEnsureUsersDotFilesArentGroupOrWorldWritable);
    }

    FREE_MEMORY(modes);
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
    char* reason = NULL;
    CheckUserAccountsNotFound(g_desiredEnsureUnnecessaryAccountsAreRemoved ? 
        g_desiredEnsureUnnecessaryAccountsAreRemoved : g_defaultEnsureUnnecessaryAccountsAreRemoved, &reason, log);
    return reason;
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

static int ReplaceString(char* target, char* source, const char* defaultValue)
{
    bool isValidValue = ((NULL == source) || (0 == source[0])) ? false : true;
    int status = 0;

    FREE_MEMORY(target);
    status = (NULL != (target = DuplicateString(isValidValue ? source : defaultValue))) ? 0 : ENOMEM;

    return status;
}

static int InitEnsurePermissionsOnEtcIssue(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcIssue, value, g_defaultEnsurePermissionsOnEtcIssue);
}

static int InitEnsurePermissionsOnEtcIssueNet(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcIssueNet, value, g_defaultEnsurePermissionsOnEtcIssueNet);
}

static int InitEnsurePermissionsOnEtcHostsAllow(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcHostsAllow, value, g_defaultEnsurePermissionsOnEtcHostsAllow);
}

static int InitEnsurePermissionsOnEtcHostsDeny(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcHostsDeny, value, g_defaultEnsurePermissionsOnEtcHostsDeny);
}

static int InitEnsurePermissionsOnEtcShadow(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcShadow, value, g_defaultEnsurePermissionsOnEtcShadow);
}

static int InitEnsurePermissionsOnEtcShadowDash(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcShadowDash, value, g_defaultEnsurePermissionsOnEtcShadowDash);
}

static int InitEnsurePermissionsOnEtcGShadow(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcGShadow, value, g_defaultEnsurePermissionsOnEtcGShadow);
}

static int InitEnsurePermissionsOnEtcGShadowDash(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcGShadowDash, value, g_defaultEnsurePermissionsOnEtcGShadowDash);
}

static int InitEnsurePermissionsOnEtcPasswd(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcPasswd, value, g_defaultEnsurePermissionsOnEtcPasswd);
}

static int InitEnsurePermissionsOnEtcPasswdDash(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcPasswdDash, value, g_defaultEnsurePermissionsOnEtcPasswdDash);
}

static int InitEnsurePermissionsOnEtcGroup(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcGroup, value, g_defaultEnsurePermissionsOnEtcGroup);
}

static int InitEnsurePermissionsOnEtcGroupDash(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcGroupDash, value, g_defaultEnsurePermissionsOnEtcGroupDash);
}

static int InitEnsurePermissionsOnEtcAnacronTab(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcAnacronTab, value, g_defaultEnsurePermissionsOnEtcAnacronTab);
}

static int InitEnsurePermissionsOnEtcCronD(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcCronD, value, g_defaultEnsurePermissionsOnEtcCronD);
}

static int InitEnsurePermissionsOnEtcCronDaily(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcCronDaily, value, g_defaultEnsurePermissionsOnEtcCronDaily);
}

static int InitEnsurePermissionsOnEtcCronHourly(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcCronHourly, value, g_defaultEnsurePermissionsOnEtcCronHourly);
}

static int InitEnsurePermissionsOnEtcCronMonthly(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcCronMonthly, value, g_defaultEnsurePermissionsOnEtcCronMonthly);
}

static int InitEnsurePermissionsOnEtcCronWeekly(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcCronWeekly, value, g_defaultEnsurePermissionsOnEtcCronWeekly);
}

static int InitEnsurePermissionsOnEtcMotd(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnEtcMotd, value, g_defaultEnsurePermissionsOnEtcMotd);
}

static int InitEnsureRestrictedUserHomeDirectories(char* value)
{
    return ReplaceString(g_desiredEnsureRestrictedUserHomeDirectories, value, g_defaultEnsureRestrictedUserHomeDirectories);
}

static int InitEnsurePasswordHashingAlgorithm(char* value)
{
    return ReplaceString(g_desiredEnsurePasswordHashingAlgorithm, value, g_defaultEnsurePasswordHashingAlgorithm);
}

static int InitEnsureMinDaysBetweenPasswordChanges(char* value)
{
    return ReplaceString(g_desiredEnsureMinDaysBetweenPasswordChanges, value, g_defaultEnsureMinDaysBetweenPasswordChanges);
}

static int InitEnsureInactivePasswordLockPeriod(char* value)
{
    return ReplaceString(g_desiredEnsureInactivePasswordLockPeriod, value, g_defaultEnsureInactivePasswordLockPeriod);
}

static int InitEnsureMaxDaysBetweenPasswordChanges(char* value)
{
    return ReplaceString(g_desiredEnsureMaxDaysBetweenPasswordChanges, value, g_defaultEnsureMaxDaysBetweenPasswordChanges);
}

static int InitEnsurePasswordExpiration(char* value)
{
    return ReplaceString(g_desiredEnsurePasswordExpiration, value, g_defaultEnsurePasswordExpiration);
}

static int InitEnsurePasswordExpirationWarning(char* value)
{
    return ReplaceString(g_desiredEnsurePasswordExpirationWarning, value, g_defaultEnsurePasswordExpirationWarning);
}

static int InitEnsureDefaultUmaskForAllUsers(char* value)
{
    return ReplaceString(g_desiredEnsureDefaultUmaskForAllUsers, value, g_defaultEnsureDefaultUmaskForAllUsers);
}

static int InitEnsurePermissionsOnBootloaderConfig(char* value)
{
    return ReplaceString(g_desiredEnsurePermissionsOnBootloaderConfig, value, g_defaultEnsurePermissionsOnBootloaderConfig);
}

static int InitEnsurePasswordReuseIsLimited(char* value)
{
    return ReplaceString(g_desiredEnsurePasswordReuseIsLimited, value, g_defaultEnsurePasswordReuseIsLimited);
}

static int InitEnsurePasswordCreationRequirements(char* value)
{
    return ReplaceString(g_desiredEnsurePasswordCreationRequirements, value, g_defaultEnsurePasswordCreationRequirements);
}

static int InitEnsureFilePermissionsForAllRsyslogLogFiles(char* value)
{
    return ReplaceString(g_desiredEnsureFilePermissionsForAllRsyslogLogFiles, value, g_defaultEnsureFilePermissionsForAllRsyslogLogFiles);
}

static int InitEnsureUsersDotFilesArentGroupOrWorldWritable(char* value)
{
    return ReplaceString(g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable, value, g_defaultEnsureUsersDotFilesArentGroupOrWorldWritable);
}

static int InitEnsureUnnecessaryAccountsAreRemoved(char* value)
{
    return ReplaceString(g_desiredEnsureUnnecessaryAccountsAreRemoved, value, g_defaultEnsureUnnecessaryAccountsAreRemoved);
}

static int RemediateEnsurePermissionsOnEtcIssue(char* value, void* log)
{
    InitEnsurePermissionsOnEtcIssue(value);
    return SetFileAccess(g_etcIssue, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcIssue), log);
};

static int RemediateEnsurePermissionsOnEtcIssueNet(char* value, void* log)
{
    InitEnsurePermissionsOnEtcIssueNet(value);
    return SetFileAccess(g_etcIssueNet, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcIssueNet), log);
};

static int RemediateEnsurePermissionsOnEtcHostsAllow(char* value, void* log)
{
    InitEnsurePermissionsOnEtcHostsAllow(value);
    return SetFileAccess(g_etcHostsAllow, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcHostsAllow), log);
};

static int RemediateEnsurePermissionsOnEtcHostsDeny(char* value, void* log)
{
    InitEnsurePermissionsOnEtcHostsDeny(value);
    return SetFileAccess(g_etcHostsDeny, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcHostsDeny), log);
};

static int RemediateEnsurePermissionsOnEtcSshSshdConfig(char* value, void* log)
{
    return ProcessSshAuditCheck(g_remediateEnsurePermissionsOnEtcSshSshdConfigObject, value, NULL, log);
};

static int RemediateEnsurePermissionsOnEtcShadow(char* value, void* log)
{
    InitEnsurePermissionsOnEtcShadow(value);
    return SetFileAccess(g_etcShadow, 0, 42, atoi(g_desiredEnsurePermissionsOnEtcShadow), log);
};

static int RemediateEnsurePermissionsOnEtcShadowDash(char* value, void* log)
{
    InitEnsurePermissionsOnEtcShadowDash(value);
    return SetFileAccess(g_etcShadowDash, 0, 42, atoi(g_desiredEnsurePermissionsOnEtcShadowDash), log);
};

static int RemediateEnsurePermissionsOnEtcGShadow(char* value, void* log)
{
    InitEnsurePermissionsOnEtcGShadow(value);
    return SetFileAccess(g_etcGShadow, 0, 42, atoi(g_desiredEnsurePermissionsOnEtcGShadow), log);
};

static int RemediateEnsurePermissionsOnEtcGShadowDash(char* value, void* log)
{
    InitEnsurePermissionsOnEtcGShadowDash(value);
    return SetFileAccess(g_etcGShadowDash, 0, 42, atoi(g_desiredEnsurePermissionsOnEtcGShadowDash), log);
};

static int RemediateEnsurePermissionsOnEtcPasswd(char* value, void* log)
{
    InitEnsurePermissionsOnEtcPasswd(value);
    return SetFileAccess(g_etcPasswd, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcPasswd), log);
};

static int RemediateEnsurePermissionsOnEtcPasswdDash(char* value, void* log)
{
    InitEnsurePermissionsOnEtcPasswdDash(value);
    return SetFileAccess(g_etcPasswdDash, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcPasswdDash), log);
};

static int RemediateEnsurePermissionsOnEtcGroup(char* value, void* log)
{
    InitEnsurePermissionsOnEtcGroup(value);
    return SetFileAccess(g_etcGroup, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcGroup), log);
};

static int RemediateEnsurePermissionsOnEtcGroupDash(char* value, void* log)
{
    InitEnsurePermissionsOnEtcGroupDash(value);
    return SetFileAccess(g_etcGroupDash, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcGroupDash), log);
};

static int RemediateEnsurePermissionsOnEtcAnacronTab(char* value, void* log)
{
    InitEnsurePermissionsOnEtcAnacronTab(value);
    return SetFileAccess(g_etcAnacronTab, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcAnacronTab), log);
};

static int RemediateEnsurePermissionsOnEtcCronD(char* value, void* log)
{
    InitEnsurePermissionsOnEtcCronD(value);
    return SetFileAccess(g_etcCronD, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcCronD), log);
};

static int RemediateEnsurePermissionsOnEtcCronDaily(char* value, void* log)
{
    InitEnsurePermissionsOnEtcCronDaily(value);
    return SetFileAccess(g_etcCronDaily, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcCronDaily), log);
};

static int RemediateEnsurePermissionsOnEtcCronHourly(char* value, void* log)
{
    InitEnsurePermissionsOnEtcCronHourly(value);
    return SetFileAccess(g_etcCronHourly, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcCronHourly), log);
};

static int RemediateEnsurePermissionsOnEtcCronMonthly(char* value, void* log)
{
    InitEnsurePermissionsOnEtcCronMonthly(value);
    return SetFileAccess(g_etcCronMonthly, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcCronMonthly), log);
};

static int RemediateEnsurePermissionsOnEtcCronWeekly(char* value, void* log)
{
    InitEnsurePermissionsOnEtcCronWeekly(value);
    return SetFileAccess(g_etcCronWeekly, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcCronWeekly), log);
};

static int RemediateEnsurePermissionsOnEtcMotd(char* value, void* log)
{
    InitEnsurePermissionsOnEtcMotd(value);
    return SetFileAccess(g_etcMotd, 0, 0, atoi(g_desiredEnsurePermissionsOnEtcMotd), log);
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
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnHomePartition(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnTmpPartition(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnVarTmpPartition(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionOnTmpPartition(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionOnVarTmpPartition(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionOnVarTmpPartition(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionOnDevShmPartition(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionEnabledForAllRemovableMedia(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionEnabledForAllRemovableMedia(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionEnabledForAllRemovableMedia(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllTelnetdPackagesUninstalled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllEtcPasswdGroupsExistInEtcGroup(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateUidsExist(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateGidsExist(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateUserNamesExist(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateGroupsExist(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureShadowGroupIsEmpty(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRootGroupExists(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllAccountsHavePasswords(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcPasswd(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcShadow(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcGroup(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultRootAccountGroupIsGidZero(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRootIsOnlyUidZeroAccount(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllUsersHomeDirectoriesExist(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUsersOwnTheirHomeDirectories(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRestrictedUserHomeDirectories(char* value, void* log)
{
    int* modes = NULL;
    int numberOfModes = 0;
    int status = 0;

    InitEnsureRestrictedUserHomeDirectories(value);

    if ((0 == (status = ConvertStringToIntegers(g_desiredEnsureRestrictedUserHomeDirectories, ',', &modes, &numberOfModes, log))) && (numberOfModes >= 2))
    {
        status = SetRestrictedUserHomeDirectories((unsigned int*)modes, (unsigned int)numberOfModes, modes[0], modes[1], log);
    }

    FREE_MEMORY(modes);
    return status;
}

static int RemediateEnsurePasswordHashingAlgorithm(char* value, void* log)
{
    InitEnsurePasswordHashingAlgorithm(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMinDaysBetweenPasswordChanges(char* value, void* log)
{
    InitEnsureMinDaysBetweenPasswordChanges(value);
    return SetMinDaysBetweenPasswordChanges(atoi(g_desiredEnsureMinDaysBetweenPasswordChanges), log);
}

static int RemediateEnsureInactivePasswordLockPeriod(char* value, void* log)
{
    InitEnsureInactivePasswordLockPeriod(value);
    return SetLockoutAfterInactivityLessThan(atoi(g_desiredEnsureInactivePasswordLockPeriod), log);
}

static int RemediateEnsureMaxDaysBetweenPasswordChanges(char* value, void* log)
{
    InitEnsureMaxDaysBetweenPasswordChanges(value);
    return SetMaxDaysBetweenPasswordChanges(atoi(g_desiredEnsureMaxDaysBetweenPasswordChanges), log);
}

static int RemediateEnsurePasswordExpiration(char* value, void* log)
{
    InitEnsurePasswordExpiration(value);

    return ((0 == SetMinDaysBetweenPasswordChanges(atoi(g_desiredEnsureMinDaysBetweenPasswordChanges ? 
        g_desiredEnsureMinDaysBetweenPasswordChanges : g_defaultEnsureMinDaysBetweenPasswordChanges), log)) &&
        (0 == SetMaxDaysBetweenPasswordChanges(atoi(g_desiredEnsureMaxDaysBetweenPasswordChanges ? 
        g_desiredEnsureMaxDaysBetweenPasswordChanges : g_defaultEnsureMaxDaysBetweenPasswordChanges), log)) &&
        (0 == CheckPasswordExpirationLessThan(atoi(g_desiredEnsurePasswordExpiration), NULL, log))) ? 0 : ENOENT;
}

static int RemediateEnsurePasswordExpirationWarning(char* value, void* log)
{
    InitEnsurePasswordExpirationWarning(value);
    return SetPasswordExpirationWarning(atoi(g_desiredEnsurePasswordExpirationWarning), log);
}

static int RemediateEnsureSystemAccountsAreNonLogin(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAuthenticationRequiredForSingleUserMode(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDotDoesNotAppearInRootsPath(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRemoteLoginWarningBannerIsConfigured(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLocalLoginWarningBannerIsConfigured(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSuRestrictedToRootGroup(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultUmaskForAllUsers(char* value, void* log)
{
    InitEnsureDefaultUmaskForAllUsers(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAutomountingDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureKernelCompiledFromApprovedSources(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultDenyFirewallPolicyIsSet(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePacketRedirectSendingIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIcmpRedirectsIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSourceRoutedPacketsIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAcceptingSourceRoutedPacketsIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIgnoringBogusIcmpBroadcastResponses(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIgnoringIcmpEchoPingsToMulticast(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMartianPacketLoggingIsEnabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureReversePathSourceValidationIsEnabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTcpSynCookiesAreEnabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSystemNotActingAsNetworkSniffer(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllWirelessInterfacesAreDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIpv6ProtocolIsEnabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDccpIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSctpIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledSupportForRds(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTipcIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureZeroconfNetworkingIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePermissionsOnBootloaderConfig(char* value, void* log)
{
    InitEnsurePermissionsOnBootloaderConfig(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordReuseIsLimited(char* value, void* log)
{
    InitEnsurePasswordReuseIsLimited(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMountingOfUsbStorageDevicesIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureCoreDumpsAreRestricted(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordCreationRequirements(char* value, void* log)
{
    InitEnsurePasswordCreationRequirements(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLockoutForFailedPasswordAttempts(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfCramfsFileSystem(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfFreevxfsFileSystem(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfHfsFileSystem(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfHfsplusFileSystem(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfJffs2FileSystem(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureVirtualMemoryRandomizationIsEnabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllBootloadersHavePasswordProtectionEnabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLoggingIsConfigured(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
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
    InitEnsureFilePermissionsForAllRsyslogLogFiles(value);
    UNUSED(log);
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
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRsyslogNotAcceptingRemoteMessages(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSyslogRotaterServiceIsEnabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTelnetServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRcprshServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTftpServiceisDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAtCronIsRestrictedToAuthorizedUsers(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
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
    return (0 == strncmp(g_pass, AuditEnsureAvahiDaemonServiceIsDisabled(log), strlen(g_pass))) ? 0 : ENOENT;
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
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRpcgssdServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcgssd, log);
    StopAndDisableDaemon(g_rpcGssd, log);
    return (0 == strncmp(g_pass, AuditEnsureRpcgssdServiceIsDisabled(log), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRpcidmapdServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcidmapd, log);
    StopAndDisableDaemon(g_nfsIdmapd, log);
    return (0 == strncmp(g_pass, AuditEnsureRpcidmapdServiceIsDisabled(log), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsurePortmapServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcbind, log);
    StopAndDisableDaemon(g_rpcbindService, log);
    StopAndDisableDaemon(g_rpcbindSocket, log);
    return (0 == strncmp(g_pass, AuditEnsurePortmapServiceIsDisabled(log), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureNetworkFileSystemServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_nfsServer, log);
    return (0 == strncmp(g_pass, AuditEnsureNetworkFileSystemServiceIsDisabled(log), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRpcsvcgssdServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns    
}

static int RemediateEnsureSnmpServerIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_snmpd, log);
    return (0 == strncmp(g_pass, AuditEnsureSnmpServerIsDisabled(log), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRsynServiceIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rsync, log);
    return (0 == strncmp(g_pass, AuditEnsureRsynServiceIsDisabled(log), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureNisServerIsDisabled(char* value, void* log)
{
    UNUSED(value);
    StopAndDisableDaemon(g_ypserv, log);
    return (0 == strncmp(g_pass, AuditEnsureNisServerIsDisabled(log), strlen(g_pass))) ? 0 : ENOENT;
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
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUsersDotFilesArentGroupOrWorldWritable(char* value, void* log)
{
    int* modes = NULL;
    int numberOfModes = 0;
    int status = 0;

    InitEnsureUsersDotFilesArentGroupOrWorldWritable(value);

    if (0 == (status = ConvertStringToIntegers(g_desiredEnsureUsersDotFilesArentGroupOrWorldWritable, ',', &modes, &numberOfModes, log)))
    {
        status = SetUsersRestrictedDotFiles((unsigned int*)modes, (unsigned int)numberOfModes, modes[(numberOfModes > 1) ? (numberOfModes - 1) : 0], log);
    }

    FREE_MEMORY(modes);
    return status;
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
    UNUSED(log);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUnnecessaryAccountsAreRemoved(char* value, void* log)
{
    InitEnsureUnnecessaryAccountsAreRemoved(value);
    return RemoveUserAccounts(g_desiredEnsureUnnecessaryAccountsAreRemoved, log);
}

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
                *payloadSizeBytes = (int)strlen(serializedValue);

                if ((maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > maxPayloadSizeBytes))
                {
                    OsConfigLogError(log, "MmiGet(%s, %s) insufficient max size (%d bytes) vs actual size (%d bytes), report will be truncated",
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

    return status;
}

int AsbMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, void* log)
{
    JSON_Value* jsonValue = NULL;
    char* jsonString = NULL;
    char* payloadString = NULL;
    int status = 0;

    // No payload is accepted for now, this may change once the complete Azure Security Baseline is implemented
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