// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <version.h>
#include <Mmi.h>
#include <CommonUtils.h>
#include <SecurityBaseline.h>

using namespace std;

class SecurityBaselineTest : public ::testing::Test
{
    protected:
        const char* m_expectedMmiInfo = "{\"Name\": \"SecurityBaseline\","
            "\"Description\": \"Provides functionality to audit and remediate Security Baseline policies on device\","
            "\"Manufacturer\": \"Microsoft\","
            "\"VersionMajor\": 1,"
            "\"VersionMinor\": 0,"
            "\"VersionInfo\": \"Zinc\","
            "\"Components\": [\"SecurityBaseline\"],"
            "\"Lifetime\": 2,"
            "\"UserAccount\": 0}";

        const char* m_securityBaselineModuleName = "OSConfig SecurityBaseline module";
        const char* m_securityBaselineComponentName = "SecurityBaseline";

        const char* m_auditSecurityBaselineObject = "auditSecurityBaseline";
        const char* m_auditEnsurePermissionsOnEtcIssueObject = "auditEnsurePermissionsOnEtcIssue";
        const char* m_auditEnsurePermissionsOnEtcIssueNetObject = "auditEnsurePermissionsOnEtcIssueNet";
        const char* m_auditEnsurePermissionsOnEtcHostsAllowObject = "auditEnsurePermissionsOnEtcHostsAllow";
        const char* m_auditEnsurePermissionsOnEtcHostsDenyObject = "auditEnsurePermissionsOnEtcHostsDeny";
        const char* m_auditEnsurePermissionsOnEtcSshSshdConfigObject = "auditEnsurePermissionsOnEtcSshSshdConfig";
        const char* m_auditEnsurePermissionsOnEtcShadowObject = "auditEnsurePermissionsOnEtcShadow";
        const char* m_auditEnsurePermissionsOnEtcShadowDashObject = "auditEnsurePermissionsOnEtcShadowDash";
        const char* m_auditEnsurePermissionsOnEtcGShadowObject = "auditEnsurePermissionsOnEtcGShadow";
        const char* m_auditEnsurePermissionsOnEtcGShadowDashObject = "auditEnsurePermissionsOnEtcGShadowDash";
        const char* m_auditEnsurePermissionsOnEtcPasswdObject = "auditEnsurePermissionsOnEtcPasswd";
        const char* m_auditEnsurePermissionsOnEtcPasswdDashObject = "auditEnsurePermissionsOnEtcPasswdDash";
        const char* m_auditEnsurePermissionsOnEtcGroupObject = "auditEnsurePermissionsOnEtcGroup";
        const char* m_auditEnsurePermissionsOnEtcGroupDashObject = "auditEnsurePermissionsOnEtcGroupDash";
        const char* m_auditEnsurePermissionsOnEtcAnacronTabObject = "auditEnsurePermissionsOnEtcAnacronTab";
        const char* m_auditEnsurePermissionsOnEtcCronDObject = "auditEnsurePermissionsOnEtcCronD";
        const char* m_auditEnsurePermissionsOnEtcCronDailyObject = "auditEnsurePermissionsOnEtcCronDaily";
        const char* m_auditEnsurePermissionsOnEtcCronHourlyObject = "auditEnsurePermissionsOnEtcCronHourly";
        const char* m_auditEnsurePermissionsOnEtcCronMonthlyObject = "auditEnsurePermissionsOnEtcCronMonthly";
        const char* m_auditEnsurePermissionsOnEtcCronWeeklyObject = "auditEnsurePermissionsOnEtcCronWeekly";
        const char* m_auditEnsurePermissionsOnEtcMotdObject = "auditEnsurePermissionsOnEtcMotd";
        const char* m_auditEnsureInetdNotInstalledObject = "auditEnsureInetdNotInstalled";
        const char* m_auditEnsureXinetdNotInstalledObject = "auditEnsureXinetdNotInstalled";
        const char* m_auditEnsureRshServerNotInstalledObject = "auditEnsureRshServerNotInstalled";
        const char* m_auditEnsureNisNotInstalledObject = "auditEnsureNisNotInstalled";
        const char* m_auditEnsureTftpdNotInstalledObject = "auditEnsureTftpdNotInstalled";
        const char* m_auditEnsureReadaheadFedoraNotInstalledObject = "auditEnsureReadaheadFedoraNotInstalled";
        const char* m_auditEnsureBluetoothHiddNotInstalledObject = "auditEnsureBluetoothHiddNotInstalled";
        const char* m_auditEnsureIsdnUtilsBaseNotInstalledObject = "auditEnsureIsdnUtilsBaseNotInstalled";
        const char* m_auditEnsureIsdnUtilsKdumpToolsNotInstalledObject = "auditEnsureIsdnUtilsKdumpToolsNotInstalled";
        const char* m_auditEnsureIscDhcpdServerNotInstalledObject = "auditEnsureIscDhcpdServerNotInstalled";
        const char* m_auditEnsureSendmailNotInstalledObject = "auditEnsureSendmailNotInstalled";
        const char* m_auditEnsureSldapdNotInstalledObject = "auditEnsureSldapdNotInstalled";
        const char* m_auditEnsureBind9NotInstalledObject = "auditEnsureBind9NotInstalled";
        const char* m_auditEnsureDovecotCoreNotInstalledObject = "auditEnsureDovecotCoreNotInstalled";
        const char* m_auditEnsureAuditdInstalledObject = "auditEnsureAuditdInstalled";
        const char* m_auditEnsurePrelinkIsDisabledObject = "auditEnsurePrelinkIsDisabled";
        const char* m_auditEnsureTalkClientIsNotInstalledObject = "auditEnsureTalkClientIsNotInstalled";
        const char* m_auditEnsureCronServiceIsEnabledObject = "auditEnsureCronServiceIsEnabled";
        const char* m_auditEnsureAuditdServiceIsRunningObject = "auditEnsureAuditdServiceIsRunning";
        // Audit-only
        const char* m_auditEnsureKernelSupportForCpuNxObject = "auditEnsureKernelSupportForCpuNx";
        const char* m_auditEnsureAllTelnetdPackagesUninstalledObject = "auditEnsureAllTelnetdPackagesUninstalled";
        const char* m_auditEnsureNodevOptionOnHomePartitionObject = "auditEnsureNodevOptionOnHomePartition";
        const char* m_auditEnsureNodevOptionOnTmpPartitionObject = "auditEnsureNodevOptionOnTmpPartition";
        const char* m_auditEnsureNodevOptionOnVarTmpPartitionObject = "auditEnsureNodevOptionOnVarTmpPartition";
        const char* m_auditEnsureNosuidOptionOnTmpPartitionObject = "auditEnsureNosuidOptionOnTmpPartition";
        const char* m_auditEnsureNosuidOptionOnVarTmpPartitionObject = "auditEnsureNosuidOptionOnVarTmpPartition";
        const char* m_auditEnsureNoexecOptionOnVarTmpPartitionObject = "auditEnsureNoexecOptionOnVarTmpPartition";
        const char* m_auditEnsureNoexecOptionOnDevShmPartitionObject = "auditEnsureNoexecOptionOnDevShmPartition";
        const char* m_auditEnsureNodevOptionEnabledForAllRemovableMediaObject = "auditEnsureNodevOptionEnabledForAllRemovableMedia";
        const char* m_auditEnsureNoexecOptionEnabledForAllRemovableMediaObject = "auditEnsureNoexecOptionEnabledForAllRemovableMedia";
        const char* m_auditEnsureNosuidOptionEnabledForAllRemovableMediaObject = "auditEnsureNosuidOptionEnabledForAllRemovableMedia";
        const char* m_auditEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject = "auditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts";
        const char* m_auditEnsureAllEtcPasswdGroupsExistInEtcGroupObject = "auditEnsureAllEtcPasswdGroupsExistInEtcGroup";
        const char* m_auditEnsureNoDuplicateUidsExistObject = "auditEnsureNoDuplicateUidsExist";
        const char* m_auditEnsureNoDuplicateGidsExistObject = "auditEnsureNoDuplicateGidsExist";
        const char* m_auditEnsureNoDuplicateUserNamesExistObject = "auditEnsureNoDuplicateUserNamesExist";
        const char* m_auditEnsureNoDuplicateGroupsExistObject = "auditEnsureNoDuplicateGroupsExist";
        const char* m_auditEnsureShadowGroupIsEmptyObject = "auditEnsureShadowGroupIsEmpty";
        const char* m_auditEnsureRootGroupExistsObject = "auditEnsureRootGroupExists";
        const char* m_auditEnsureAllAccountsHavePasswordsObject = "auditEnsureAllAccountsHavePasswords";
        const char* m_auditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject = "auditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZero";
        const char* m_auditEnsureNoLegacyPlusEntriesInEtcPasswdObject = "auditEnsureNoLegacyPlusEntriesInEtcPasswd";
        const char* m_auditEnsureNoLegacyPlusEntriesInEtcShadowObject = "auditEnsureNoLegacyPlusEntriesInEtcShadow";
        const char* m_auditEnsureNoLegacyPlusEntriesInEtcGroupObject = "auditEnsureNoLegacyPlusEntriesInEtcGroup";
        const char* m_auditEnsureDefaultRootAccountGroupIsGidZeroObject = "auditEnsureDefaultRootAccountGroupIsGidZero";
        const char* m_auditEnsureRootIsOnlyUidZeroAccountObject = "auditEnsureRootIsOnlyUidZeroAccount";
        const char* m_auditEnsureAllUsersHomeDirectoriesExistObject = "auditEnsureAllUsersHomeDirectoriesExist";
        const char* m_auditEnsureUsersOwnTheirHomeDirectoriesObject = "auditEnsureUsersOwnTheirHomeDirectories";
        const char* m_auditEnsureRestrictedUserHomeDirectoriesObject = "auditEnsureRestrictedUserHomeDirectories";
        const char* m_auditEnsurePasswordHashingAlgorithmObject = "auditEnsurePasswordHashingAlgorithm";
        const char* m_auditEnsureMinDaysBetweenPasswordChangesObject = "auditEnsureMinDaysBetweenPasswordChanges";
        const char* m_auditEnsureInactivePasswordLockPeriodObject = "auditEnsureInactivePasswordLockPeriod";
        const char* m_auditMaxDaysBetweenPasswordChangesObject = "auditEnsureMaxDaysBetweenPasswordChanges";
        const char* m_auditEnsurePasswordExpirationObject = "auditEnsurePasswordExpiration";
        const char* m_auditEnsurePasswordExpirationWarningObject = "auditEnsurePasswordExpirationWarning";
        const char* m_auditEnsureSystemAccountsAreNonLoginObject = "auditEnsureSystemAccountsAreNonLogin";
        const char* m_auditEnsureAuthenticationRequiredForSingleUserModeObject = "auditEnsureAuthenticationRequiredForSingleUserMode";
        const char* m_auditEnsureDotDoesNotAppearInRootsPathObject = "auditEnsureDotDoesNotAppearInRootsPath";
        const char* m_auditEnsureRemoteLoginWarningBannerIsConfiguredObject = "auditEnsureRemoteLoginWarningBannerIsConfigured";
        const char* m_auditEnsureLocalLoginWarningBannerIsConfiguredObject = "auditEnsureLocalLoginWarningBannerIsConfigured";
        const char* m_auditEnsureSuRestrictedToRootGroupObject = "auditEnsureSuRestrictedToRootGroup";
        const char* m_auditEnsureDefaultUmaskForAllUsersObject = "auditEnsureDefaultUmaskForAllUsers";
        const char* m_auditEnsureAutomountingDisabledObject = "auditEnsureAutomountingDisabled";
        const char* m_auditEnsureKernelCompiledFromApprovedSourcesObject = "auditEnsureKernelCompiledFromApprovedSources";
        const char* m_auditEnsureDefaultDenyFirewallPolicyIsSetObject = "auditEnsureDefaultDenyFirewallPolicyIsSet";
        const char* m_auditEnsurePacketRedirectSendingIsDisabledObject = "auditEnsurePacketRedirectSendingIsDisabled";
        const char* m_auditEnsureIcmpRedirectsIsDisabledObject = "auditEnsureIcmpRedirectsIsDisabled";
        const char* m_auditEnsureSourceRoutedPacketsIsDisabledObject = "auditEnsureSourceRoutedPacketsIsDisabled";
        const char* m_auditEnsureAcceptingSourceRoutedPacketsIsDisabledObject = "auditEnsureAcceptingSourceRoutedPacketsIsDisabled";
        const char* m_auditEnsureIgnoringBogusIcmpBroadcastResponsesObject = "auditEnsureIgnoringBogusIcmpBroadcastResponses";
        const char* m_auditEnsureIgnoringIcmpEchoPingsToMulticastObject = "auditEnsureIgnoringIcmpEchoPingsToMulticast";
        const char* m_auditEnsureMartianPacketLoggingIsEnabledObject = "auditEnsureMartianPacketLoggingIsEnabled";
        const char* m_auditEnsureReversePathSourceValidationIsEnabledObject = "auditEnsureReversePathSourceValidationIsEnabled";
        const char* m_auditEnsureTcpSynCookiesAreEnabledObject = "auditEnsureTcpSynCookiesAreEnabled";
        const char* m_auditEnsureSystemNotActingAsNetworkSnifferObject = "auditEnsureSystemNotActingAsNetworkSniffer";
        const char* m_auditEnsureAllWirelessInterfacesAreDisabledObject = "auditEnsureAllWirelessInterfacesAreDisabled";
        const char* m_auditEnsureIpv6ProtocolIsEnabledObject = "auditEnsureIpv6ProtocolIsEnabled";
        const char* m_auditEnsureDccpIsDisabledObject = "auditEnsureDccpIsDisabled";
        const char* m_auditEnsureSctpIsDisabledObject = "auditEnsureSctpIsDisabled";
        const char* m_auditEnsureDisabledSupportForRdsObject = "auditEnsureDisabledSupportForRds";
        const char* m_auditEnsureTipcIsDisabledObject = "auditEnsureTipcIsDisabled";
        const char* m_auditEnsureZeroconfNetworkingIsDisabledObject = "auditEnsureZeroconfNetworkingIsDisabled";
        const char* m_auditEnsurePermissionsOnBootloaderConfigAreConfiguredObject = "auditEnsurePermissionsOnBootloaderConfigAreConfigured";
        const char* m_auditEnsurePasswordReuseIsLimitedObject = "auditEnsurePasswordReuseIsLimited";
        const char* m_auditEnsureMountingOfUsbStorageDevicesIsDisabledObject = "auditEnsureMountingOfUsbStorageDevicesIsDisabled";
        const char* m_auditEnsureCoreDumpsAreRestrictedObject = "auditEnsureCoreDumpsAreRestricted";
        const char* m_auditEnsurePasswordCreationRequirementsAreConfiguredObject = "auditEnsurePasswordCreationRequirementsAreConfigured";
        const char* m_auditEnsureLockoutForFailedPasswordAttemptsIsConfiguredObject = "auditEnsureLockoutForFailedPasswordAttemptsIsConfigured";
        const char* m_auditEnsureDisabledInstallationOfCramfsFileSystemObject = "auditEnsureDisabledInstallationOfCramfsFileSystem";
        const char* m_auditEnsureDisabledInstallationOfFreevxfsFileSystemObject = "auditEnsureDisabledInstallationOfFreevxfsFileSystem";
        const char* m_auditEnsureDisabledInstallationOfHfsFileSystemObject = "auditEnsureDisabledInstallationOfHfsFileSystem";
        const char* m_auditEnsureDisabledInstallationOfHfsplusFileSystemObject = "auditEnsureDisabledInstallationOfHfsplusFileSystem";
        const char* m_auditEnsureDisabledInstallationOfJffs2FileSystemObject = "auditEnsureDisabledInstallationOfJffs2FileSystem";
        const char* m_auditEnsureRandomizedPlacementOfVirtualMemoryRegionsIsEnabledObject = "auditEnsureRandomizedPlacementOfVirtualMemoryRegionsIsEnabled";
        const char* m_auditEnsureAllBootloadersHavePasswordProtectionEnabledObject = "auditEnsureAllBootloadersHavePasswordProtectionEnabled";
        const char* m_auditEnsureLoggingIsConfiguredObject = "auditEnsureLoggingIsConfigured";
        const char* m_auditEnsureSyslogPackageIsInstalledObject = "auditEnsureSyslogPackageIsInstalled";
        const char* m_auditEnsureSystemdJournaldServicePersistsLogMessagesObject = "auditEnsureSystemdJournaldServicePersistsLogMessages";
        const char* m_auditEnsureALoggingServiceIsSnabledObject = "auditEnsureALoggingServiceIsSnabled";
        const char* m_auditEnsureFilePermissionsForAllRsyslogLogFilesObject = "auditEnsureFilePermissionsForAllRsyslogLogFiles";
        const char* m_auditEnsureLoggerConfigurationFilesAreRestrictedObject = "auditEnsureLoggerConfigurationFilesAreRestricted";
        const char* m_auditEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject = "auditEnsureAllRsyslogLogFilesAreOwnedByAdmGroup";
        const char* m_auditEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject = "auditEnsureAllRsyslogLogFilesAreOwnedBySyslogUser";
        const char* m_auditEnsureRsyslogNotAcceptingRemoteMessagesObject = "auditEnsureRsyslogNotAcceptingRemoteMessages";
        const char* m_auditEnsureSyslogRotaterServiceIsEnabledObject = "auditEnsureSyslogRotaterServiceIsEnabled";
        const char* m_auditEnsureTelnetServiceIsDisabledObject = "auditEnsureTelnetServiceIsDisabled";
        const char* m_auditEnsureRcprshServiceIsDisabledObject = "auditEnsureRcprshServiceIsDisabled";
        const char* m_auditEnsureTftpServiceisDisabledObject = "auditEnsureTftpServiceisDisabled";
        const char* m_auditEnsureAtCronIsRestrictedToAuthorizedUsersObject = "auditEnsureAtCronIsRestrictedToAuthorizedUsers";
        const char* m_auditEnsureSshBestPracticeProtocolObject = "auditEnsureSshBestPracticeProtocol";
        const char* m_auditEnsureSshBestPracticeIgnoreRhostsObject = "auditEnsureSshBestPracticeIgnoreRhosts";
        const char* m_auditEnsureSshLogLevelIsSetObject = "auditEnsureSshLogLevelIsSet";
        const char* m_auditEnsureSshMaxAuthTriesIsSetObject = "auditEnsureSshMaxAuthTriesIsSet";
        const char* m_auditEnsureSshAccessIsLimitedObject = "auditEnsureSshAccessIsLimited";
        const char* m_auditEnsureSshRhostsRsaAuthenticationIsDisabledObject = "auditEnsureSshRhostsRsaAuthenticationIsDisabled";
        const char* m_auditEnsureSshHostbasedAuthenticationIsDisabledObject = "auditEnsureSshHostbasedAuthenticationIsDisabled";
        const char* m_auditEnsureSshPermitRootLoginIsDisabledObject = "auditEnsureSshPermitRootLoginIsDisabled";
        const char* m_auditEnsureSshPermitEmptyPasswordsIsDisabledObject = "auditEnsureSshPermitEmptyPasswordsIsDisabled";
        const char* m_auditEnsureSshIdleTimeoutIntervalIsConfiguredObject = "auditEnsureSshIdleTimeoutIntervalIsConfigured";
        const char* m_auditEnsureSshLoginGraceTimeIsSetObject = "auditEnsureSshLoginGraceTimeIsSet";
        const char* m_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject = "auditEnsureOnlyApprovedMacAlgorithmsAreUsed";
        const char* m_auditEnsureSshWarningBannerIsEnabledObject = "auditEnsureSshWarningBannerIsEnabled";
        const char* m_auditEnsureUsersCannotSetSshEnvironmentOptionsObject = "auditEnsureUsersCannotSetSshEnvironmentOptions";
        const char* m_auditEnsureAppropriateCiphersForSshObject = "auditEnsureAppropriateCiphersForSsh";
        const char* m_auditEnsureAvahiDaemonServiceIsDisabledObject = "auditEnsureAvahiDaemonServiceIsDisabled";
        const char* m_auditEnsureCupsServiceisDisabledObject = "auditEnsureCupsServiceisDisabled";
        const char* m_auditEnsurePostfixPackageIsUninstalledObject = "auditEnsurePostfixPackageIsUninstalled";
        const char* m_auditEnsurePostfixNetworkListeningIsDisabledObject = "auditEnsurePostfixNetworkListeningIsDisabled";
        const char* m_auditEnsureRpcgssdServiceIsDisabledObject = "auditEnsureRpcgssdServiceIsDisabled";
        const char* m_auditEnsureRpcidmapdServiceIsDisabledObject = "auditEnsureRpcidmapdServiceIsDisabled";
        const char* m_auditEnsurePortmapServiceIsDisabledObject = "auditEnsurePortmapServiceIsDisabled";
        const char* m_auditEnsureNetworkFileSystemServiceIsDisabledObject = "auditEnsureNetworkFileSystemServiceIsDisabled";
        const char* m_auditEnsureRpcsvcgssdServiceIsDisabledObject = "auditEnsureRpcsvcgssdServiceIsDisabled";
        const char* m_auditEnsureSnmpServerIsDisabledObject = "auditEnsureSnmpServerIsDisabled";
        const char* m_auditEnsureRsynServiceIsDisabledObject = "auditEnsureRsynServiceIsDisabled";
        const char* m_auditEnsureNisServerIsDisabledObject = "auditEnsureNisServerIsDisabled";
        const char* m_auditEnsureRshClientNotInstalledObject = "auditEnsureRshClientNotInstalled";
        const char* m_auditEnsureSmbWithSambaIsDisabledObject = "auditEnsureSmbWithSambaIsDisabled";
        const char* m_auditEnsureUsersDotFilesArentGroupOrWorldWritableObject = "auditEnsureUsersDotFilesArentGroupOrWorldWritable";
        const char* m_auditEnsureNoUsersHaveDotForwardFilesObject = "auditEnsureNoUsersHaveDotForwardFiles";
        const char* m_auditEnsureNoUsersHaveDotNetrcFilesObject = "auditEnsureNoUsersHaveDotNetrcFiles";
        const char* m_auditEnsureNoUsersHaveDotRhostsFilesObject = "auditEnsureNoUsersHaveDotRhostsFiles";
        const char* m_auditEnsureRloginServiceIsDisabledObject = "auditEnsureRloginServiceIsDisabled";
        const char* m_auditEnsureUnnecessaryAccountsAreRemovedObject = "auditEnsureUnnecessaryAccountsAreRemoved";

