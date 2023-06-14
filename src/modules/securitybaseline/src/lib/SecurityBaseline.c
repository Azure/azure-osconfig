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

typedef int(*AuditRemediate)(void);

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
// Remaining 125 remediation checks
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
static const char* g_remediateEnsureALoggingServiceIsSnabledObject = "remediateEnsureALoggingServiceIsSnabled";
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
static const char* g_remediateEnsureSshBestPracticeProtocolObject = "remediateEnsureSshBestPracticeProtocol";
static const char* g_remediateEnsureSshBestPracticeIgnoreRhostsObject = "remediateEnsureSshBestPracticeIgnoreRhosts";
static const char* g_remediateEnsureSshLogLevelIsSetObject = "remediateEnsureSshLogLevelIsSet";
static const char* g_remediateEnsureSshMaxAuthTriesIsSetObject = "remediateEnsureSshMaxAuthTriesIsSet";
static const char* g_remediateEnsureSshAccessIsLimitedObject = "remediateEnsureSshAccessIsLimited";
static const char* g_remediateEnsureSshRhostsRsaAuthenticationIsDisabledObject = "remediateEnsureSshRhostsRsaAuthenticationIsDisabled";
static const char* g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject = "remediateEnsureSshHostbasedAuthenticationIsDisabled";
static const char* g_remediateEnsureSshPermitRootLoginIsDisabledObject = "remediateEnsureSshPermitRootLoginIsDisabled";
static const char* g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject = "remediateEnsureSshPermitEmptyPasswordsIsDisabled";
static const char* g_remediateEnsureSshIdleTimeoutIntervalIsConfiguredObject = "remediateEnsureSshIdleTimeoutIntervalIsConfigured";
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
static const char* g_etcEnvironment = "/etc/environment";
static const char* g_etcFstab = "/etc/fstab";
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
static const char* g_tftpd = "tftpd";
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
    return IsCpuFlagSupported("nx", SecurityBaselineGetLog()) ? 0 : ENOENT;
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
    return CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_nodev, SecurityBaselineGetLog());
}

static int AuditEnsureNoexecOptionEnabledForAllRemovableMedia(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_noexec, SecurityBaselineGetLog());
}

static int AuditEnsureNosuidOptionEnabledForAllRemovableMedia(void)
{
    return CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_nosuid, SecurityBaselineGetLog());
}

