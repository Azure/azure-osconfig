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
    compliance::CommonContext ctx(nullptr);
    auto result = ctx.ExecuteCommand("echo test");
    EXPECT_TRUE(result);
    EXPECT_NE(result.Value().find("test"), std::string::npos);
}

TEST_F(CommonContextTest, ExecuteCommand_Failure)
{
    compliance::CommonContext ctx(nullptr);
    auto result = ctx.ExecuteCommand("someinvalidcommand");
    EXPECT_FALSE(result);
    auto err = result.Error();
    std::cout << "Error: code: " << err.code << " message: " << err.message << std::endl;
}

TEST_F(CommonContextTest, GetFileContents_NotFound)
{
    compliance::CommonContext ctx(nullptr);
    auto result = ctx.GetFileContents("/non_existent_file");
    EXPECT_FALSE(result);
}

TEST_F(CommonContextTest, GetFileContents_ExistingFile)
{
    compliance::CommonContext ctx(nullptr);
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

TEST_F(CommonContextTest, LogStream_Test)
{
    compliance::CommonContext ctx(nullptr);
    ctx.GetLogstream() << "Log message";
    auto logContent = ctx.ConsumeLogstream();
    EXPECT_EQ(logContent, "Log message");
}

TEST_F(CommonContextTest, LogStream_MultipleWrites)
{
    compliance::CommonContext ctx(nullptr);

    // First write and read
    ctx.GetLogstream() << "First message ";
    ctx.GetLogstream() << "Second message";
    auto logContent = ctx.ConsumeLogstream();
    EXPECT_EQ(logContent, "First message Second message");

    // No content left
    logContent = ctx.ConsumeLogstream();
    EXPECT_TRUE(logContent.empty());

    // Write again
    ctx.GetLogstream() << "Third message";
    logContent = ctx.ConsumeLogstream();
    EXPECT_EQ(logContent, "Third message");

    // No content left
    logContent = ctx.ConsumeLogstream();
    EXPECT_TRUE(logContent.empty());
}
