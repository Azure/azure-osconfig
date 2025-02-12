// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <version.h>
#include <Mmi.h>
#include <CommonUtils.h>
#include "ComplianceInterface.h"

using namespace std;

class ComplianceTest : public ::testing::Test
{
protected:
    MMI_HANDLE mHandle = nullptr;
    static const unsigned int cMaxPayloadSize = 100;
    void SetUp() override
    {
        ComplianceInitialize(nullptr);
        mHandle = ComplianceMmiOpen("test", cMaxPayloadSize);
    }

    void TearDown() override
    {
        ComplianceMmiClose(mHandle);
        ComplianceShutdown();
    }
};

TEST_F(ComplianceTest, ComplianceMmiOpen_1)
{
    ASSERT_NE(mHandle, nullptr);
}

TEST_F(ComplianceTest, ComplianceMmiGetInfo_InvalidArguments_1)
{
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceMmiGetInfo("test", nullptr, &payloadSizeBytes));
}

TEST_F(ComplianceTest, ComplianceMmiGetInfo_InvalidArguments_2)
{
    char* payload = nullptr;
    ASSERT_NE(MMI_OK, ComplianceMmiGetInfo("test", &payload, nullptr));
}

TEST_F(ComplianceTest, ComplianceMmiGetInfo_1)
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_EQ(MMI_OK, ComplianceMmiGetInfo("test", &payload, &payloadSizeBytes));
    free(payload);
}

TEST_F(ComplianceTest, ComplianceMmiSet_InvalidArguments_1)
{
    auto payload = std::string("eyJhdWRpdCI6eyJhbnlPZiI6W119fQ=="); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceMmiSet(nullptr, "Compliance", "procedureX", payload.c_str(), static_cast<int>(payload.size())));
}

TEST_F(ComplianceTest, ComplianceMmiSet_InvalidArguments_2)
{
    auto payload = std::string("eyJhdWRpdCI6eyJhbnlPZiI6W119fQ=="); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceMmiSet(mHandle, nullptr, "procedureX", payload.c_str(), static_cast<int>(payload.size())));
}

TEST_F(ComplianceTest, ComplianceMmiSet_InvalidArguments_3)
{
    auto payload = std::string("eyJhdWRpdCI6eyJhbnlPZiI6W119fQ=="); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceMmiSet(mHandle, "wrong module name", "procedureX", payload.c_str(), static_cast<int>(payload.size())));
}

TEST_F(ComplianceTest, ComplianceMmiSet_InvalidArguments_4)
{
    auto payload = std::string("eyJhdWRpdCI6eyJhbnlPZiI6W119fQ=="); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceMmiSet(mHandle, "Compliance", nullptr, payload.c_str(), static_cast<int>(payload.size())));
}

TEST_F(ComplianceTest, ComplianceMmiSet_InvalidArguments_5)
{
    auto payload = std::string("eyJhdWRpdCI6eyJhbnlPZiI6W119fQ=="); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceMmiSet(mHandle, "Compliance", "procedureX", nullptr, static_cast<int>(payload.size())));
}

TEST_F(ComplianceTest, ComplianceMmiSet_InvalidArguments_6)
{
    auto payload = std::string("eyJhdWRpdCI6eyJhbnlPZiI6W119fQ=="); // {"audit":{"anyOf":[]}} in base64
    ASSERT_NE(MMI_OK, ComplianceMmiSet(mHandle, "Compliance", "procedureX", payload.c_str(), -1));
}

TEST_F(ComplianceTest, ComplianceMmiSet_SetProcedure_1)
{
    auto payload = std::string("eyJhdWRpdCI6eyJhbnlPZiI6W119fQ=="); // {"audit":{"anyOf":[]}} in base64
    ASSERT_EQ(MMI_OK, ComplianceMmiSet(mHandle, "Compliance", "procedureX", payload.c_str(), static_cast<int>(payload.size())));
}

TEST_F(ComplianceTest, ComplianceMmiGet_InvalidArguments_1)
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceMmiGet(nullptr, "Compliance", "auditX", &payload, &payloadSizeBytes));
}

TEST_F(ComplianceTest, ComplianceMmiGet_InvalidArguments_2)
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceMmiGet(mHandle, nullptr, "auditX", &payload, &payloadSizeBytes));
}

TEST_F(ComplianceTest, ComplianceMmiGet_InvalidArguments_3)
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceMmiGet(mHandle, "Compliance", "auditX", &payload, &payloadSizeBytes));
}

TEST_F(ComplianceTest, ComplianceMmiGet_InvalidArguments_4)
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceMmiGet(mHandle, "Compliance", nullptr, &payload, &payloadSizeBytes));
}

TEST_F(ComplianceTest, ComplianceMmiGet_InvalidArguments_5)
{
    int payloadSizeBytes = 0;
    ASSERT_NE(MMI_OK, ComplianceMmiGet(mHandle, "Compliance", "auditX", nullptr, &payloadSizeBytes));
}

TEST_F(ComplianceTest, ComplianceMmiGet_InvalidArguments_6)
{
    char* payload = nullptr;
    ASSERT_NE(MMI_OK, ComplianceMmiGet(mHandle, "Compliance", "auditX", &payload, nullptr));
}

TEST_F(ComplianceTest, ComplianceMmiGet_1)
{
    auto procedurePayload = std::string("eyJhdWRpdCI6eyJhbnlPZiI6W119fQ=="); // {"audit":{"anyOf":[]}} in base64
    ASSERT_EQ(MMI_OK, ComplianceMmiSet(mHandle, "Compliance", "procedureX", procedurePayload.c_str(), static_cast<int>(procedurePayload.size())));
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_EQ(MMI_OK, ComplianceMmiGet(mHandle, "Compliance", "auditX", &payload, &payloadSizeBytes));
    ASSERT_NE(payload, nullptr);
    EXPECT_NE(0, strncmp(payload, "\"PASS", 5));
    free(payload);
}

TEST_F(ComplianceTest, ComplianceMmiGet_2)
{
    auto procedurePayload = std::string("eyJhdWRpdCI6eyJhbGxPZiI6W119fQ=="); // {"audit":{"allOf":[]}} in base64
    ASSERT_EQ(MMI_OK, ComplianceMmiSet(mHandle, "Compliance", "procedureX", procedurePayload.c_str(), static_cast<int>(procedurePayload.size())));
    char* payload = nullptr;
    int payloadSizeBytes = 0;
    ASSERT_EQ(MMI_OK, ComplianceMmiGet(mHandle, "Compliance", "auditX", &payload, &payloadSizeBytes));
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(0, strncmp(payload, "\"PASS", 5));
    free(payload);
}
