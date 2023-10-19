// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <version.h>
#include <parson.h>
#include <CommonUtils.h>
#include <UserUtils.h>
#include <Logging.h>
#include <Mmi.h>

#include "SecurityBaseline.h"

typedef int(*RemediationCall)(void);
typedef char*(*AuditCall)(void);

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

static const char* g_pass = "PASS";
static const char* g_fail = "FAIL";

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

static char* AuditEnsurePermissionsOnEtcIssue(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcIssue, 0, 0, 644, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcIssueNet(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcIssueNet, 0, 0, 644, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcHostsAllow(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcHostsAllow, 0, 0, 644, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcHostsDeny(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcHostsDeny, 0, 0, 644, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcSshSshdConfig(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcSshSshdConfig, 0, 0, 600, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcShadow(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcShadow, 0, 42, 400, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcShadowDash(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcShadowDash, 0, 42, 400, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcGShadow(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcGShadow, 0, 42, 400, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcGShadowDash(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcGShadowDash, 0, 42, 400, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcPasswd(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcPasswd, 0, 0, 644, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcPasswdDash(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcPasswdDash, 0, 0, 600, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcGroup(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcGroup, 0, 0, 644, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcGroupDash(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcGroupDash, 0, 0, 644, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcAnacronTab(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcAnacronTab, 0, 0, 600, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcCronD(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcCronD, 0, 0, 700, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcCronDaily(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcCronDaily, 0, 0, 700, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcCronHourly(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcCronHourly, 0, 0, 700, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcCronMonthly(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcCronMonthly, 0, 0, 700, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcCronWeekly(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcCronWeekly, 0, 0, 700, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsurePermissionsOnEtcMotd(void)
{
    char* reason = NULL;
    return CheckFileAccess(g_etcMotd, 0, 0, 644, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
};

static char* AuditEnsureKernelSupportForCpuNx(void)
{
    return IsCpuFlagSupported("nx", SecurityBaselineGetLog()) ? DuplicateString(g_pass) : 
        DuplicateString("The device's processor does not have support for the NX bit technology");
}

static char* AuditEnsureNodevOptionOnHomePartition(void)
{
    const char* home = "/home";
    char* reason = NULL;

    return ((0 == CheckFileSystemMountingOption(g_etcFstab, home, NULL, g_nodev, &reason, SecurityBaselineGetLog())) ||
        (0 == CheckFileSystemMountingOption(g_etcMtab, home, NULL, g_nodev, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureNodevOptionOnTmpPartition(void)
{
    char* reason = NULL;
    return ((0 == CheckFileSystemMountingOption(g_etcFstab, g_tmp, NULL, g_nodev, &reason, SecurityBaselineGetLog())) ||
        (0 == CheckFileSystemMountingOption(g_etcMtab, g_tmp, NULL, g_nodev, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureNodevOptionOnVarTmpPartition(void)
{
    char* reason = NULL;
    return ((0 == CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_nodev, &reason, SecurityBaselineGetLog())) ||
        (0 == CheckFileSystemMountingOption(g_etcMtab, g_varTmp, NULL, g_nodev, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureNosuidOptionOnTmpPartition(void)
{
    char* reason = NULL;
    return ((0 == CheckFileSystemMountingOption(g_etcFstab, g_tmp, NULL, g_nosuid, &reason, SecurityBaselineGetLog())) || 
        (0 == CheckFileSystemMountingOption(g_etcMtab, g_tmp, NULL, g_nosuid, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureNosuidOptionOnVarTmpPartition(void)
{
    char* reason = NULL;
    return ((0 == CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_nosuid, &reason, SecurityBaselineGetLog())) ||
        (0 == CheckFileSystemMountingOption(g_etcMtab, g_varTmp, NULL, g_nosuid, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureNoexecOptionOnVarTmpPartition(void)
{
    char* reason = NULL;
    return ((0 == CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_noexec, &reason, SecurityBaselineGetLog())) ||
        (0 == CheckFileSystemMountingOption(g_etcMtab, g_varTmp, NULL, g_noexec, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureNoexecOptionOnDevShmPartition(void)
{
    const char* devShm = "/dev/shm";
    char* reason = NULL;
    return ((0 == CheckFileSystemMountingOption(g_etcFstab, devShm, NULL, g_noexec, &reason, SecurityBaselineGetLog())) ||
        (0 == CheckFileSystemMountingOption(g_etcMtab, devShm, NULL, g_noexec, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureNodevOptionEnabledForAllRemovableMedia(void)
{
    char* reason = NULL;
    return ((0 == CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_nodev, &reason, SecurityBaselineGetLog())) ||
        (0 == CheckFileSystemMountingOption(g_etcMtab, g_media, NULL, g_nodev, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureNoexecOptionEnabledForAllRemovableMedia(void)
{
    char* reason = NULL;
    return ((0 == CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_noexec, &reason, SecurityBaselineGetLog())) ||
        (0 == CheckFileSystemMountingOption(g_etcMtab, g_media, NULL, g_noexec, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureNosuidOptionEnabledForAllRemovableMedia(void)
{
    char* reason = NULL;
    return ((0 == CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_nosuid, &reason, SecurityBaselineGetLog())) ||
        (0 == CheckFileSystemMountingOption(g_etcMtab, g_media, NULL, g_nosuid, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(void)
{
    const char* nfs = "nfs";
    char* reason = NULL;
    return (((0 == CheckFileSystemMountingOption(g_etcFstab, NULL, nfs, g_noexec, &reason, SecurityBaselineGetLog())) &&
        (0 == CheckFileSystemMountingOption(g_etcFstab, NULL, nfs, g_nosuid, &reason, SecurityBaselineGetLog()))) ||
        ((0 == CheckFileSystemMountingOption(g_etcMtab, NULL, nfs, g_noexec, &reason, SecurityBaselineGetLog())) &&
        (0 == CheckFileSystemMountingOption(g_etcMtab, NULL, nfs, g_nosuid, &reason, SecurityBaselineGetLog())))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureInetdNotInstalled(void)
{
    return (CheckPackageInstalled(g_inetd, SecurityBaselineGetLog()) && 
        CheckPackageInstalled(g_inetUtilsInetd, SecurityBaselineGetLog())) ? DuplicateString(g_pass) : 
        FormatAllocateString("Package '%s' is installed or package '%s' is installed", g_inetd, g_inetUtilsInetd);
}

static char* AuditEnsureXinetdNotInstalled(void)
{
    return CheckPackageInstalled(g_xinetd, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_xinetd);
}

static char* AuditEnsureAllTelnetdPackagesUninstalled(void)
{
    return CheckPackageInstalled("*telnetd*", SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : DuplicateString("A 'telnetd' package is installed");
}

static char* AuditEnsureRshServerNotInstalled(void)
{
    return CheckPackageInstalled(g_rshServer, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_rshServer);
}

static char* AuditEnsureNisNotInstalled(void)
{
    return CheckPackageInstalled(g_nis, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_nis);
}

static char* AuditEnsureTftpdNotInstalled(void)
{
    return CheckPackageInstalled(g_tftpd, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_tftpd);
}

static char* AuditEnsureReadaheadFedoraNotInstalled(void)
{
    return CheckPackageInstalled(g_readAheadFedora, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_readAheadFedora);
}

static char* AuditEnsureBluetoothHiddNotInstalled(void)
{
    return ((0 != CheckPackageInstalled(g_bluetooth, SecurityBaselineGetLog())) && (false == IsDaemonActive(g_bluetooth, SecurityBaselineGetLog()))) ?
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed or service '%s' is active", g_bluetooth, g_bluetooth);
}

static char* AuditEnsureIsdnUtilsBaseNotInstalled(void)
{
    return CheckPackageInstalled(g_isdnUtilsBase, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_isdnUtilsBase);
}

static char* AuditEnsureIsdnUtilsKdumpToolsNotInstalled(void)
{
    return CheckPackageInstalled(g_kdumpTools, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_kdumpTools);
}

static char* AuditEnsureIscDhcpdServerNotInstalled(void)
{
    return CheckPackageInstalled(g_iscDhcpServer, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_iscDhcpServer);
}

static char* AuditEnsureSendmailNotInstalled(void)
{
    return CheckPackageInstalled(g_sendmail, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("%s is installed", g_sendmail);
}

static char* AuditEnsureSldapdNotInstalled(void)
{
    return CheckPackageInstalled(g_slapd, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("%s is installed", g_slapd);
}

static char* AuditEnsureBind9NotInstalled(void)
{
    return CheckPackageInstalled(g_bind9, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_bind9);
}

static char* AuditEnsureDovecotCoreNotInstalled(void)
{
    return CheckPackageInstalled(g_dovecotCore, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_dovecotCore);
}

static char* AuditEnsureAuditdInstalled(void)
{
    return CheckPackageInstalled(g_auditd, SecurityBaselineGetLog()) ? 
        FormatAllocateString("Package '%s' is not installed", g_auditd) : DuplicateString(g_pass);
}

static char* AuditEnsureAllEtcPasswdGroupsExistInEtcGroup(void)
{
    char* reason = NULL;
    return CheckAllEtcPasswdGroupsExistInEtcGroup(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureNoDuplicateUidsExist(void)
{
    char* reason = NULL;
    return CheckNoDuplicateUidsExist(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureNoDuplicateGidsExist(void)
{
    char* reason = NULL;
    return CheckNoDuplicateGidsExist(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureNoDuplicateUserNamesExist(void)
{
    char* reason = NULL;
    return CheckNoDuplicateUserNamesExist(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureNoDuplicateGroupsExist(void)
{
    char* reason = NULL;
    return CheckNoDuplicateGroupsExist(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureShadowGroupIsEmpty(void)
{
    char* reason = NULL;
    return CheckShadowGroupIsEmpty(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureRootGroupExists(void)
{
    char* reason = NULL;
    return CheckRootGroupExists(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureAllAccountsHavePasswords(void)
{
    char* reason = NULL;
    return CheckAllUsersHavePasswordsSet(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(void)
{
    char* reason = NULL;
    return CheckRootIsOnlyUidZeroAccount(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcPasswd(void)
{
    return CheckNoLegacyPlusEntriesInFile("etc/passwd", SecurityBaselineGetLog()) ? 
        DuplicateString("'+' lines found in /etc/passwd") : DuplicateString(g_pass);
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcShadow(void)
{
    return CheckNoLegacyPlusEntriesInFile("etc/shadow", SecurityBaselineGetLog()) ? 
        DuplicateString(DuplicateString("'+' lines found in /etc/shadow")) : DuplicateString(g_pass);
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcGroup(void)
{
    return CheckNoLegacyPlusEntriesInFile("etc/group", SecurityBaselineGetLog()) ? 
        DuplicateString(DuplicateString("'+' lines found in /etc/group")) : DuplicateString(g_pass);
}

static char* AuditEnsureDefaultRootAccountGroupIsGidZero(void)
{
    char* reason = NULL;
    return CheckDefaultRootAccountGroupIsGidZero(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureRootIsOnlyUidZeroAccount(void)
{
    char* reason = NULL;
    return ((0 == CheckRootGroupExists(&reason, SecurityBaselineGetLog())) && 
        (0 == CheckRootIsOnlyUidZeroAccount(&reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureAllUsersHomeDirectoriesExist(void)
{
    char* reason = NULL;
    return CheckAllUsersHomeDirectoriesExist(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureUsersOwnTheirHomeDirectories(void)
{
    char* reason = NULL;
    return CheckUsersOwnTheirHomeDirectories(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureRestrictedUserHomeDirectories(void)
{
    unsigned int modes[] = {700, 750};
    char* reason = NULL;
    
    return CheckRestrictedUserHomeDirectories(modes, ARRAY_SIZE(modes), &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsurePasswordHashingAlgorithm(void)
{
    char* reason = NULL;
    return CheckPasswordHashingAlgorithm(sha512, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureMinDaysBetweenPasswordChanges(void)
{
    char* reason = NULL;
    return CheckMinDaysBetweenPasswordChanges(g_minDaysBetweenPasswordChanges, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureInactivePasswordLockPeriod(void)
{
    char* reason = NULL;
    return ((0 == CheckLockoutAfterInactivityLessThan(g_maxInactiveDays, &reason, SecurityBaselineGetLog())) &&
        (0 == CheckUsersRecordedPasswordChangeDates(&reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureMaxDaysBetweenPasswordChanges(void)
{
    char* reason = NULL;
    return CheckMaxDaysBetweenPasswordChanges(g_maxDaysBetweenPasswordChanges, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsurePasswordExpiration(void)
{
    char* reason = NULL;
    return CheckPasswordExpirationLessThan(g_passwordExpiration, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsurePasswordExpirationWarning(void)
{
    char* reason = NULL;
    return CheckPasswordExpirationWarning(g_passwordExpirationWarning, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureSystemAccountsAreNonLogin(void)
{
    char* reason = NULL;
    return CheckSystemAccountsAreNonLogin(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureAuthenticationRequiredForSingleUserMode(void)
{
    char* reason = NULL;
    return CheckRootPasswordForSingleUserMode(&reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsurePrelinkIsDisabled(void)
{
    return CheckPackageInstalled(g_prelink, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_prelink);
}

static char* AuditEnsureTalkClientIsNotInstalled(void)
{
    return CheckPackageInstalled(g_talk, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is installed", g_talk);
}

static char* AuditEnsureDotDoesNotAppearInRootsPath(void)
{
    const char* path = "PATH";
    const char* dot = ".";
    char* reason = NULL;

    return ((0 != FindTextInEnvironmentVariable(path, dot, false, &reason, SecurityBaselineGetLog()) &&
        (0 != FindMarkedTextInFile("/etc/sudoers", "secure_path", dot, &reason, SecurityBaselineGetLog())) &&
        (0 != FindMarkedTextInFile(g_etcEnvironment, path, dot, &reason, SecurityBaselineGetLog())) &&
        (0 != FindMarkedTextInFile(g_etcProfile, path, dot, &reason, SecurityBaselineGetLog())) &&
        (0 != FindMarkedTextInFile("/root/.profile", path, dot, &reason, SecurityBaselineGetLog())))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureCronServiceIsEnabled(void)
{
    return (0 == CheckPackageInstalled(g_cron, SecurityBaselineGetLog()) &&
        CheckIfDaemonActive(g_cron, SecurityBaselineGetLog())) ? DuplicateString(g_pass) : 
        FormatAllocateString("Package '%s' is not installed or service '%s' is not running", g_cron, g_cron);
}

static char* AuditEnsureRemoteLoginWarningBannerIsConfigured(void)
{
    return ((0 != FindTextInFile(g_etcIssueNet, "\\m", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssueNet, "\\r", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssueNet, "\\s", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssueNet, "\\v", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'\\m', '\\r', '\\s' or '\\v' is found in %s", g_etcIssueNet);
}

static char* AuditEnsureLocalLoginWarningBannerIsConfigured(void)
{
    return ((0 != FindTextInFile(g_etcIssue, "\\m", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssue, "\\r", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssue, "\\s", SecurityBaselineGetLog())) &&
        (0 != FindTextInFile(g_etcIssue, "\\v", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'\\m', '\\r', '\\s' or '\\v' is found in %s", g_etcIssue);
}

static char* AuditEnsureAuditdServiceIsRunning(void)
{
    return CheckIfDaemonActive(g_auditd, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Service '%s' is not running", g_auditd);
}

static char* AuditEnsureSuRestrictedToRootGroup(void)
{
    return (0 == FindTextInFile("/etc/pam.d/su", "use_uid", SecurityBaselineGetLog())) ? 
        DuplicateString(g_pass) : DuplicateString("'use_uid' is not found in /etc/pam.d/su");
}

static char* AuditEnsureDefaultUmaskForAllUsers(void)
{
    char* reason = NULL;
    return CheckLoginUmask("077", &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureAutomountingDisabled(void)
{
    const char* autofs = "autofs";
    return (CheckPackageInstalled(autofs, SecurityBaselineGetLog()) 
        && (false == CheckIfDaemonActive(autofs, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("Package '%s' is not installed or service '%s' is not running", autofs, autofs);
}

static char* AuditEnsureKernelCompiledFromApprovedSources(void)
{
    char* reason = NULL;
    return (true == CheckOsAndKernelMatchDistro(&reason, SecurityBaselineGetLog())) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureDefaultDenyFirewallPolicyIsSet(void)
{
    const char* readIpTables = "iptables -S";
    char* reason = NULL;

    return ((0 == FindTextInCommandOutput(readIpTables, "-P INPUT DROP", &reason, SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(readIpTables, "-P FORWARD DROP", &reason, SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(readIpTables, "-P OUTPUT DROP", &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsurePacketRedirectSendingIsDisabled(void)
{
    const char* command = "sysctl -a";
    char* reason = NULL;

    return ((0 == FindTextInCommandOutput(command, "net.ipv4.conf.all.send_redirects = 0", &reason, SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv4.conf.default.send_redirects = 0", &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureIcmpRedirectsIsDisabled(void)
{
    const char* command = "sysctl -a";
    char* reason = NULL;

    return ((0 == FindTextInCommandOutput(command, "net.ipv4.conf.default.accept_redirects = 0", &reason, SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv6.conf.default.accept_redirects = 0", &reason, SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv4.conf.all.accept_redirects = 0", &reason, SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv6.conf.all.accept_redirects = 0", &reason, SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv4.conf.default.secure_redirects = 0", &reason, SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv4.conf.all.secure_redirects = 0", &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureSourceRoutedPacketsIsDisabled(void)
{
    return ((EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/conf/all/accept_source_route", '#', "0", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv6/conf/all/accept_source_route", '#', "0", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        DuplicateString("'0' is not found in /proc/sys/net/ipv4/conf/all/accept_source_route or in /proc/sys/net/ipv6/conf/all/accept_source_route");
}

static char* AuditEnsureAcceptingSourceRoutedPacketsIsDisabled(void)
{
    return ((EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/conf/all/accept_source_route", '#', "0", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv6/conf/default/accept_source_route", '#', "0", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        DuplicateString("'0' is not found in /proc/sys/net/ipv4/conf/all/accept_source_route or in /proc/sys/net/ipv6/conf/default/accept_source_route");
}

static char* AuditEnsureIgnoringBogusIcmpBroadcastResponses(void)
{
    return (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/icmp_ignore_bogus_error_responses", '#', "1", SecurityBaselineGetLog())) ? 
        DuplicateString(g_pass) : DuplicateString("'1' is not found in /proc/sys/net/ipv4/icmp_ignore_bogus_error_responses");
}

static char* AuditEnsureIgnoringIcmpEchoPingsToMulticast(void)
{
    return (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/icmp_echo_ignore_broadcasts", '#', "1", SecurityBaselineGetLog())) ? DuplicateString(g_pass) : 
        DuplicateString("'1' is not found in /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts");
}

static char* AuditEnsureMartianPacketLoggingIsEnabled(void)
{
    const char* command = "sysctl -a";
    char* reason = NULL;

    return ((0 == FindTextInCommandOutput(command, "net.ipv4.conf.all.log_martians = 1", &reason, SecurityBaselineGetLog())) &&
        (0 == FindTextInCommandOutput(command, "net.ipv4.conf.default.log_martians = 1", &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureReversePathSourceValidationIsEnabled(void)
{
    return ((EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/conf/all/rp_filter", '#', "1", SecurityBaselineGetLog())) && 
        (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/conf/default/rp_filter", '#', "1", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        DuplicateString("'1' not found in /proc/sys/net/ipv4/conf/all/rp_filter or in /proc/sys/net/ipv4/conf/default/rp_filter");
}

static char* AuditEnsureTcpSynCookiesAreEnabled(void)
{
    return (EEXIST == CheckLineNotFoundOrCommentedOut("/proc/sys/net/ipv4/tcp_syncookies", '#', "1", SecurityBaselineGetLog())) ? DuplicateString(g_pass) : 
        DuplicateString("'1' not found in /proc/sys/net/ipv4/tcp_syncookies");
}

static char* AuditEnsureSystemNotActingAsNetworkSniffer(void)
{
    const char* command = "/sbin/ip addr list";
    const char* text = "PROMISC";

    return (FindTextInCommandOutput(command, text, NULL, SecurityBaselineGetLog()) &&
        (0 == CheckLineNotFoundOrCommentedOut("/etc/network/interfaces", '#', text, SecurityBaselineGetLog())) &&
        (0 == CheckLineNotFoundOrCommentedOut("/etc/rc.local", '#', text, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'%s' is not found in command '%s' output or found in /etc/network/interfaces or in /etc/rc.local", text, command);
}

static char* AuditEnsureAllWirelessInterfacesAreDisabled(void)
{
    return FindTextInCommandOutput("/sbin/iwconfig 2>&1 | /bin/egrep -v 'no wireless extensions|not found'", "Frequency", NULL, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : DuplicateString("'Frequency' found in '/sbin/iwconfig 2>&1 | /bin/egrep -v 'no wireless extensions|not found' output, indicating at least one active wireless interface");
}

static char* AuditEnsureIpv6ProtocolIsEnabled(void)
{
    char* reason = NULL;
    return (0 == FindTextInCommandOutput("cat /sys/module/ipv6/parameters/disable", "0", &reason, SecurityBaselineGetLog())) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureDccpIsDisabled(void)
{
    return FindTextInFolder(g_etcModProbeD, "install dccp /bin/true", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'%s' is not found in any file under %s", "install dccp /bin/true", g_etcModProbeD) : DuplicateString(g_pass);
}

static char* AuditEnsureSctpIsDisabled(void)
{
    return FindTextInFolder(g_etcModProbeD, "install sctp /bin/true", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'%s' is not found in any file under %s", "install sctp /bin/true", g_etcModProbeD) : DuplicateString(g_pass);
}

static char* AuditEnsureDisabledSupportForRds(void)
{
    return FindTextInFolder(g_etcModProbeD, "install rds /bin/true", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'%s' is not found in any file under %s", "install rds /bin/true", g_etcModProbeD) : DuplicateString(g_pass);
}

static char* AuditEnsureTipcIsDisabled(void)
{
    return FindTextInFolder(g_etcModProbeD, "install tipc /bin/true", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'%s' is not found in any file under %s", "install tipc /bin/true", g_etcModProbeD) : DuplicateString(g_pass);
}

static char* AuditEnsureZeroconfNetworkingIsDisabled(void)
{
    return CheckLineNotFoundOrCommentedOut("/etc/network/interfaces", '#', "ipv4ll", SecurityBaselineGetLog()) ? 
        DuplicateString("'ipv4ll' is found in /etc/network/interfaces") : DuplicateString(g_pass);
}

static char* AuditEnsurePermissionsOnBootloaderConfig(void)
{
    char* reason = NULL;
    return ((0 == CheckFileAccess("/boot/grub/grub.conf", 0, 0, 400, &reason, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess("/boot/grub/grub.cfg", 0, 0, 400, &reason, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess("/boot/grub2/grub.cfg", 0, 0, 400, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsurePasswordReuseIsLimited(void)
{
    //TBD: refine this and expand to other distros
    int option = 0;
    return (5 >= (option = GetIntegerOptionFromFile(g_etcPamdCommonPassword, "remember", '=', SecurityBaselineGetLog()))) ?
        ((-999 == option) ? FormatAllocateString("A 'remember' option is not found in %s", g_etcPamdCommonPassword) :
        FormatAllocateString("A 'remember' option is set to '%d' in %s instead of expected '5' or greater", option, g_etcPamdCommonPassword)) :
        DuplicateString(g_pass);
}

static char* AuditEnsureMountingOfUsbStorageDevicesIsDisabled(void)
{
    return FindTextInFolder(g_etcModProbeD, "install usb-storage /bin/true", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'install usb-storage /bin/true' is not found in any file under %s", g_etcModProbeD) : DuplicateString(g_pass);
}

static char* AuditEnsureCoreDumpsAreRestricted(void)
{
    const char* fsSuidDumpable = "fs.suid_dumpable = 0";

    return (((EEXIST == CheckLineNotFoundOrCommentedOut("/etc/security/limits.conf", '#', "hard core 0", SecurityBaselineGetLog())) ||
        (0 == FindTextInFolder("/etc/security/limits.d", fsSuidDumpable, SecurityBaselineGetLog()))) &&
        (0 == FindTextInCommandOutput("sysctl -a", fsSuidDumpable, NULL, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        DuplicateString("Line 'hard core 0' is not found in /etc/security/limits.conf, or 'fs.suid_dumpable = 0' is not found in /etc/security/limits.d or in output from 'sysctl -a'");
}

static char* AuditEnsurePasswordCreationRequirements(void)
{
    int minlenOption = 0;
    int minclassOption = 0;
    int dcreditOption = 0;
    int ucreditOption = 0;
    int ocreditOption = 0;
    int lcreditOption = 0;
    
    //TBD: expand to other distros
    return ((14 == (minlenOption = GetIntegerOptionFromFile(g_etcPamdCommonPassword, "minlen", '=', SecurityBaselineGetLog()))) &&
        (4 == (minclassOption = GetIntegerOptionFromFile(g_etcPamdCommonPassword, "minclass", '=', SecurityBaselineGetLog()))) &&
        (-1 == (dcreditOption = GetIntegerOptionFromFile(g_etcPamdCommonPassword, "dcredit", '=', SecurityBaselineGetLog()))) &&
        (-1 == (ucreditOption = GetIntegerOptionFromFile(g_etcPamdCommonPassword, "ucredit", '=', SecurityBaselineGetLog()))) &&
        (-1 == (ocreditOption = GetIntegerOptionFromFile(g_etcPamdCommonPassword, "ocredit", '=', SecurityBaselineGetLog()))) &&
        (-1 == (lcreditOption = GetIntegerOptionFromFile(g_etcPamdCommonPassword, "lcredit", '=', SecurityBaselineGetLog())))) ? DuplicateString(g_pass) :
        FormatAllocateString("In %s, 'minlen' missing or set to %d instead of 14, 'minclass' missing or set to %d instead of 4, "
            "or: 'dcredit', 'ucredit', 'ocredit' or 'lcredit' missing or set to %d, %d, %d, %d respectively instead of -1 each",
            g_etcPamdCommonPassword, minlenOption, minclassOption, dcreditOption, ucreditOption, ocreditOption, lcreditOption);
}

static char* AuditEnsureLockoutForFailedPasswordAttempts(void)
{
    //TBD: expand to other distros
    const char* passwordAuth = "/etc/pam.d/password-auth";
    
    return ((0 == CheckLockoutForFailedPasswordAttempts(passwordAuth, SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(passwordAuth, '#', "auth", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(passwordAuth, '#', "pam_tally2.so", SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(passwordAuth, '#', "file=/var/log/tallylog", SecurityBaselineGetLog())) &&
        (0 < GetIntegerOptionFromFile(passwordAuth, "deny", '=', SecurityBaselineGetLog())) &&
        (0 < GetIntegerOptionFromFile(passwordAuth, "unlock_time", '=', SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("In %s: lockout for failed password attempts not set, 'auth', 'pam_tally2.so', 'file=/var/log/tallylog' "
            "not found, 'deny' or 'unlock_time' is not found or not set to greater than 0", passwordAuth);
}

static char* AuditEnsureDisabledInstallationOfCramfsFileSystem(void)
{
    return FindTextInFolder(g_etcModProbeD, "install cramfs", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'install cramfs' is not found in any file under %s", g_etcModProbeD) : DuplicateString(g_pass);
}

static char* AuditEnsureDisabledInstallationOfFreevxfsFileSystem(void)
{
    return FindTextInFolder(g_etcModProbeD, "install freevxfs", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'install freevxfs' is not found in any file under %s", g_etcModProbeD) : DuplicateString(g_pass);
}

static char* AuditEnsureDisabledInstallationOfHfsFileSystem(void)
{
    return FindTextInFolder(g_etcModProbeD, "install hfs", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'install hfs' is not found  in any file under %s", g_etcModProbeD) : DuplicateString(g_pass);
}

static char* AuditEnsureDisabledInstallationOfHfsplusFileSystem(void)
{
    return FindTextInFolder(g_etcModProbeD, "install hfsplus", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'install hfsplus' is not found  in any file under %s", g_etcModProbeD) : DuplicateString(g_pass);
}

static char* AuditEnsureDisabledInstallationOfJffs2FileSystem(void)
{
    return FindTextInFolder(g_etcModProbeD, "install jffs2", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'install jffs2' is not found  in any file under %s", g_etcModProbeD) : DuplicateString(g_pass);
}

static char* AuditEnsureVirtualMemoryRandomizationIsEnabled(void)
{
    return ((0 == CompareFileContents("/proc/sys/kernel/randomize_va_space", "2", SecurityBaselineGetLog())) ||
        (0 == CompareFileContents("/proc/sys/kernel/randomize_va_space", "1", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        DuplicateString("/proc/sys/kernel/randomize_va_space content is not '2' and /proc/sys/kernel/randomize_va_space content is not '1'");
}

static char* AuditEnsureAllBootloadersHavePasswordProtectionEnabled(void)
{
    const char* password = "password";
    return ((EEXIST == CheckLineNotFoundOrCommentedOut("/boot/grub/grub.cfg", '#', password, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut("/boot/grub/grub.conf", '#', password, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut("/boot/grub2/grub.conf", '#', password, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        DuplicateString("Line 'password' is not found in /boot/grub/grub.cfg, in /boot/grub/grub.conf and in /boot/grub2/grub.conf");
}

static char* AuditEnsureLoggingIsConfigured(void)
{
    return CheckFileExists("/var/log/syslog", SecurityBaselineGetLog()) ? 
        DuplicateString("/var/log/syslog is not found") : DuplicateString(g_pass);
}

static char* AuditEnsureSyslogPackageIsInstalled(void)
{
    return ((0 == CheckPackageInstalled(g_syslog, SecurityBaselineGetLog())) ||
        (0 == CheckPackageInstalled(g_rsyslog, SecurityBaselineGetLog())) ||
        (0 == CheckPackageInstalled(g_syslogNg, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("Packages '%s', '%s' and '%s' are not installed", g_syslog, g_rsyslog, g_syslogNg);
}

static char* AuditEnsureSystemdJournaldServicePersistsLogMessages(void)
{
    char* reason = NULL;
    char* status = ((0 == CheckPackageInstalled(g_systemd, SecurityBaselineGetLog())) && 
        (0 == CheckDirectoryAccess("/var/log/journal", 0, -1, 2775, false, &reason, SecurityBaselineGetLog()))) ? 
        DuplicateString(g_pass) : FormatAllocateString("Package '%s' is not installed, or: %s", g_systemd, reason);
    FREE_MEMORY(reason);
    return status;
}

static char* AuditEnsureALoggingServiceIsEnabled(void)
{
    return ((CheckPackageInstalled(g_syslogNg, SecurityBaselineGetLog()) && CheckPackageInstalled(g_systemd, SecurityBaselineGetLog()) && CheckIfDaemonActive(g_rsyslog, SecurityBaselineGetLog())) ||
        (CheckPackageInstalled(g_rsyslog, SecurityBaselineGetLog()) && CheckPackageInstalled(g_systemd, SecurityBaselineGetLog()) && CheckIfDaemonActive(g_syslogNg, SecurityBaselineGetLog())) ||
        ((0 == CheckPackageInstalled(g_systemd, SecurityBaselineGetLog())) && CheckIfDaemonActive(g_systemdJournald, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'%s' or '%s' is not installed or '%s' is not running, and: '%s' or '%s' are not installed or '%s' is not running, and: '%s' is not installed or '%s' is not running",
            g_syslogNg, g_systemd, g_rsyslog, g_rsyslog, g_systemd, g_syslogNg, g_systemd, g_systemdJournald);
}

static char* AuditEnsureFilePermissionsForAllRsyslogLogFiles(void)
{
    const char* fileCreateMode = "$FileCreateMode";
    int mode = 0, modeNg = 0;
    return ((600 == (mode = GetIntegerOptionFromFile(g_etcRsyslogConf, fileCreateMode, ' ', SecurityBaselineGetLog())) || (640 == mode)) &&
        ((EEXIST == CheckFileExists(g_etcSyslogNgSyslogNgConf, SecurityBaselineGetLog())) ||
        ((600 == (modeNg = GetIntegerOptionFromFile(g_etcSyslogNgSyslogNgConf, fileCreateMode, ' ', SecurityBaselineGetLog()))) || (640 == modeNg)))) ? DuplicateString(g_pass) : 
        FormatAllocateString("Option '%d' is not found in %s or is found set to %d instead of 600 or 640, or %s exists, or option '%s' is not found in %s or found set to %d instead of 600 or 640",
            fileCreateMode, g_etcRsyslogConf, mode, g_etcSyslogNgSyslogNgConf, fileCreateMode, g_etcSyslogNgSyslogNgConf, modeNg);
}

static char* AuditEnsureLoggerConfigurationFilesAreRestricted(void)
{
    char* reason = NULL;
    return ((0 == CheckFileAccess(g_etcSyslogNgSyslogNgConf, 0, 0, 640, &reason, SecurityBaselineGetLog())) && 
        (0 == CheckFileAccess(g_etcRsyslogConf, 0, 0, 640, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : reason;
}

static char* AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(void)
{
    return ((0 == FindTextInFile(g_etcRsyslogConf, "FileGroup adm", SecurityBaselineGetLog())) &&
        (0 != CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "FileGroup adm", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'FileGroup adm' is not found in %s or is found in %s", g_etcRsyslogConf, g_etcRsyslogConf);
}

static char* AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(void)
{
    return ((0 == FindTextInFile(g_etcRsyslogConf, "FileOwner syslog", SecurityBaselineGetLog())) &&
        (0 != CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "FileOwner syslog", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) :
        FormatAllocateString("'FileOwner syslog' is not found in %s, or 'FileOwner syslog' is found in %s", g_etcRsyslogConf, g_etcRsyslogConf);
}

static char* AuditEnsureRsyslogNotAcceptingRemoteMessages(void)
{
    return ((0 == CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "ModLoad imudp", SecurityBaselineGetLog())) &&
        (0 == CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "ModLoad imtcp", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'ModLoad imudp' is not found in %s, or 'ModLoad imtcp' is not found in %s", g_etcRsyslogConf, g_etcRsyslogConf);
}

static char* AuditEnsureSyslogRotaterServiceIsEnabled(void)
{
    const char* version = "18.04"; 
    char* osName = NULL;
    char* osVersion = NULL;
    char* reason = NULL;
    char* status = ((0 == CheckPackageInstalled("logrotate", SecurityBaselineGetLog())) &&
        ((((NULL != (osName = GetOsName(SecurityBaselineGetLog()))) && (0 == strcmp(osName, "Ubuntu")) && FreeAndReturnTrue(osName)) &&
        ((NULL != (osVersion = GetOsVersion(SecurityBaselineGetLog()))) && (0 == strncmp(osVersion, version, strlen(version))) && FreeAndReturnTrue(osVersion))) ||
        CheckIfDaemonActive("logrotate.timer", SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess("/etc/cron.daily/logrotate", 0, 0, 755, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("The 'logrotate' package is not installed, or the 'logrotate.timer' service is not running, or: %s", reason);
    FREE_MEMORY(reason);
    return status;
}

static char* AuditEnsureTelnetServiceIsDisabled(void)
{
    return CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "telnet", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'telnet' is not found in %s", g_etcInetdConf) : DuplicateString(g_pass);
}

static char* AuditEnsureRcprshServiceIsDisabled(void)
{
    return CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "shell", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'shell' is not found in %s", g_etcInetdConf) : DuplicateString(g_pass);
}

static char* AuditEnsureTftpServiceisDisabled(void)
{
    return CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "tftp", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'tftp' is not found in %s", g_etcInetdConf) : DuplicateString(g_pass);
}

static char* AuditEnsureAtCronIsRestrictedToAuthorizedUsers(void)
{
    const char* etcCronAllow = "/etc/cron.allow";
    const char* etcAtAllow = "/etc/at.allow";
    char* reason = NULL;
    char* status = ((EEXIST == CheckFileExists("/etc/cron.deny", SecurityBaselineGetLog())) &&
        (EEXIST == CheckFileExists("/etc/at.deny", SecurityBaselineGetLog())) &&
        (0 == CheckFileExists(etcCronAllow, SecurityBaselineGetLog())) &&
        (0 == CheckFileExists(etcAtAllow, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess(etcCronAllow, 0, 0, 600, &reason, SecurityBaselineGetLog())) &&
        (0 == CheckFileAccess(etcAtAllow, 0, 0, 600, &reason, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("/etc/cron.deny, or /etc/at.deny, or %s, or %s missing, or: %s", 
            etcCronAllow, etcAtAllow, reason ? reason : "/etc/at.allow access not set to 600");
    FREE_MEMORY(reason);
    return status;
}

static char* AuditEnsureSshBestPracticeProtocol(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "Protocol 2", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'Protocol 2' is not found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureSshBestPracticeIgnoreRhosts(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "IgnoreRhosts yes", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'IgnoreRhosts yes' is not found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureSshLogLevelIsSet(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "LogLevel INFO", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'LogLevel INFO' is not found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureSshMaxAuthTriesIsSet(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "MaxAuthTries 6", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'MaxAuthTries 6' is not found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureSshAccessIsLimited(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "AllowUsers", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "AllowGroups", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "DenyUsers", SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "DenyGroups", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'AllowUsers', 'AllowGroups', 'DenyUsers' and 'DenyGroups' are not all found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureSshRhostsRsaAuthenticationIsDisabled(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "RhostsRSAAuthentication no", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'RhostsRSAAuthentication no' is not found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureSshHostbasedAuthenticationIsDisabled(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "HostbasedAuthentication no", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'HostbasedAuthentication no' is not found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureSshPermitRootLoginIsDisabled(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "PermitRootLogin no", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'PermitRootLogin no' is not found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureSshPermitEmptyPasswordsIsDisabled(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "PermitEmptyPasswords no", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'PermitEmptyPasswords no' is not found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureSshIdleTimeoutIntervalIsConfigured(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) || 
        ((0 == GetIntegerOptionFromFile(g_etcSshSshdConfig, "ClientAliveCountMax", ' ', SecurityBaselineGetLog())) &&
        (0 < GetIntegerOptionFromFile(g_etcSshSshdConfig, "ClientAliveInterval", ' ', SecurityBaselineGetLog())))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'ClientAliveCountMax' set to 0 and 'ClientAliveInterval' set to a positive value are not both found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureSshLoginGraceTimeIsSet(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "LoginGraceTime", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'LoginGraceTime' is not found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureOnlyApprovedMacAlgorithmsAreUsed(void)
{
    const char* macs[] = {"hmac-sha2-256", "hmac-sha2-256-etm@openssh.com", "hmac-sha2-512", "hmac-sha2-512-etm@openssh.com"};
    char* reason = NULL;
    return CheckOnlyApprovedMacAlgorithmsAreUsed(macs, ARRAY_SIZE(macs), &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureSshWarningBannerIsEnabled(void)
{
    return ((EEXIST == CheckFileExists(g_etcSshSshdConfig, SecurityBaselineGetLog())) ||
        (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "Banner /etc/azsec/banner.txt", SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("'Banner /etc/azsec/banner.txt' is not found uncommented with '#' in %s", g_etcSshSshdConfig);
}

static char* AuditEnsureUsersCannotSetSshEnvironmentOptions(void)
{
    return CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', "PermitUserEnvironment yes", SecurityBaselineGetLog()) ? 
        FormatAllocateString("'PermitUserEnvironment yes' is not found uncommented with '#' in %s", g_etcSshSshdConfig) : DuplicateString(g_pass);
}

static char* AuditEnsureAppropriateCiphersForSsh(void)
{
    const char* ciphers[] = {"aes128-ctr", "aes192-ctr", "aes256-ctr"};
    char* reason = NULL;
    return CheckAppropriateCiphersForSsh(ciphers, ARRAY_SIZE(ciphers), &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureAvahiDaemonServiceIsDisabled(void)
{
    return (false == CheckIfDaemonActive(g_avahiDaemon, SecurityBaselineGetLog())) ? DuplicateString(g_pass) : 
        FormatAllocateString("Sevice '%s' is not running", g_avahiDaemon);
}

static char* AuditEnsureCupsServiceisDisabled(void)
{
    return (CheckPackageInstalled(g_cups, SecurityBaselineGetLog()) &&
        (false == CheckIfDaemonActive(g_cups, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("Package '%s' is not installed or service '%s' is not running", g_cups, g_cups);
}

static char* AuditEnsurePostfixPackageIsUninstalled(void)
{
    return CheckPackageInstalled(g_postfix, SecurityBaselineGetLog()) ? DuplicateString(g_pass) : 
        FormatAllocateString("Package '%s' is not installed", g_postfix);
}

static char* AuditEnsurePostfixNetworkListeningIsDisabled(void)
{
    return ((0 == CheckFileExists("/etc/postfix/main.cf", SecurityBaselineGetLog())) && 
        ((0 == FindTextInFile("/etc/postfix/main.cf", "inet_interfaces localhost", SecurityBaselineGetLog())))) ? DuplicateString(g_pass) : 
        DuplicateString("/etc/postfix/main.cf is not found, or 'inet_interfaces localhost' is not found in /etc/postfix/main.cf");
}

static char* AuditEnsureRpcgssdServiceIsDisabled(void)
{
    return ((false == CheckIfDaemonActive(g_rpcgssd, SecurityBaselineGetLog())) && 
        (false == CheckIfDaemonActive(g_rpcGssd, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("Service '%s' is not running or service '%s' is not running", g_rpcgssd, g_rpcGssd);
}

static char* AuditEnsureRpcidmapdServiceIsDisabled(void)
{
    return ((false == CheckIfDaemonActive(g_rpcidmapd, SecurityBaselineGetLog())) &&
        (false == CheckIfDaemonActive(g_nfsIdmapd, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("Service '%s' is not running or service '%s' is not running", g_rpcidmapd, g_nfsIdmapd);
}

static char* AuditEnsurePortmapServiceIsDisabled(void)
{
    return ((false == CheckIfDaemonActive(g_rpcbind, SecurityBaselineGetLog())) &&
        (false == CheckIfDaemonActive(g_rpcbindService, SecurityBaselineGetLog())) &&
        (false == CheckIfDaemonActive(g_rpcbindSocket, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("Service '%s', '%s' or '%s' is not running", g_rpcbind, g_rpcbindService, g_rpcbindSocket);
}

static char* AuditEnsureNetworkFileSystemServiceIsDisabled(void)
{
    return CheckIfDaemonActive(g_nfsServer, SecurityBaselineGetLog()) ? 
        FormatAllocateString("Service '%s' is not running", g_nfsServer) : DuplicateString(g_pass);
}

static char* AuditEnsureRpcsvcgssdServiceIsDisabled(void)
{
    return CheckLineNotFoundOrCommentedOut(g_etcInetdConf, '#', "NEED_SVCGSSD = yes", SecurityBaselineGetLog()) ?
        FormatAllocateString("'NEED_SVCGSSD = yes' is not found in %s", g_etcInetdConf) : DuplicateString(g_pass);
}

static char* AuditEnsureSnmpServerIsDisabled(void)
{
    return CheckIfDaemonActive(g_snmpd, SecurityBaselineGetLog()) ? 
        FormatAllocateString("Service '%s' is not running", g_snmpd) : DuplicateString(g_pass);
}

static char* AuditEnsureRsynServiceIsDisabled(void)
{
    return CheckIfDaemonActive(g_rsync, SecurityBaselineGetLog()) ? 
        FormatAllocateString("Service '%s' is not running", g_rsync) : DuplicateString(g_pass);
}

static char* AuditEnsureNisServerIsDisabled(void)
{
    return CheckIfDaemonActive(g_ypserv, SecurityBaselineGetLog()) ? 
        FormatAllocateString("Service '%s' is not running", g_ypserv) : DuplicateString(g_pass);
}

static char* AuditEnsureRshClientNotInstalled(void)
{
    return ((0 != CheckPackageInstalled(g_rsh, SecurityBaselineGetLog())) && 
        (0 != CheckPackageInstalled(g_rshClient, SecurityBaselineGetLog()))) ? DuplicateString(g_pass) : 
        FormatAllocateString("Package '%s' or package '%s' is installed", g_rsh, g_rshClient);
}

static char* AuditEnsureSmbWithSambaIsDisabled(void)
{
    const char* etcSambaConf = "/etc/samba/smb.conf";
    const char* minProtocol = "min protocol = SMB2";

    return (CheckPackageInstalled("samba", SecurityBaselineGetLog()) || 
        ((EEXIST == CheckLineNotFoundOrCommentedOut(etcSambaConf, '#', minProtocol, SecurityBaselineGetLog())) &&
        (EEXIST == CheckLineNotFoundOrCommentedOut(etcSambaConf, ';', minProtocol, SecurityBaselineGetLog())))) ? DuplicateString(g_pass) : 
        FormatAllocateString("Package 'samba' is not installed or '%s' is not found in %s", minProtocol);
}

static char* AuditEnsureUsersDotFilesArentGroupOrWorldWritable(void)
{
    unsigned int modes[] = {600, 644, 664, 700, 744};
    char* reason = NULL;

    return CheckUsersRestrictedDotFiles(modes, ARRAY_SIZE(modes), &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureNoUsersHaveDotForwardFiles(void)
{
    char* reason = NULL;
    return CheckOrEnsureUsersDontHaveDotFiles(g_forward, false, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureNoUsersHaveDotNetrcFiles(void)
{
    char* reason = NULL;
    return CheckOrEnsureUsersDontHaveDotFiles(g_netrc, false, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureNoUsersHaveDotRhostsFiles(void)
{
    char* reason = NULL;
    return CheckOrEnsureUsersDontHaveDotFiles(g_rhosts, false, &reason, SecurityBaselineGetLog()) ? reason : DuplicateString(g_pass);
}

static char* AuditEnsureRloginServiceIsDisabled(void)
{
    return (CheckPackageInstalled(g_inetd, SecurityBaselineGetLog()) && 
        CheckPackageInstalled(g_inetUtilsInetd, SecurityBaselineGetLog()) &&
        FindTextInFile(g_etcInetdConf, "login", SecurityBaselineGetLog())) ? DuplicateString(g_pass) : 
        FormatAllocateString("Package '%s' or '%s' is not installed, or 'login' is not found in %s", g_inetd, g_inetUtilsInetd, g_etcInetdConf);
}

static char* AuditEnsureUnnecessaryAccountsAreRemoved(void)
{
    const char* names[] = {"games"};
    char* reason = NULL;

    return (0 == CheckIfUserAccountsExist(names, ARRAY_SIZE(names), &reason, SecurityBaselineGetLog())) ? reason : DuplicateString(g_pass);
}

AuditCall g_auditChecks[] =
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
    &AuditEnsureALoggingServiceIsEnabled,
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

char* AuditSecurityBaseline(void)
{
    size_t numChecks = ARRAY_SIZE(g_auditChecks);
    size_t i = 0;
    char* _status = NULL;
    char* status = DuplicateString(g_pass);

    for (i = 0; i < numChecks; i++)
    {
        if ((NULL == (_status = g_auditChecks[i]())) || strcmp(_status, g_pass))
        {
            FREE_MEMORY(status);
            status = DuplicateString(_status ? _status : g_fail);
        }
        FREE_MEMORY(_status);
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
    StopAndDisableDaemon(g_bluetooth, SecurityBaselineGetLog());
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
    unsigned int modes[] = {700, 750};

    return SetRestrictedUserHomeDirectories(modes, ARRAY_SIZE(modes), 700, 750, SecurityBaselineGetLog());
}

static int RemediateEnsurePasswordHashingAlgorithm(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMinDaysBetweenPasswordChanges(void)
{
    return SetMinDaysBetweenPasswordChanges(g_minDaysBetweenPasswordChanges, SecurityBaselineGetLog());
}

static int RemediateEnsureInactivePasswordLockPeriod(void)
{
    return SetLockoutAfterInactivityLessThan(g_maxInactiveDays, SecurityBaselineGetLog());
}

static int RemediateEnsureMaxDaysBetweenPasswordChanges(void)
{
    return SetMaxDaysBetweenPasswordChanges(g_maxDaysBetweenPasswordChanges, SecurityBaselineGetLog());
}

static int RemediateEnsurePasswordExpiration(void)
{
    return ((0 == SetMinDaysBetweenPasswordChanges(g_minDaysBetweenPasswordChanges, SecurityBaselineGetLog())) &&
        (0 == SetMaxDaysBetweenPasswordChanges(g_maxDaysBetweenPasswordChanges, SecurityBaselineGetLog())) &&
        (0 == CheckPasswordExpirationLessThan(g_passwordExpiration, NULL, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int RemediateEnsurePasswordExpirationWarning(void)
{
    return SetPasswordExpirationWarning(g_passwordExpirationWarning, SecurityBaselineGetLog());
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
    return ((0 == InstallPackage(g_systemd, SecurityBaselineGetLog()) && 
        ((0 == InstallPackage(g_rsyslog, SecurityBaselineGetLog())) || (0 == InstallPackage(g_syslog, SecurityBaselineGetLog())))) ||
        ((0 == InstallPackage(g_syslogNg, SecurityBaselineGetLog())))) ? 0 : ENOENT;
}

static int RemediateEnsureSystemdJournaldServicePersistsLogMessages(void)
{
    return ((0 == InstallPackage(g_systemd, SecurityBaselineGetLog())) &&
        (0 == SetDirectoryAccess("/var/log/journal", 0, -1, 2775, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int RemediateEnsureALoggingServiceIsEnabled(void)
{
    return ((((0 == InstallPackage(g_systemd, SecurityBaselineGetLog())) && EnableAndStartDaemon(g_systemdJournald, SecurityBaselineGetLog())) &&
        (((0 == InstallPackage(g_rsyslog, SecurityBaselineGetLog())) && EnableAndStartDaemon(g_rsyslog, SecurityBaselineGetLog())) || 
        (((0 == InstallPackage(g_syslog, SecurityBaselineGetLog()) && EnableAndStartDaemon(g_syslog, SecurityBaselineGetLog())))))) ||
        (((0 == InstallPackage(g_syslogNg, SecurityBaselineGetLog())) && EnableAndStartDaemon(g_syslogNg, SecurityBaselineGetLog())))) ? 0 : ENOENT;
}

static int RemediateEnsureFilePermissionsForAllRsyslogLogFiles(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLoggerConfigurationFilesAreRestricted(void)
{
    return ((0 == SetFileAccess(g_etcSyslogNgSyslogNgConf, 0, 0, 640, SecurityBaselineGetLog())) &&
        (0 == SetFileAccess(g_etcRsyslogConf, 0, 0, 640, SecurityBaselineGetLog()))) ? 0 : ENOENT;
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
    StopAndDisableDaemon(g_avahiDaemon, SecurityBaselineGetLog());
    return (0 == strcmp(g_pass, AuditEnsureAvahiDaemonServiceIsDisabled())) ? 0 : ENOENT;
}

static int RemediateEnsureCupsServiceisDisabled(void)
{
    StopAndDisableDaemon(g_cups, SecurityBaselineGetLog());
    return UninstallPackage(g_cups, SecurityBaselineGetLog());
}

static int RemediateEnsurePostfixPackageIsUninstalled(void)
{
    return UninstallPackage(g_postfix, SecurityBaselineGetLog());
}

static int RemediateEnsurePostfixNetworkListeningIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRpcgssdServiceIsDisabled(void)
{
    StopAndDisableDaemon(g_rpcgssd, SecurityBaselineGetLog());
    StopAndDisableDaemon(g_rpcGssd, SecurityBaselineGetLog());
    return (0 == strcmp(g_pass, AuditEnsureRpcgssdServiceIsDisabled())) ? 0 : ENOENT;
}

static int RemediateEnsureRpcidmapdServiceIsDisabled(void)
{
    StopAndDisableDaemon(g_rpcidmapd, SecurityBaselineGetLog());
    StopAndDisableDaemon(g_nfsIdmapd, SecurityBaselineGetLog());
    return (0 == strcmp(g_pass, AuditEnsureRpcidmapdServiceIsDisabled())) ? 0 : ENOENT;
}

static int RemediateEnsurePortmapServiceIsDisabled(void)
{
    StopAndDisableDaemon(g_rpcbind, SecurityBaselineGetLog());
    StopAndDisableDaemon(g_rpcbindService, SecurityBaselineGetLog());
    StopAndDisableDaemon(g_rpcbindSocket, SecurityBaselineGetLog());
    return (0 == strcmp(g_pass, AuditEnsurePortmapServiceIsDisabled())) ? 0 : ENOENT;
}

static int RemediateEnsureNetworkFileSystemServiceIsDisabled(void)
{
    StopAndDisableDaemon(g_nfsServer, SecurityBaselineGetLog());
    return (0 == strcmp(g_pass, AuditEnsureNetworkFileSystemServiceIsDisabled())) ? 0 : ENOENT;
}

static int RemediateEnsureRpcsvcgssdServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns    
}

static int RemediateEnsureSnmpServerIsDisabled(void)
{
    StopAndDisableDaemon(g_snmpd, SecurityBaselineGetLog());
    return (0 == strcmp(g_pass, AuditEnsureSnmpServerIsDisabled())) ? 0 : ENOENT;
}

static int RemediateEnsureRsynServiceIsDisabled(void)
{
    StopAndDisableDaemon(g_rsync, SecurityBaselineGetLog());
    return (0 == strcmp(g_pass, AuditEnsureRsynServiceIsDisabled())) ? 0 : ENOENT;
}

static int RemediateEnsureNisServerIsDisabled(void)
{
    StopAndDisableDaemon(g_ypserv, SecurityBaselineGetLog());
    return (0 == strcmp(g_pass, AuditEnsureNisServerIsDisabled())) ? 0 : ENOENT;
}

static int RemediateEnsureRshClientNotInstalled(void)
{
    return ((0 == UninstallPackage(g_rsh, SecurityBaselineGetLog())) && 
        (0 == UninstallPackage(g_rshClient, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int RemediateEnsureSmbWithSambaIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUsersDotFilesArentGroupOrWorldWritable(void)
{
    unsigned int modes[] = {600, 644, 664, 700, 744};

    return SetUsersRestrictedDotFiles(modes, ARRAY_SIZE(modes), 744, SecurityBaselineGetLog());
}

static int RemediateEnsureNoUsersHaveDotForwardFiles(void)
{
    return CheckOrEnsureUsersDontHaveDotFiles(g_forward, true, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureNoUsersHaveDotNetrcFiles(void)
{
    return CheckOrEnsureUsersDontHaveDotFiles(g_netrc, true, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureNoUsersHaveDotRhostsFiles(void)
{
    return CheckOrEnsureUsersDontHaveDotFiles(g_rhosts, true, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureRloginServiceIsDisabled(void)
{
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUnnecessaryAccountsAreRemoved(void)
{
    const char* names[] = {"games"};

    return RemoveUserAccounts(names, ARRAY_SIZE(names), SecurityBaselineGetLog());
}

RemediationCall g_remediateChecks[] =
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
    &RemediateEnsureALoggingServiceIsEnabled,
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
    char* result = NULL;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

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
            result = AuditSecurityBaseline();
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcIssueObject))
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
        else if (0 == strcmp(objectName, g_auditEnsureSshAccessIsLimitedObject))
        {
            result = AuditEnsureSshAccessIsLimited();
        }
        else if (0 == strcmp(objectName, g_auditEnsureSshRhostsRsaAuthenticationIsDisabledObject))
        {
            result = AuditEnsureSshRhostsRsaAuthenticationIsDisabled();
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
        else if (0 == strcmp(objectName, g_auditEnsureSshIdleTimeoutIntervalIsConfiguredObject))
        {
            result = AuditEnsureSshIdleTimeoutIntervalIsConfigured();
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
            OsConfigLogError(SecurityBaselineGetLog(), "MmiGet called for an unsupported object (%s)", objectName);
            status = EINVAL;
        }
    }

    if (MMI_OK == status)
    {
        if (NULL == result)
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s): audit failure without a reason", componentName, objectName);
            result = DuplicateString(g_fail);

            if (NULL == result)
            {
                OsConfigLogError(SecurityBaselineGetLog(), "MmiGet: DuplicateString failed");
                status = ENOMEM;
            }
        }
        
        if (result)
        {
            *payloadSizeBytes = strlen(result) + 2;

            if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
            {
                OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s) insufficient max size (%d bytes) vs actual size (%d bytes), report will be truncated",
                    componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);

                *payloadSizeBytes = g_maxPayloadSizeBytes;
            }

            *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes + 1);
            if (*payload)
            {
                memset(*payload, 0, *payloadSizeBytes + 1);
                snprintf(*payload, *payloadSizeBytes + 1, "\"%s\"", result);
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "MmiGet: failed to allocate %d bytes", *payloadSizeBytes + 1);
                *payloadSizeBytes = 0;
                status = ENOMEM;
            }
        }
    }    

    OsConfigLogInfo(SecurityBaselineGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);

    FREE_MEMORY(result);

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
        else if (0 == strcmp(objectName, g_remediateEnsureALoggingServiceIsEnabledObject))
        {
            status = RemediateEnsureALoggingServiceIsEnabled();
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