static int AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(void)
{
    const char* nfs = "nfs";
    return ((0 == CheckFileSystemMountingOption(g_etcFstab, NULL, nfs, g_noexec, SecurityBaselineGetLog())) &&
        (0 == CheckFileSystemMountingOption(g_etcFstab, NULL, nfs, g_nosuid, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureInetdNotInstalled(void)
{
    return (CheckPackageInstalled(g_inetd, SecurityBaselineGetLog()) && 
        CheckPackageInstalled(g_inetUtilsInetd, SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int AuditEnsureXinetdNotInstalled(void)
{
    return CheckPackageInstalled(g_xinetd, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureAllTelnetdPackagesUninstalled(void)
{
    return CheckPackageInstalled("*telnetd*", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureRshServerNotInstalled(void)
{
    return CheckPackageInstalled(g_rshServer, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureNisNotInstalled(void)
{
    return CheckPackageInstalled(g_nis, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureTftpdNotInstalled(void)
{
    return CheckPackageInstalled(g_tftpd, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureReadaheadFedoraNotInstalled(void)
{
    return CheckPackageInstalled(g_readAheadFedora, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureBluetoothHiddNotInstalled(void)
{
    return CheckPackageInstalled(g_bluetooth, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureIsdnUtilsBaseNotInstalled(void)
{
    return CheckPackageInstalled(g_isdnUtilsBase, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureIsdnUtilsKdumpToolsNotInstalled(void)
{
    return CheckPackageInstalled(g_kdumpTools, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureIscDhcpdServerNotInstalled(void)
{
    return CheckPackageInstalled(g_iscDhcpServer, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureSendmailNotInstalled(void)
{
    return CheckPackageInstalled(g_sendmail, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureSldapdNotInstalled(void)
{
    return CheckPackageInstalled(g_slapd, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureBind9NotInstalled(void)
{
    return CheckPackageInstalled(g_bind9, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureDovecotCoreNotInstalled(void)
{
    return CheckPackageInstalled(g_dovecotCore, SecurityBaselineGetLog()) ? 0 : ENOENT;
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
    return CheckRootIsOnlyUidZeroAccount(SecurityBaselineGetLog());
}

static int AuditEnsureNoLegacyPlusEntriesInEtcPasswd(void)
{
    return CheckNoLegacyPlusEntriesInFile("etc/passwd", SecurityBaselineGetLog());
}

static int AuditEnsureNoLegacyPlusEntriesInEtcShadow(void)
{
    return CheckNoLegacyPlusEntriesInFile("etc/shadow", SecurityBaselineGetLog());
}

static int AuditEnsureNoLegacyPlusEntriesInEtcGroup(void)
{
    return CheckNoLegacyPlusEntriesInFile("etc/group", SecurityBaselineGetLog());
}

static int AuditEnsureDefaultRootAccountGroupIsGidZero(void)
{
    return CheckDefaultRootAccountGroupIsGidZero(SecurityBaselineGetLog());
}

static int AuditEnsureRootIsOnlyUidZeroAccount(void)
{
    return ((0 == CheckRootGroupExists(SecurityBaselineGetLog())) && 
        (0 == CheckRootIsOnlyUidZeroAccount(SecurityBaselineGetLog()))) ? 0 : EACCES;
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
    return CheckPackageInstalled(g_prelink, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureTalkClientIsNotInstalled(void)
{
    return CheckPackageInstalled(g_talk, SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureDotDoesNotAppearInRootsPath(void)
{
    const char* path = "PATH";
    const char* dot = ".";

    return ((0 != FindTextInEnvironmentVariable(path, dot, false, SecurityBaselineGetLog()) &&
        (0 != FindMarkedTextInFile("/etc/sudoers", "secure_path", dot, SecurityBaselineGetLog())) &&
        (0 != FindMarkedTextInFile(g_etcEnvironment, path, dot, SecurityBaselineGetLog())) &&
        (0 != FindMarkedTextInFile(g_etcProfile, path, dot, SecurityBaselineGetLog())) &&
        (0 != FindMarkedTextInFile("/root/.profile", path, dot, SecurityBaselineGetLog())))) ? 0 : ENOENT;
}

static int AuditEnsureCronServiceIsEnabled(void)
{
    return (0 == CheckPackageInstalled(g_cron, SecurityBaselineGetLog()) &&
        IsDaemonActive(g_cron, SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int AuditEnsureRemoteLoginWarningBannerIsConfigured(void)
{
    return ((0 != FindTextInFile(g_etcIssueNet, "\\m", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssueNet, "\\r", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssueNet, "\\s", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssueNet, "\\v", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureLocalLoginWarningBannerIsConfigured(void)
{
    return ((0 != FindTextInFile(g_etcIssue, "\\m", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssue, "\\r", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssue, "\\s", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssue, "\\v", SecurityBaselineGetLog()))) ? 0 : ENOENT;
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
    return (CheckPackageInstalled(autofs, SecurityBaselineGetLog()) && 
        (false == IsDaemonActive(autofs, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureKernelCompiledFromApprovedSources(void)
{
    return (true == CheckOsAndKernelMatchDistro(SecurityBaselineGetLog())) ? 0 : 1;
}

static int AuditEnsureDefaultDenyFirewallPolicyIsSet(void)
{
    const char* readIpTables = "iptables -S";

    return ((0 == FindTextInCommandOutput(readIpTables, "-P INPUT DROP", SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(readIpTables, "-P FORWARD DROP", SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(readIpTables, "-P OUTPUT DROP", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsurePacketRedirectSendingIsDisabled(void)
{
    const char* command = "sysctl -a";
    return ((0 == FindTextInCommandOutput(command, "net.ipv4.conf.all.send_redirects = 0", SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv4.conf.default.send_redirects = 0", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureIcmpRedirectsIsDisabled(void)
{
    const char* command = "sysctl -a";
    return ((0 == FindTextInCommandOutput(command, "net.ipv4.conf.default.accept_redirects = 0", SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv6.conf.default.accept_redirects = 0", SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv4.conf.all.accept_redirects = 0", SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv6.conf.all.accept_redirects = 0", SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv4.conf.default.secure_redirects = 0", SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv4.conf.all.secure_redirects = 0", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSourceRoutedPacketsIsDisabled(void)
{
    return ((EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/conf/all/accept_source_route", '#', "0", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv6/conf/all/accept_source_route", '#', "0", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureAcceptingSourceRoutedPacketsIsDisabled(void)
{
    return ((EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/conf/all/accept_source_route", '#', "0", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv6/conf/default/accept_source_route", '#', "0", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureIgnoringBogusIcmpBroadcastResponses(void)
{
    return (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/icmp_ignore_bogus_error_responses", '#', "1", SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int AuditEnsureIgnoringIcmpEchoPingsToMulticast(void)
{
    return (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/icmp_echo_ignore_broadcasts", '#', "1", SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int AuditEnsureMartianPacketLoggingIsEnabled(void)
{
    const char* command = "sysctl -a";
    return ((0 == FindTextInCommandOutput(command, "net.ipv4.conf.all.log_martians = 1", SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv4.conf.default.log_martians = 1", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureReversePathSourceValidationIsEnabled(void)
{
    return ((EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/conf/all/rp_filter", '#', "1", SecurityBaselineGetLog())) && 
        (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/conf/default/rp_filter", '#', "1", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureTcpSynCookiesAreEnabled(void)
{
    return (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/tcp_syncookies", '#', "1", SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int AuditEnsureSystemNotActingAsNetworkSniffer(void)
{
    const char* command = "/sbin/ip addr list";
    const char* text = "PROMISC";

    return (FindTextInCommandOutput(command, text, SecurityBaselineGetLog()) &&
        (0 == CheckLineNotFoundOrCommentedOut("/etc/network/interfaces", '#', text, SecurityBaselineGetLog())) &&
        (0 == CheckLineNotFoundOrCommentedOut("/etc/rc.local", '#', text, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureAllWirelessInterfacesAreDisabled(void)
{
    return FindTextInCommandOutput("/sbin/iwconfig 2>&1 | /bin/egrep -v 'no wireless extensions|not found'", "Frequency", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureIpv6ProtocolIsEnabled(void)
{
    static const char* etcSysCtlConf = "/etc/sysctl.conf";

    return ((0 == CheckFileExists("/proc/net/if_inet6", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(etcSysCtlConf, '#', "net.ipv6.conf.all.disable_ipv6 = 0", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(etcSysCtlConf, '#', "net.ipv6.conf.default.disable_ipv6 = 0", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureDccpIsDisabled(void)
{
    return FindTextInFolder(g_etcModProbeD, "install dccp /bin/true", SecurityBaselineGetLog());
}

static int AuditEnsureSctpIsDisabled(void)
{
    return FindTextInFolder(g_etcModProbeD, "install sctp /bin/true", SecurityBaselineGetLog());
}

static int AuditEnsureDisabledSupportForRds(void)
{
    return FindTextInFolder(g_etcModProbeD, "install rds /bin/true", SecurityBaselineGetLog());
}

static int AuditEnsureTipcIsDisabled(void)
{
    return FindTextInFolder(g_etcModProbeD, "install tipc /bin/true", SecurityBaselineGetLog());
}

static int AuditEnsureZeroconfNetworkingIsDisabled(void)
{
    return CheckLineNotFoundOrCommentedOut("/etc/network/interfaces", '#', "ipv4ll", SecurityBaselineGetLog());
}

static int AuditEnsurePermissionsOnBootloaderConfig(void)
{
    return ((0 == CheckFileAccess("/boot/grub/grub.conf", 0, 0, 400, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess("/boot/grub/grub.cfg", 0, 0, 400, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess("/boot/grub2/grub.cfg", 0, 0, 400, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsurePasswordReuseIsLimited(void)
{
    //TBD: refine this and expand to other distros
    return (EEXIST == CheckLineNotFoundOrCommentedOut("/etc/pam.d/common-password", '#', "remember", SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int AuditEnsureMountingOfUsbStorageDevicesIsDisabled(void)
{
    return FindTextInFolder(g_etcModProbeD, "install usb-storage /bin/true", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureCoreDumpsAreRestricted(void)
{
    const char* fsSuidDumpable = "fs.suid_dumpable";

    return (((0 == FindTextInEnvironmentVariable(fsSuidDumpable, "0 ", true, SecurityBaselineGetLog())) ||
        (0 == FindMarkedTextInFile(g_etcEnvironment, fsSuidDumpable, "0", SecurityBaselineGetLog())) ||
        (0 == FindMarkedTextInFile(g_etcProfile, fsSuidDumpable, "0", SecurityBaselineGetLog()))) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut("/etc/security/limits.conf", '#', "hard core 0", SecurityBaselineGetLog())) &&
        (0 == FindTextInFolder("/etc/security/limits.d", "fs.suid_dumpable = 0", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsurePasswordCreationRequirements(void)
{
    const char* etcSecurityPwQualityConf = "/etc/security/pwquality.conf";

    return ((EEXIST == CheckLineNotFoundOrCommentedOut(etcSecurityPwQualityConf, '#', "minlen=14", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(etcSecurityPwQualityConf, '#', "minclass=4", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(etcSecurityPwQualityConf, '#', "dcredit=-1", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(etcSecurityPwQualityConf, '#', "ucredit=-1", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(etcSecurityPwQualityConf, '#', "ocredit=-1", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(etcSecurityPwQualityConf, '#', "lcredit=-1", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureLockoutForFailedPasswordAttempts(void)
{
    //TBD: refine this and expand to other distros
    return ((EEXIST == CheckLineNotFoundOrCommentedOut("/etc/pam.d/common-auth", '#', "pam_tally", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut("/etc/pam.d/password-auth", '#', "pam_faillock", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut("/etc/pam.d/system-auth", '#', "pam_faillock", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureDisabledInstallationOfCramfsFileSystem(void)
{
    return FindTextInFolder(g_etcModProbeD, "install cramfs", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureDisabledInstallationOfFreevxfsFileSystem(void)
{
    return FindTextInFolder(g_etcModProbeD, "install freevxfs", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureDisabledInstallationOfHfsFileSystem(void)
{
    return FindTextInFolder(g_etcModProbeD, "install hfs", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureDisabledInstallationOfHfsplusFileSystem(void)
{
    return FindTextInFolder(g_etcModProbeD, "install hfsplus", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureDisabledInstallationOfJffs2FileSystem(void)
{
    return FindTextInFolder(g_etcModProbeD, "install jffs2", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureVirtualMemoryRandomizationIsEnabled(void)
{
    return ((0 == CompareFileContents("/proc/sys/kernel/randomize_va_space", "2", SecurityBaselineGetLog())) ||
        (0 == CompareFileContents("/proc/sys/kernel/randomize_va_space", "1", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureAllBootloadersHavePasswordProtectionEnabled(void)
{
    const char* password = "password";
    return ((EEXIST == CheckLineNotFoundOrCommentedOut("/boot/grub/grub.cfg", '#', password, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut("/boot/grub/grub.conf", '#', password, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut("/boot/grub2/grub.conf", '#', password, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureLoggingIsConfigured(void)
{
    return CheckFileExists("/var/log/syslog", SecurityBaselineGetLog());
}

static int AuditEnsureSyslogPackageIsInstalled(void)
{
    return ((0 == CheckPackageInstalled(g_syslog, SecurityBaselineGetLog())) ||
        (0 == CheckPackageInstalled(g_rsyslog, SecurityBaselineGetLog())) ||
        (0 == CheckPackageInstalled(g_syslogNg, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSystemdJournaldServicePersistsLogMessages(void)
{
    return ((0 == CheckPackageInstalled(g_systemd, SecurityBaselineGetLog())) && 
        (0 == CheckDirectoryAccess("/var/log/journal", 0, -1, 2775, false, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureALoggingServiceIsSnabled(void)
{
    return (((0 == CheckPackageInstalled(g_rsyslog, SecurityBaselineGetLog())) && IsDaemonActive(g_rsyslog, SecurityBaselineGetLog())) ||
        ((0 == CheckPackageInstalled(g_syslogNg, SecurityBaselineGetLog())) && IsDaemonActive(g_syslogNg, SecurityBaselineGetLog())) ||
        ((0 == CheckPackageInstalled(g_systemd, SecurityBaselineGetLog())) && IsDaemonActive("systemd-journald", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureFilePermissionsForAllRsyslogLogFiles(void)
{
    return ((0 == CheckFileAccess(g_etcRsyslogConf, 0, 0, 644, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess(g_etcSyslogNgSyslogNgConf, 0, 0, 644, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureLoggerConfigurationFilesAreRestricted(void)
{
    return ((0 == CheckFileAccess(g_etcSyslogNgSyslogNgConf, 0, 0, 644, SecurityBaselineGetLog())) && 
        (0 == CheckFileAccess(g_etcRsyslogConf, 0, 0, 644, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(void)
{
    return ((0 == FindTextInFile(g_etcRsyslogConf, "FileGroup adm", SecurityBaselineGetLog())) &&
        (0 != CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "FileGroup adm", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(void)
{
    return ((0 == FindTextInFile(g_etcRsyslogConf, "FileOwner syslog", SecurityBaselineGetLog())) &&
        (0 != CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "FileOwner syslog", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureRsyslogNotAcceptingRemoteMessages(void)
{
    return ((0 == CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "ModLoad imudp", SecurityBaselineGetLog())) &&
        (0 == CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "ModLoad imtcp", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSyslogRotaterServiceIsEnabled(void)
{
    return ((0 == CheckPackageInstalled("logrotate", SecurityBaselineGetLog())) &&
        IsDaemonActive("logrotate.timer", SecurityBaselineGetLog()) &&
        (0 == CheckFileAccess("/etc/cron.daily/logrotate", 0, 0, 755, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureTelnetServiceIsDisabled(void)
{
    return CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "telnet", SecurityBaselineGetLog());
}                                                                         

static int AuditEnsureRcprshServiceIsDisabled(void)
{
    return CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "shell", SecurityBaselineGetLog());
}

static int AuditEnsureTftpServiceisDisabled(void)
{
    return CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "tftp", SecurityBaselineGetLog());
}

static int AuditEnsureAtCronIsRestrictedToAuthorizedUsers(void)
{
    const char* etcCronAllow = "/etc/cron.allow";
    const char* etcAtAllow = "/etc/at.allow";

    return ((EEXIST == CheckFileExists("/etc/cron.deny", SecurityBaselineGetLog())) &&
        (EEXIST == CheckFileExists("/etc/at.deny", SecurityBaselineGetLog())) &&
        (0 == CheckFileExists(etcCronAllow, SecurityBaselineGetLog())) &&
        (0 == CheckFileExists(etcAtAllow, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess(etcCronAllow, 0, 0, 600, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess(etcAtAllow, 0, 0, 600, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSshBestPracticeProtocol(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "Protocol 2", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSshBestPracticeIgnoreRhosts(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "IgnoreRhosts yes", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSshLogLevelIsSet(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "LogLevel INFO", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSshMaxAuthTriesIsSet(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "MaxAuthTries 6", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSshAccessIsLimited(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "AllowUsers", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "AllowGroups", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "DenyUsers", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "DenyGroups", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSshRhostsRsaAuthenticationIsDisabled(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "RhostsRSAAuthentication no", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSshHostbasedAuthenticationIsDisabled(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "HostbasedAuthentication no", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSshPermitRootLoginIsDisabled(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "PermitRootLogin no", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSshPermitEmptyPasswordsIsDisabled(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "PermitEmptyPasswords no", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureSshIdleTimeoutIntervalIsConfigured(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        ((EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "ClientAliveCountMax 0", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "ClientAliveInterval", SecurityBaselineGetLog())))) ? 0 : ENOENT;
}

static int AuditEnsureSshLoginGraceTimeIsSet(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "LoginGraceTime", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureOnlyApprovedMacAlgorithmsAreUsed(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        ((EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "MACs", SecurityBaselineGetLog())) &&
        ((EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "bhmac-sha2-512-etm@openssh.com", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "bhmac-sha2-256-etm@openssh.com", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "bhmac-sha2-512", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "bhmac-sha2-256", SecurityBaselineGetLog()))))) ? 0 : ENOENT;
}

static int AuditEnsureSshWarningBannerIsEnabled(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "Banner /etc/azsec/banner.txt", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureUsersCannotSetSshEnvironmentOptions(void)
{
    return CheckLineNotFoundOrCommentedOut("/etc/ssh/ssh_config", '#', "PermitUserEnvironment yes", SecurityBaselineGetLog());
}

static int AuditEnsureAppropriateCiphersForSsh(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        ((EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "Ciphers", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "aes128-ctr", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "aes192-ctr", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "aes256-ctr", SecurityBaselineGetLog())))) ? 0 : ENOENT;
}

static int AuditEnsureAvahiDaemonServiceIsDisabled(void)
{
    return (false == IsDaemonActive("avahi-daemon", SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int AuditEnsureCupsServiceisDisabled(void)
{
    const char* cups = "cups";
    return (CheckPackageInstalled(cups, SecurityBaselineGetLog()) &&
        (false == IsDaemonActive(cups, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsurePostfixPackageIsUninstalled(void)
{
    return CheckPackageInstalled("postfix", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsurePostfixNetworkListeningIsDisabled(void)
{
    return (0 == CheckFileExists("/etc/postfix/main.cf", SecurityBaselineGetLog())) ? 
        FindTextInFile("/etc/postfix/main.cf", "inet_interfaces localhost", SecurityBaselineGetLog()) : 0;
}

static int AuditEnsureRpcgssdServiceIsDisabled(void)
{
    return (false == IsDaemonActive("rpcgssd", SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int AuditEnsureRpcidmapdServiceIsDisabled(void)
{
    return (false == IsDaemonActive("rpcidmapd", SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int AuditEnsurePortmapServiceIsDisabled(void)
{
    return ((false == IsDaemonActive("rpcbind", SecurityBaselineGetLog())) &&
        (false == IsDaemonActive("rpcbind.service", SecurityBaselineGetLog())) &&
        (false == IsDaemonActive("rpcbind.socket", SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int AuditEnsureNetworkFileSystemServiceIsDisabled(void)
{
    return IsDaemonActive("nfs-server", SecurityBaselineGetLog()) ? ENOENT : 0;
}

static int AuditEnsureRpcsvcgssdServiceIsDisabled(void)
{
    return CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "NEED_SVCGSSD = yes", SecurityBaselineGetLog());
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
    return CheckPackageInstalled("rsh", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

static int AuditEnsureSmbWithSambaIsDisabled(void)
{
    const char* etcSambaConf = "/etc/samba/smb.conf";
    const char* minProtocol = "min protocol = SMB2";

    return (CheckPackageInstalled("samba", SecurityBaselineGetLog()) || 
        ((EEXIST == CheckLineNotFoundOrCommentedOut(etcSambaConf, '#', minProtocol, SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(etcSambaConf, ';', minProtocol, SecurityBaselineGetLog())))) ? 0 : ENOENT;
}

static int AuditEnsureUsersDotFilesArentGroupOrWorldWritable(void)
{
    return CheckUsersRestrictedDotFiles(744, SecurityBaselineGetLog());
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
    return (CheckPackageInstalled(g_inetd, SecurityBaselineGetLog()) && 
        CheckPackageInstalled(g_inetUtilsInetd, SecurityBaselineGetLog()) &&
        FindTextInFile(g_etcInetdConf, "login", SecurityBaselineGetLog())) ? 0: ENOENT;
}

static int AuditEnsureUnnecessaryAccountsAreRemoved(void)
{
    return FindTextInFile(g_etcPasswd, "games", SecurityBaselineGetLog()) ? 0 : ENOENT;
}

AuditRemediate g_auditChecks[] =
{
    &AuditEnsurePermissionsOnEtcIssue,
    &AuditEnsurePermissionsOnEtcIssueNet,
    &AuditEnsurePermissionsOnEtcHostsAllow,
    &AuditEnsurePermissionsOnEtcHostsDeny,
    &AuditEnsurePermissionsOnEtcSshSshdConfig,
    &AuditEnsurePermissionsOnEtcShadow,
    &AuditEnsurePermissionsOnEtcShadowDash,
    &AuditEnsurePermissionsOnEtcGShadow,
    &AuditEnsurePermissionsOnEtcGShadowDash,
    &AuditEnsurePermissionsOnEtcPasswd,
    &AuditEnsurePermissionsOnEtcPasswdDash,
    &AuditEnsurePermissionsOnEtcGroup,
    &AuditEnsurePermissionsOnEtcGroupDash,
    &AuditEnsurePermissionsOnEtcAnacronTab,
    &AuditEnsurePermissionsOnEtcCronD,
    &AuditEnsurePermissionsOnEtcCronDaily,
    &AuditEnsurePermissionsOnEtcCronHourly,
    &AuditEnsurePermissionsOnEtcCronMonthly,
    &AuditEnsurePermissionsOnEtcCronWeekly,
    &AuditEnsurePermissionsOnEtcMotd,
    &AuditEnsureKernelSupportForCpuNx,
    &AuditEnsureNodevOptionOnHomePartition,
    &AuditEnsureNodevOptionOnTmpPartition,
    &AuditEnsureNodevOptionOnVarTmpPartition,
    &AuditEnsureNosuidOptionOnTmpPartition,
    &AuditEnsureNosuidOptionOnVarTmpPartition,
    &AuditEnsureNoexecOptionOnVarTmpPartition,
    &AuditEnsureNoexecOptionOnDevShmPartition,
    &AuditEnsureNodevOptionEnabledForAllRemovableMedia,
    &AuditEnsureNoexecOptionEnabledForAllRemovableMedia,
    &AuditEnsureNosuidOptionEnabledForAllRemovableMedia,
    &AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts,
    &AuditEnsureInetdNotInstalled,
    &AuditEnsureXinetdNotInstalled,
    &AuditEnsureAllTelnetdPackagesUninstalled,
    &AuditEnsureRshServerNotInstalled,
    &AuditEnsureNisNotInstalled,
    &AuditEnsureTftpdNotInstalled,
    &AuditEnsureReadaheadFedoraNotInstalled,
    &AuditEnsureBluetoothHiddNotInstalled,
    &AuditEnsureIsdnUtilsBaseNotInstalled,
    &AuditEnsureIsdnUtilsKdumpToolsNotInstalled,
    &AuditEnsureIscDhcpdServerNotInstalled,
    &AuditEnsureSendmailNotInstalled,
    &AuditEnsureSldapdNotInstalled,
    &AuditEnsureBind9NotInstalled,
    &AuditEnsureDovecotCoreNotInstalled,
    &AuditEnsureAuditdInstalled,
    &AuditEnsureAllEtcPasswdGroupsExistInEtcGroup,
    &AuditEnsureNoDuplicateUidsExist,
    &AuditEnsureNoDuplicateGidsExist,
    &AuditEnsureNoDuplicateUserNamesExist,
    &AuditEnsureNoDuplicateGroupsExist,
    &AuditEnsureShadowGroupIsEmpty,
    &AuditEnsureRootGroupExists,
    &AuditEnsureAllAccountsHavePasswords,
    &AuditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero,
    &AuditEnsureNoLegacyPlusEntriesInEtcPasswd,
    &AuditEnsureNoLegacyPlusEntriesInEtcShadow,
    &AuditEnsureNoLegacyPlusEntriesInEtcGroup,
    &AuditEnsureDefaultRootAccountGroupIsGidZero,
    &AuditEnsureRootIsOnlyUidZeroAccount,
    &AuditEnsureAllUsersHomeDirectoriesExist,
    &AuditEnsureUsersOwnTheirHomeDirectories,
    &AuditEnsureRestrictedUserHomeDirectories,
    &AuditEnsurePasswordHashingAlgorithm,
    &AuditEnsureMinDaysBetweenPasswordChanges,
    &AuditEnsureInactivePasswordLockPeriod,
    &AuditEnsureMaxDaysBetweenPasswordChanges,
    &AuditEnsurePasswordExpiration,
    &AuditEnsurePasswordExpirationWarning,
    &AuditEnsureSystemAccountsAreNonLogin,
    &AuditEnsureAuthenticationRequiredForSingleUserMode,
    &AuditEnsurePrelinkIsDisabled,
    &AuditEnsureTalkClientIsNotInstalled,
    &AuditEnsureDotDoesNotAppearInRootsPath,
    &AuditEnsureCronServiceIsEnabled,
    &AuditEnsureRemoteLoginWarningBannerIsConfigured,
    &AuditEnsureLocalLoginWarningBannerIsConfigured,
    &AuditEnsureAuditdServiceIsRunning,
    &AuditEnsureSuRestrictedToRootGroup,
    &AuditEnsureDefaultUmaskForAllUsers,
    &AuditEnsureAutomountingDisabled,
    &AuditEnsureKernelCompiledFromApprovedSources,
    &AuditEnsureDefaultDenyFirewallPolicyIsSet,
    &AuditEnsurePacketRedirectSendingIsDisabled,
    &AuditEnsureIcmpRedirectsIsDisabled,
    &AuditEnsureSourceRoutedPacketsIsDisabled,
    &AuditEnsureAcceptingSourceRoutedPacketsIsDisabled,
    &AuditEnsureIgnoringBogusIcmpBroadcastResponses,
    &AuditEnsureIgnoringIcmpEchoPingsToMulticast,
    &AuditEnsureMartianPacketLoggingIsEnabled,
    &AuditEnsureReversePathSourceValidationIsEnabled,
    &AuditEnsureTcpSynCookiesAreEnabled,
    &AuditEnsureSystemNotActingAsNetworkSniffer,
    &AuditEnsureAllWirelessInterfacesAreDisabled,
    &AuditEnsureIpv6ProtocolIsEnabled,
    &AuditEnsureDccpIsDisabled,
    &AuditEnsureSctpIsDisabled,
    &AuditEnsureDisabledSupportForRds,
    &AuditEnsureTipcIsDisabled,
    &AuditEnsureZeroconfNetworkingIsDisabled,
    &AuditEnsurePermissionsOnBootloaderConfig,
    &AuditEnsurePasswordReuseIsLimited,
    &AuditEnsureMountingOfUsbStorageDevicesIsDisabled,
    &AuditEnsureCoreDumpsAreRestricted,
    &AuditEnsurePasswordCreationRequirements,
    &AuditEnsureLockoutForFailedPasswordAttempts,
    &AuditEnsureDisabledInstallationOfCramfsFileSystem,
    &AuditEnsureDisabledInstallationOfFreevxfsFileSystem,
    &AuditEnsureDisabledInstallationOfHfsFileSystem,
    &AuditEnsureDisabledInstallationOfHfsplusFileSystem,
    &AuditEnsureDisabledInstallationOfJffs2FileSystem,
    &AuditEnsureVirtualMemoryRandomizationIsEnabled,
    &AuditEnsureAllBootloadersHavePasswordProtectionEnabled,
    &AuditEnsureLoggingIsConfigured,
    &AuditEnsureSyslogPackageIsInstalled,
    &AuditEnsureSystemdJournaldServicePersistsLogMessages,
    &AuditEnsureALoggingServiceIsSnabled,
    &AuditEnsureFilePermissionsForAllRsyslogLogFiles,
    &AuditEnsureLoggerConfigurationFilesAreRestricted,
    &AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup,
    &AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser,
    &AuditEnsureRsyslogNotAcceptingRemoteMessages,
    &AuditEnsureSyslogRotaterServiceIsEnabled,
    &AuditEnsureTelnetServiceIsDisabled,
    &AuditEnsureRcprshServiceIsDisabled,
    &AuditEnsureTftpServiceisDisabled,
    &AuditEnsureAtCronIsRestrictedToAuthorizedUsers,
    &AuditEnsureSshBestPracticeProtocol,
    &AuditEnsureSshBestPracticeIgnoreRhosts,
    &AuditEnsureSshLogLevelIsSet,
    &AuditEnsureSshMaxAuthTriesIsSet,
    &AuditEnsureSshAccessIsLimited,
    &AuditEnsureSshRhostsRsaAuthenticationIsDisabled,
    &AuditEnsureSshHostbasedAuthenticationIsDisabled,
    &AuditEnsureSshPermitRootLoginIsDisabled,
    &AuditEnsureSshPermitEmptyPasswordsIsDisabled,
    &AuditEnsureSshIdleTimeoutIntervalIsConfigured,
    &AuditEnsureSshLoginGraceTimeIsSet,
    &AuditEnsureOnlyApprovedMacAlgorithmsAreUsed,
    &AuditEnsureSshWarningBannerIsEnabled,
    &AuditEnsureUsersCannotSetSshEnvironmentOptions,
    &AuditEnsureAppropriateCiphersForSsh,
    &AuditEnsureAvahiDaemonServiceIsDisabled,
    &AuditEnsureCupsServiceisDisabled,
    &AuditEnsurePostfixPackageIsUninstalled,
    &AuditEnsurePostfixNetworkListeningIsDisabled,
    &AuditEnsureRpcgssdServiceIsDisabled,
    &AuditEnsureRpcidmapdServiceIsDisabled,
    &AuditEnsurePortmapServiceIsDisabled,
    &AuditEnsureNetworkFileSystemServiceIsDisabled,
    &AuditEnsureRpcsvcgssdServiceIsDisabled,
    &AuditEnsureSnmpServerIsDisabled,
    &AuditEnsureRsynServiceIsDisabled,
    &AuditEnsureNisServerIsDisabled,
    &AuditEnsureRshClientNotInstalled,
    &AuditEnsureSmbWithSambaIsDisabled,
    &AuditEnsureUsersDotFilesArentGroupOrWorldWritable,
    &AuditEnsureNoUsersHaveDotForwardFiles,
    &AuditEnsureNoUsersHaveDotNetrcFiles,
    &AuditEnsureNoUsersHaveDotRhostsFiles,
    &AuditEnsureRloginServiceIsDisabled,
    &AuditEnsureUnnecessaryAccountsAreRemoved
};

int AuditSecurityBaseline(void)
{
    size_t numChecks = ARRAY_SIZE(g_auditChecks);
    size_t i = 0;
    int status = 0;

    for (i = 0; i < numChecks; i++)
    {
        if ((0 != g_auditChecks[i]()) && (0 == status))
        {
            status = ENOENT;
        }
    }

    return status;
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
    return ((0 == UninstallPackage(g_inetd, SecurityBaselineGetLog())) &&
        (0 == UninstallPackage(g_inetUtilsInetd, SecurityBaselineGetLog()))) ? 0 : ENOENT;
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
    return UninstallPackage(g_iscDhcpServer, SecurityBaselineGetLog());
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

static int RemediateEnsureKernelSupportForCpuNx(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnHomePartition(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnTmpPartition(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnVarTmpPartition(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionOnTmpPartition(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionOnVarTmpPartition(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionOnVarTmpPartition(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionOnDevShmPartition(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionEnabledForAllRemovableMedia(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionEnabledForAllRemovableMedia(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionEnabledForAllRemovableMedia(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllTelnetdPackagesUninstalled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllEtcPasswdGroupsExistInEtcGroup(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateUidsExist(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateGidsExist(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateUserNamesExist(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateGroupsExist(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureShadowGroupIsEmpty(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRootGroupExists(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllAccountsHavePasswords(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcPasswd(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcShadow(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcGroup(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultRootAccountGroupIsGidZero(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRootIsOnlyUidZeroAccount(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllUsersHomeDirectoriesExist(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUsersOwnTheirHomeDirectories(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRestrictedUserHomeDirectories(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordHashingAlgorithm(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMinDaysBetweenPasswordChanges(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureInactivePasswordLockPeriod(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMaxDaysBetweenPasswordChanges(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordExpiration(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordExpirationWarning(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSystemAccountsAreNonLogin(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAuthenticationRequiredForSingleUserMode(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDotDoesNotAppearInRootsPath(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRemoteLoginWarningBannerIsConfigured(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLocalLoginWarningBannerIsConfigured(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSuRestrictedToRootGroup(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultUmaskForAllUsers(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAutomountingDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureKernelCompiledFromApprovedSources(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultDenyFirewallPolicyIsSet(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePacketRedirectSendingIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIcmpRedirectsIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSourceRoutedPacketsIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAcceptingSourceRoutedPacketsIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIgnoringBogusIcmpBroadcastResponses(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIgnoringIcmpEchoPingsToMulticast(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMartianPacketLoggingIsEnabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureReversePathSourceValidationIsEnabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTcpSynCookiesAreEnabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSystemNotActingAsNetworkSniffer(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllWirelessInterfacesAreDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIpv6ProtocolIsEnabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDccpIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSctpIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledSupportForRds(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTipcIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureZeroconfNetworkingIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePermissionsOnBootloaderConfig(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordReuseIsLimited(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMountingOfUsbStorageDevicesIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureCoreDumpsAreRestricted(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordCreationRequirements(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLockoutForFailedPasswordAttempts(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfCramfsFileSystem(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfFreevxfsFileSystem(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfHfsFileSystem(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfHfsplusFileSystem(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfJffs2FileSystem(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureVirtualMemoryRandomizationIsEnabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllBootloadersHavePasswordProtectionEnabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLoggingIsConfigured(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSyslogPackageIsInstalled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSystemdJournaldServicePersistsLogMessages(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureALoggingServiceIsSnabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureFilePermissionsForAllRsyslogLogFiles(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLoggerConfigurationFilesAreRestricted(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRsyslogNotAcceptingRemoteMessages(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSyslogRotaterServiceIsEnabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTelnetServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRcprshServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTftpServiceisDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAtCronIsRestrictedToAuthorizedUsers(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshBestPracticeProtocol(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshBestPracticeIgnoreRhosts(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshLogLevelIsSet(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshMaxAuthTriesIsSet(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshAccessIsLimited(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshRhostsRsaAuthenticationIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshHostbasedAuthenticationIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshPermitRootLoginIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshPermitEmptyPasswordsIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshIdleTimeoutIntervalIsConfigured(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshLoginGraceTimeIsSet(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureOnlyApprovedMacAlgorithmsAreUsed(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshWarningBannerIsEnabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUsersCannotSetSshEnvironmentOptions(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAppropriateCiphersForSsh(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAvahiDaemonServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureCupsServiceisDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePostfixPackageIsUninstalled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePostfixNetworkListeningIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRpcgssdServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRpcidmapdServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePortmapServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNetworkFileSystemServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRpcsvcgssdServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSnmpServerIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRsynServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNisServerIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRshClientNotInstalled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSmbWithSambaIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUsersDotFilesArentGroupOrWorldWritable(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoUsersHaveDotForwardFiles(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoUsersHaveDotNetrcFiles(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoUsersHaveDotRhostsFiles(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRloginServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUnnecessaryAccountsAreRemoved(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

AuditRemediate g_remediateChecks[] =
{
    &RemediateEnsurePermissionsOnEtcIssue,
    &RemediateEnsurePermissionsOnEtcIssueNet,
    &RemediateEnsurePermissionsOnEtcHostsAllow,
    &RemediateEnsurePermissionsOnEtcHostsDeny,
    &RemediateEnsurePermissionsOnEtcSshSshdConfig,
    &RemediateEnsurePermissionsOnEtcShadow,
    &RemediateEnsurePermissionsOnEtcShadowDash,
    &RemediateEnsurePermissionsOnEtcGShadow,
    &RemediateEnsurePermissionsOnEtcGShadowDash,
    &RemediateEnsurePermissionsOnEtcPasswd,
    &RemediateEnsurePermissionsOnEtcPasswdDash,
    &RemediateEnsurePermissionsOnEtcGroup,
    &RemediateEnsurePermissionsOnEtcGroupDash,
    &RemediateEnsurePermissionsOnEtcAnacronTab,
    &RemediateEnsurePermissionsOnEtcCronD,
    &RemediateEnsurePermissionsOnEtcCronDaily,
    &RemediateEnsurePermissionsOnEtcCronHourly,
    &RemediateEnsurePermissionsOnEtcCronMonthly,
    &RemediateEnsurePermissionsOnEtcCronWeekly,
    &RemediateEnsurePermissionsOnEtcMotd,
    &RemediateEnsureInetdNotInstalled,
    &RemediateEnsureXinetdNotInstalled,
    &RemediateEnsureRshServerNotInstalled,
    &RemediateEnsureNisNotInstalled,
    &RemediateEnsureTftpdNotInstalled,
    &RemediateEnsureReadaheadFedoraNotInstalled,
    &RemediateEnsureBluetoothHiddNotInstalled,
    &RemediateEnsureIsdnUtilsBaseNotInstalled,
    &RemediateEnsureIsdnUtilsKdumpToolsNotInstalled,
    &RemediateEnsureIscDhcpdServerNotInstalled,
    &RemediateEnsureSendmailNotInstalled,
    &RemediateEnsureSldapdNotInstalled,
    &RemediateEnsureBind9NotInstalled,
    &RemediateEnsureDovecotCoreNotInstalled,
    &RemediateEnsureAuditdInstalled,
    &RemediateEnsurePrelinkIsDisabled,
    &RemediateEnsureTalkClientIsNotInstalled,
    &RemediateEnsureCronServiceIsEnabled,
    &RemediateEnsureAuditdServiceIsRunning,
    &RemediateEnsureKernelSupportForCpuNx,
    &RemediateEnsureNodevOptionOnHomePartition,
    &RemediateEnsureNodevOptionOnTmpPartition,
    &RemediateEnsureNodevOptionOnVarTmpPartition,
    &RemediateEnsureNosuidOptionOnTmpPartition,
    &RemediateEnsureNosuidOptionOnVarTmpPartition,
    &RemediateEnsureNoexecOptionOnVarTmpPartition,
    &RemediateEnsureNoexecOptionOnDevShmPartition,
    &RemediateEnsureNodevOptionEnabledForAllRemovableMedia,
    &RemediateEnsureNoexecOptionEnabledForAllRemovableMedia,
    &RemediateEnsureNosuidOptionEnabledForAllRemovableMedia,
    &RemediateEnsureNoexecNosuidOptionsEnabledForAllNfsMounts,
    &RemediateEnsureAllTelnetdPackagesUninstalled,
    &RemediateEnsureAllEtcPasswdGroupsExistInEtcGroup,
    &RemediateEnsureNoDuplicateUidsExist,
    &RemediateEnsureNoDuplicateGidsExist,
    &RemediateEnsureNoDuplicateUserNamesExist,
    &RemediateEnsureNoDuplicateGroupsExist,
    &RemediateEnsureShadowGroupIsEmpty,
    &RemediateEnsureRootGroupExists,
    &RemediateEnsureAllAccountsHavePasswords,
    &RemediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero,
    &RemediateEnsureNoLegacyPlusEntriesInEtcPasswd,
    &RemediateEnsureNoLegacyPlusEntriesInEtcShadow,
    &RemediateEnsureNoLegacyPlusEntriesInEtcGroup,
    &RemediateEnsureDefaultRootAccountGroupIsGidZero,
    &RemediateEnsureRootIsOnlyUidZeroAccount,
    &RemediateEnsureAllUsersHomeDirectoriesExist,
    &RemediateEnsureUsersOwnTheirHomeDirectories,
    &RemediateEnsureRestrictedUserHomeDirectories,
    &RemediateEnsurePasswordHashingAlgorithm,
    &RemediateEnsureMinDaysBetweenPasswordChanges,
    &RemediateEnsureInactivePasswordLockPeriod,
    &RemediateEnsureMaxDaysBetweenPasswordChanges,
    &RemediateEnsurePasswordExpiration,
    &RemediateEnsurePasswordExpirationWarning,
    &RemediateEnsureSystemAccountsAreNonLogin,
    &RemediateEnsureAuthenticationRequiredForSingleUserMode,
    &RemediateEnsureDotDoesNotAppearInRootsPath,
    &RemediateEnsureRemoteLoginWarningBannerIsConfigured,
    &RemediateEnsureLocalLoginWarningBannerIsConfigured,
    &RemediateEnsureSuRestrictedToRootGroup,
    &RemediateEnsureDefaultUmaskForAllUsers,
    &RemediateEnsureAutomountingDisabled,
    &RemediateEnsureKernelCompiledFromApprovedSources,
    &RemediateEnsureDefaultDenyFirewallPolicyIsSet,
    &RemediateEnsurePacketRedirectSendingIsDisabled,
    &RemediateEnsureIcmpRedirectsIsDisabled,
    &RemediateEnsureSourceRoutedPacketsIsDisabled,
    &RemediateEnsureAcceptingSourceRoutedPacketsIsDisabled,
    &RemediateEnsureIgnoringBogusIcmpBroadcastResponses,
    &RemediateEnsureIgnoringIcmpEchoPingsToMulticast,
    &RemediateEnsureMartianPacketLoggingIsEnabled,
    &RemediateEnsureReversePathSourceValidationIsEnabled,
    &RemediateEnsureTcpSynCookiesAreEnabled,
    &RemediateEnsureSystemNotActingAsNetworkSniffer,
    &RemediateEnsureAllWirelessInterfacesAreDisabled,
    &RemediateEnsureIpv6ProtocolIsEnabled,
    &RemediateEnsureDccpIsDisabled,
    &RemediateEnsureSctpIsDisabled,
    &RemediateEnsureDisabledSupportForRds,
    &RemediateEnsureTipcIsDisabled,
    &RemediateEnsureZeroconfNetworkingIsDisabled,
    &RemediateEnsurePermissionsOnBootloaderConfig,
    &RemediateEnsurePasswordReuseIsLimited,
    &RemediateEnsureMountingOfUsbStorageDevicesIsDisabled,
    &RemediateEnsureCoreDumpsAreRestricted,
    &RemediateEnsurePasswordCreationRequirements,
    &RemediateEnsureLockoutForFailedPasswordAttempts,
    &RemediateEnsureDisabledInstallationOfCramfsFileSystem,
    &RemediateEnsureDisabledInstallationOfFreevxfsFileSystem,
    &RemediateEnsureDisabledInstallationOfHfsFileSystem,
    &RemediateEnsureDisabledInstallationOfHfsplusFileSystem,
    &RemediateEnsureDisabledInstallationOfJffs2FileSystem,
    &RemediateEnsureVirtualMemoryRandomizationIsEnabled,
    &RemediateEnsureAllBootloadersHavePasswordProtectionEnabled,
    &RemediateEnsureLoggingIsConfigured,
    &RemediateEnsureSyslogPackageIsInstalled,
    &RemediateEnsureSystemdJournaldServicePersistsLogMessages,
    &RemediateEnsureALoggingServiceIsSnabled,
    &RemediateEnsureFilePermissionsForAllRsyslogLogFiles,
    &RemediateEnsureLoggerConfigurationFilesAreRestricted,
    &RemediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroup,
    &RemediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUser,
    &RemediateEnsureRsyslogNotAcceptingRemoteMessages,
    &RemediateEnsureSyslogRotaterServiceIsEnabled,
    &RemediateEnsureTelnetServiceIsDisabled,
    &RemediateEnsureRcprshServiceIsDisabled,
    &RemediateEnsureTftpServiceisDisabled,
    &RemediateEnsureAtCronIsRestrictedToAuthorizedUsers,
    &RemediateEnsureSshBestPracticeProtocol,
    &RemediateEnsureSshBestPracticeIgnoreRhosts,
    &RemediateEnsureSshLogLevelIsSet,
    &RemediateEnsureSshMaxAuthTriesIsSet,
    &RemediateEnsureSshAccessIsLimited,
    &RemediateEnsureSshRhostsRsaAuthenticationIsDisabled,
    &RemediateEnsureSshHostbasedAuthenticationIsDisabled,
    &RemediateEnsureSshPermitRootLoginIsDisabled,
    &RemediateEnsureSshPermitEmptyPasswordsIsDisabled,
    &RemediateEnsureSshIdleTimeoutIntervalIsConfigured,
    &RemediateEnsureSshLoginGraceTimeIsSet,
    &RemediateEnsureOnlyApprovedMacAlgorithmsAreUsed,
    &RemediateEnsureSshWarningBannerIsEnabled,
    &RemediateEnsureUsersCannotSetSshEnvironmentOptions,
    &RemediateEnsureAppropriateCiphersForSsh,
    &RemediateEnsureAvahiDaemonServiceIsDisabled,
    &RemediateEnsureCupsServiceisDisabled,
    &RemediateEnsurePostfixPackageIsUninstalled,
    &RemediateEnsurePostfixNetworkListeningIsDisabled,
    &RemediateEnsureRpcgssdServiceIsDisabled,
    &RemediateEnsureRpcidmapdServiceIsDisabled,
    &RemediateEnsurePortmapServiceIsDisabled,
    &RemediateEnsureNetworkFileSystemServiceIsDisabled,
    &RemediateEnsureRpcsvcgssdServiceIsDisabled,
    &RemediateEnsureSnmpServerIsDisabled,
    &RemediateEnsureRsynServiceIsDisabled,
    &RemediateEnsureNisServerIsDisabled,
    &RemediateEnsureRshClientNotInstalled,
    &RemediateEnsureSmbWithSambaIsDisabled,
    &RemediateEnsureUsersDotFilesArentGroupOrWorldWritable,
    &RemediateEnsureNoUsersHaveDotForwardFiles,
    &RemediateEnsureNoUsersHaveDotNetrcFiles,
    &RemediateEnsureNoUsersHaveDotRhostsFiles,
    &RemediateEnsureRloginServiceIsDisabled,
    &RemediateEnsureUnnecessaryAccountsAreRemoved
};

int RemediateSecurityBaseline(void)
{
    size_t numChecks = ARRAY_SIZE(g_remediateChecks);
    size_t i = 0;
    int status = 0;

    for (i = 0; i < numChecks; i++)
    {
        if ((0 != g_remediateChecks[i]()) && (0 == status))
        {
            status = ENOENT;
        }
    }

    return status;
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
            result = AuditEnsureAllTelnetdPackagesUninstalled() ? g_fail : g_pass;
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
        else if (0 == strcmp(objectName, g_remediateEnsureKernelSupportForCpuNxObject))
        {
            status = RemediateEnsureKernelSupportForCpuNx();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionOnHomePartitionObject))
        {
            status = RemediateEnsureNodevOptionOnHomePartition();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionOnTmpPartitionObject))
        {
            status = RemediateEnsureNodevOptionOnTmpPartition();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionOnVarTmpPartitionObject))
        {
            status = RemediateEnsureNodevOptionOnVarTmpPartition();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNosuidOptionOnTmpPartitionObject))
        {
            status = RemediateEnsureNosuidOptionOnTmpPartition();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNosuidOptionOnVarTmpPartitionObject))
        {
            status = RemediateEnsureNosuidOptionOnVarTmpPartition();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecOptionOnVarTmpPartitionObject))
        {
            status = RemediateEnsureNoexecOptionOnVarTmpPartition();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecOptionOnDevShmPartitionObject))
        {
            status = RemediateEnsureNoexecOptionOnDevShmPartition();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNodevOptionEnabledForAllRemovableMediaObject))
        {
            status = RemediateEnsureNodevOptionEnabledForAllRemovableMedia();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecOptionEnabledForAllRemovableMediaObject))
        {
            status = RemediateEnsureNoexecOptionEnabledForAllRemovableMedia();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNosuidOptionEnabledForAllRemovableMediaObject))
        {
            status = RemediateEnsureNosuidOptionEnabledForAllRemovableMedia();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject))
        {
            status = RemediateEnsureNoexecNosuidOptionsEnabledForAllNfsMounts();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllTelnetdPackagesUninstalledObject))
        {
            status = RemediateEnsureAllTelnetdPackagesUninstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllEtcPasswdGroupsExistInEtcGroupObject))
        {
            status = RemediateEnsureAllEtcPasswdGroupsExistInEtcGroup();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateUidsExistObject))
        {
            status = RemediateEnsureNoDuplicateUidsExist();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateGidsExistObject))
        {
            status = RemediateEnsureNoDuplicateGidsExist();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateUserNamesExistObject))
        {
            status = RemediateEnsureNoDuplicateUserNamesExist();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoDuplicateGroupsExistObject))
        {
            status = RemediateEnsureNoDuplicateGroupsExist();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureShadowGroupIsEmptyObject))
        {
            status = RemediateEnsureShadowGroupIsEmpty();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRootGroupExistsObject))
        {
            status = RemediateEnsureRootGroupExists();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllAccountsHavePasswordsObject))
        {
            status = RemediateEnsureAllAccountsHavePasswords();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject))
        {
            status = RemediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoLegacyPlusEntriesInEtcPasswdObject))
        {
            status = RemediateEnsureNoLegacyPlusEntriesInEtcPasswd();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoLegacyPlusEntriesInEtcShadowObject))
        {
            status = RemediateEnsureNoLegacyPlusEntriesInEtcShadow();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoLegacyPlusEntriesInEtcGroupObject))
        {
            status = RemediateEnsureNoLegacyPlusEntriesInEtcGroup();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDefaultRootAccountGroupIsGidZeroObject))
        {
            status = RemediateEnsureDefaultRootAccountGroupIsGidZero();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRootIsOnlyUidZeroAccountObject))
        {
            status = RemediateEnsureRootIsOnlyUidZeroAccount();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllUsersHomeDirectoriesExistObject))
        {
            status = RemediateEnsureAllUsersHomeDirectoriesExist();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUsersOwnTheirHomeDirectoriesObject))
        {
            status = RemediateEnsureUsersOwnTheirHomeDirectories();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRestrictedUserHomeDirectoriesObject))
        {
            status = RemediateEnsureRestrictedUserHomeDirectories();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordHashingAlgorithmObject))
        {
            status = RemediateEnsurePasswordHashingAlgorithm();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureMinDaysBetweenPasswordChangesObject))
        {
            status = RemediateEnsureMinDaysBetweenPasswordChanges();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureInactivePasswordLockPeriodObject))
        {
            status = RemediateEnsureInactivePasswordLockPeriod();
        }
        else if (0 == strcmp(objectName, g_remediateMaxDaysBetweenPasswordChangesObject))
        {
            status = RemediateEnsureMaxDaysBetweenPasswordChanges();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordExpirationObject))
        {
            status = RemediateEnsurePasswordExpiration();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordExpirationWarningObject))
        {
            status = RemediateEnsurePasswordExpirationWarning();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSystemAccountsAreNonLoginObject))
        {
            status = RemediateEnsureSystemAccountsAreNonLogin();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuthenticationRequiredForSingleUserModeObject))
        {
            status = RemediateEnsureAuthenticationRequiredForSingleUserMode();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDotDoesNotAppearInRootsPathObject))
        {
            status = RemediateEnsureDotDoesNotAppearInRootsPath();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRemoteLoginWarningBannerIsConfiguredObject))
        {
            status = RemediateEnsureRemoteLoginWarningBannerIsConfigured();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLocalLoginWarningBannerIsConfiguredObject))
        {
            status = RemediateEnsureLocalLoginWarningBannerIsConfigured();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAuditdServiceIsRunningObject))
        {
            status = RemediateEnsureAuditdServiceIsRunning();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSuRestrictedToRootGroupObject))
        {
            status = RemediateEnsureSuRestrictedToRootGroup();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDefaultUmaskForAllUsersObject))
        {
            status = RemediateEnsureDefaultUmaskForAllUsers();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAutomountingDisabledObject))
        {
            status = RemediateEnsureAutomountingDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureKernelCompiledFromApprovedSourcesObject))
        {
            status = RemediateEnsureKernelCompiledFromApprovedSources();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDefaultDenyFirewallPolicyIsSetObject))
        {
            status = RemediateEnsureDefaultDenyFirewallPolicyIsSet();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePacketRedirectSendingIsDisabledObject))
        {
            status = RemediateEnsurePacketRedirectSendingIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIcmpRedirectsIsDisabledObject))
        {
            status = RemediateEnsureIcmpRedirectsIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSourceRoutedPacketsIsDisabledObject))
        {
            status = RemediateEnsureSourceRoutedPacketsIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAcceptingSourceRoutedPacketsIsDisabledObject))
        {
            status = RemediateEnsureAcceptingSourceRoutedPacketsIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIgnoringBogusIcmpBroadcastResponsesObject))
        {
            status = RemediateEnsureIgnoringBogusIcmpBroadcastResponses();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIgnoringIcmpEchoPingsToMulticastObject))
        {
            status = RemediateEnsureIgnoringIcmpEchoPingsToMulticast();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureMartianPacketLoggingIsEnabledObject))
        {
            status = RemediateEnsureMartianPacketLoggingIsEnabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureReversePathSourceValidationIsEnabledObject))
        {
            status = RemediateEnsureReversePathSourceValidationIsEnabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTcpSynCookiesAreEnabledObject))
        {
            status = RemediateEnsureTcpSynCookiesAreEnabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSystemNotActingAsNetworkSnifferObject))
        {
            status = RemediateEnsureSystemNotActingAsNetworkSniffer();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllWirelessInterfacesAreDisabledObject))
        {
            status = RemediateEnsureAllWirelessInterfacesAreDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureIpv6ProtocolIsEnabledObject))
        {
            status = RemediateEnsureIpv6ProtocolIsEnabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDccpIsDisabledObject))
        {
            status = RemediateEnsureDccpIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSctpIsDisabledObject))
        {
            status = RemediateEnsureSctpIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledSupportForRdsObject))
        {
            status = RemediateEnsureDisabledSupportForRds();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTipcIsDisabledObject))
        {
            status = RemediateEnsureTipcIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureZeroconfNetworkingIsDisabledObject))
        {
            status = RemediateEnsureZeroconfNetworkingIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnBootloaderConfigObject))
        {
            status = RemediateEnsurePermissionsOnBootloaderConfig();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordReuseIsLimitedObject))
        {
            status = RemediateEnsurePasswordReuseIsLimited();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureMountingOfUsbStorageDevicesIsDisabledObject))
        {
            status = RemediateEnsureMountingOfUsbStorageDevicesIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureCoreDumpsAreRestrictedObject))
        {
            status = RemediateEnsureCoreDumpsAreRestricted();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePasswordCreationRequirementsObject))
        {
            status = RemediateEnsurePasswordCreationRequirements();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLockoutForFailedPasswordAttemptsObject))
        {
            status = RemediateEnsureLockoutForFailedPasswordAttempts();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfCramfsFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfCramfsFileSystem();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfFreevxfsFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfFreevxfsFileSystem();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfHfsFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfHfsFileSystem();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfHfsplusFileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfHfsplusFileSystem();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureDisabledInstallationOfJffs2FileSystemObject))
        {
            status = RemediateEnsureDisabledInstallationOfJffs2FileSystem();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureVirtualMemoryRandomizationIsEnabledObject))
        {
            status = RemediateEnsureVirtualMemoryRandomizationIsEnabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllBootloadersHavePasswordProtectionEnabledObject))
        {
            status = RemediateEnsureAllBootloadersHavePasswordProtectionEnabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLoggingIsConfiguredObject))
        {
            status = RemediateEnsureLoggingIsConfigured();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSyslogPackageIsInstalledObject))
        {
            status = RemediateEnsureSyslogPackageIsInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSystemdJournaldServicePersistsLogMessagesObject))
        {
            status = RemediateEnsureSystemdJournaldServicePersistsLogMessages();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureALoggingServiceIsSnabledObject))
        {
            status = RemediateEnsureALoggingServiceIsSnabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureFilePermissionsForAllRsyslogLogFilesObject))
        {
            status = RemediateEnsureFilePermissionsForAllRsyslogLogFiles();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureLoggerConfigurationFilesAreRestrictedObject))
        {
            status = RemediateEnsureLoggerConfigurationFilesAreRestricted();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject))
        {
            status = RemediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroup();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject))
        {
            status = RemediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUser();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRsyslogNotAcceptingRemoteMessagesObject))
        {
            status = RemediateEnsureRsyslogNotAcceptingRemoteMessages();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSyslogRotaterServiceIsEnabledObject))
        {
            status = RemediateEnsureSyslogRotaterServiceIsEnabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTelnetServiceIsDisabledObject))
        {
            status = RemediateEnsureTelnetServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRcprshServiceIsDisabledObject))
        {
            status = RemediateEnsureRcprshServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureTftpServiceisDisabledObject))
        {
            status = RemediateEnsureTftpServiceisDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAtCronIsRestrictedToAuthorizedUsersObject))
        {
            status = RemediateEnsureAtCronIsRestrictedToAuthorizedUsers();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshBestPracticeProtocolObject))
        {
            status = RemediateEnsureSshBestPracticeProtocol();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshBestPracticeIgnoreRhostsObject))
        {
            status = RemediateEnsureSshBestPracticeIgnoreRhosts();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshLogLevelIsSetObject))
        {
            status = RemediateEnsureSshLogLevelIsSet();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshMaxAuthTriesIsSetObject))
        {
            status = RemediateEnsureSshMaxAuthTriesIsSet();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshAccessIsLimitedObject))
        {
            status = RemediateEnsureSshAccessIsLimited();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshRhostsRsaAuthenticationIsDisabledObject))
        {
            status = RemediateEnsureSshRhostsRsaAuthenticationIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject))
        {
            status = RemediateEnsureSshHostbasedAuthenticationIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshPermitRootLoginIsDisabledObject))
        {
            status = RemediateEnsureSshPermitRootLoginIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject))
        {
            status = RemediateEnsureSshPermitEmptyPasswordsIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshIdleTimeoutIntervalIsConfiguredObject))
        {
            status = RemediateEnsureSshIdleTimeoutIntervalIsConfigured();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshLoginGraceTimeIsSetObject))
        {
            status = RemediateEnsureSshLoginGraceTimeIsSet();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject))
        {
            status = RemediateEnsureOnlyApprovedMacAlgorithmsAreUsed();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSshWarningBannerIsEnabledObject))
        {
            status = RemediateEnsureSshWarningBannerIsEnabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject))
        {
            status = RemediateEnsureUsersCannotSetSshEnvironmentOptions();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAppropriateCiphersForSshObject))
        {
            status = RemediateEnsureAppropriateCiphersForSsh();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureAvahiDaemonServiceIsDisabledObject))
        {
            status = RemediateEnsureAvahiDaemonServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureCupsServiceisDisabledObject))
        {
            status = RemediateEnsureCupsServiceisDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePostfixPackageIsUninstalledObject))
        {
            status = RemediateEnsurePostfixPackageIsUninstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePostfixNetworkListeningIsDisabledObject))
        {
            status = RemediateEnsurePostfixNetworkListeningIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRpcgssdServiceIsDisabledObject))
        {
            status = RemediateEnsureRpcgssdServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRpcidmapdServiceIsDisabledObject))
        {
            status = RemediateEnsureRpcidmapdServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePortmapServiceIsDisabledObject))
        {
            status = RemediateEnsurePortmapServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNetworkFileSystemServiceIsDisabledObject))
        {
            status = RemediateEnsureNetworkFileSystemServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRpcsvcgssdServiceIsDisabledObject))
        {
            status = RemediateEnsureRpcsvcgssdServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSnmpServerIsDisabledObject))
        {
            status = RemediateEnsureSnmpServerIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRsynServiceIsDisabledObject))
        {
            status = RemediateEnsureRsynServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNisServerIsDisabledObject))
        {
            status = RemediateEnsureNisServerIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRshClientNotInstalledObject))
        {
            status = RemediateEnsureRshClientNotInstalled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureSmbWithSambaIsDisabledObject))
        {
            status = RemediateEnsureSmbWithSambaIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUsersDotFilesArentGroupOrWorldWritableObject))
        {
            status = RemediateEnsureUsersDotFilesArentGroupOrWorldWritable();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoUsersHaveDotForwardFilesObject))
        {
            status = RemediateEnsureNoUsersHaveDotForwardFiles();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoUsersHaveDotNetrcFilesObject))
        {
            status = RemediateEnsureNoUsersHaveDotNetrcFiles();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureNoUsersHaveDotRhostsFilesObject))
        {
            status = RemediateEnsureNoUsersHaveDotRhostsFiles();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureRloginServiceIsDisabledObject))
        {
            status = RemediateEnsureRloginServiceIsDisabled();
        }
        else if (0 == strcmp(objectName, g_remediateEnsureUnnecessaryAccountsAreRemovedObject))
        {
            status = RemediateEnsureUnnecessaryAccountsAreRemoved();
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