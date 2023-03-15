// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <unistd.h>
#include <version.h>
#include <parson.h>
#include <CommonUtils.h>
#include <UserUtils.h>
#include <Logging.h>
#include <Mmi.h>

#include "SecurityBaseline.h"

static const char* g_securityBaselineModuleName = "OSConfig SecurityBaseline module";
static const char* g_securityBaselineComponentName = "SecurityBaseline";

static const char* g_auditSecurityBaselineObject = "auditSecurityBaseline";
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
// Audit-only
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
static const char* g_auditEnsureALoggingServiceIsSnabledObject = "auditEnsureALoggingServiceIsSnabled";
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
static const char* g_auditEnsureSshBestPracticeProtocolObject = "auditEnsureSshBestPracticeProtocol";
static const char* g_auditEnsureSshBestPracticeIgnoreRhostsObject = "auditEnsureSshBestPracticeIgnoreRhosts";
static const char* g_auditEnsureSshLogLevelIsSetObject = "auditEnsureSshLogLevelIsSet";
static const char* g_auditEnsureSshMaxAuthTriesIsSetObject = "auditEnsureSshMaxAuthTriesIsSet";
static const char* g_auditEnsureSshAccessIsLimitedObject = "auditEnsureSshAccessIsLimited";
static const char* g_auditEnsureSshRhostsRsaAuthenticationIsDisabledObject = "auditEnsureSshRhostsRsaAuthenticationIsDisabled";
static const char* g_auditEnsureSshHostbasedAuthenticationIsDisabledObject = "auditEnsureSshHostbasedAuthenticationIsDisabled";
static const char* g_auditEnsureSshPermitRootLoginIsDisabledObject = "auditEnsureSshPermitRootLoginIsDisabled";
static const char* g_auditEnsureSshPermitEmptyPasswordsIsDisabledObject = "auditEnsureSshPermitEmptyPasswordsIsDisabled";
static const char* g_auditEnsureSshIdleTimeoutIntervalIsConfiguredObject = "auditEnsureSshIdleTimeoutIntervalIsConfigured";
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
static const char* g_remediateSecurityBaselineObject = "remediateSecurityBaseline";
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

static const char* g_securityBaselineLogFile = "/var/log/osconfig_securitybaseline.log";
static const char* g_securityBaselineRolledLogFile = "/var/log/osconfig_securitybaseline.bak";

static const char* g_securityBaselineModuleInfo = "{\"Name\": \"SecurityBaseline\","
    "\"Description\": \"Provides functionality to audit and remediate Security Baseline policies on device\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Zinc\","
    "\"Components\": [\"SecurityBaseline\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static const char* g_etcIssue = "/etc/issue";
static const char* g_etcIssueNet = "/etc/issue.net";
static const char* g_etcHostsAllow = "/etc/hosts.allow";
static const char* g_etcHostsDeny = "/etc/hosts.deny";
static const char* g_etcSshSshdConfig = "/etc/ssh/sshd_config";
static const char* g_etcShadow = "/etc/shadow";
static const char* g_etcShadowDash = "/etc/shadow-";
static const char* g_etcGShadow = "/etc/gshadow";
static const char* g_etcGShadowDash = "/etc/gshadow-";
static const char* g_etcPasswd = "/etc/passwd";
static const char* g_etcPasswdDash = "/etc/passwd-";
static const char* g_etcGroup = "/etc/group";
static const char* g_etcGroupDash = "/etc/group-";
static const char* g_etcAnacronTab = "/etc/anacrontab";
static const char* g_etcCronD = "/etc/cron.d";
static const char* g_etcCronDaily = "/etc/cron.daily";
static const char* g_etcCronHourly = "/etc/cron.hourly";
static const char* g_etcCronMonthly = "/etc/cron.monthly";
static const char* g_etcCronWeekly = "/etc/cron.weekly";
static const char* g_etcMotd = "/etc/motd";
static const char* g_etcFstab = "/etc/fstab";
static const char* g_tmp = "/tmp";
static const char* g_varTmp = "/var/tmp";
static const char* g_media = "/media/";
static const char* g_nodev = "nodev";
static const char* g_nosuid = "nosuid";
static const char* g_noexec = "noexec";
static const char* g_inetd = "inetd";
static const char* g_xinetd = "xinetd";
static const char* g_rshServer = "rsh-server";
static const char* g_nis = "nis";
static const char* g_tftpd = "tftpd";
static const char* g_readAheadFedora = "readahead-fedora";
static const char* g_bluetooth = "bluetooth";
static const char* g_isdnUtilsBase = "isdnutils-base";
static const char* g_kdumpTools = "kdump-tools";
static const char* g_dhcpServer = "dhcp-server";
static const char* g_sendmail = "sendmail";
static const char* g_slapd = "slapd";
static const char* g_bind9 = "bind9";
static const char* g_dovecotCore = "dovecot-core";
static const char* g_auditd = "auditd";
static char* g_prelink = "prelink";
static char* g_talk = "talk";
static char* g_cron = "cron";

static long g_minDaysBetweenPasswordChanges = 7;
static long g_maxDaysBetweenPasswordChanges = 365;
static long g_passwordExpirationWarning = 7;
static long g_passwordExpiration = 365;

static const char* g_pass = "\"PASS\"";
static const char* g_fail = "\"FAIL\"";

static OSCONFIG_LOG_HANDLE g_log = NULL;

static atomic_int g_referenceCount = 0;
static unsigned int g_maxPayloadSizeBytes = 0;

static OSCONFIG_LOG_HANDLE SecurityBaselineGetLog(void)
{
    return g_log;
}

void SecurityBaselineInitialize(void)
{
    g_log = OpenLog(g_securityBaselineLogFile, g_securityBaselineRolledLogFile);

    OsConfigLogInfo(SecurityBaselineGetLog(), "%s initialized", g_securityBaselineModuleName);
}

void SecurityBaselineShutdown(void)
{
    OsConfigLogInfo(SecurityBaselineGetLog(), "%s shutting down", g_securityBaselineModuleName);
    
    CloseLog(&g_log);
}