        // Remediation
        const char* m_remediateSecurityBaselineObject = "remediateSecurityBaseline";
        const char* m_remediateEnsurePermissionsOnEtcIssueObject = "remediateEnsurePermissionsOnEtcIssue";
        const char* m_remediateEnsurePermissionsOnEtcIssueNetObject = "remediateEnsurePermissionsOnEtcIssueNet";
        const char* m_remediateEnsurePermissionsOnEtcHostsAllowObject = "remediateEnsurePermissionsOnEtcHostsAllow";
        const char* m_remediateEnsurePermissionsOnEtcHostsDenyObject = "remediateEnsurePermissionsOnEtcHostsDeny";
        const char* m_remediateEnsurePermissionsOnEtcSshSshdConfigObject = "remediateEnsurePermissionsOnEtcSshSshdConfig";
        const char* m_remediateEnsurePermissionsOnEtcShadowObject = "remediateEnsurePermissionsOnEtcShadow";
        const char* m_remediateEnsurePermissionsOnEtcShadowDashObject = "remediateEnsurePermissionsOnEtcShadowDash";
        const char* m_remediateEnsurePermissionsOnEtcGShadowObject = "remediateEnsurePermissionsOnEtcGShadow";
        const char* m_remediateEnsurePermissionsOnEtcGShadowDashObject = "remediateEnsurePermissionsOnEtcGShadowDash";
        const char* m_remediateEnsurePermissionsOnEtcPasswdObject = "remediateEnsurePermissionsOnEtcPasswd";
        const char* m_remediateEnsurePermissionsOnEtcPasswdDashObject = "remediateEnsurePermissionsOnEtcPasswdDash";
        const char* m_remediateEnsurePermissionsOnEtcGroupObject = "remediateEnsurePermissionsOnEtcGroup";
        const char* m_remediateEnsurePermissionsOnEtcGroupDashObject = "remediateEnsurePermissionsOnEtcGroupDash";
        const char* m_remediateEnsurePermissionsOnEtcAnacronTabObject = "remediateEnsurePermissionsOnEtcAnacronTab";
        const char* m_remediateEnsurePermissionsOnEtcCronDObject = "remediateEnsurePermissionsOnEtcCronD";
        const char* m_remediateEnsurePermissionsOnEtcCronDailyObject = "remediateEnsurePermissionsOnEtcCronDaily";
        const char* m_remediateEnsurePermissionsOnEtcCronHourlyObject = "remediateEnsurePermissionsOnEtcCronHourly";
        const char* m_remediateEnsurePermissionsOnEtcCronMonthlyObject = "remediateEnsurePermissionsOnEtcCronMonthly";
        const char* m_remediateEnsurePermissionsOnEtcCronWeeklyObject = "remediateEnsurePermissionsOnEtcCronWeekly";
        const char* m_remediateEnsurePermissionsOnEtcMotdObject = "remediateEnsurePermissionsOnEtcMotd";
        const char* m_remediateEnsureInetdNotInstalledObject = "remediateEnsureInetdNotInstalled";
        const char* m_remediateEnsureXinetdNotInstalledObject = "remediateEnsureXinetdNotInstalled";
        const char* m_remediateEnsureRshServerNotInstalledObject = "remediateEnsureRshServerNotInstalled";
        const char* m_remediateEnsureNisNotInstalledObject = "remediateEnsureNisNotInstalled";
        const char* m_remediateEnsureTftpdNotInstalledObject = "remediateEnsureTftpdNotInstalled";
        const char* m_remediateEnsureReadaheadFedoraNotInstalledObject = "remediateEnsureReadaheadFedoraNotInstalled";
        const char* m_remediateEnsureBluetoothHiddNotInstalledObject = "remediateEnsureBluetoothHiddNotInstalled";
        const char* m_remediateEnsureIsdnUtilsBaseNotInstalledObject = "remediateEnsureIsdnUtilsBaseNotInstalled";
        const char* m_remediateEnsureIsdnUtilsKdumpToolsNotInstalledObject = "remediateEnsureIsdnUtilsKdumpToolsNotInstalled";
        const char* m_remediateEnsureIscDhcpdServerNotInstalledObject = "remediateEnsureIscDhcpdServerNotInstalled";
        const char* m_remediateEnsureSendmailNotInstalledObject = "remediateEnsureSendmailNotInstalled";
        const char* m_remediateEnsureSldapdNotInstalledObject = "remediateEnsureSldapdNotInstalled";
        const char* m_remediateEnsureBind9NotInstalledObject = "remediateEnsureBind9NotInstalled";
        const char* m_remediateEnsureDovecotCoreNotInstalledObject = "remediateEnsureDovecotCoreNotInstalled";
        const char* m_remediateEnsureAuditdInstalledObject = "remediateEnsureAuditdInstalled";
        const char* m_remediateEnsurePrelinkIsDisabledObject = "remediateEnsurePrelinkIsDisabled";
        const char* m_remediateEnsureTalkClientIsNotInstalledObject = "remediateEnsureTalkClientIsNotInstalled";
        const char* m_remediateEnsureCronServiceIsEnabledObject = "remediateEnsureCronServiceIsEnabled";
        const char* m_remediateEnsureAuditdServiceIsRunningObject = "remediateEnsureAuditdServiceIsRunning";

