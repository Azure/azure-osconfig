// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
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
#include <Mmi.h>

#include "SecurityBaseline.h"

static const char* g_securityBaselineModuleName = "OSConfig SecurityBaseline module";
static const char* g_securityBaselineComponentName = "SecurityBaseline";

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

static const char* g_pass = SECURITY_AUDIT_PASS;
static const char* g_fail = SECURITY_AUDIT_FAIL;

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
    InitializeSshAudit(SecurityBaselineGetLog());
    OsConfigLogInfo(SecurityBaselineGetLog(), "%s initialized", g_securityBaselineModuleName);
}

void SecurityBaselineShutdown(void)
{
    OsConfigLogInfo(SecurityBaselineGetLog(), "%s shutting down", g_securityBaselineModuleName);
    SshAuditCleanup(SecurityBaselineGetLog());
    CloseLog(&g_log);
}

static char* AuditEnsurePermissionsOnEtcIssue(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcIssue, 0, 0, 644, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcIssueNet(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcIssueNet, 0, 0, 644, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcHostsAllow(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcHostsAllow, 0, 0, 644, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcHostsDeny(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcHostsDeny, 0, 0, 644, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcSshSshdConfig(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsurePermissionsOnEtcSshSshdConfigObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcShadow(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcShadow, 0, 42, 400, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcShadowDash(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcShadowDash, 0, 42, 400, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcGShadow(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGShadow, 0, 42, 400, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcGShadowDash(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGShadowDash, 0, 42, 400, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcPasswd(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcPasswd, 0, 0, 644, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcPasswdDash(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcPasswdDash, 0, 0, 600, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcGroup(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGroup, 0, 0, 644, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcGroupDash(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcGroupDash, 0, 0, 644, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcAnacronTab(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcAnacronTab, 0, 0, 600, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcCronD(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronD, 0, 0, 700, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcCronDaily(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronDaily, 0, 0, 700, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcCronHourly(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronHourly, 0, 0, 700, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcCronMonthly(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronMonthly, 0, 0, 700, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcCronWeekly(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcCronWeekly, 0, 0, 700, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsurePermissionsOnEtcMotd(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcMotd, 0, 0, 644, &reason, SecurityBaselineGetLog());
    return reason;
};

static char* AuditEnsureKernelSupportForCpuNx(void)
{
    char* reason = NULL;
    CheckCpuFlagSupported("nx", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNodevOptionOnHomePartition(void)
{
    const char* home = "/home";
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, home, NULL, g_nodev, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, home, NULL, g_nodev, &reason, SecurityBaselineGetLog()); 
    return reason;
}

static char* AuditEnsureNodevOptionOnTmpPartition(void)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_tmp, NULL, g_nodev, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, g_tmp, NULL, g_nodev, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNodevOptionOnVarTmpPartition(void)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_nodev, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, g_varTmp, NULL, g_nodev, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNosuidOptionOnTmpPartition(void)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_tmp, NULL, g_nosuid, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, g_tmp, NULL, g_nosuid, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNosuidOptionOnVarTmpPartition(void)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_nosuid, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, g_varTmp, NULL, g_nosuid, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoexecOptionOnVarTmpPartition(void)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_varTmp, NULL, g_noexec, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, g_varTmp, NULL, g_noexec, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoexecOptionOnDevShmPartition(void)
{
    const char* devShm = "/dev/shm";
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, devShm, NULL, g_noexec, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, devShm, NULL, g_noexec, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNodevOptionEnabledForAllRemovableMedia(void)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_nodev, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, g_media, NULL, g_nodev, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoexecOptionEnabledForAllRemovableMedia(void)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_noexec, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, g_media, NULL, g_noexec, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNosuidOptionEnabledForAllRemovableMedia(void)
{
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, g_media, NULL, g_nosuid, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, g_media, NULL, g_nosuid, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(void)
{
    const char* nfs = "nfs";
    char* reason = NULL;
    CheckFileSystemMountingOption(g_etcFstab, NULL, nfs, g_noexec, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcFstab, NULL, nfs, g_nosuid, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, NULL, nfs, g_noexec, &reason, SecurityBaselineGetLog());
    CheckFileSystemMountingOption(g_etcMtab, NULL, nfs, g_nosuid, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureInetdNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_inetd, &reason, SecurityBaselineGetLog());
    CheckPackageNotInstalled(g_inetUtilsInetd, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureXinetdNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_xinetd, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAllTelnetdPackagesUninstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled("*telnetd*", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRshServerNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_rshServer, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNisNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_nis, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureTftpdNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_tftpd, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureReadaheadFedoraNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_readAheadFedora, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureBluetoothHiddNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_bluetooth, &reason, SecurityBaselineGetLog());
    CheckDaemonNotActive(g_bluetooth, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureIsdnUtilsBaseNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_isdnUtilsBase, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureIsdnUtilsKdumpToolsNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_kdumpTools, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureIscDhcpdServerNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_iscDhcpServer, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSendmailNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_sendmail, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSldapdNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_slapd, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureBind9NotInstalled(void)
{
    char* reason = NULL;
    CheckPackageInstalled(g_bind9, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDovecotCoreNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_dovecotCore, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAuditdInstalled(void)
{
    char* reason = NULL;
    CheckPackageInstalled(g_auditd, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAllEtcPasswdGroupsExistInEtcGroup(void)
{
    char* reason = NULL;
    CheckAllEtcPasswdGroupsExistInEtcGroup(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoDuplicateUidsExist(void)
{
    char* reason = NULL;
    CheckNoDuplicateUidsExist(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoDuplicateGidsExist(void)
{
    char* reason = NULL;
    CheckNoDuplicateGidsExist(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoDuplicateUserNamesExist(void)
{
    char* reason = NULL;
    CheckNoDuplicateUserNamesExist(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoDuplicateGroupsExist(void)
{
    char* reason = NULL;
    CheckNoDuplicateGroupsExist(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureShadowGroupIsEmpty(void)
{
    char* reason = NULL;
    CheckShadowGroupIsEmpty(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRootGroupExists(void)
{
    char* reason = NULL;
    CheckRootGroupExists(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAllAccountsHavePasswords(void)
{
    char* reason = NULL;
    CheckAllUsersHavePasswordsSet(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(void)
{
    char* reason = NULL;
    CheckRootIsOnlyUidZeroAccount(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcPasswd(void)
{
    char* reason = NULL;
    CheckNoLegacyPlusEntriesInFile("etc/passwd", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcShadow(void)
{
    char* reason = NULL;
    CheckNoLegacyPlusEntriesInFile("etc/shadow", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoLegacyPlusEntriesInEtcGroup(void)
{
    char* reason = NULL;
    CheckNoLegacyPlusEntriesInFile("etc/group", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDefaultRootAccountGroupIsGidZero(void)
{
    char* reason = NULL;
    CheckDefaultRootAccountGroupIsGidZero(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRootIsOnlyUidZeroAccount(void)
{
    char* reason = NULL;
    CheckRootGroupExists(&reason, SecurityBaselineGetLog());
    CheckRootIsOnlyUidZeroAccount(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAllUsersHomeDirectoriesExist(void)
{
    char* reason = NULL;
    CheckAllUsersHomeDirectoriesExist(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureUsersOwnTheirHomeDirectories(void)
{
    char* reason = NULL;
    CheckUsersOwnTheirHomeDirectories(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRestrictedUserHomeDirectories(void)
{
    unsigned int modes[] = {700, 750};
    char* reason = NULL;
    CheckRestrictedUserHomeDirectories(modes, ARRAY_SIZE(modes), &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePasswordHashingAlgorithm(void)
{
    char* reason = NULL;
    CheckPasswordHashingAlgorithm(sha512, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureMinDaysBetweenPasswordChanges(void)
{
    char* reason = NULL;
    CheckMinDaysBetweenPasswordChanges(g_minDaysBetweenPasswordChanges, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureInactivePasswordLockPeriod(void)
{
    char* reason = NULL;
    CheckLockoutAfterInactivityLessThan(g_maxInactiveDays, &reason, SecurityBaselineGetLog());
    CheckUsersRecordedPasswordChangeDates(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureMaxDaysBetweenPasswordChanges(void)
{
    char* reason = NULL;
    CheckMaxDaysBetweenPasswordChanges(g_maxDaysBetweenPasswordChanges, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePasswordExpiration(void)
{
    char* reason = NULL;
    CheckPasswordExpirationLessThan(g_passwordExpiration, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePasswordExpirationWarning(void)
{
    char* reason = NULL;
    CheckPasswordExpirationWarning(g_passwordExpirationWarning, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSystemAccountsAreNonLogin(void)
{
    char* reason = NULL;
    CheckSystemAccountsAreNonLogin(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAuthenticationRequiredForSingleUserMode(void)
{
    char* reason = NULL;
    CheckRootPasswordForSingleUserMode(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePrelinkIsDisabled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_prelink, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureTalkClientIsNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_talk, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDotDoesNotAppearInRootsPath(void)
{
    const char* path = "PATH";
    const char* dot = ".";
    char* reason = NULL;
    CheckTextNotFoundInEnvironmentVariable(path, dot, false, &reason, SecurityBaselineGetLog());
    CheckMarkedTextNotFoundInFile("/etc/sudoers", "secure_path", dot, &reason, SecurityBaselineGetLog());
    CheckMarkedTextNotFoundInFile(g_etcEnvironment, path, dot, &reason, SecurityBaselineGetLog());
    CheckMarkedTextNotFoundInFile(g_etcProfile, path, dot, &reason, SecurityBaselineGetLog());
    CheckMarkedTextNotFoundInFile("/root/.profile", path, dot, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureCronServiceIsEnabled(void)
{
    char* reason = NULL;
    CheckPackageInstalled(g_cron, &reason, SecurityBaselineGetLog());
    CheckDaemonActive(g_cron, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRemoteLoginWarningBannerIsConfigured(void)
{
    char* reason = NULL;
    CheckTextIsNotFoundInFile(g_etcIssueNet, "\\m", &reason, SecurityBaselineGetLog());
    CheckTextIsNotFoundInFile(g_etcIssueNet, "\\r", &reason, SecurityBaselineGetLog());
    CheckTextIsNotFoundInFile(g_etcIssueNet, "\\s", &reason, SecurityBaselineGetLog());
    CheckTextIsNotFoundInFile(g_etcIssueNet, "\\v", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureLocalLoginWarningBannerIsConfigured(void)
{
    char* reason = NULL;
    CheckTextIsNotFoundInFile(g_etcIssue, "\\m", &reason, SecurityBaselineGetLog());
    CheckTextIsNotFoundInFile(g_etcIssue, "\\r", &reason, SecurityBaselineGetLog());
    CheckTextIsNotFoundInFile(g_etcIssue, "\\s", &reason, SecurityBaselineGetLog());
    CheckTextIsNotFoundInFile(g_etcIssue, "\\v", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAuditdServiceIsRunning(void)
{
    char* reason = NULL;
    CheckDaemonActive(g_auditd, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSuRestrictedToRootGroup(void)
{
    char* reason = NULL;
    CheckTextIsFoundInFile("/etc/pam.d/su", "use_uid", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDefaultUmaskForAllUsers(void)
{
    char* reason = NULL;
    CheckLoginUmask("077", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAutomountingDisabled(void)
{
    const char* autofs = "autofs";
    char* reason = NULL;
    CheckPackageInstalled(autofs, &reason, SecurityBaselineGetLog());
    CheckDaemonNotActive(autofs, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureKernelCompiledFromApprovedSources(void)
{
    char* reason = NULL;
    CheckOsAndKernelMatchDistro(&reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDefaultDenyFirewallPolicyIsSet(void)
{
    const char* readIpTables = "iptables -S";
    char* reason = NULL;
    CheckTextFoundInCommandOutput(readIpTables, "-P INPUT DROP", &reason, SecurityBaselineGetLog());
    CheckTextFoundInCommandOutput(readIpTables, "-P FORWARD DROP", &reason, SecurityBaselineGetLog());
    CheckTextFoundInCommandOutput(readIpTables, "-P OUTPUT DROP", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePacketRedirectSendingIsDisabled(void)
{
    const char* command = "sysctl -a";
    char* reason = NULL;
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.all.send_redirects = 0", &reason, SecurityBaselineGetLog());
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.default.send_redirects = 0", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureIcmpRedirectsIsDisabled(void)
{
    const char* command = "sysctl -a";
    char* reason = NULL;
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.default.accept_redirects = 0", &reason, SecurityBaselineGetLog());
    CheckTextFoundInCommandOutput(command, "net.ipv6.conf.default.accept_redirects = 0", &reason, SecurityBaselineGetLog());
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.all.accept_redirects = 0", &reason, SecurityBaselineGetLog());
    CheckTextFoundInCommandOutput(command, "net.ipv6.conf.all.accept_redirects = 0", &reason, SecurityBaselineGetLog());
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.default.secure_redirects = 0", &reason, SecurityBaselineGetLog());
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.all.secure_redirects = 0", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSourceRoutedPacketsIsDisabled(void)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/all/accept_source_route", '#', "0", &reason, SecurityBaselineGetLog());
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv6/conf/all/accept_source_route", '#', "0", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAcceptingSourceRoutedPacketsIsDisabled(void)
{
    char* reason = 0;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/all/accept_source_route", '#', "0", &reason, SecurityBaselineGetLog());
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv6/conf/default/accept_source_route", '#', "0", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureIgnoringBogusIcmpBroadcastResponses(void)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/icmp_ignore_bogus_error_responses", '#', "1", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureIgnoringIcmpEchoPingsToMulticast(void)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/icmp_echo_ignore_broadcasts", '#', "1", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureMartianPacketLoggingIsEnabled(void)
{
    const char* command = "sysctl -a";
    char* reason = NULL;
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.all.log_martians = 1", &reason, SecurityBaselineGetLog());
    CheckTextFoundInCommandOutput(command, "net.ipv4.conf.default.log_martians = 1", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureReversePathSourceValidationIsEnabled(void)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/all/rp_filter", '#', "1", &reason, SecurityBaselineGetLog());
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/conf/default/rp_filter", '#', "1", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureTcpSynCookiesAreEnabled(void)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/proc/sys/net/ipv4/tcp_syncookies", '#', "1", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSystemNotActingAsNetworkSniffer(void)
{
    const char* command = "/sbin/ip addr list";
    const char* text = "PROMISC";
    char* reason = NULL;
    CheckTextNotFoundInCommandOutput(command, text, &reason, SecurityBaselineGetLog());
    CheckLineNotFoundOrCommentedOut("/etc/network/interfaces", '#', text, &reason, SecurityBaselineGetLog());
    CheckLineNotFoundOrCommentedOut("/etc/rc.local", '#', text, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAllWirelessInterfacesAreDisabled(void)
{
    char* reason = NULL;
    if (0 != CheckTextNotFoundInCommandOutput("/sbin/iwconfig 2>&1 | /bin/egrep -v 'no wireless extensions|not found'", "Frequency", &reason, SecurityBaselineGetLog()))
    {
        OsConfigCaptureReason(&reason, "at least one active wireless interface is present");
    }
    return reason;
}

static char* AuditEnsureIpv6ProtocolIsEnabled(void)
{
    char* reason = NULL;
    CheckTextFoundInCommandOutput("cat /sys/module/ipv6/parameters/disable", "0", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDccpIsDisabled(void)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install dccp /bin/true", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSctpIsDisabled(void)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install sctp /bin/true", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDisabledSupportForRds(void)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install rds /bin/true", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureTipcIsDisabled(void)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install tipc /bin/true", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureZeroconfNetworkingIsDisabled(void)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/etc/network/interfaces", '#', "ipv4ll", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePermissionsOnBootloaderConfig(void)
{
    char* reason = NULL;
    CheckFileAccess("/boot/grub/grub.conf", 0, 0, 400, &reason, SecurityBaselineGetLog());
    CheckFileAccess("/boot/grub/grub.cfg", 0, 0, 400, &reason, SecurityBaselineGetLog());
    CheckFileAccess("/boot/grub2/grub.cfg", 0, 0, 400, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePasswordReuseIsLimited(void)
{
    const char* etcPamdSystemAuth = "/etc/pam.d/system-auth";
    char* reason = NULL;
    if (0 == CheckIntegerOptionFromFileLessOrEqualWith(g_etcPamdCommonPassword, "remember", '=', 5, &reason, SecurityBaselineGetLog()))
    {
        return reason;
    }
    CheckIntegerOptionFromFileLessOrEqualWith(etcPamdSystemAuth, "remember", '=', 5, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureMountingOfUsbStorageDevicesIsDisabled(void)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install usb-storage /bin/true", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureCoreDumpsAreRestricted(void)
{
    const char* fsSuidDumpable = "fs.suid_dumpable = 0";
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/etc/security/limits.conf", '#', "hard core 0", &reason, SecurityBaselineGetLog());
    CheckTextFoundInFolder("/etc/security/limits.d", fsSuidDumpable, &reason, SecurityBaselineGetLog());
    CheckTextFoundInCommandOutput("sysctl -a", fsSuidDumpable, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePasswordCreationRequirements(void)
{
    char* reason = NULL;
    CheckPasswordCreationRequirements(14, 4, -1, -1, -1, -1, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureLockoutForFailedPasswordAttempts(void)
{
    const char* passwordAuth = "/etc/pam.d/password-auth";
    const char* commonAuth = "/etc/pam.d/common-auth";
    char* reason = NULL;
    if (0 == CheckLockoutForFailedPasswordAttempts(passwordAuth, &reason, SecurityBaselineGetLog()))
    {
        return reason;
    }
    OsConfigResetReason(&reason);
    CheckLockoutForFailedPasswordAttempts(commonAuth, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDisabledInstallationOfCramfsFileSystem(void)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install cramfs", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDisabledInstallationOfFreevxfsFileSystem(void)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install freevxfs", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDisabledInstallationOfHfsFileSystem(void)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install hfs", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDisabledInstallationOfHfsplusFileSystem(void)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install hfsplus", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDisabledInstallationOfJffs2FileSystem(void)
{
    char* reason = NULL;
    CheckTextNotFoundInFolder(g_etcModProbeD, "install jffs2", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureVirtualMemoryRandomizationIsEnabled(void)
{
    char* reason = NULL;
    if (0 == CheckFileContents("/proc/sys/kernel/randomize_va_space", "2", &reason, SecurityBaselineGetLog()))
    {
        return reason;
    }
    OsConfigResetReason(&reason);
    if (0 != CheckFileContents("/proc/sys/kernel/randomize_va_space", "1", &reason, SecurityBaselineGetLog()))
    {
        OsConfigCaptureReason(&reason, "neither 2");
    }
    return reason;
}

static char* AuditEnsureAllBootloadersHavePasswordProtectionEnabled(void)
{
    const char* password = "password";
    char* reason = NULL;
    CheckLineFoundNotCommentedOut("/boot/grub/grub.cfg", '#', password, &reason, SecurityBaselineGetLog());
    CheckLineFoundNotCommentedOut("/boot/grub/grub.conf", '#', password, &reason, SecurityBaselineGetLog());
    CheckLineFoundNotCommentedOut("/boot/grub2/grub.conf", '#', password, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureLoggingIsConfigured(void)
{
    char* reason = NULL;
    CheckFileExists("/var/log/syslog", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSyslogPackageIsInstalled(void)
{
    char* reason = NULL;
    CheckPackageInstalled(g_syslog, &reason, SecurityBaselineGetLog());
    CheckPackageInstalled(g_rsyslog, &reason, SecurityBaselineGetLog());
    CheckPackageInstalled(g_syslogNg, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSystemdJournaldServicePersistsLogMessages(void)
{
    char* reason = NULL;
    CheckPackageInstalled(g_systemd, &reason, SecurityBaselineGetLog());
    CheckDirectoryAccess("/var/log/journal", 0, -1, 2775, false, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureALoggingServiceIsEnabled(void)
{
    char* reason = NULL;
    if ((0 == CheckPackageNotInstalled(g_syslogNg, &reason, SecurityBaselineGetLog())) && 
        (0 == CheckPackageNotInstalled(g_systemd, &reason, SecurityBaselineGetLog())) && 
        CheckDaemonActive(g_rsyslog, &reason, SecurityBaselineGetLog()))
    {
        return reason;
    }
    OsConfigResetReason(&reason);
    if ((0 == CheckPackageNotInstalled(g_rsyslog, &reason, SecurityBaselineGetLog())) && 
        (0 == CheckPackageNotInstalled(g_systemd, &reason, SecurityBaselineGetLog())) && 
        CheckDaemonActive(g_syslogNg, &reason, SecurityBaselineGetLog())) 
    {
        return reason;
    }
    OsConfigResetReason(&reason);
    CheckPackageInstalled(g_systemd, &reason, SecurityBaselineGetLog());
    CheckDaemonActive(g_systemdJournald, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureFilePermissionsForAllRsyslogLogFiles(void)
{
    const char* fileCreateMode = "$FileCreateMode";
    char* reason = NULL;
    int modes[] = {600, 640};
    CheckIntegerOptionFromFileEqualWithAny(g_etcRsyslogConf, fileCreateMode, ' ', modes, ARRAY_SIZE(modes), &reason, SecurityBaselineGetLog());
    if (0 == CheckFileExists(g_etcSyslogNgSyslogNgConf, &reason, SecurityBaselineGetLog()))
    {
        CheckIntegerOptionFromFileEqualWithAny(g_etcSyslogNgSyslogNgConf, fileCreateMode, ' ', modes, ARRAY_SIZE(modes), &reason, SecurityBaselineGetLog());
    }
    return reason;
}

static char* AuditEnsureLoggerConfigurationFilesAreRestricted(void)
{
    char* reason = NULL;
    CheckFileAccess(g_etcSyslogNgSyslogNgConf, 0, 0, 640, &reason, SecurityBaselineGetLog());
    CheckFileAccess(g_etcRsyslogConf, 0, 0, 640, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(void)
{
    char* reason = NULL;
    CheckTextIsFoundInFile(g_etcRsyslogConf, "FileGroup adm", &reason, SecurityBaselineGetLog());
    CheckLineFoundNotCommentedOut(g_etcRsyslogConf, '#', "FileGroup adm", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(void)
{
    char* reason = NULL;
    CheckTextIsFoundInFile(g_etcRsyslogConf, "FileOwner syslog", &reason, SecurityBaselineGetLog());
    CheckLineFoundNotCommentedOut(g_etcRsyslogConf, '#', "FileOwner syslog", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRsyslogNotAcceptingRemoteMessages(void)
{
    char* reason = NULL;
    CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "ModLoad imudp", &reason, SecurityBaselineGetLog());
    CheckLineNotFoundOrCommentedOut(g_etcRsyslogConf, '#', "ModLoad imtcp", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSyslogRotaterServiceIsEnabled(void)
{
    char* reason = NULL;
    CheckPackageInstalled("logrotate", &reason, SecurityBaselineGetLog());
    CheckFileAccess("/etc/cron.daily/logrotate", 0, 0, 755, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureTelnetServiceIsDisabled(void)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut(g_etcInetdConf, '#', "telnet", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRcprshServiceIsDisabled(void)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut(g_etcInetdConf, '#', "shell", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureTftpServiceisDisabled(void)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut(g_etcInetdConf, '#', "tftp", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAtCronIsRestrictedToAuthorizedUsers(void)
{
    const char* etcCronAllow = "/etc/cron.allow";
    const char* etcAtAllow = "/etc/at.allow";
    char* reason = NULL;
    CheckFileNotFound("/etc/cron.deny", &reason, SecurityBaselineGetLog());
    CheckFileNotFound("/etc/at.deny", &reason, SecurityBaselineGetLog());
    CheckFileExists(etcCronAllow, &reason, SecurityBaselineGetLog());
    CheckFileExists(etcAtAllow, &reason, SecurityBaselineGetLog());
    CheckFileAccess(etcCronAllow, 0, 0, 600, &reason, SecurityBaselineGetLog());
    CheckFileAccess(etcAtAllow, 0, 0, 600, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshPortIsConfigured(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshPortIsConfiguredObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshBestPracticeProtocol(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshBestPracticeProtocolObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshBestPracticeIgnoreRhosts(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshBestPracticeIgnoreRhostsObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshLogLevelIsSet(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshLogLevelIsSetObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshMaxAuthTriesIsSet(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshMaxAuthTriesIsSetObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAllowUsersIsConfigured(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureAllowUsersIsConfiguredObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDenyUsersIsConfigured(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureDenyUsersIsConfiguredObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAllowGroupsIsConfigured(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureAllowGroupsIsConfiguredObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureDenyGroupsConfigured(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureDenyGroupsConfiguredObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshHostbasedAuthenticationIsDisabled(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshHostbasedAuthenticationIsDisabledObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshPermitRootLoginIsDisabled(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshPermitRootLoginIsDisabledObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshPermitEmptyPasswordsIsDisabled(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshPermitEmptyPasswordsIsDisabledObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshClientIntervalCountMaxIsConfigured(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshClientIntervalCountMaxIsConfiguredObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshClientAliveIntervalIsConfigured(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshClientAliveIntervalIsConfiguredObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshLoginGraceTimeIsSet(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshLoginGraceTimeIsSetObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureOnlyApprovedMacAlgorithmsAreUsed(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSshWarningBannerIsEnabled(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureSshWarningBannerIsEnabledObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureUsersCannotSetSshEnvironmentOptions(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureUsersCannotSetSshEnvironmentOptionsObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAppropriateCiphersForSsh(void)
{
    char* reason = NULL;
    ProcessSshAuditCheck(g_auditEnsureAppropriateCiphersForSshObject, NULL, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureAvahiDaemonServiceIsDisabled(void)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_avahiDaemon, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureCupsServiceisDisabled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_cups, &reason, SecurityBaselineGetLog());
    CheckDaemonNotActive(g_cups, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePostfixPackageIsUninstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_postfix, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePostfixNetworkListeningIsDisabled(void)
{
    char* reason = NULL;
    CheckFileExists("/etc/postfix/main.cf", &reason, SecurityBaselineGetLog());
    CheckTextIsFoundInFile("/etc/postfix/main.cf", "inet_interfaces localhost", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRpcgssdServiceIsDisabled(void)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_rpcgssd, &reason, SecurityBaselineGetLog());
    CheckDaemonNotActive(g_rpcGssd, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRpcidmapdServiceIsDisabled(void)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_rpcidmapd, &reason, SecurityBaselineGetLog());
    CheckDaemonNotActive(g_nfsIdmapd, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsurePortmapServiceIsDisabled(void)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_rpcbind, &reason, SecurityBaselineGetLog());
    CheckDaemonNotActive(g_rpcbindService, &reason, SecurityBaselineGetLog());
    CheckDaemonNotActive(g_rpcbindSocket, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNetworkFileSystemServiceIsDisabled(void)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_nfsServer, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRpcsvcgssdServiceIsDisabled(void)
{
    char* reason = NULL;
    CheckLineFoundNotCommentedOut(g_etcInetdConf, '#', "NEED_SVCGSSD = yes", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSnmpServerIsDisabled(void)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_snmpd, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRsynServiceIsDisabled(void)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_rsync, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNisServerIsDisabled(void)
{
    char* reason = NULL;
    CheckDaemonNotActive(g_ypserv, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRshClientNotInstalled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_rsh, &reason, SecurityBaselineGetLog());
    CheckPackageNotInstalled(g_rshClient, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureSmbWithSambaIsDisabled(void)
{
    const char* etcSambaConf = "/etc/samba/smb.conf";
    const char* minProtocol = "min protocol = SMB2";
    char* reason = NULL;
    CheckPackageInstalled("samba", &reason, SecurityBaselineGetLog());
    CheckLineFoundNotCommentedOut(etcSambaConf, '#', minProtocol, &reason, SecurityBaselineGetLog());
    CheckLineFoundNotCommentedOut(etcSambaConf, ';', minProtocol, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureUsersDotFilesArentGroupOrWorldWritable(void)
{
    unsigned int modes[] = {600, 644, 664, 700, 744};
    char* reason = NULL;
    CheckUsersRestrictedDotFiles(modes, ARRAY_SIZE(modes), &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoUsersHaveDotForwardFiles(void)
{
    char* reason = NULL;
    CheckOrEnsureUsersDontHaveDotFiles(g_forward, false, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoUsersHaveDotNetrcFiles(void)
{
    char* reason = NULL;
    CheckOrEnsureUsersDontHaveDotFiles(g_netrc, false, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureNoUsersHaveDotRhostsFiles(void)
{
    char* reason = NULL;
    CheckOrEnsureUsersDontHaveDotFiles(g_rhosts, false, &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureRloginServiceIsDisabled(void)
{
    char* reason = NULL;
    CheckPackageNotInstalled(g_inetd, &reason, SecurityBaselineGetLog());
    CheckPackageNotInstalled(g_inetUtilsInetd, &reason, SecurityBaselineGetLog());
    CheckTextIsFoundInFile(g_etcInetdConf, "login", &reason, SecurityBaselineGetLog());
    return reason;
}

static char* AuditEnsureUnnecessaryAccountsAreRemoved(void)
{
    const char* names[] = {"games"};
    char* reason = NULL;
    CheckUserAccountsNotFound(names, ARRAY_SIZE(names), &reason, SecurityBaselineGetLog());
    return reason;
}

static int RemediateEnsurePermissionsOnEtcIssue(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcIssue, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcIssueNet(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcIssueNet, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcHostsAllow(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcHostsAllow, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcHostsDeny(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcHostsDeny, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcSshSshdConfig(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsurePermissionsOnEtcSshSshdConfigObject, value, NULL, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcShadow(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcShadow, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcShadowDash(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcShadowDash, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGShadow(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcGShadow, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGShadowDash(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcGShadowDash, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcPasswd(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcPasswd, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcPasswdDash(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcPasswdDash, 0, 0, 600, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGroup(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcGroup, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGroupDash(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcGroupDash, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcAnacronTab(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcAnacronTab, 0, 0, 600, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronD(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcCronD, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronDaily(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcCronDaily, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronHourly(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcCronHourly, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronMonthly(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcCronMonthly, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronWeekly(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcCronWeekly, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcMotd(char* value)
{
    UNUSED(value);
    return SetFileAccess(g_etcMotd, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsureInetdNotInstalled(char* value)
{
    UNUSED(value);
    return ((0 == UninstallPackage(g_inetd, SecurityBaselineGetLog())) &&
        (0 == UninstallPackage(g_inetUtilsInetd, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int RemediateEnsureXinetdNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_xinetd, SecurityBaselineGetLog());
}

static int RemediateEnsureRshServerNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_rshServer, SecurityBaselineGetLog());
}

static int RemediateEnsureNisNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_nis, SecurityBaselineGetLog());
}

static int RemediateEnsureTftpdNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_tftpd, SecurityBaselineGetLog());
}

static int RemediateEnsureReadaheadFedoraNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_readAheadFedora, SecurityBaselineGetLog());
}

static int RemediateEnsureBluetoothHiddNotInstalled(char* value)
{
    UNUSED(value);
    StopAndDisableDaemon(g_bluetooth, SecurityBaselineGetLog());
    return UninstallPackage(g_bluetooth, SecurityBaselineGetLog());
}

static int RemediateEnsureIsdnUtilsBaseNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_isdnUtilsBase, SecurityBaselineGetLog());
}

static int RemediateEnsureIsdnUtilsKdumpToolsNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_kdumpTools, SecurityBaselineGetLog());
}

static int RemediateEnsureIscDhcpdServerNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_iscDhcpServer, SecurityBaselineGetLog());
}

static int RemediateEnsureSendmailNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_sendmail, SecurityBaselineGetLog());
}

static int RemediateEnsureSldapdNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_slapd, SecurityBaselineGetLog());
}

static int RemediateEnsureBind9NotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_bind9, SecurityBaselineGetLog());
}

static int RemediateEnsureDovecotCoreNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_dovecotCore, SecurityBaselineGetLog());
}

static int RemediateEnsureAuditdInstalled(char* value)
{
    UNUSED(value);
    return InstallPackage(g_auditd, SecurityBaselineGetLog());
}

static int RemediateEnsurePrelinkIsDisabled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_prelink, SecurityBaselineGetLog());
}

static int RemediateEnsureTalkClientIsNotInstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_talk, SecurityBaselineGetLog());
}

static int RemediateEnsureCronServiceIsEnabled(char* value)
{
    UNUSED(value);
    return (0 == InstallPackage(g_cron, SecurityBaselineGetLog()) &&
        EnableAndStartDaemon(g_cron, SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int RemediateEnsureAuditdServiceIsRunning(char* value)
{
    UNUSED(value);
    return (0 == InstallPackage(g_auditd, SecurityBaselineGetLog()) &&
        EnableAndStartDaemon(g_auditd, SecurityBaselineGetLog())) ? 0 : ENOENT;
}

static int RemediateEnsureKernelSupportForCpuNx(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnHomePartition(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnTmpPartition(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionOnVarTmpPartition(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionOnTmpPartition(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionOnVarTmpPartition(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionOnVarTmpPartition(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionOnDevShmPartition(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNodevOptionEnabledForAllRemovableMedia(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecOptionEnabledForAllRemovableMedia(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNosuidOptionEnabledForAllRemovableMedia(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoexecNosuidOptionsEnabledForAllNfsMounts(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllTelnetdPackagesUninstalled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllEtcPasswdGroupsExistInEtcGroup(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateUidsExist(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateGidsExist(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateUserNamesExist(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoDuplicateGroupsExist(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureShadowGroupIsEmpty(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRootGroupExists(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllAccountsHavePasswords(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcPasswd(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcShadow(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureNoLegacyPlusEntriesInEtcGroup(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultRootAccountGroupIsGidZero(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRootIsOnlyUidZeroAccount(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllUsersHomeDirectoriesExist(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUsersOwnTheirHomeDirectories(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRestrictedUserHomeDirectories(char* value)
{
    unsigned int modes[] = {700, 750};
    UNUSED(value);
    return SetRestrictedUserHomeDirectories(modes, ARRAY_SIZE(modes), 700, 750, SecurityBaselineGetLog());
}

static int RemediateEnsurePasswordHashingAlgorithm(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMinDaysBetweenPasswordChanges(char* value)
{
    UNUSED(value);
    return SetMinDaysBetweenPasswordChanges(g_minDaysBetweenPasswordChanges, SecurityBaselineGetLog());
}

static int RemediateEnsureInactivePasswordLockPeriod(char* value)
{
    UNUSED(value);
    return SetLockoutAfterInactivityLessThan(g_maxInactiveDays, SecurityBaselineGetLog());
}

static int RemediateEnsureMaxDaysBetweenPasswordChanges(char* value)
{
    UNUSED(value);
    return SetMaxDaysBetweenPasswordChanges(g_maxDaysBetweenPasswordChanges, SecurityBaselineGetLog());
}

static int RemediateEnsurePasswordExpiration(char* value)
{
    UNUSED(value);
    return ((0 == SetMinDaysBetweenPasswordChanges(g_minDaysBetweenPasswordChanges, SecurityBaselineGetLog())) &&
        (0 == SetMaxDaysBetweenPasswordChanges(g_maxDaysBetweenPasswordChanges, SecurityBaselineGetLog())) &&
        (0 == CheckPasswordExpirationLessThan(g_passwordExpiration, NULL, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int RemediateEnsurePasswordExpirationWarning(char* value)
{
    UNUSED(value);
    return SetPasswordExpirationWarning(g_passwordExpirationWarning, SecurityBaselineGetLog());
}

static int RemediateEnsureSystemAccountsAreNonLogin(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAuthenticationRequiredForSingleUserMode(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDotDoesNotAppearInRootsPath(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRemoteLoginWarningBannerIsConfigured(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLocalLoginWarningBannerIsConfigured(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSuRestrictedToRootGroup(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultUmaskForAllUsers(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAutomountingDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureKernelCompiledFromApprovedSources(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDefaultDenyFirewallPolicyIsSet(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePacketRedirectSendingIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIcmpRedirectsIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSourceRoutedPacketsIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAcceptingSourceRoutedPacketsIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIgnoringBogusIcmpBroadcastResponses(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIgnoringIcmpEchoPingsToMulticast(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMartianPacketLoggingIsEnabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureReversePathSourceValidationIsEnabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTcpSynCookiesAreEnabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSystemNotActingAsNetworkSniffer(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllWirelessInterfacesAreDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureIpv6ProtocolIsEnabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDccpIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSctpIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledSupportForRds(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTipcIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureZeroconfNetworkingIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePermissionsOnBootloaderConfig(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordReuseIsLimited(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureMountingOfUsbStorageDevicesIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureCoreDumpsAreRestricted(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsurePasswordCreationRequirements(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLockoutForFailedPasswordAttempts(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfCramfsFileSystem(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfFreevxfsFileSystem(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfHfsFileSystem(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfHfsplusFileSystem(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureDisabledInstallationOfJffs2FileSystem(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureVirtualMemoryRandomizationIsEnabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllBootloadersHavePasswordProtectionEnabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLoggingIsConfigured(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSyslogPackageIsInstalled(char* value)
{
    UNUSED(value);
    return ((0 == InstallPackage(g_systemd, SecurityBaselineGetLog()) && 
        ((0 == InstallPackage(g_rsyslog, SecurityBaselineGetLog())) || (0 == InstallPackage(g_syslog, SecurityBaselineGetLog())))) ||
        ((0 == InstallPackage(g_syslogNg, SecurityBaselineGetLog())))) ? 0 : ENOENT;
}

static int RemediateEnsureSystemdJournaldServicePersistsLogMessages(char* value)
{
    UNUSED(value);
    return ((0 == InstallPackage(g_systemd, SecurityBaselineGetLog())) &&
        (0 == SetDirectoryAccess("/var/log/journal", 0, -1, 2775, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int RemediateEnsureALoggingServiceIsEnabled(char* value)
{
    UNUSED(value);
    return ((((0 == InstallPackage(g_systemd, SecurityBaselineGetLog())) && EnableAndStartDaemon(g_systemdJournald, SecurityBaselineGetLog())) &&
        (((0 == InstallPackage(g_rsyslog, SecurityBaselineGetLog())) && EnableAndStartDaemon(g_rsyslog, SecurityBaselineGetLog())) || 
        (((0 == InstallPackage(g_syslog, SecurityBaselineGetLog()) && EnableAndStartDaemon(g_syslog, SecurityBaselineGetLog())))))) ||
        (((0 == InstallPackage(g_syslogNg, SecurityBaselineGetLog())) && EnableAndStartDaemon(g_syslogNg, SecurityBaselineGetLog())))) ? 0 : ENOENT;
}

static int RemediateEnsureFilePermissionsForAllRsyslogLogFiles(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureLoggerConfigurationFilesAreRestricted(char* value)
{
    UNUSED(value);
    return ((0 == SetFileAccess(g_etcSyslogNgSyslogNgConf, 0, 0, 640, SecurityBaselineGetLog())) &&
        (0 == SetFileAccess(g_etcRsyslogConf, 0, 0, 640, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int RemediateEnsureAllRsyslogLogFilesAreOwnedByAdmGroup(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAllRsyslogLogFilesAreOwnedBySyslogUser(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRsyslogNotAcceptingRemoteMessages(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSyslogRotaterServiceIsEnabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTelnetServiceIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRcprshServiceIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureTftpServiceisDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureAtCronIsRestrictedToAuthorizedUsers(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureSshPortIsConfigured(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshPortIsConfiguredObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshBestPracticeProtocol(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshBestPracticeProtocolObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshBestPracticeIgnoreRhosts(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshBestPracticeIgnoreRhostsObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshLogLevelIsSet(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshLogLevelIsSetObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshMaxAuthTriesIsSet(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshMaxAuthTriesIsSetObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureAllowUsersIsConfigured(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureAllowUsersIsConfiguredObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureDenyUsersIsConfigured(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureDenyUsersIsConfiguredObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureAllowGroupsIsConfigured(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureAllowGroupsIsConfiguredObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureDenyGroupsConfigured(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureDenyGroupsConfiguredObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshHostbasedAuthenticationIsDisabled(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshPermitRootLoginIsDisabled(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshPermitRootLoginIsDisabledObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshPermitEmptyPasswordsIsDisabled(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshClientIntervalCountMaxIsConfigured(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshClientIntervalCountMaxIsConfiguredObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshClientAliveIntervalIsConfigured(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshClientAliveIntervalIsConfiguredObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshLoginGraceTimeIsSet(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshLoginGraceTimeIsSetObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureOnlyApprovedMacAlgorithmsAreUsed(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureSshWarningBannerIsEnabled(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureSshWarningBannerIsEnabledObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureUsersCannotSetSshEnvironmentOptions(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureAppropriateCiphersForSsh(char* value)
{
    return ProcessSshAuditCheck(g_remediateEnsureAppropriateCiphersForSshObject, value, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureAvahiDaemonServiceIsDisabled(char* value)
{
    UNUSED(value);
    StopAndDisableDaemon(g_avahiDaemon, SecurityBaselineGetLog());
    return (0 == strncmp(g_pass, AuditEnsureAvahiDaemonServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureCupsServiceisDisabled(char* value)
{
    UNUSED(value);
    StopAndDisableDaemon(g_cups, SecurityBaselineGetLog());
    return UninstallPackage(g_cups, SecurityBaselineGetLog());
}

static int RemediateEnsurePostfixPackageIsUninstalled(char* value)
{
    UNUSED(value);
    return UninstallPackage(g_postfix, SecurityBaselineGetLog());
}

static int RemediateEnsurePostfixNetworkListeningIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureRpcgssdServiceIsDisabled(char* value)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcgssd, SecurityBaselineGetLog());
    StopAndDisableDaemon(g_rpcGssd, SecurityBaselineGetLog());
    return (0 == strncmp(g_pass, AuditEnsureRpcgssdServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRpcidmapdServiceIsDisabled(char* value)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcidmapd, SecurityBaselineGetLog());
    StopAndDisableDaemon(g_nfsIdmapd, SecurityBaselineGetLog());
    return (0 == strncmp(g_pass, AuditEnsureRpcidmapdServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsurePortmapServiceIsDisabled(char* value)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rpcbind, SecurityBaselineGetLog());
    StopAndDisableDaemon(g_rpcbindService, SecurityBaselineGetLog());
    StopAndDisableDaemon(g_rpcbindSocket, SecurityBaselineGetLog());
    return (0 == strncmp(g_pass, AuditEnsurePortmapServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureNetworkFileSystemServiceIsDisabled(char* value)
{
    UNUSED(value);
    StopAndDisableDaemon(g_nfsServer, SecurityBaselineGetLog());
    return (0 == strncmp(g_pass, AuditEnsureNetworkFileSystemServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRpcsvcgssdServiceIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns    
}

static int RemediateEnsureSnmpServerIsDisabled(char* value)
{
    UNUSED(value);
    StopAndDisableDaemon(g_snmpd, SecurityBaselineGetLog());
    return (0 == strncmp(g_pass, AuditEnsureSnmpServerIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRsynServiceIsDisabled(char* value)
{
    UNUSED(value);
    StopAndDisableDaemon(g_rsync, SecurityBaselineGetLog());
    return (0 == strncmp(g_pass, AuditEnsureRsynServiceIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureNisServerIsDisabled(char* value)
{
    UNUSED(value);
    StopAndDisableDaemon(g_ypserv, SecurityBaselineGetLog());
    return (0 == strncmp(g_pass, AuditEnsureNisServerIsDisabled(), strlen(g_pass))) ? 0 : ENOENT;
}

static int RemediateEnsureRshClientNotInstalled(char* value)
{
    UNUSED(value);
    return ((0 == UninstallPackage(g_rsh, SecurityBaselineGetLog())) && 
        (0 == UninstallPackage(g_rshClient, SecurityBaselineGetLog()))) ? 0 : ENOENT;
}

static int RemediateEnsureSmbWithSambaIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUsersDotFilesArentGroupOrWorldWritable(char* value)
{
    unsigned int modes[] = {600, 644, 664, 700, 744};
    UNUSED(value);
    return SetUsersRestrictedDotFiles(modes, ARRAY_SIZE(modes), 744, SecurityBaselineGetLog());
}

static int RemediateEnsureNoUsersHaveDotForwardFiles(char* value)
{
    UNUSED(value);
    return CheckOrEnsureUsersDontHaveDotFiles(g_forward, true, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureNoUsersHaveDotNetrcFiles(char* value)
{
    UNUSED(value);
    return CheckOrEnsureUsersDontHaveDotFiles(g_netrc, true, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureNoUsersHaveDotRhostsFiles(char* value)
{
    UNUSED(value);
    return CheckOrEnsureUsersDontHaveDotFiles(g_rhosts, true, NULL, SecurityBaselineGetLog());
}

static int RemediateEnsureRloginServiceIsDisabled(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}

static int RemediateEnsureUnnecessaryAccountsAreRemoved(char* value)
{
    const char* names[] = {"games"};
    UNUSED(value);
    return RemoveUserAccounts(names, ARRAY_SIZE(names), SecurityBaselineGetLog());
}

static int InitEnsurePermissionsOnEtcSshSshdConfig(char* value)
{
    return InitializeSshAuditCheck(g_initEnsurePermissionsOnEtcSshSshdConfigObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshPortIsConfigured(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshPortIsConfiguredObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshBestPracticeProtocol(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshBestPracticeProtocolObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshBestPracticeIgnoreRhosts(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshBestPracticeIgnoreRhostsObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshLogLevelIsSet(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshLogLevelIsSetObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshMaxAuthTriesIsSet(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshMaxAuthTriesIsSetObject, value, SecurityBaselineGetLog());
}

static int InitEnsureAllowUsersIsConfigured(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureAllowUsersIsConfiguredObject, value, SecurityBaselineGetLog());
}

static int InitEnsureDenyUsersIsConfigured(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureDenyUsersIsConfiguredObject, value, SecurityBaselineGetLog());
}

static int InitEnsureAllowGroupsIsConfigured(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureAllowGroupsIsConfiguredObject, value, SecurityBaselineGetLog());
}

static int InitEnsureDenyGroupsConfigured(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureDenyGroupsConfiguredObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshHostbasedAuthenticationIsDisabled(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshHostbasedAuthenticationIsDisabledObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshPermitRootLoginIsDisabled(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshPermitRootLoginIsDisabledObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshPermitEmptyPasswordsIsDisabled(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshPermitEmptyPasswordsIsDisabledObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshClientIntervalCountMaxIsConfigured(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshClientIntervalCountMaxIsConfiguredObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshClientAliveIntervalIsConfigured(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshClientAliveIntervalIsConfiguredObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshLoginGraceTimeIsSet(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshLoginGraceTimeIsSetObject, value, SecurityBaselineGetLog());
}

static int InitEnsureOnlyApprovedMacAlgorithmsAreUsed(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureOnlyApprovedMacAlgorithmsAreUsedObject, value, SecurityBaselineGetLog());
}

static int InitEnsureSshWarningBannerIsEnabled(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureSshWarningBannerIsEnabledObject, value, SecurityBaselineGetLog());
}

static int InitEnsureUsersCannotSetSshEnvironmentOptions(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureUsersCannotSetSshEnvironmentOptionsObject, value, SecurityBaselineGetLog());
}

static int InitEnsureAppropriateCiphersForSsh(char* value)
{
    return InitializeSshAuditCheck(g_initEnsureAppropriateCiphersForSshObject, value, SecurityBaselineGetLog());
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
    JSON_Value* jsonValue = NULL;
    char* serializedValue = NULL;
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
        
        if (NULL != result)
        {
            if (NULL == (jsonValue = json_value_init_string(result)))
            {
                OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s): json_value_init_string(%s) failed", componentName, objectName, result);
                status = ENOMEM;
            }
            else if (NULL == (serializedValue = json_serialize_to_string(jsonValue)))
            {
                OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s): json_serialize_to_string(%s) failed", componentName, objectName, result);
                status = ENOMEM;
            }
            else
            {
                *payloadSizeBytes = (int)strlen(serializedValue);
                if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
                {
                    OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s) insufficient max size (%d bytes) vs actual size (%d bytes), report will be truncated",
                        componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);

                    *payloadSizeBytes = g_maxPayloadSizeBytes;
                }

                if (NULL != (*payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes + 1)))
                {
                    memset(*payload, 0, *payloadSizeBytes + 1);
                    memcpy(*payload, serializedValue, *payloadSizeBytes);
                }
                else
                {
                    OsConfigLogError(SecurityBaselineGetLog(), "MmiGet: failed to allocate %d bytes", *payloadSizeBytes + 1);
                    *payloadSizeBytes = 0;
                    status = ENOMEM;
                }
            }
        }
    }    

    OsConfigLogInfo(SecurityBaselineGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);

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

int SecurityBaselineMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    JSON_Value* jsonValue = NULL;
    char* jsonString = NULL;
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

            if (NULL != (jsonValue = json_parse_string(payloadString)))
            {
                if (NULL == (jsonString = (char*)json_value_get_string(jsonValue)))
                {
                    status = EINVAL;
                    OsConfigLogError(SecurityBaselineGetLog(), "MmiSet: json_value_get_string(%s) failed", payloadString);
                }
            }
            else
            {
                status = EINVAL;
                OsConfigLogError(SecurityBaselineGetLog(), "MmiSet: json_parse_string(%s) failed", payloadString);
            }
        }
        else
        {
            status = ENOMEM;
            OsConfigLogError(SecurityBaselineGetLog(), "MmiSet: failed to allocate %d bytes of memory", payloadSizeBytes + 1);
        }
    }
    
    if (MMI_OK == status)
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
            OsConfigLogError(SecurityBaselineGetLog(), "MmiSet called for an unsupported object name: %s", objectName);
            status = EINVAL;
        }
    }
    
    OsConfigLogInfo(SecurityBaselineGetLog(), "MmiSet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);

    if (NULL != jsonValue)
    {
        json_value_free(jsonValue);
    }
    
    FREE_MEMORY(payloadString);

    return status;
}

void SecurityBaselineMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}