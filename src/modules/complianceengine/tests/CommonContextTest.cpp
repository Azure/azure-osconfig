// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonContext.h"

#include <fstream>
#include <gtest/gtest.h>

class CommonContextTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize resources if needed
    }

    void TearDown() override
    {
        // Clean up resources if needed
    }
};

TEST_F(CommonContextTest, ExecuteCommand_Success)
{
    ComplianceEngine::CommonContext ctx(nullptr);
    auto result = ctx.ExecuteCommand("echo test");
    EXPECT_TRUE(result);
    EXPECT_NE(result.Value().find("test"), std::string::npos);
}

TEST_F(CommonContextTest, ExecuteCommand_Failure)
{
    ComplianceEngine::CommonContext ctx(nullptr);
    auto result = ctx.ExecuteCommand("someinvalidcommand");
    EXPECT_FALSE(result);
    auto err = result.Error();
    std::cout << "Error: code: " << err.code << " message: " << err.message << std::endl;
}

TEST_F(CommonContextTest, GetFileContents_NotFound)
{
    ComplianceEngine::CommonContext ctx(nullptr);
    auto result = ctx.GetFileContents("/non_existent_file");
    EXPECT_FALSE(result);
}

TEST_F(CommonContextTest, GetFileContents_ExistingFile)
{
    ComplianceEngine::CommonContext ctx(nullptr);
    // Create a dummy file with known content
    std::string filePath = "/tmp/test_common_context.txt";
    std::string expectedContent = "Hello from dummy file";

    {
        std::ofstream tempFile(filePath);
        ASSERT_TRUE(tempFile.is_open());
        tempFile << expectedContent;
    }

    auto result = ctx.GetFileContents(filePath);
    EXPECT_TRUE(result);
    EXPECT_EQ(result.Value(), expectedContent);

    remove(filePath.c_str());
}