        const char* m_pass = "\"PASS\"";

        const char* m_clientName = "SecurityBaselineTest";

        int m_normalMaxPayloadSizeBytes = 1024;
        int m_truncatedMaxPayloadSizeBytes = 1;

        void SetUp()
        {
            SecurityBaselineInitialize();
        }

        void TearDown()
        {
            SecurityBaselineShutdown();
        }
};

TEST_F(SecurityBaselineTest, MmiOpen)
{
    MMI_HANDLE handle = nullptr;
    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    SecurityBaselineMmiClose(handle);
}

char* CopyPayloadToString(const char* payload, int payloadSizeBytes)
{
    char* output = nullptr;
    
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, output = (char*)malloc(payloadSizeBytes + 1));

    if (nullptr != output)
    {
        memcpy(output, payload, payloadSizeBytes);
        output[payloadSizeBytes] = 0;
    }

    return output;
}

TEST_F(SecurityBaselineTest, MmiGetInfo)
{
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, SecurityBaselineMmiGetInfo(m_clientName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);

    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_STREQ(m_expectedMmiInfo, payloadString);
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);

    FREE_MEMORY(payloadString);
    SecurityBaselineMmiFree(payload);
}

TEST_F(SecurityBaselineTest, MmiSet)
{
    MMI_HANDLE handle = nullptr;

    const char* payload = "PASS";

    const char* mimObjects[] = {
        m_remediateSecurityBaselineObject,
        m_remediateEnsurePermissionsOnEtcIssueObject,
        m_remediateEnsurePermissionsOnEtcIssueNetObject,
        m_remediateEnsurePermissionsOnEtcHostsAllowObject,
        m_remediateEnsurePermissionsOnEtcHostsDenyObject,
        m_remediateEnsurePermissionsOnEtcSshSshdConfigObject,
        m_remediateEnsurePermissionsOnEtcShadowObject,
        m_remediateEnsurePermissionsOnEtcShadowDashObject,
        m_remediateEnsurePermissionsOnEtcGShadowObject,
        m_remediateEnsurePermissionsOnEtcGShadowDashObject,
        m_remediateEnsurePermissionsOnEtcPasswdObject,
        m_remediateEnsurePermissionsOnEtcPasswdDashObject,
        m_remediateEnsurePermissionsOnEtcGroupObject,
        m_remediateEnsurePermissionsOnEtcGroupDashObject,
        m_remediateEnsurePermissionsOnEtcAnacronTabObject,
        m_remediateEnsurePermissionsOnEtcCronDObject,
        m_remediateEnsurePermissionsOnEtcCronDailyObject,
        m_remediateEnsurePermissionsOnEtcCronHourlyObject,
        m_remediateEnsurePermissionsOnEtcCronMonthlyObject,
        m_remediateEnsurePermissionsOnEtcCronWeeklyObject,
        m_remediateEnsurePermissionsOnEtcMotdObject,
        m_remediateEnsureInetdNotInstalledObject,
        m_remediateEnsureXinetdNotInstalledObject,
        m_remediateEnsureRshServerNotInstalledObject,
        m_remediateEnsureNisNotInstalledObject,
        m_remediateEnsureTftpdNotInstalledObject,
        m_remediateEnsureReadaheadFedoraNotInstalledObject,
        m_remediateEnsureBluetoothHiddNotInstalledObject,
        m_remediateEnsureIsdnUtilsBaseNotInstalledObject,
        m_remediateEnsureIsdnUtilsKdumpToolsNotInstalledObject,
        m_remediateEnsureIscDhcpdServerNotInstalledObject,
        m_remediateEnsureSendmailNotInstalledObject,
        m_remediateEnsureSldapdNotInstalledObject,
        m_remediateEnsureBind9NotInstalledObject,
        m_remediateEnsureDovecotCoreNotInstalledObject,
        m_remediateEnsureAuditdInstalledObject,
        m_remediateEnsurePrelinkIsDisabledObject,
        m_remediateEnsureTalkClientIsNotInstalledObject,
        m_remediateEnsureCronServiceIsEnabledObject,
        m_remediateEnsureAuditdServiceIsRunningObject
    };

    int mimObjectsNumber = ARRAY_SIZE(mimObjects);

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < mimObjectsNumber; i++)
    {
        // All optional, do not expect to suceed with MMI_OK (0), just run to validate no crashes, etc.
        SecurityBaselineMmiSet(handle, m_securityBaselineComponentName, mimObjects[i], (MMI_JSON_STRING)payload, strlen(payload));
    }

    SecurityBaselineMmiClose(handle);
}

