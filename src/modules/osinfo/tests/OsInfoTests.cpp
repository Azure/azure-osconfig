// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <Mmi.h>
#include <OsInfo.h>

using namespace std;

#define STRFTIME_DATE_FORMAT "%Y%m%d"
#define SSCANF_DATE_FORMAT "%4d%2d%2d"
#define DATE_FORMAT_LENGTH 9

class OsInfoTest : public ::testing::Test
{
    protected:
        const char* m_expectedMmiInfo = "{\"Name\": \"OsInfo\","
            "\"Description\": \"Provides functionality to observe OS and device information\","
            "\"Manufacturer\": \"Microsoft\","
            "\"VersionMajor\": 1,"
            "\"VersionMinor\": 0,"
            "\"VersionInfo\": \"Copper\","
            "\"Components\": [\"OsInfo\"],"
            "\"Lifetime\": 2,"
            "\"UserAccount\": 0}";

        const char* m_osInfoModuleName = "OsInfo module";
        const char* m_osInfoComponentName = "OsInfo";
        const char* m_osNameObject = "OsName";
        const char* m_osVersionObject = "OsVersion";
        const char* m_cpuTypeObject = "Processor";
        const char* m_kernelNameObject = "KernelName";
        const char* m_kernelReleaseObject = "KernelRelease";
        const char* m_kernelVersionObject = "KernelVersion";
        const char* m_productNameObject = "ProductName";
        const char* m_productVendorObject = "ProductVendor";

        void InitializeModule()
        {
            OsInfoInitialize();
        }

        void Cleanup()
        {
            OsInfoShutdown();
        }
};

TEST_F(OsInfoTest, MmiOpen)
{
    MMI_HANDLE handle = NULL;
    EXPECT_NE(nullptr, handle = OsInfoMmiOpen("Test", 1024));
    OsInfoMmiClose(handle);
}
