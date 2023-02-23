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

    const char* mimRequiredObjects[] = {
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
        m_remediateEnsureAuditdInstalledObject
    };

    int mimRequiredObjectsNumber = ARRAY_SIZE(mimRequiredObjects);

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < mimRequiredObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, SecurityBaselineMmiSet(handle, m_securityBaselineComponentName, mimRequiredObjects[i], (MMI_JSON_STRING)payload, strlen(payload)));
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

    const char* mimRequiredObjects[] = {
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
        m_auditEnsureAuditdInstalledObject
    };
    
    int mimRequiredObjectsNumber = ARRAY_SIZE(mimRequiredObjects);

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < mimRequiredObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, SecurityBaselineMmiGet(handle, m_securityBaselineComponentName, mimRequiredObjects[i], &payload, &payloadSizeBytes));
        EXPECT_NE(nullptr, payload);
        EXPECT_NE(0, payloadSizeBytes);
        EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
        EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
        EXPECT_STREQ(payloadString, m_pass);
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

    const char* mimRequiredObjects[] = {
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
        m_auditEnsureAuditdInstalledObject
    };

    int mimRequiredObjectsNumber = ARRAY_SIZE(mimRequiredObjects);

    EXPECT_NE(nullptr, handle = SecurityBaselineMmiOpen(m_clientName, m_truncatedMaxPayloadSizeBytes));

    for (int i = 0; i < mimRequiredObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, SecurityBaselineMmiGet(handle, m_securityBaselineComponentName, mimRequiredObjects[i], &payload, &payloadSizeBytes));
        EXPECT_NE(nullptr, payload);
        EXPECT_NE(0, payloadSizeBytes);
        EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
        EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
        EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
        FREE_MEMORY(payloadString);
        SecurityBaselineMmiFree(payload);
    }

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