TEST_F(SecurityBaselineTest, MmiSetInvalidComponent)
{
    MMI_HANDLE handle = NULL;
    const char* payload = "PASS";
    int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, SecurityBaselineMmiSet(handle, "Test123", m_remediateSecurityBaselineObject, (MMI_JSON_STRING)payload, payloadSizeBytes));
    SecurityBaselineMmiClose(handle);
}

TEST_F(SecurityBaselineTest, MmiSetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    const char* payload = "PASS";
    int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, SecurityBaselineMmiSet(handle, m_securityBaselineComponentName, "Test123", (MMI_JSON_STRING)payload, payloadSizeBytes));
    SecurityBaselineMmiClose(handle);
}

TEST_F(SecurityBaselineTest, MmiSetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    const char* payload = "PASS";
    int payloadSizeBytes = strlen(payload);

    EXPECT_EQ(EINVAL, SecurityBaselineMmiSet(handle, m_securityBaselineComponentName, m_remediateSecurityBaselineObject, (MMI_JSON_STRING)payload, payloadSizeBytes));

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    SecurityBaselineMmiClose(handle);
    EXPECT_EQ(EINVAL, SecurityBaselineMmiSet(handle, m_securityBaselineComponentName, m_remediateSecurityBaselineObject, (MMI_JSON_STRING)payload, payloadSizeBytes));
}