static int AuditEnsurePermissionsOnEtcIssue(void)
{
    return CheckFileAccess(g_etcIssue, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcIssueNet(void)
{
    return CheckFileAccess(g_etcIssueNet, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcHostsAllow(void)
{
    return CheckFileAccess(g_etcHostsAllow, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcHostsDeny(void)
{
    return CheckFileAccess(g_etcHostsDeny, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcSshSshdConfig(void)
{
    return CheckFileAccess(g_etcSshSshdConfig, 0, 0, 600, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcShadow(void)
{
    return CheckFileAccess(g_etcShadow, 0, 42, 400, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcShadowDash(void)
{
    return CheckFileAccess(g_etcShadowDash, 0, 42, 400, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcGShadow(void)
{
    return CheckFileAccess(g_etcGShadow, 0, 42, 400, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcGShadowDash(void)
{
    return CheckFileAccess(g_etcGShadowDash, 0, 42, 400, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcPasswd(void)
{
    return CheckFileAccess(g_etcPasswd, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcPasswdDash(void)
{
    return CheckFileAccess(g_etcPasswdDash, 0, 0, 600, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcGroup(void)
{
    return CheckFileAccess(g_etcGroup, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcGroupDash(void)
{
    return CheckFileAccess(g_etcGroupDash, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcAnacronTab(void)
{
    return CheckFileAccess(g_etcAnacronTab, 0, 0, 600, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcCronD(void)
{
    return CheckFileAccess(g_etcCronD, 0, 0, 700, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcCronDaily(void)
{
    return CheckFileAccess(g_etcCronDaily, 0, 0, 700, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcCronHourly(void)
{
    return CheckFileAccess(g_etcCronHourly, 0, 0, 700, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcCronMonthly(void)
{
    return CheckFileAccess(g_etcCronMonthly, 0, 0, 700, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcCronWeekly(void)
{
    return CheckFileAccess(g_etcCronWeekly, 0, 0, 700, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcMotd(void)
{
    return CheckFileAccess(g_etcMotd, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsureKernelSupportForCpuNx(void)
{
    return (true == IsCpuFlagSupported("nx", SecurityBaselineGetLog())) ? 0 : 1;
}

static int AuditEnsureNodevOptionOnHomePartition(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, "/home", NULL, g_nodev, SecurityBaselineGetLog());
}

static int AuditEnsureNodevOptionOnTmpPartition(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, g_tmp, NULL, g_nodev, SecurityBaselineGetLog());
}

static int AuditEnsureNodevOptionOnVarTmpPartition(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_nodev, SecurityBaselineGetLog());
}

static int AuditEnsureNosuidOptionOnTmpPartition(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, g_tmp, NULL, g_nosuid, SecurityBaselineGetLog());
}

static int AuditEnsureNosuidOptionOnVarTmpPartition(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_nosuid, SecurityBaselineGetLog());
}

static int AuditEnsureNoexecOptionOnVarTmpPartition(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_noexec, SecurityBaselineGetLog());
}

static int AuditEnsureNoexecOptionOnDevShmPartition(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, "/dev/shm", NULL, g_noexec, SecurityBaselineGetLog());
}

static int AuditEnsureNodevOptionEnabledForAllRemovableMedia(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, NULL, g_media, g_nodev, SecurityBaselineGetLog());
}

static int AuditEnsureNoexecOptionEnabledForAllRemovableMedia(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, NULL, g_media, g_noexec, SecurityBaselineGetLog());
}

static int AuditEnsureNosuidOptionEnabledForAllRemovableMedia(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, NULL, g_media, g_nosuid, SecurityBaselineGetLog());
}

static int AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(void)
{
    const char* nfs = "nfs";
    return (CheckFileSystemMountingOption(g_etcFstab, NULL, nfs, g_noexec, SecurityBaselineGetLog()) &&
        (CheckFileSystemMountingOption(g_etcFstab, NULL, nfs, g_nosuid, SecurityBaselineGetLog())));
}

static int AuditEnsureInetdNotInstalled(void)
{
    return !CheckPackageInstalled(g_inetd, SecurityBaselineGetLog());
}

static int AuditEnsureXinetdNotInstalled(void)
{
    return !CheckPackageInstalled(g_xinetd, SecurityBaselineGetLog());
}

static int auditEnsureAllTelnetdPackagesUninstalled(void)
{
    return !CheckPackageInstalled("*telnetd*", SecurityBaselineGetLog());
}

static int AuditEnsureRshServerNotInstalled(void)
{
    return !CheckPackageInstalled(g_rshServer, SecurityBaselineGetLog());
}

static int AuditEnsureNisNotInstalled(void)
{
    return !CheckPackageInstalled(g_nis, SecurityBaselineGetLog());
}

static int AuditEnsureTftpdNotInstalled(void)
{
    return !CheckPackageInstalled(g_tftpd, SecurityBaselineGetLog());
}

static int AuditEnsureReadaheadFedoraNotInstalled(void)
{
    return !CheckPackageInstalled(g_readAheadFedora, SecurityBaselineGetLog());
}

static int AuditEnsureBluetoothHiddNotInstalled(void)
{
    return !CheckPackageInstalled(g_bluetooth, SecurityBaselineGetLog());
}

static int AuditEnsureIsdnUtilsBaseNotInstalled(void)
{
    return !CheckPackageInstalled(g_isdnUtilsBase, SecurityBaselineGetLog());
}

static int AuditEnsureIsdnUtilsKdumpToolsNotInstalled(void)
{
    return !CheckPackageInstalled(g_kdumpTools, SecurityBaselineGetLog());
}

static int AuditEnsureIscDhcpdServerNotInstalled(void)
{
    return !CheckPackageInstalled(g_dhcpServer, SecurityBaselineGetLog());
}

static int AuditEnsureSendmailNotInstalled(void)
{
    return !CheckPackageInstalled(g_sendmail, SecurityBaselineGetLog());
}

static int AuditEnsureSldapdNotInstalled(void)
{
    return !CheckPackageInstalled(g_slapd, SecurityBaselineGetLog());
}

static int AuditEnsureBind9NotInstalled(void)
{
    return !CheckPackageInstalled(g_bind9, SecurityBaselineGetLog());
}

static int AuditEnsureDovecotCoreNotInstalled(void)
{
    return !CheckPackageInstalled(g_dovecotCore, SecurityBaselineGetLog());
}

static int AuditEnsureAuditdInstalled(void)
{
    return CheckPackageInstalled(g_auditd, SecurityBaselineGetLog());
}

static int AuditEnsureAllEtcPasswdGroupsExistInEtcGroup(void)
{
    return CheckAllEtcPasswdGroupsExistInEtcGroup(SecurityBaselineGetLog());
}

static int AuditEnsureNoDuplicateUidsExist(void)
{
    return CheckNoDuplicateUidsExist(SecurityBaselineGetLog());
}

static int AuditEnsureNoDuplicateGidsExist(void)
{
    return CheckNoDuplicateGidsExist(SecurityBaselineGetLog());
}

static int AuditEnsureNoDuplicateUserNamesExist(void)
{
    return CheckNoDuplicateUserNamesExist(SecurityBaselineGetLog());
}

static int AuditEnsureNoDuplicateGroupsExist(void)
{
    return CheckNoDuplicateGroupsExist(SecurityBaselineGetLog());
}

static int AuditEnsureShadowGroupIsEmpty(void)
{
    return CheckShadowGroupIsEmpty(SecurityBaselineGetLog());
}

static int AuditEnsureRootGroupExists(void)
{
    return CheckRootGroupExists(SecurityBaselineGetLog());
}

static int AuditEnsureAllAccountsHavePasswords(void)
{
    return CheckAllUsersHavePasswordsSet(SecurityBaselineGetLog());
}

static int AuditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(void)
{
    return CheckNonRootAccountsHaveUniqueUidsGreaterThanZero(SecurityBaselineGetLog());
}

static int AuditEnsureNoLegacyPlusEntriesInEtcPasswd(void)
{
    return CheckNoLegacyPlusEntriesInEtcPasswd(SecurityBaselineGetLog());
}

static int AuditEnsureNoLegacyPlusEntriesInEtcShadow(void)
{
    return CheckNoLegacyPlusEntriesInEtcShadow(SecurityBaselineGetLog());
}

static int AuditEnsureNoLegacyPlusEntriesInEtcGroup(void)
{
    return CheckNoLegacyPlusEntriesInEtcGroup(SecurityBaselineGetLog());
}

static int AuditEnsureDefaultRootAccountGroupIsGidZero(void)
{
    return CheckDefaultRootAccountGroupIsGidZero(SecurityBaselineGetLog());
}

static int AuditEnsureRootIsOnlyUidZeroAccount(void)
{
    return CheckRootIsOnlyUidZeroAccount(SecurityBaselineGetLog());
}

static int AuditEnsureAllUsersHomeDirectoriesExist(void)
{
    return CheckAllUsersHomeDirectoriesExist(SecurityBaselineGetLog());
}

static int AuditEnsureUsersOwnTheirHomeDirectories(void)
{
    return CheckUsersOwnTheirHomeDirectories(SecurityBaselineGetLog());
}

static int AuditEnsureRestrictedUserHomeDirectories(void)
{
    return CheckRestrictedUserHomeDirectories(750, SecurityBaselineGetLog());
}

static int AuditEnsurePasswordHashingAlgorithm(void)
{
    return CheckPasswordHashingAlgorithm(sha512, SecurityBaselineGetLog());
}

static int AuditEnsureMinDaysBetweenPasswordChanges(void)
{
    return CheckMinDaysBetweenPasswordChanges(g_minDaysBetweenPasswordChanges, SecurityBaselineGetLog());
}

static int AuditEnsureInactivePasswordLockPeriod(void)
{
    return CheckUsersRecordedPasswordChangeDates(SecurityBaselineGetLog());
}

static int AuditEnsureMaxDaysBetweenPasswordChanges(void)
{
    return CheckMaxDaysBetweenPasswordChanges(g_maxDaysBetweenPasswordChanges, SecurityBaselineGetLog());
}

static int AuditEnsurePasswordExpiration(void)
{
    return CheckPasswordExpirationLessThan(g_passwordExpiration, SecurityBaselineGetLog());
}

static int AuditEnsurePasswordExpirationWarning(void)
{
    return CheckPasswordExpirationWarning(g_passwordExpirationWarning, SecurityBaselineGetLog());
}

static int AuditEnsureSystemAccountsAreNonLogin(void)
{
    return CheckSystemAccountsAreNonLogin(SecurityBaselineGetLog());
}

static int AuditEnsureAuthenticationRequiredForSingleUserMode(void)
{
    return CheckRootPasswordForSingleUserMode(SecurityBaselineGetLog());
}

static int AuditEnsurePrelinkIsDisabled(void)
{
    return (!CheckPackageInstalled(g_prelink, SecurityBaselineGetLog()));
}

static int AuditEnsureTalkClientIsNotInstalled(void)
{
    return !CheckPackageInstalled(g_talk, SecurityBaselineGetLog());
}

static int AuditEnsureDotDoesNotAppearInRootsPath(void)
{
    return !FindTextInEnvironmentVariable("PATH", ".", SecurityBaselineGetLog());
}

static int AuditEnsureCronServiceIsEnabled(void)
{
    return (0 == CheckPackageInstalled(g_cron, SecurityBaselineGetLog()) &&
        IsDaemonActive(g_cron, SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int AuditEnsureRemoteLoginWarningBannerIsConfigured(void)
{
    return (!FindTextInFile(g_etcIssueNet, "\\m", SecurityBaselineGetLog()) &&
        !FindTextInFile(g_etcIssueNet, "\\r", SecurityBaselineGetLog()) &&
        !FindTextInFile(g_etcIssueNet, "\\s", SecurityBaselineGetLog()) &&
        !FindTextInFile(g_etcIssueNet, "\\v", SecurityBaselineGetLog()));
}

static int AuditEnsureLocalLoginWarningBannerIsConfigured(void)
{
    return (!FindTextInFile(g_etcIssue, "\\m", SecurityBaselineGetLog()) &&
        !FindTextInFile(g_etcIssue, "\\r", SecurityBaselineGetLog()) &&
        !FindTextInFile(g_etcIssue, "\\s", SecurityBaselineGetLog()) &&
        !FindTextInFile(g_etcIssue, "\\v", SecurityBaselineGetLog()));
}

static int AuditEnsureAuditdServiceIsRunning(void)
{
    return IsDaemonActive(g_auditd, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureSuRestrictedToRootGroup(void)
{
    return FindTextInFile("/etc/pam.d/su", "use_uid", SecurityBaselineGetLog());
}

static int AuditEnsureDefaultUmaskForAllUsers(void)
{
    return CheckLoginUmask("077", SecurityBaselineGetLog());
}

static int AuditEnsureAutomountingDisabled(void)
{
    const char* autofs = "autofs";
    return ((0 != CheckPackageInstalled(autofs, SecurityBaselineGetLog())) && 
        (false == IsDaemonActive(autofs, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureKernelCompiledFromApprovedSources(void)
{
    return (true == CheckOsAndKernelMatchDistro(SecurityBaselineGetLog())) ? 0 : 1;
}

static int AuditEnsureDefaultDenyFirewallPolicyIsSet(void)
{
    return 0; //TBD
}

static int AuditEnsurePacketRedirectSendingIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureIcmpRedirectsIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureSourceRoutedPacketsIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureAcceptingSourceRoutedPacketsIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureIgnoringBogusIcmpBroadcastResponses(void)
{
    return 0; //TBD
}

static int AuditEnsureIgnoringIcmpEchoPingsToMulticast(void)
{
    return 0; //TBD
}

static int AuditEnsureMartianPacketLoggingIsEnabled(void)
{
    return 0; //TBD
}

static int AuditEnsureReversePathSourceValidationIsEnabled(void)
{
    return 0; //TBD
}

static int AuditEnsureTcpSynCookiesAreEnabled(void)
{
    return 0; //TBD
}

static int AuditEnsureSystemNotActingAsNetworkSniffer(void)
{
    return 0; //TBD
}

static int AuditEnsureAllWirelessInterfacesAreDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureIpv6ProtocolIsEnabled(void)
{
    return 0; //TBD
}

static int AuditEnsureDccpIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureSctpIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureDisabledSupportForRds(void)
{
    return 0; //TBD
}

static int AuditEnsureTipcIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureZeroconfNetworkingIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsurePermissionsOnBootloaderConfig(void)
{
    return ((0 == CheckFileAccess("/boot/grub/grub.conf", 0, 0, 400, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess("/boot/grub/grub.cfg", 0, 0, 400, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess("/boot/grub2/grub.cfg", 0, 0, 400, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsurePasswordReuseIsLimited(void)
{
    return 0; //TBD
}

static int AuditEnsureMountingOfUsbStorageDevicesIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureCoreDumpsAreRestricted(void)
{
    return 0; //TBD
}

static int AuditEnsurePasswordCreationRequirements(void)
{
    return 0; //TBD
}

static int AuditEnsureLockoutForFailedPasswordAttempts(void)
{
    return 0; //TBD
}

static int AuditEnsureDisabledInstallationOfCramfsFileSystem(void)
{
    return 0; //TBD
}

static int AuditEnsureDisabledInstallationOfFreevxfsFileSystem(void)
{
    return 0; //TBD
}

static int AuditEnsureDisabledInstallationOfHfsFileSystem(void)
{
    return 0; //TBD
}

static int AuditEnsureDisabledInstallationOfHfsplusFileSystem(void)
{
    return 0; //TBD
}

static int AuditEnsureDisabledInstallationOfJffs2FileSystem(void)
{
    return 0; //TBD
}

static int AuditEnsureVirtualMemoryRandomizationIsEnabled(void)
{
    char* text = LoadStringFromFile("/proc/sys/kernel/randomize_va_space", false, SecurityBaselineGetLog());
    int status = ((0 == strcmp(text, "1") || (0 == strcmp(text, "2")))) ? 0 : ENOENT;
    FREE_MEMORY(text);
    return status;
}

static int AuditEnsureAllBootloadersHavePasswordProtectionEnabled(void)
{
    return 0; //TBD
}

static int AuditEnsureLoggingIsConfigured(void)
{
    return 0; //TBD
}

static int AuditEnsureSyslogPackageIsInstalled(void)
{
    return 0; //TBD
}

static int AuditEnsureSystemdJournaldServicePersistsLogMessages(void)
{
    return 0; //TBD
}

static int AuditEnsureALoggingServiceIsSnabled(void)
{
    return ((IsDaemonActive("rsyslog", SecurityBaselineGetLog()) && 
        (0 != CheckPackageInstalled("syslog-ng", SecurityBaselineGetLog())) && (0 != CheckPackageInstalled("systemd", SecurityBaselineGetLog()))) ||
        (IsDaemonActive("syslog-ng", SecurityBaselineGetLog()) && 
        (0 != CheckPackageInstalled("rsyslog", SecurityBaselineGetLog())) && (0 != CheckPackageInstalled("systemd", SecurityBaselineGetLog()))) ||
        (IsDaemonActive("systemd-journald", SecurityBaselineGetLog()) && 
        (0 == CheckPackageInstalled("systemd", SecurityBaselineGetLog())))) ? 0 : ENOENT;
}

static int AuditEnsureFilePermissionsForAllRsyslogLogFiles(void)
{
    return ((0 == CheckFileAccess("/etc/rsyslog.conf", 0, 0, 644, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess("/etc/syslog-ng/syslog-ng.conf", 0, 0, 644, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureLoggerConfigurationFilesAreRestricted(void)
{
    return 0; //TBD
}

static int AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(void)
{
    return 0; //TBD
}

static int AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(void)
{
    return 0; //TBD
}

static int AuditEnsureRsyslogNotAcceptingRemoteMessages(void)
{
    return 0; //TBD
}

static int AuditEnsureSyslogRotaterServiceIsEnabled(void)
{
    return ((0 == CheckPackageInstalled("logrotate", SecurityBaselineGetLog())) &&
        (IsDaemonActive("logrotate.timer", SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess("/etc/cron.daily/logrotate", 0, 0, 755, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureTelnetServiceIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureRcprshServiceIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureTftpServiceisDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureAtCronIsRestrictedToAuthorizedUsers(void)
{
    return 0; //TBD
}

static int AuditEnsureSshBestPracticeProtocol(void)
{
    return 0; //TBD
}

static int AuditEnsureSshBestPracticeIgnoreRhosts(void)
{
    return 0; //TBD
}

static int AuditEnsureSshLogLevelIsSet(void)
{
    return 0; //TBD
}

static int AuditEnsureSshMaxAuthTriesIsSet(void)
{
    return 0; //TBD
}

static int AuditEnsureSshAccessIsLimited(void)
{
    return 0; //TBD
}

static int AuditEnsureSshRhostsRsaAuthenticationIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureSshHostbasedAuthenticationIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureSshPermitRootLoginIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureSshPermitEmptyPasswordsIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureSshIdleTimeoutIntervalIsConfigured(void)
{
    return 0; //TBD
}

static int AuditEnsureSshLoginGraceTimeIsSet(void)
{
    return 0; //TBD
}

static int AuditEnsureOnlyApprovedMacAlgorithmsAreUsed(void)
{
    return 0; //TBD
}

static int AuditEnsureSshWarningBannerIsEnabled(void)
{
    return 0; //TBD
}

static int AuditEnsureUsersCannotSetSshEnvironmentOptions(void)
{
    return 0; //TBD
}

static int AuditEnsureAppropriateCiphersForSsh(void)
{
    return 0; //TBD
}

static int AuditEnsureAvahiDaemonServiceIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureCupsServiceisDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsurePostfixPackageIsUninstalled(void)
{
    return 0; //TBD
}

static int AuditEnsurePostfixNetworkListeningIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureRpcgssdServiceIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureRpcidmapdServiceIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsurePortmapServiceIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureNetworkFileSystemServiceIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureRpcsvcgssdServiceIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureSnmpServerIsDisabled(void)
{
    return IsDaemonActive("snmpd", SecurityBaselineGetLog()) ? ENOENT : 0;
}

static int AuditEnsureRsynServiceIsDisabled(void)
{
    return IsDaemonActive("rsyncd", SecurityBaselineGetLog()) ? ENOENT : 0;
}

static int AuditEnsureNisServerIsDisabled(void)
{
    return IsDaemonActive("ypserv", SecurityBaselineGetLog()) ? ENOENT : 0;
}

static int AuditEnsureRshClientNotInstalled(void)
{
    return 0; //TBD
}

static int AuditEnsureSmbWithSambaIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureUsersDotFilesArentGroupOrWorldWritable(void)
{
    return 0; //TBD
}

static int AuditEnsureNoUsersHaveDotForwardFiles(void)
{
    return CheckUsersDontHaveDotFiles("forward", SecurityBaselineGetLog());
}

static int AuditEnsureNoUsersHaveDotNetrcFiles(void)
{
    return CheckUsersDontHaveDotFiles("netrc", SecurityBaselineGetLog());
}

static int AuditEnsureNoUsersHaveDotRhostsFiles(void)
{
    return CheckUsersDontHaveDotFiles("rhosts", SecurityBaselineGetLog());
}

static int AuditEnsureRloginServiceIsDisabled(void)
{
    return 0; //TBD
}

static int AuditEnsureUnnecessaryAccountsAreRemoved(void)
{
    return 0; //TBD
}

int AuditSecurityBaseline(void)
{
    return ((0 == AuditEnsurePermissionsOnEtcIssue()) && 
        (0 == AuditEnsurePermissionsOnEtcIssueNet()) && 
        (0 == AuditEnsurePermissionsOnEtcHostsAllow()) && 
        (0 == AuditEnsurePermissionsOnEtcHostsDeny()) &&
        (0 == AuditEnsurePermissionsOnEtcSshSshdConfig()) &&
        (0 == AuditEnsurePermissionsOnEtcShadow()) &&
        (0 == AuditEnsurePermissionsOnEtcShadowDash()) &&
        (0 == AuditEnsurePermissionsOnEtcGShadow()) &&
        (0 == AuditEnsurePermissionsOnEtcGShadowDash()) &&
        (0 == AuditEnsurePermissionsOnEtcPasswd()) &&
        (0 == AuditEnsurePermissionsOnEtcPasswdDash()) &&
        (0 == AuditEnsurePermissionsOnEtcGroup()) &&
        (0 == AuditEnsurePermissionsOnEtcGroupDash()) &&
        (0 == AuditEnsurePermissionsOnEtcAnacronTab()) &&
        (0 == AuditEnsurePermissionsOnEtcCronD()) &&
        (0 == AuditEnsurePermissionsOnEtcCronDaily()) &&
        (0 == AuditEnsurePermissionsOnEtcCronHourly()) &&
        (0 == AuditEnsurePermissionsOnEtcCronMonthly()) &&
        (0 == AuditEnsurePermissionsOnEtcCronWeekly()) &&
        (0 == AuditEnsurePermissionsOnEtcMotd()) &&
        (0 == AuditEnsureKernelSupportForCpuNx()) &&
        (0 == AuditEnsureNodevOptionOnHomePartition()) &&
        (0 == AuditEnsureNodevOptionOnTmpPartition()) &&
        (0 == AuditEnsureNodevOptionOnVarTmpPartition()) &&
        (0 == AuditEnsureNosuidOptionOnTmpPartition()) &&
        (0 == AuditEnsureNosuidOptionOnVarTmpPartition()) &&
        (0 == AuditEnsureNoexecOptionOnVarTmpPartition()) &&
        (0 == AuditEnsureNoexecOptionOnDevShmPartition()) &&
        (0 == AuditEnsureNodevOptionEnabledForAllRemovableMedia()) &&
        (0 == AuditEnsureNoexecOptionEnabledForAllRemovableMedia()) &&
        (0 == AuditEnsureNosuidOptionEnabledForAllRemovableMedia()) &&
        (0 == AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts()) &&
        (0 == AuditEnsureInetdNotInstalled()) &&
        (0 == AuditEnsureXinetdNotInstalled()) &&
        (0 == auditEnsureAllTelnetdPackagesUninstalled()) &&
        (0 == AuditEnsureRshServerNotInstalled()) &&
        (0 == AuditEnsureNisNotInstalled()) &&
        (0 == AuditEnsureTftpdNotInstalled()) &&
        (0 == AuditEnsureReadaheadFedoraNotInstalled()) &&
        (0 == AuditEnsureBluetoothHiddNotInstalled()) &&
        (0 == AuditEnsureIsdnUtilsBaseNotInstalled()) &&
        (0 == AuditEnsureIsdnUtilsKdumpToolsNotInstalled()) &&
        (0 == AuditEnsureIscDhcpdServerNotInstalled()) &&
        (0 == AuditEnsureSendmailNotInstalled()) &&
        (0 == AuditEnsureSldapdNotInstalled()) &&
        (0 == AuditEnsureBind9NotInstalled()) &&
        (0 == AuditEnsureDovecotCoreNotInstalled()) &&
        (0 == AuditEnsureAuditdInstalled()) &&
        (0 == AuditEnsureAllEtcPasswdGroupsExistInEtcGroup()) &&
        (0 == AuditEnsureNoDuplicateUidsExist()) &&
        (0 == AuditEnsureNoDuplicateGidsExist()) &&
        (0 == AuditEnsureNoDuplicateUserNamesExist()) &&
        (0 == AuditEnsureNoDuplicateGroupsExist()) &&
        (0 == AuditEnsureShadowGroupIsEmpty()) &&
        (0 == AuditEnsureRootGroupExists()) &&
        (0 == AuditEnsureAllAccountsHavePasswords()) &&
        (0 == AuditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero()) &&
        (0 == AuditEnsureNoLegacyPlusEntriesInEtcPasswd()) &&
        (0 == AuditEnsureNoLegacyPlusEntriesInEtcShadow()) &&
        (0 == AuditEnsureNoLegacyPlusEntriesInEtcGroup()) &&
        (0 == AuditEnsureDefaultRootAccountGroupIsGidZero()) &&
        (0 == AuditEnsureRootIsOnlyUidZeroAccount()) &&
        (0 == AuditEnsureAllUsersHomeDirectoriesExist()) &&
        (0 == AuditEnsureUsersOwnTheirHomeDirectories()) &&
        (0 == AuditEnsureRestrictedUserHomeDirectories()) &&
        (0 == AuditEnsurePasswordHashingAlgorithm()) &&
        (0 == AuditEnsureMinDaysBetweenPasswordChanges()) &&
        (0 == AuditEnsureInactivePasswordLockPeriod()) &&
        (0 == AuditEnsureMaxDaysBetweenPasswordChanges()) &&
        (0 == AuditEnsurePasswordExpiration()) &&
        (0 == AuditEnsurePasswordExpirationWarning()) &&
        (0 == AuditEnsureSystemAccountsAreNonLogin()) &&
        (0 == AuditEnsureAuthenticationRequiredForSingleUserMode()) &&
        (0 == AuditEnsurePrelinkIsDisabled()) &&
        (0 == AuditEnsureTalkClientIsNotInstalled()) &&
        (0 == AuditEnsureDotDoesNotAppearInRootsPath()) &&
        (0 == AuditEnsureCronServiceIsEnabled()) &&
        (0 == AuditEnsureRemoteLoginWarningBannerIsConfigured()) &&
        (0 == AuditEnsureLocalLoginWarningBannerIsConfigured()) &&
        (0 == AuditEnsureAuditdServiceIsRunning()) &&
        (0 == AuditEnsureSuRestrictedToRootGroup()) &&
        (0 == AuditEnsureDefaultUmaskForAllUsers()) &&
        (0 == AuditEnsureAutomountingDisabled()) &&
        (0 == AuditEnsureKernelCompiledFromApprovedSources()) &&
        (0 == AuditEnsureDefaultDenyFirewallPolicyIsSet()) &&
        (0 == AuditEnsurePacketRedirectSendingIsDisabled()) &&
        (0 == AuditEnsureIcmpRedirectsIsDisabled()) &&
        (0 == AuditEnsureSourceRoutedPacketsIsDisabled()) &&
        (0 == AuditEnsureAcceptingSourceRoutedPacketsIsDisabled()) &&
        (0 == AuditEnsureIgnoringBogusIcmpBroadcastResponses()) &&
        (0 == AuditEnsureIgnoringIcmpEchoPingsToMulticast()) &&
        (0 == AuditEnsureMartianPacketLoggingIsEnabled()) &&
        (0 == AuditEnsureReversePathSourceValidationIsEnabled()) &&
        (0 == AuditEnsureTcpSynCookiesAreEnabled()) &&
        (0 == AuditEnsureSystemNotActingAsNetworkSniffer()) &&
        (0 == AuditEnsureAllWirelessInterfacesAreDisabled()) &&
        (0 == AuditEnsureIpv6ProtocolIsEnabled()) &&
        (0 == AuditEnsureDccpIsDisabled()) &&
        (0 == AuditEnsureSctpIsDisabled()) &&
        (0 == AuditEnsureDisabledSupportForRds()) &&
        (0 == AuditEnsureTipcIsDisabled()) &&
        (0 == AuditEnsureZeroconfNetworkingIsDisabled()) &&
        (0 == AuditEnsurePermissionsOnBootloaderConfig()) &&
        (0 == AuditEnsurePasswordReuseIsLimited()) &&
        (0 == AuditEnsureMountingOfUsbStorageDevicesIsDisabled()) &&
        (0 == AuditEnsureCoreDumpsAreRestricted()) &&
        (0 == AuditEnsurePasswordCreationRequirements()) &&
        (0 == AuditEnsureLockoutForFailedPasswordAttempts()) &&
        (0 == AuditEnsureDisabledInstallationOfCramfsFileSystem()) &&
        (0 == AuditEnsureDisabledInstallationOfFreevxfsFileSystem()) &&
        (0 == AuditEnsureDisabledInstallationOfHfsFileSystem()) &&
        (0 == AuditEnsureDisabledInstallationOfHfsplusFileSystem()) &&
        (0 == AuditEnsureDisabledInstallationOfJffs2FileSystem()) &&
        (0 == AuditEnsureVirtualMemoryRandomizationIsEnabled()) &&
        (0 == AuditEnsureAllBootloadersHavePasswordProtectionEnabled()) &&
        (0 == AuditEnsureLoggingIsConfigured()) &&
        (0 == AuditEnsureSyslogPackageIsInstalled()) &&
        (0 == AuditEnsureSystemdJournaldServicePersistsLogMessages()) &&
        (0 == AuditEnsureALoggingServiceIsSnabled()) &&
        (0 == AuditEnsureFilePermissionsForAllRsyslogLogFiles()) &&
        (0 == AuditEnsureLoggerConfigurationFilesAreRestricted()) &&
        (0 == AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup()) &&
        (0 == AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser()) &&
        (0 == AuditEnsureRsyslogNotAcceptingRemoteMessages()) &&
        (0 == AuditEnsureSyslogRotaterServiceIsEnabled()) &&
        (0 == AuditEnsureTelnetServiceIsDisabled()) &&
        (0 == AuditEnsureRcprshServiceIsDisabled()) &&
        (0 == AuditEnsureTftpServiceisDisabled()) &&
        (0 == AuditEnsureAtCronIsRestrictedToAuthorizedUsers()) &&
        (0 == AuditEnsureSshBestPracticeProtocol()) &&
        (0 == AuditEnsureSshBestPracticeIgnoreRhosts()) &&
        (0 == AuditEnsureSshLogLevelIsSet()) &&
        (0 == AuditEnsureSshMaxAuthTriesIsSet()) &&
        (0 == AuditEnsureSshAccessIsLimited()) &&
        (0 == AuditEnsureSshRhostsRsaAuthenticationIsDisabled()) &&
        (0 == AuditEnsureSshHostbasedAuthenticationIsDisabled()) &&
        (0 == AuditEnsureSshPermitRootLoginIsDisabled()) &&
        (0 == AuditEnsureSshPermitEmptyPasswordsIsDisabled()) &&
        (0 == AuditEnsureSshIdleTimeoutIntervalIsConfigured()) &&
        (0 == AuditEnsureSshLoginGraceTimeIsSet()) &&
        (0 == AuditEnsureOnlyApprovedMacAlgorithmsAreUsed()) &&
        (0 == AuditEnsureSshWarningBannerIsEnabled()) &&
        (0 == AuditEnsureUsersCannotSetSshEnvironmentOptions()) &&
        (0 == AuditEnsureAppropriateCiphersForSsh()) &&
        (0 == AuditEnsureAvahiDaemonServiceIsDisabled()) &&
        (0 == AuditEnsureCupsServiceisDisabled()) &&
        (0 == AuditEnsurePostfixPackageIsUninstalled()) &&
        (0 == AuditEnsurePostfixNetworkListeningIsDisabled()) &&
        (0 == AuditEnsureRpcgssdServiceIsDisabled()) &&
        (0 == AuditEnsureRpcidmapdServiceIsDisabled()) &&
        (0 == AuditEnsurePortmapServiceIsDisabled()) &&
        (0 == AuditEnsureNetworkFileSystemServiceIsDisabled()) &&
        (0 == AuditEnsureRpcsvcgssdServiceIsDisabled()) &&
        (0 == AuditEnsureSnmpServerIsDisabled()) &&
        (0 == AuditEnsureRsynServiceIsDisabled()) &&
        (0 == AuditEnsureNisServerIsDisabled()) &&
        (0 == AuditEnsureRshClientNotInstalled()) &&
        (0 == AuditEnsureSmbWithSambaIsDisabled()) &&
        (0 == AuditEnsureUsersDotFilesArentGroupOrWorldWritable()) &&
        (0 == AuditEnsureNoUsersHaveDotForwardFiles()) &&
        (0 == AuditEnsureNoUsersHaveDotNetrcFiles()) &&
        (0 == AuditEnsureNoUsersHaveDotRhostsFiles()) &&
        (0 == AuditEnsureRloginServiceIsDisabled()) &&
        (0 == AuditEnsureUnnecessaryAccountsAreRemoved())) ? 0 : ENOENT;
}

static int RemediateEnsurePermissionsOnEtcIssue(void)
{
    return SetFileAccess(g_etcIssue, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcIssueNet(void)
{
    return SetFileAccess(g_etcIssueNet, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcHostsAllow(void)
{
    return SetFileAccess(g_etcHostsAllow, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcHostsDeny(void)
{
    return SetFileAccess(g_etcHostsDeny, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcSshSshdConfig(void)
{
    return SetFileAccess(g_etcSshSshdConfig, 0, 0, 600, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcShadow(void)
{
    return SetFileAccess(g_etcShadow, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcShadowDash(void)
{
    return SetFileAccess(g_etcShadowDash, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGShadow(void)
{
    return SetFileAccess(g_etcGShadow, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGShadowDash(void)
{
    return SetFileAccess(g_etcGShadowDash, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcPasswd(void)
{
    return SetFileAccess(g_etcPasswd, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcPasswdDash(void)
{
    return SetFileAccess(g_etcPasswdDash, 0, 0, 600, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGroup(void)
{
    return SetFileAccess(g_etcGroup, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGroupDash(void)
{
    return SetFileAccess(g_etcGroupDash, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcAnacronTab(void)
{
    return SetFileAccess(g_etcAnacronTab, 0, 0, 600, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronD(void)
{
    return SetFileAccess(g_etcCronD, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronDaily(void)
{
    return SetFileAccess(g_etcCronDaily, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronHourly(void)
{
    return SetFileAccess(g_etcCronHourly, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronMonthly(void)
{
    return SetFileAccess(g_etcCronMonthly, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronWeekly(void)
{
    return SetFileAccess(g_etcCronWeekly, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcMotd(void)
{
    return SetFileAccess(g_etcMotd, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsureInetdNotInstalled(void)
{
    return UninstallPackage(g_inetd, SecurityBaselineGetLog());
}

static int RemediateEnsureXinetdNotInstalled(void)
{
    return UninstallPackage(g_xinetd, SecurityBaselineGetLog());
}

static int RemediateEnsureRshServerNotInstalled(void)
{
    return UninstallPackage(g_rshServer, SecurityBaselineGetLog());
}

static int RemediateEnsureNisNotInstalled(void)
{
    return UninstallPackage(g_nis, SecurityBaselineGetLog());
}

static int RemediateEnsureTftpdNotInstalled(void)
{
    return UninstallPackage(g_tftpd, SecurityBaselineGetLog());
}

static int RemediateEnsureReadaheadFedoraNotInstalled(void)
{
    return UninstallPackage(g_readAheadFedora, SecurityBaselineGetLog());
}

static int RemediateEnsureBluetoothHiddNotInstalled(void)
{
    return UninstallPackage(g_bluetooth, SecurityBaselineGetLog());
}

static int RemediateEnsureIsdnUtilsBaseNotInstalled(void)
{
    return UninstallPackage(g_isdnUtilsBase, SecurityBaselineGetLog());
}

static int RemediateEnsureIsdnUtilsKdumpToolsNotInstalled(void)
{
    return UninstallPackage(g_kdumpTools, SecurityBaselineGetLog());
}

static int RemediateEnsureIscDhcpdServerNotInstalled(void)
{
    return UninstallPackage(g_dhcpServer, SecurityBaselineGetLog());
}

static int RemediateEnsureSendmailNotInstalled(void)
{
    return UninstallPackage(g_sendmail, SecurityBaselineGetLog());
}

static int RemediateEnsureSldapdNotInstalled(void)
{
    return UninstallPackage(g_slapd, SecurityBaselineGetLog());
}

static int RemediateEnsureBind9NotInstalled(void)
{
    return UninstallPackage(g_bind9, SecurityBaselineGetLog());
}

static int RemediateEnsureDovecotCoreNotInstalled(void)
{
    return UninstallPackage(g_dovecotCore, SecurityBaselineGetLog());
}

static int RemediateEnsureAuditdInstalled(void)
{
    return InstallPackage(g_auditd, SecurityBaselineGetLog());
}

static int RemediateEnsurePrelinkIsDisabled(void)
{
    return UninstallPackage(g_prelink, SecurityBaselineGetLog());
}

static int RemediateEnsureTalkClientIsNotInstalled(void)
{
    return UninstallPackage(g_talk, SecurityBaselineGetLog());
}

static int RemediateEnsureCronServiceIsEnabled(void)
{
    return (0 == InstallPackage(g_cron, SecurityBaselineGetLog()) &&
        EnableAndStartDaemon(g_cron, SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int RemediateEnsureAuditdServiceIsRunning(void)
{
    return (0 == InstallPackage(g_auditd, SecurityBaselineGetLog()) &&
        EnableAndStartDaemon(g_auditd, SecurityBaselineGetLog())) ? 0 : ENOENT;
}

int RemediateSecurityBaseline(void)
{
    return ((0 == RemediateEnsurePermissionsOnEtcIssue()) && 
        (0 == RemediateEnsurePermissionsOnEtcIssueNet()) &&
        (0 == RemediateEnsurePermissionsOnEtcHostsAllow()) && 
        (0 == RemediateEnsurePermissionsOnEtcHostsDeny()) &&
        (0 == RemediateEnsurePermissionsOnEtcSshSshdConfig()) &&
        (0 == RemediateEnsurePermissionsOnEtcShadow()) &&
        (0 == RemediateEnsurePermissionsOnEtcShadowDash()) &&
        (0 == RemediateEnsurePermissionsOnEtcGShadow()) &&
        (0 == RemediateEnsurePermissionsOnEtcGShadowDash()) &&
        (0 == RemediateEnsurePermissionsOnEtcPasswd()) &&
        (0 == RemediateEnsurePermissionsOnEtcPasswdDash()) &&
        (0 == RemediateEnsurePermissionsOnEtcGroup()) &&
        (0 == RemediateEnsurePermissionsOnEtcGroupDash()) &&
        (0 == RemediateEnsurePermissionsOnEtcAnacronTab()) &&
        (0 == RemediateEnsurePermissionsOnEtcCronD()) &&
        (0 == RemediateEnsurePermissionsOnEtcCronDaily()) &&
        (0 == RemediateEnsurePermissionsOnEtcCronHourly()) &&
        (0 == RemediateEnsurePermissionsOnEtcCronMonthly()) &&
        (0 == RemediateEnsurePermissionsOnEtcCronWeekly()) &&
        (0 == RemediateEnsurePermissionsOnEtcMotd()) &&
        (0 == RemediateEnsureInetdNotInstalled()) &&
        (0 == RemediateEnsureXinetdNotInstalled()) &&
        (0 == RemediateEnsureRshServerNotInstalled()) &&
        (0 == RemediateEnsureNisNotInstalled()) &&
        (0 == RemediateEnsureTftpdNotInstalled()) &&
        (0 == RemediateEnsureReadaheadFedoraNotInstalled()) &&
        (0 == RemediateEnsureBluetoothHiddNotInstalled()) &&
        (0 == RemediateEnsureIsdnUtilsBaseNotInstalled()) &&
        (0 == RemediateEnsureIsdnUtilsKdumpToolsNotInstalled()) &&
        (0 == RemediateEnsureIscDhcpdServerNotInstalled()) &&
        (0 == RemediateEnsureSendmailNotInstalled()) &&
        (0 == RemediateEnsureSldapdNotInstalled()) &&
        (0 == RemediateEnsureBind9NotInstalled()) &&
        (0 == RemediateEnsureDovecotCoreNotInstalled()) &&
        (0 == RemediateEnsureAuditdInstalled()) &&
        (0 == RemediateEnsurePrelinkIsDisabled()) &&
        (0 == RemediateEnsureTalkClientIsNotInstalled()) &&
        (0 == RemediateEnsureCronServiceIsEnabled()) &&
        (0 == RemediateEnsureAuditdServiceIsRunning())) ? 0 : ENOENT;
}

MMI_HANDLE SecurityBaselineMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = (MMI_HANDLE)g_securityBaselineModuleName;
    g_maxPayloadSizeBytes = maxPayloadSizeBytes;
    ++g_referenceCount;
    OsConfigLogInfo(SecurityBaselineGetLog(), "MmiOpen(%s, %d) returning %p", clientName, maxPayloadSizeBytes, handle);
    return handle;
}

static bool IsValidSession(MMI_HANDLE clientSession)
{
    return ((NULL == clientSession) || (0 != strcmp(g_securityBaselineModuleName, (char*)clientSession)) || (g_referenceCount <= 0)) ? false : true;
}

void SecurityBaselineMmiClose(MMI_HANDLE clientSession)
{
    if (IsValidSession(clientSession))
    {
        --g_referenceCount;
        OsConfigLogInfo(SecurityBaselineGetLog(), "MmiClose(%p)", clientSession);
    }
    else
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiClose() called outside of a valid session");
    }
}

int SecurityBaselineMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = EINVAL;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGetInfo(%s, %p, %p) called with invalid arguments", clientName, payload, payloadSizeBytes);
        return status;
    }
    
    *payload = NULL;
    *payloadSizeBytes = (int)strlen(g_securityBaselineModuleInfo);
    
    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
    if (*payload)
    {
        memcpy(*payload, g_securityBaselineModuleInfo, *payloadSizeBytes);
        status = MMI_OK;
    }
    else
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(SecurityBaselineGetLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int SecurityBaselineMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    char* buffer = NULL;
    const char* result = NULL;
    const size_t length = strlen(g_pass) + 1;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (NULL == (buffer = malloc(length)))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s) failed due to out of memory condition", componentName, objectName);
        status = ENOMEM;
        return status;
    }

    memset(buffer, 0, length);

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }
    else if (0 != strcmp(componentName, g_securityBaselineComponentName))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }
    else
    {
        if (0 == strcmp(objectName, g_auditSecurityBaselineObject))
        {
            result = AuditSecurityBaseline() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcIssueObject))
        {
            result = AuditEnsurePermissionsOnEtcIssue() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcIssueNetObject))
        {
            result = AuditEnsurePermissionsOnEtcIssueNet() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcHostsAllowObject))
        {
            result = AuditEnsurePermissionsOnEtcHostsAllow() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcHostsDenyObject))
        {
            result = AuditEnsurePermissionsOnEtcHostsDeny() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcSshSshdConfigObject))
        {
            result = AuditEnsurePermissionsOnEtcSshSshdConfig() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcShadowObject))
        {
            result = AuditEnsurePermissionsOnEtcShadow() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcShadowDashObject))
        {
            result = AuditEnsurePermissionsOnEtcShadowDash() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGShadowObject))
        {
            result = AuditEnsurePermissionsOnEtcGShadow() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGShadowDashObject))
        {
            result = AuditEnsurePermissionsOnEtcGShadowDash() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcPasswdObject))
        {
            result = AuditEnsurePermissionsOnEtcPasswd() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcPasswdDashObject))
        {
            result = AuditEnsurePermissionsOnEtcPasswdDash() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGroupObject))
        {
            result = AuditEnsurePermissionsOnEtcGroup() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGroupDashObject))
        {
            result = AuditEnsurePermissionsOnEtcGroupDash() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcAnacronTabObject))
        {
            result = AuditEnsurePermissionsOnEtcAnacronTab() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronDObject))
        {
            result = AuditEnsurePermissionsOnEtcCronD() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronDailyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronDaily() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronHourlyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronHourly() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronMonthlyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronMonthly() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronWeeklyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronWeekly() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcMotdObject))
        {
            result = AuditEnsurePermissionsOnEtcMotd() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureKernelSupportForCpuNxObject))
        {
            result = AuditEnsureKernelSupportForCpuNx() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionOnHomePartitionObject))
        {
            result = AuditEnsureNodevOptionOnHomePartition() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionOnTmpPartitionObject))
        {
            result = AuditEnsureNodevOptionOnTmpPartition() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionOnVarTmpPartitionObject))
        {
            result = AuditEnsureNodevOptionOnVarTmpPartition() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNosuidOptionOnTmpPartitionObject))
        {
            result = AuditEnsureNosuidOptionOnTmpPartition() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNosuidOptionOnVarTmpPartitionObject))
        {
            result = AuditEnsureNosuidOptionOnVarTmpPartition() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecOptionOnVarTmpPartitionObject))
        {
            result = AuditEnsureNoexecOptionOnVarTmpPartition() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecOptionOnDevShmPartitionObject))
        {
            result = AuditEnsureNoexecOptionOnDevShmPartition() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNodevOptionEnabledForAllRemovableMediaObject))
        {
            result = AuditEnsureNodevOptionEnabledForAllRemovableMedia() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecOptionEnabledForAllRemovableMediaObject))
        {
            result = AuditEnsureNoexecOptionEnabledForAllRemovableMedia() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNosuidOptionEnabledForAllRemovableMediaObject))
        {
            result = AuditEnsureNosuidOptionEnabledForAllRemovableMedia() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject))
        {
            result = AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureInetdNotInstalledObject))
        {
            result = AuditEnsureInetdNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureXinetdNotInstalledObject))
        {
            result = AuditEnsureXinetdNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllTelnetdPackagesUninstalledObject))
        {
            result = auditEnsureAllTelnetdPackagesUninstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRshServerNotInstalledObject))
        {
            result = AuditEnsureRshServerNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNisNotInstalledObject))
        {
            result = AuditEnsureNisNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureTftpdNotInstalledObject))
        {
            result = AuditEnsureTftpdNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureReadaheadFedoraNotInstalledObject))
        {
            result = AuditEnsureReadaheadFedoraNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureBluetoothHiddNotInstalledObject))
        {
            result = AuditEnsureBluetoothHiddNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureIsdnUtilsBaseNotInstalledObject))
        {
            result = AuditEnsureIsdnUtilsBaseNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureIsdnUtilsKdumpToolsNotInstalledObject))
        {
            result = AuditEnsureIsdnUtilsKdumpToolsNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureIscDhcpdServerNotInstalledObject))
        {
            result = AuditEnsureIscDhcpdServerNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSendmailNotInstalledObject))
        {
            result = AuditEnsureSendmailNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSldapdNotInstalledObject))
        {
            result = AuditEnsureSldapdNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureBind9NotInstalledObject))
        {
            result = AuditEnsureBind9NotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDovecotCoreNotInstalledObject))
        {
            result = AuditEnsureDovecotCoreNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAuditdInstalledObject))
        {
            result = AuditEnsureAuditdInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllEtcPasswdGroupsExistInEtcGroupObject))
        {
            result = AuditEnsureAllEtcPasswdGroupsExistInEtcGroup() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateUidsExistObject))
        {
            result = AuditEnsureNoDuplicateUidsExist() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateGidsExistObject))
        {
            result = AuditEnsureNoDuplicateGidsExist() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateUserNamesExistObject))
        {
            result = AuditEnsureNoDuplicateUserNamesExist() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoDuplicateGroupsExistObject))
        {
            result = AuditEnsureNoDuplicateGroupsExist() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureShadowGroupIsEmptyObject))
        {
            result = AuditEnsureShadowGroupIsEmpty() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRootGroupExistsObject))
        {
            result = AuditEnsureRootGroupExists() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllAccountsHavePasswordsObject))
        {
            result = AuditEnsureAllAccountsHavePasswords() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject))
        {
            result = AuditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoLegacyPlusEntriesInEtcPasswdObject))
        {
            result = AuditEnsureNoLegacyPlusEntriesInEtcPasswd() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoLegacyPlusEntriesInEtcShadowObject))
        {
            result = AuditEnsureNoLegacyPlusEntriesInEtcShadow() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoLegacyPlusEntriesInEtcGroupObject))
        {
            result = AuditEnsureNoLegacyPlusEntriesInEtcGroup() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDefaultRootAccountGroupIsGidZeroObject))
        {
            result = AuditEnsureDefaultRootAccountGroupIsGidZero() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRootIsOnlyUidZeroAccountObject))
        {
            result = AuditEnsureRootIsOnlyUidZeroAccount() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllUsersHomeDirectoriesExistObject))
        {
            result = AuditEnsureAllUsersHomeDirectoriesExist() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureUsersOwnTheirHomeDirectoriesObject))
        {
            result = AuditEnsureUsersOwnTheirHomeDirectories() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRestrictedUserHomeDirectoriesObject))
        {
            result = AuditEnsureRestrictedUserHomeDirectories() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordHashingAlgorithmObject))
        {
            result = AuditEnsurePasswordHashingAlgorithm() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureMinDaysBetweenPasswordChangesObject))
        {
            result = AuditEnsureMinDaysBetweenPasswordChanges() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureInactivePasswordLockPeriodObject))
        {
            result = AuditEnsureInactivePasswordLockPeriod() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditMaxDaysBetweenPasswordChangesObject))
        {
            result = AuditEnsureMaxDaysBetweenPasswordChanges() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordExpirationObject))
        {
            result = AuditEnsurePasswordExpiration() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordExpirationWarningObject))
        {
            result = AuditEnsurePasswordExpirationWarning() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSystemAccountsAreNonLoginObject))
        {
            result = AuditEnsureSystemAccountsAreNonLogin() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAuthenticationRequiredForSingleUserModeObject))
        {
            result = AuditEnsureAuthenticationRequiredForSingleUserMode() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePrelinkIsDisabledObject))
        {
            result = AuditEnsurePrelinkIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureTalkClientIsNotInstalledObject))
        {
            result = AuditEnsureTalkClientIsNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDotDoesNotAppearInRootsPathObject))
        {
            result = AuditEnsureDotDoesNotAppearInRootsPath() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureCronServiceIsEnabledObject))
        {
            result = AuditEnsureCronServiceIsEnabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRemoteLoginWarningBannerIsConfiguredObject))
        {
            result = AuditEnsureRemoteLoginWarningBannerIsConfigured() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureLocalLoginWarningBannerIsConfiguredObject))
        {
            result = AuditEnsureLocalLoginWarningBannerIsConfigured() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAuditdServiceIsRunningObject))
        {
            result = AuditEnsureAuditdServiceIsRunning() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSuRestrictedToRootGroupObject))
        {
            result = AuditEnsureSuRestrictedToRootGroup() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDefaultUmaskForAllUsersObject))
        {
            result = AuditEnsureDefaultUmaskForAllUsers() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAutomountingDisabledObject))
        {
            result = AuditEnsureAutomountingDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureKernelCompiledFromApprovedSourcesObject))
        {
            result = AuditEnsureKernelCompiledFromApprovedSources() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDefaultDenyFirewallPolicyIsSetObject))
        {
            result = AuditEnsureDefaultDenyFirewallPolicyIsSet() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePacketRedirectSendingIsDisabledObject))
        {
            result = AuditEnsurePacketRedirectSendingIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureIcmpRedirectsIsDisabledObject))
        {
            result = AuditEnsureIcmpRedirectsIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSourceRoutedPacketsIsDisabledObject))
        {
            result = AuditEnsureSourceRoutedPacketsIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAcceptingSourceRoutedPacketsIsDisabledObject))
        {
            result = AuditEnsureAcceptingSourceRoutedPacketsIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureIgnoringBogusIcmpBroadcastResponsesObject))
        {
            result = AuditEnsureIgnoringBogusIcmpBroadcastResponses() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureIgnoringIcmpEchoPingsToMulticastObject))
        {
            result = AuditEnsureIgnoringIcmpEchoPingsToMulticast() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureMartianPacketLoggingIsEnabledObject))
        {
            result = AuditEnsureMartianPacketLoggingIsEnabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureReversePathSourceValidationIsEnabledObject))
        {
            result = AuditEnsureReversePathSourceValidationIsEnabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureTcpSynCookiesAreEnabledObject))
        {
            result = AuditEnsureTcpSynCookiesAreEnabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSystemNotActingAsNetworkSnifferObject))
        {
            result = AuditEnsureSystemNotActingAsNetworkSniffer() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllWirelessInterfacesAreDisabledObject))
        {
            result = AuditEnsureAllWirelessInterfacesAreDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureIpv6ProtocolIsEnabledObject))
        {
            result = AuditEnsureIpv6ProtocolIsEnabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDccpIsDisabledObject))
        {
            result = AuditEnsureDccpIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSctpIsDisabledObject))
        {
            result = AuditEnsureSctpIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledSupportForRdsObject))
        {
            result = AuditEnsureDisabledSupportForRds() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureTipcIsDisabledObject))
        {
            result = AuditEnsureTipcIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureZeroconfNetworkingIsDisabledObject))
        {
            result = AuditEnsureZeroconfNetworkingIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnBootloaderConfigObject))
        {
            result = AuditEnsurePermissionsOnBootloaderConfig() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordReuseIsLimitedObject))
        {
            result = AuditEnsurePasswordReuseIsLimited() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureMountingOfUsbStorageDevicesIsDisabledObject))
        {
            result = AuditEnsureMountingOfUsbStorageDevicesIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureCoreDumpsAreRestrictedObject))
        {
            result = AuditEnsureCoreDumpsAreRestricted() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePasswordCreationRequirementsObject))
        {
            result = AuditEnsurePasswordCreationRequirements() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureLockoutForFailedPasswordAttemptsObject))
        {
            result = AuditEnsureLockoutForFailedPasswordAttempts() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfCramfsFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfCramfsFileSystem() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfFreevxfsFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfFreevxfsFileSystem() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfHfsFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfHfsFileSystem() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfHfsplusFileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfHfsplusFileSystem() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureDisabledInstallationOfJffs2FileSystemObject))
        {
            result = AuditEnsureDisabledInstallationOfJffs2FileSystem() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureVirtualMemoryRandomizationIsEnabledObject))
        {
            result = AuditEnsureVirtualMemoryRandomizationIsEnabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllBootloadersHavePasswordProtectionEnabledObject))
        {
            result = AuditEnsureAllBootloadersHavePasswordProtectionEnabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureLoggingIsConfiguredObject))
        {
            result = AuditEnsureLoggingIsConfigured() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSyslogPackageIsInstalledObject))
        {
            result = AuditEnsureSyslogPackageIsInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSystemdJournaldServicePersistsLogMessagesObject))
        {
            result = AuditEnsureSystemdJournaldServicePersistsLogMessages() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureALoggingServiceIsSnabledObject))
        {
            result = AuditEnsureALoggingServiceIsSnabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureFilePermissionsForAllRsyslogLogFilesObject))
        {
            result = AuditEnsureFilePermissionsForAllRsyslogLogFiles() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureLoggerConfigurationFilesAreRestrictedObject))
        {
            result = AuditEnsureLoggerConfigurationFilesAreRestricted() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject))
        {
            result = AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject))
        {
            result = AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRsyslogNotAcceptingRemoteMessagesObject))
        {
            result = AuditEnsureRsyslogNotAcceptingRemoteMessages() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSyslogRotaterServiceIsEnabledObject))
        {
            result = AuditEnsureSyslogRotaterServiceIsEnabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureTelnetServiceIsDisabledObject))
        {
            result = AuditEnsureTelnetServiceIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRcprshServiceIsDisabledObject))
        {
            result = AuditEnsureRcprshServiceIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureTftpServiceisDisabledObject))
        {
            result = AuditEnsureTftpServiceisDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAtCronIsRestrictedToAuthorizedUsersObject))
        {
            result = AuditEnsureAtCronIsRestrictedToAuthorizedUsers() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshBestPracticeProtocolObject))
        {
            result = AuditEnsureSshBestPracticeProtocol() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshBestPracticeIgnoreRhostsObject))
        {
            result = AuditEnsureSshBestPracticeIgnoreRhosts() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshLogLevelIsSetObject))
        {
            result = AuditEnsureSshLogLevelIsSet() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshMaxAuthTriesIsSetObject))
        {
            result = AuditEnsureSshMaxAuthTriesIsSet() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshAccessIsLimitedObject))
        {
            result = AuditEnsureSshAccessIsLimited() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshRhostsRsaAuthenticationIsDisabledObject))
        {
            result = AuditEnsureSshRhostsRsaAuthenticationIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshHostbasedAuthenticationIsDisabledObject))
        {
            result = AuditEnsureSshHostbasedAuthenticationIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshPermitRootLoginIsDisabledObject))
        {
            result = AuditEnsureSshPermitRootLoginIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshPermitEmptyPasswordsIsDisabledObject))
        {
            result = AuditEnsureSshPermitEmptyPasswordsIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshIdleTimeoutIntervalIsConfiguredObject))
        {
            result = AuditEnsureSshIdleTimeoutIntervalIsConfigured() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshLoginGraceTimeIsSetObject))
        {
            result = AuditEnsureSshLoginGraceTimeIsSet() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject))
        {
            result = AuditEnsureOnlyApprovedMacAlgorithmsAreUsed() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshWarningBannerIsEnabledObject))
        {
            result = AuditEnsureSshWarningBannerIsEnabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureUsersCannotSetSshEnvironmentOptionsObject))
        {
            result = AuditEnsureUsersCannotSetSshEnvironmentOptions() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAppropriateCiphersForSshObject))
        {
            result = AuditEnsureAppropriateCiphersForSsh() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureAvahiDaemonServiceIsDisabledObject))
        {
            result = AuditEnsureAvahiDaemonServiceIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureCupsServiceisDisabledObject))
        {
            result = AuditEnsureCupsServiceisDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePostfixPackageIsUninstalledObject))
        {
            result = AuditEnsurePostfixPackageIsUninstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePostfixNetworkListeningIsDisabledObject))
        {
            result = AuditEnsurePostfixNetworkListeningIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRpcgssdServiceIsDisabledObject))
        {
            result = AuditEnsureRpcgssdServiceIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRpcidmapdServiceIsDisabledObject))
        {
            result = AuditEnsureRpcidmapdServiceIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePortmapServiceIsDisabledObject))
        {
            result = AuditEnsurePortmapServiceIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNetworkFileSystemServiceIsDisabledObject))
        {
            result = AuditEnsureNetworkFileSystemServiceIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRpcsvcgssdServiceIsDisabledObject))
        {
            result = AuditEnsureRpcsvcgssdServiceIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSnmpServerIsDisabledObject))
        {
            result = AuditEnsureSnmpServerIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRsynServiceIsDisabledObject))
        {
            result = AuditEnsureRsynServiceIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNisServerIsDisabledObject))
        {
            result = AuditEnsureNisServerIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRshClientNotInstalledObject))
        {
            result = AuditEnsureRshClientNotInstalled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureSmbWithSambaIsDisabledObject))
        {
            result = AuditEnsureSmbWithSambaIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureUsersDotFilesArentGroupOrWorldWritableObject))
        {
            result = AuditEnsureUsersDotFilesArentGroupOrWorldWritable() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoUsersHaveDotForwardFilesObject))
        {
            result = AuditEnsureNoUsersHaveDotForwardFiles() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoUsersHaveDotNetrcFilesObject))
        {
            result = AuditEnsureNoUsersHaveDotNetrcFiles() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureNoUsersHaveDotRhostsFilesObject))
        {
            result = AuditEnsureNoUsersHaveDotRhostsFiles() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureRloginServiceIsDisabledObject))
        {
            result = AuditEnsureRloginServiceIsDisabled() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsureUnnecessaryAccountsAreRemovedObject))
        {
            result = AuditEnsureUnnecessaryAccountsAreRemoved() ? g_fail : g_pass;
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiGet called for an unsupported object (%s)", objectName);
            status = EINVAL;
        }
    }

    if (MMI_OK == status)
    {
        *payloadSizeBytes = strlen(result);

        if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s) insufficient max size (%d bytes) vs actual size (%d bytes), report will be truncated",
                componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);

            *payloadSizeBytes = g_maxPayloadSizeBytes;
        }

        *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
        if (*payload)
        {
            memcpy(*payload, result, *payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiGet: failed to allocate %d bytes", *payloadSizeBytes + 1);
            *payloadSizeBytes = 0;
            status = ENOMEM;
        }
    }    

    OsConfigLogInfo(SecurityBaselineGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);

    FREE_MEMORY(buffer);

    return status;
}

int SecurityBaselineMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    char* payloadString = NULL;

    int status = MMI_OK;

    // No payload is accepted for now, this may change once the complete Security Baseline is implemented
    if ((NULL == componentName) || (NULL == objectName))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiSet(%s, %s, %s, %d) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiSet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    } 
    else if (0 != strcmp(componentName, g_securityBaselineComponentName))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiSet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }

    if ((MMI_OK == status) && (NULL != payload) && (0 < payloadSizeBytes))
    {
        if (NULL != (payloadString = malloc(payloadSizeBytes + 1)))
        {
            memset(payloadString, 0, payloadSizeBytes + 1);
            memcpy(payloadString, payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "Failed to allocate %d bytes of memory, MmiSet failed", payloadSizeBytes + 1);
            status = ENOMEM;
        }
    }
    
    if (MMI_OK == status)
    {
        if (0 == strcmp(objectName, g_remediateSecurityBaselineObject))
        {
            status = RemediateSecurityBaseline();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcIssueObject))
        {
            status = RemediateEnsurePermissionsOnEtcIssue();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcIssueNetObject))
        {
            status = RemediateEnsurePermissionsOnEtcIssueNet();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcHostsAllowObject))
        {
            status = RemediateEnsurePermissionsOnEtcHostsAllow();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcHostsDenyObject))
        {
            status = RemediateEnsurePermissionsOnEtcHostsDeny();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcSshSshdConfigObject))
        {
            status = RemediateEnsurePermissionsOnEtcSshSshdConfig();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcShadowObject))
        {
            status = RemediateEnsurePermissionsOnEtcShadow();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcShadowDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcShadowDash();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGShadowObject))
        {
            status = RemediateEnsurePermissionsOnEtcGShadow();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGShadowDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcGShadowDash();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcPasswdObject))
        {
            status = RemediateEnsurePermissionsOnEtcPasswd();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcPasswdDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcPasswdDash();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGroupObject))
        {
            status = RemediateEnsurePermissionsOnEtcGroup();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGroupDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcGroupDash();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcAnacronTabObject))
        {
            status = RemediateEnsurePermissionsOnEtcAnacronTab();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronDObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronD();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronDailyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronDaily();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronHourlyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronHourly();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronMonthlyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronMonthly();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronWeeklyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronWeekly();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcMotdObject))
        {
            status = RemediateEnsurePermissionsOnEtcMotd();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureInetdNotInstalledObject))
        {
            status = RemediateEnsureInetdNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureXinetdNotInstalledObject))
        {
            status = RemediateEnsureXinetdNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRshServerNotInstalledObject))
        {
            status = RemediateEnsureRshServerNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNisNotInstalledObject))
        {
            status = RemediateEnsureNisNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTftpdNotInstalledObject))
        {
            status = RemediateEnsureTftpdNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureReadaheadFedoraNotInstalledObject))
        {
            status = RemediateEnsureReadaheadFedoraNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureBluetoothHiddNotInstalledObject))
        {
            status = RemediateEnsureBluetoothHiddNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIsdnUtilsBaseNotInstalledObject))
        {
            status = RemediateEnsureIsdnUtilsBaseNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIsdnUtilsKdumpToolsNotInstalledObject))
        {
            status = RemediateEnsureIsdnUtilsKdumpToolsNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIscDhcpdServerNotInstalledObject))
        {
            status = RemediateEnsureIscDhcpdServerNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSendmailNotInstalledObject))
        {
            status = RemediateEnsureSendmailNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSldapdNotInstalledObject))
        {
            status = RemediateEnsureSldapdNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureBind9NotInstalledObject))
        {
            status = RemediateEnsureBind9NotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDovecotCoreNotInstalledObject))
        {
            status = RemediateEnsureDovecotCoreNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuditdInstalledObject))
        {
            status = RemediateEnsureAuditdInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePrelinkIsDisabledObject))
        {
            status = RemediateEnsurePrelinkIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTalkClientIsNotInstalledObject))
        {
            status = RemediateEnsureTalkClientIsNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureCronServiceIsEnabledObject))
        {
            status = RemediateEnsureCronServiceIsEnabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuditdServiceIsRunningObject))
        {
            status = RemediateEnsureAuditdServiceIsRunning();
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiSet called for an unsupported object name: %s", objectName);
            status = EINVAL;
        }
    }

    FREE_MEMORY(payloadString);

    OsConfigLogInfo(SecurityBaselineGetLog(), "MmiSet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);

    return status;
}

void SecurityBaselineMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}