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
        //audit-only
        const char* m_auditEnsureKernelSupportForCpuNxObject = "auditEnsureKernelSupportForCpuNx";
        const char* m_auditEnsureNodevOptionOnHomePartition = "auditEnsureNodevOptionOnHomePartition";
        const char* m_auditEnsureNodevOptionOnTmpPartition = "auditEnsureNodevOptionOnTmpPartition";
        const char* m_auditEnsureNodevOptionOnVarTmpPartition = "auditEnsureNodevOptionOnVarTmpPartition";
        const char* m_auditEnsureNosuidOptionOnTmpPartition = "auditEnsureNosuidOptionOnTmpPartition";
        const char* m_auditEnsureNosuidOptionOnVarTmpPartition = "auditEnsureNosuidOptionOnVarTmpPartition";
        const char* m_auditEnsureNoexecOptionOnVarTmpPartition = "auditEnsureNoexecOptionOnVarTmpPartition";
        const char* m_auditEnsureNoexecOptionOnDevShmPartition = "auditEnsureNoexecOptionOnDevShmPartition";
        const char* m_auditEnsureNodevOptionEnabledForAllRemovableMedia = "auditEnsureNodevOptionEnabledForAllRemovableMedia";
        const char* m_auditEnsureNoexecOptionEnabledForAllRemovableMedia = "auditEnsureNoexecOptionEnabledForAllRemovableMedia";
        const char* m_auditEnsureNosuidOptionEnabledForAllRemovableMedia = "auditEnsureNosuidOptionEnabledForAllRemovableMedia";
        const char* m_auditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts = "auditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts";
        const char* m_auditEnsureInetdNotInstalled = "auditEnsureInetdNotInstalled";
        const char* m_auditEnsureXinetdNotInstalled = "auditEnsureXinetdNotInstalled";
        const char* m_auditEnsureAllelnetdPackagesUninstalled = "auditEnsureAllelnetdPackagesUninstalled";
        const char* m_auditEnsureRshServerNotInstalled = "auditEnsureRshServerNotInstalled";
        const char* m_auditEnsureNisNotInstalled = "auditEnsureNisNotInstalled";
        const char* m_auditEnsureTftpdNotInstalled = "auditEnsureTftpdNotInstalled";
        const char* m_auditEnsureReadaheadFedoraNotInstalled = "auditEnsureReadaheadFedoraNotInstalled";
        const char* m_auditEnsureBluetoothHiddNotInstalled = "auditEnsureBluetoothHiddNotInstalled";
        const char* m_auditEnsureIsdnUtilsBaseNotInstalled = "auditEnsureIsdnUtilsBaseNotInstalled";
        const char* m_auditEnsureIsdnUtilsKdumpToolsNotInstalled = "auditEnsureIsdnUtilsKdumpToolsNotInstalled";
        const char* m_auditEnsureIscDhcpdServerNotInstalled = "auditEnsureIscDhcpdServerNotInstalled";
        const char* m_auditEnsureSendmailNotInstalled = "auditEnsureSendmailNotInstalled";
        const char* m_auditEnsureSldapdNotInstalled = "auditEnsureSldapdNotInstalled";
        const char* m_auditEnsureBind9NotInstalled = "auditEnsureBind9NotInstalled";
        const char* m_auditEnsureDovecotCoreNotInstalled = "auditEnsureDovecotCoreNotInstalled";

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
        m_auditEnsureNodevOptionOnHomePartition,
        m_auditEnsureNodevOptionOnTmpPartition,
        m_auditEnsureNodevOptionOnVarTmpPartition,
        m_auditEnsureNosuidOptionOnTmpPartition,
        m_auditEnsureNosuidOptionOnVarTmpPartition,
        m_auditEnsureNoexecOptionOnVarTmpPartition,
        m_auditEnsureNoexecOptionOnDevShmPartition,
        m_auditEnsureNodevOptionEnabledForAllRemovableMedia,
        m_auditEnsureNoexecOptionEnabledForAllRemovableMedia,
        m_auditEnsureNosuidOptionEnabledForAllRemovableMedia,
        m_auditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts,
        m_auditEnsureInetdNotInstalled,
        m_auditEnsureXinetdNotInstalled,
        m_auditEnsureAllelnetdPackagesUninstalled,
        m_auditEnsureRshServerNotInstalled,
        m_auditEnsureNisNotInstalled,
        m_auditEnsureTftpdNotInstalled,
        m_auditEnsureReadaheadFedoraNotInstalled,
        m_auditEnsureBluetoothHiddNotInstalled,
        m_auditEnsureIsdnUtilsBaseNotInstalled,
        m_auditEnsureIsdnUtilsKdumpToolsNotInstalled,
        m_auditEnsureIscDhcpdServerNotInstalled,
        m_auditEnsureSendmailNotInstalled,
        m_auditEnsureSldapdNotInstalled,
        m_auditEnsureBind9NotInstalled,
        m_auditEnsureDovecotCoreNotInstalled
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
        m_auditEnsureNodevOptionOnHomePartition,
        m_auditEnsureNodevOptionOnTmpPartition,
        m_auditEnsureNodevOptionOnVarTmpPartition,
        m_auditEnsureNosuidOptionOnTmpPartition,
        m_auditEnsureNosuidOptionOnVarTmpPartition,
        m_auditEnsureNoexecOptionOnVarTmpPartition,
        m_auditEnsureNoexecOptionOnDevShmPartition,
        m_auditEnsureNodevOptionEnabledForAllRemovableMedia,
        m_auditEnsureNoexecOptionEnabledForAllRemovableMedia,
        m_auditEnsureNosuidOptionEnabledForAllRemovableMedia,
        m_auditEnsureNoexecNosuidOptionsEnabledForAllNfsMounts,
        m_auditEnsureInetdNotInstalled,
        m_auditEnsureXinetdNotInstalled,
        m_auditEnsureAllelnetdPackagesUninstalled,
        m_auditEnsureRshServerNotInstalled,
        m_auditEnsureNisNotInstalled,
        m_auditEnsureTftpdNotInstalled,
        m_auditEnsureReadaheadFedoraNotInstalled,
        m_auditEnsureBluetoothHiddNotInstalled,
        m_auditEnsureIsdnUtilsBaseNotInstalled,
        m_auditEnsureIsdnUtilsKdumpToolsNotInstalled,
        m_auditEnsureIscDhcpdServerNotInstalled,
        m_auditEnsureSendmailNotInstalled,
        m_auditEnsureSldapdNotInstalled,
        m_auditEnsureBind9NotInstalled,
        m_auditEnsureDovecotCoreNotInstalled
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

struct SecurityBaselineCombination
{
    const char* desiredObject;
    const char* reportedObject;
    const char* reportedValue;
};

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
        m_remediateEnsurePermissionsOnEtcMotdObject
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