TEST_F(SecurityBaselineTest, MmiGet)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    const char* mimObjects[] = {
        m_auditSecurityBaselineObject,
        m_auditEnsurePermissionsOnEtcIssueObject,
        m_auditEnsurePermissionsOnEtcIssueNetObject,
        m_auditEnsurePermissionsOnEtcHostsAllowObject,
        m_auditEnsurePermissionsOnEtcHostsDenyObject,
        m_auditEnsurePermissionsOnEtcSshSshdConfigObject,
        m_auditEnsurePermissionsOnEtcShadowObject,
        m_auditEnsurePermissionsOnEtcShadowDashObject,
        m_auditEnsurePermissionsOnEtcGShadowObject,
        m_auditEnsurePermissionsOnEtcGShadowDashObject,
        m_auditEnsurePermissionsOnEtcPasswdObject,
        m_auditEnsurePermissionsOnEtcPasswdDashObject,
        m_auditEnsurePermissionsOnEtcGroupObject,
        m_auditEnsurePermissionsOnEtcGroupDashObject,
        m_auditEnsurePermissionsOnEtcAnacronTabObject,
        m_auditEnsurePermissionsOnEtcCronDObject,
        m_auditEnsurePermissionsOnEtcCronDailyObject,
        m_auditEnsurePermissionsOnEtcCronHourlyObject,
        m_auditEnsurePermissionsOnEtcCronMonthlyObject,
        m_auditEnsurePermissionsOnEtcCronWeeklyObject,
        m_auditEnsurePermissionsOnEtcMotdObject,
        m_auditEnsureKernelSupportForCpuNxObject,
        m_auditEnsureNodevOptionOnHomePartitionObject,
        m_auditEnsureNodevOptionOnTmpPartitionObject,
        m_auditEnsureNodevOptionOnVarTmpPartitionObject,
        m_auditEnsureNosuidOptionOnTmpPartitionObject,
        m_auditEnsureNosuidOptionOnVarTmpPartitionObject,
        m_auditEnsureNoexecOptionOnVarTmpPartitionObject,
        m_auditEnsureNoexecOptionOnDevShmPartitionObject,
        m_auditEnsureNodevOptionEnabledForAllRemovableMediaObject,
        m_auditEnsureNoexecOptionEnabledForAllRemovableMediaObject,
        m_auditEnsureNosuidOptionEnabledForAllRemovableMediaObject,
        m_auditEnsureNoexecNosuidOptionsEnabledForAllNfsMountsObject,
        m_auditEnsureInetdNotInstalledObject,
        m_auditEnsureXinetdNotInstalledObject,
        m_auditEnsureAllTelnetdPackagesUninstalledObject,
        m_auditEnsureRshServerNotInstalledObject,
        m_auditEnsureNisNotInstalledObject,
        m_auditEnsureTftpdNotInstalledObject,
        m_auditEnsureReadaheadFedoraNotInstalledObject,
        m_auditEnsureBluetoothHiddNotInstalledObject,
        m_auditEnsureIsdnUtilsBaseNotInstalledObject,
        m_auditEnsureIsdnUtilsKdumpToolsNotInstalledObject,
        m_auditEnsureIscDhcpdServerNotInstalledObject,
        m_auditEnsureSendmailNotInstalledObject,
        m_auditEnsureSldapdNotInstalledObject,
        m_auditEnsureBind9NotInstalledObject,
        m_auditEnsureDovecotCoreNotInstalledObject,
        m_auditEnsureAuditdInstalledObject,
        m_auditEnsureAllEtcPasswdGroupsExistInEtcGroupObject,
        m_auditEnsureNoDuplicateUidsExistObject,
        m_auditEnsureNoDuplicateGidsExistObject,
        m_auditEnsureNoDuplicateUserNamesExistObject,
        m_auditEnsureNoDuplicateGroupsExistObject,
        m_auditEnsureShadowGroupIsEmptyObject,
        m_auditEnsureRootGroupExistsObject,
        m_auditEnsureAllAccountsHavePasswordsObject,
        m_auditEnsureNonRootAccountsHaveUniqueUidsGreaterThanZeroObject,
        m_auditEnsureNoLegacyPlusEntriesInEtcPasswdObject,
        m_auditEnsureNoLegacyPlusEntriesInEtcShadowObject,
        m_auditEnsureNoLegacyPlusEntriesInEtcGroupObject,
        m_auditEnsureDefaultRootAccountGroupIsGidZeroObject,
        m_auditEnsureRootIsOnlyUidZeroAccountObject,
        m_auditEnsureAllUsersHomeDirectoriesExistObject,
        m_auditEnsureUsersOwnTheirHomeDirectoriesObject,
        m_auditEnsureRestrictedUserHomeDirectoriesObject,
        m_auditEnsurePasswordHashingAlgorithmObject,
        m_auditEnsureSystemAccountsAreNonLoginObject,
        m_auditEnsurePrelinkIsDisabledObject,
        m_auditEnsureTalkClientIsNotInstalledObject,
        m_auditEnsureDotDoesNotAppearInRootsPathObject,
        m_auditEnsureCronServiceIsEnabledObject,
        m_auditEnsureRemoteLoginWarningBannerIsConfiguredObject,
        m_auditEnsureLocalLoginWarningBannerIsConfiguredObject,
        m_auditEnsureAuditdServiceIsRunningObject,
        m_auditEnsureMinDaysBetweenPasswordChangesObject,
        m_auditEnsureInactivePasswordLockPeriodObject,
        m_auditMaxDaysBetweenPasswordChangesObject,
        m_auditEnsurePasswordExpirationObject,
        m_auditEnsurePasswordExpirationWarningObject,
        m_auditEnsureAuthenticationRequiredForSingleUserModeObject,
        m_auditEnsureSuRestrictedToRootGroupObject,
        m_auditEnsureDefaultUmaskForAllUsersObject,
        m_auditEnsureAutomountingDisabledObject,
        m_auditEnsureKernelCompiledFromApprovedSourcesObject,
        m_auditEnsureDefaultDenyFirewallPolicyIsSetObject,
        m_auditEnsurePacketRedirectSendingIsDisabledObject,
        m_auditEnsureIcmpRedirectsIsDisabledObject,
        m_auditEnsureSourceRoutedPacketsIsDisabledObject,
        m_auditEnsureAcceptingSourceRoutedPacketsIsDisabledObject,
        m_auditEnsureIgnoringBogusIcmpBroadcastResponsesObject,
        m_auditEnsureIgnoringIcmpEchoPingsToMulticastObject,
        m_auditEnsureMartianPacketLoggingIsEnabledObject,
        m_auditEnsureReversePathSourceValidationIsEnabledObject,
        m_auditEnsureTcpSynCookiesAreEnabledObject,
        m_auditEnsureSystemNotActingAsNetworkSnifferObject,
        m_auditEnsureAllWirelessInterfacesAreDisabledObject,
        m_auditEnsureIpv6ProtocolIsEnabledObject,
        m_auditEnsureDccpIsDisabledObject,
        m_auditEnsureSctpIsDisabledObject,
        m_auditEnsureDisabledSupportForRdsObject,
        m_auditEnsureTipcIsDisabledObject,
        m_auditEnsureZeroconfNetworkingIsDisabledObject,
        m_auditEnsurePermissionsOnBootloaderConfigAreConfiguredObject,
        m_auditEnsurePasswordReuseIsLimitedObject,
        m_auditEnsureMountingOfUsbStorageDevicesIsDisabledObject,
        m_auditEnsureCoreDumpsAreRestrictedObject,
        m_auditEnsurePasswordCreationRequirementsAreConfiguredObject,
        m_auditEnsureLockoutForFailedPasswordAttemptsIsConfiguredObject,
        m_auditEnsureDisabledInstallationOfCramfsFileSystemObject,
        m_auditEnsureDisabledInstallationOfFreevxfsFileSystemObject,
        m_auditEnsureDisabledInstallationOfHfsFileSystemObject,
        m_auditEnsureDisabledInstallationOfHfsplusFileSystemObject,
        m_auditEnsureDisabledInstallationOfJffs2FileSystemObject,
        m_auditEnsureRandomizedPlacementOfVirtualMemoryRegionsIsEnabledObject,
        m_auditEnsureAllBootloadersHavePasswordProtectionEnabledObject,
        m_auditEnsureLoggingIsConfiguredObject,
        m_auditEnsureSyslogPackageIsInstalledObject,
        m_auditEnsureSystemdJournaldServicePersistsLogMessagesObject,
        m_auditEnsureALoggingServiceIsSnabledObject,
        m_auditEnsureFilePermissionsForAllRsyslogLogFilesObject,
        m_auditEnsureLoggerConfigurationFilesAreRestrictedObject,
        m_auditEnsureAllRsyslogLogFilesAreOwnedByAdmGroupObject,
        m_auditEnsureAllRsyslogLogFilesAreOwnedBySyslogUserObject,
        m_auditEnsureRsyslogNotAcceptingRemoteMessagesObject,
        m_auditEnsureSyslogRotaterServiceIsEnabledObject,
        m_auditEnsureTelnetServiceIsDisabledObject,
        m_auditEnsureRcprshServiceIsDisabledObject,
        m_auditEnsureTftpServiceisDisabledObject,
        m_auditEnsureAtCronIsRestrictedToAuthorizedUsersObject,
        m_auditEnsureSshBestPracticeProtocolObject,
        m_auditEnsureSshBestPracticeIgnoreRhostsObject,
        m_auditEnsureSshLogLevelIsSetObject,
        m_auditEnsureSshMaxAuthTriesIsSetObject,
        m_auditEnsureSshAccessIsLimitedObject,
        m_auditEnsureSshRhostsRsaAuthenticationIsDisabledObject,
        m_auditEnsureSshHostbasedAuthenticationIsDisabledObject,
        m_auditEnsureSshPermitRootLoginIsDisabledObject,
        m_auditEnsureSshPermitEmptyPasswordsIsDisabledObject,
        m_auditEnsureSshIdleTimeoutIntervalIsConfiguredObject,
        m_auditEnsureSshLoginGraceTimeIsSetObject,
        m_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject,
        m_auditEnsureSshWarningBannerIsEnabledObject,
        m_auditEnsureUsersCannotSetSshEnvironmentOptionsObject,
        m_auditEnsureAppropriateCiphersForSshObject,
        m_auditEnsureAvahiDaemonServiceIsDisabledObject,
        m_auditEnsureCupsServiceisDisabledObject,
        m_auditEnsurePostfixPackageIsUninstalledObject,
        m_auditEnsurePostfixNetworkListeningIsDisabledObject,
        m_auditEnsureRpcgssdServiceIsDisabledObject,
        m_auditEnsureRpcidmapdServiceIsDisabledObject,
        m_auditEnsurePortmapServiceIsDisabledObject,
        m_auditEnsureNetworkFileSystemServiceIsDisabledObject,
        m_auditEnsureRpcsvcgssdServiceIsDisabledObject,
        m_auditEnsureSnmpServerIsDisabledObject,
        m_auditEnsureRsynServiceIsDisabledObject,
        m_auditEnsureNisServerIsDisabledObject,
        m_auditEnsureRshClientNotInstalledObject,
        m_auditEnsureSmbWithSambaIsDisabledObject,
        m_auditEnsureUsersDotFilesArentGroupOrWorldWritableObject,
        m_auditEnsureNoUsersHaveDotForwardFilesObject,
        m_auditEnsureNoUsersHaveDotNetrcFilesObject,
        m_auditEnsureNoUsersHaveDotRhostsFilesObject,
        m_auditEnsureRloginServiceIsDisabledObject,
        m_auditEnsureUnnecessaryAccountsAreRemovedObject
    };

    int mimObjectsNumber = ARRAY_SIZE(mimObjects);

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < mimObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, SecurityBaselineMmiGet(handle, m_securityBaselineComponentName, mimObjects[i], &payload, &payloadSizeBytes));
        EXPECT_NE(nullptr, payload);
        EXPECT_NE(0, payloadSizeBytes);
        EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
        EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
        FREE_MEMORY(payloadString);
        SecurityBaselineMmiFree(payload);
    }
    
    SecurityBaselineMmiClose(handle);
}

