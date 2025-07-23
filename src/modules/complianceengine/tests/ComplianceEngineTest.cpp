// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ComplianceEngineInterface.h"

#include <CommonUtils.h>
#include <DistributionInfo.h>
#include <Engine.h>
#include <Mmi.h>
#include <fstream>
#include <gtest/gtest.h>
#include <sys/utsname.h>
#include <version.h>

using namespace std;
using ComplianceEngine::DistributionInfo;

class ComplianceEngineTest : public ::testing::Test
{
protected:
    MMI_HANDLE mHandle = nullptr;
    static const unsigned int cMaxPayloadSize = 100;
    void SetUp() override
    {
        ComplianceEngineInitialize(nullptr);
        mHandle = ComplianceEngineMmiOpen("test", cMaxPayloadSize);
    }

    void TearDown() override
    {
        ComplianceEngineMmiClose(mHandle);
        ComplianceEngineShutdown();
    }
};

TEST_F(ComplianceEngineTest, ComplianceEngineMmiOpen_1)
{
    ASSERT_NE(mHandle, nullptr);
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGetInfo_InvalidArguments_1)
{
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceEngineMmiGetInfo("test", nullptr, &payloadSizeBytes));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGetInfo_InvalidArguments_2)
{
    char* payload = nullptr;
    ASSERT_NE(MMI_OK, ComplianceEngineMmiGetInfo("test", &payload, nullptr));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGetInfo_1)
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_EQ(MMI_OK, ComplianceEngineMmiGetInfo("test", &payload, &payloadSizeBytes));
    ComplianceEngineMmiFree(payload);
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiSet_InvalidArguments_1)
{
    auto payload = std::string("\"eyJhdWRpdCI6eyJhbnlPZiI6W119fQ=="); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceEngineMmiSet(nullptr, "ComplianceEngine", "procedureX", payload.c_str(), static_cast<int>(payload.size())));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiSet_InvalidArguments_2)
{
    auto payload = std::string("\"eyJhdWRpdCI6eyJhbnlPZiI6W119fQ==\""); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceEngineMmiSet(mHandle, nullptr, "procedureX", payload.c_str(), static_cast<int>(payload.size())));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiSet_InvalidArguments_3)
{
    auto payload = std::string("\"eyJhdWRpdCI6eyJhbnlPZiI6W119fQ==\""); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceEngineMmiSet(mHandle, "wrong module name", "procedureX", payload.c_str(), static_cast<int>(payload.size())));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiSet_InvalidArguments_4)
{
    auto payload = std::string("\"eyJhdWRpdCI6eyJhbnlPZiI6W119fQ==\""); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceEngineMmiSet(mHandle, "ComplianceEngine", nullptr, payload.c_str(), static_cast<int>(payload.size())));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiSet_InvalidArguments_5)
{
    auto payload = std::string("\"eyJhdWRpdCI6eyJhbnlPZiI6W119fQ==\""); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceEngineMmiSet(mHandle, "ComplianceEngine", "procedureX", nullptr, static_cast<int>(payload.size())));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiSet_InvalidArguments_6)
{
    auto payload = std::string("\"eyJhdWRpdCI6eyJhbnlPZiI6W119fQ==\""); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceEngineMmiSet(mHandle, "ComplianceEngine", "procedureX", payload.c_str(), -1));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiSet_SetProcedure_1)
{
    auto payload = std::string("\"eyJhdWRpdCI6eyJhbnlPZiI6W119fQ==\""); // {"audit":{"anyOf":[]}} in base64
    ASSERT_EQ(MMI_OK, ComplianceEngineMmiSet(mHandle, "ComplianceEngine", "procedureX", payload.c_str(), static_cast<int>(payload.size())));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiSet_SetProcedure_2)
{
    auto procedurePayload =
        std::string("\"eyJhdWRpdCI6eyJhbnlPZiI6W3sicW0/Ijp7fX1dfX0K\""); // '{"audit":{"anyOf":[{"qm?":{}}]}}' in base64, verify that '/' is properly de-escaped
    ASSERT_EQ(MMI_OK, ComplianceEngineMmiSet(mHandle, "ComplianceEngine", "procedureX", procedurePayload.c_str(), static_cast<int>(procedurePayload.size())));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGet_InvalidArguments_1)
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceEngineMmiGet(nullptr, "ComplianceEngine", "auditX", &payload, &payloadSizeBytes));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGet_InvalidArguments_2)
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceEngineMmiGet(mHandle, nullptr, "auditX", &payload, &payloadSizeBytes));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGet_InvalidArguments_3)
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    auto result = ComplianceEngineMmiGet(mHandle, "ComplianceEngine", "auditX", &payload, &payloadSizeBytes);
    ASSERT_EQ(result, MMI_OK);
    ASSERT_NE(payload, nullptr);
    ASSERT_TRUE(std::string(payload).find("Rule not found") != std::string::npos);
    ComplianceEngineMmiFree(payload);
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGet_InvalidArguments_4)
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceEngineMmiGet(mHandle, "ComplianceEngine", nullptr, &payload, &payloadSizeBytes));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGet_InvalidArguments_5)
{
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceEngineMmiGet(mHandle, "ComplianceEngine", "auditX", nullptr, &payloadSizeBytes));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGet_InvalidArguments_6)
{
    char* payload = nullptr;
    ASSERT_NE(MMI_OK, ComplianceEngineMmiGet(mHandle, "ComplianceEngine", "auditX", &payload, nullptr));
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGet_1)
{
    auto procedurePayload = std::string("\"eyJhdWRpdCI6eyJhbnlPZiI6W119fQ==\""); // {"audit":{"anyOf":[]}} in base64
    ASSERT_EQ(MMI_OK, ComplianceEngineMmiSet(mHandle, "ComplianceEngine", "procedureX", procedurePayload.c_str(), static_cast<int>(procedurePayload.size())));
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_EQ(MMI_OK, ComplianceEngineMmiGet(mHandle, "ComplianceEngine", "auditX", &payload, &payloadSizeBytes));
    ASSERT_NE(payload, nullptr);
    EXPECT_NE(0, strncmp(payload, "\"PASS", 5));
    ComplianceEngineMmiFree(payload);
}