TEST_F(SecurityBaselineTest, MmiGetTruncatedPayload)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_truncatedMaxPayloadSizeBytes));
    EXPECT_EQ(MMI_OK, SecurityBaselineMmiGet(handle, m_securityBaselineComponentName, m_auditSecurityBaselineObject, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
    FREE_MEMORY(payloadString);
    SecurityBaselineMmiFree(payload);
    SecurityBaselineMmiClose(handle);
}

TEST_F(SecurityBaselineTest, MmiGetInvalidComponent)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, SecurityBaselineMmiGet(handle, "Test123", m_securityBaselineComponentName, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);
    
    SecurityBaselineMmiClose(handle);
}

TEST_F(SecurityBaselineTest, MmiGetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, SecurityBaselineMmiGet(handle, m_securityBaselineComponentName, "Test123", &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);
    
    SecurityBaselineMmiClose(handle);
}

TEST_F(SecurityBaselineTest, MmiGetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(EINVAL, SecurityBaselineMmiGet(handle, m_securityBaselineComponentName, m_auditSecurityBaselineObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    SecurityBaselineMmiClose(handle);

    EXPECT_EQ(EINVAL, SecurityBaselineMmiGet(handle, m_securityBaselineComponentName, m_auditSecurityBaselineObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);
}