TEST_F(ComplianceEngineTest, ComplianceEngineMmiGet_2)
{
    auto procedurePayload = std::string("\"eyJhdWRpdCI6eyJhbGxPZiI6W119fQ==\""); // {"audit":{"allOf":[]}} in base64
    ASSERT_EQ(MMI_OK, ComplianceEngineMmiSet(mHandle, "ComplianceEngine", "procedureX", procedurePayload.c_str(), static_cast<int>(procedurePayload.size())));
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_EQ(MMI_OK, ComplianceEngineMmiGet(mHandle, "ComplianceEngine", "auditX", &payload, &payloadSizeBytes));
    ASSERT_NE(payload, nullptr);
    ASSERT_TRUE(payloadSizeBytes >= 5);
    EXPECT_EQ(0, strncmp(payload, "\"PASS", 5));
    ComplianceEngineMmiFree(payload);
}

TEST_F(ComplianceEngineTest, ValidatePayload_1)
{
    ASSERT_EQ(EINVAL, ComplianceEngineValidatePayload(nullptr, nullptr, nullptr, "/cis/ubuntu/22.04/v1.1.1/x/y/z", nullptr));
    ASSERT_EQ(EINVAL, ComplianceEngineValidatePayload(mHandle, nullptr, nullptr, nullptr, nullptr));
}

TEST_F(ComplianceEngineTest, ValidatePayload_2)
{
    ASSERT_EQ(EINVAL, ComplianceEngineValidatePayload(mHandle, nullptr, "test", "/cis/foo/bar/v1.1.1/x/y/z", nullptr));
}

TEST_F(ComplianceEngineTest, ValidatePayload_3)
{
    auto& engine = *reinterpret_cast<ComplianceEngine::Engine*>(mHandle);
    auto osRelease = engine.GetDistributionInfo();
    ASSERT_TRUE(osRelease.HasValue());
    std::string payloadKey = std::string("/cis/") + to_string(osRelease->distribution) + "/" + osRelease->version + "/v1.1.1/x/y/z";
    ASSERT_EQ(0, ComplianceEngineValidatePayload(mHandle, nullptr, "test", payloadKey.c_str(), nullptr));
}

TEST_F(ComplianceEngineTest, ValidatePayload_OverrideFile)
{
    if (0 != getuid())
    {
        GTEST_SKIP() << "This test suite requires root privileges or fakeroot";
    }

    struct stat st;
    auto status = stat(DistributionInfo::cDefaultOverrideFilePath, &st);
    if ((0 == status) || (ENOENT != errno))
    {
        GTEST_SKIP() << "This test suite modifies the override file which is already present";
    }

    std::ofstream overrideFile(DistributionInfo::cDefaultOverrideFilePath);
    overrideFile << "OS=Linux ARCH=x86_64 DISTRO=ol VERSION=foo\n";
    overrideFile.flush();
    overrideFile.close();
    ComplianceEngineShutdown();
    auto handle = ComplianceEngineMmiOpen("test", cMaxPayloadSize);
    EXPECT_EQ(0, remove(DistributionInfo::cDefaultOverrideFilePath));
    ASSERT_NE(nullptr, handle);
    auto& engine = *reinterpret_cast<ComplianceEngine::Engine*>(handle);
    auto osRelease = engine.GetDistributionInfo();
    ASSERT_TRUE(osRelease.HasValue());
    EXPECT_EQ(0, ComplianceEngineValidatePayload(handle, nullptr, "test", "/cis/ol/foo/v2.0.1/x/y/z", nullptr));
    ComplianceEngineMmiClose(handle);
}
