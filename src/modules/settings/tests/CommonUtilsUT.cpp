// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <iostream>
#include <cstdio>
#include <string>
#include <list>
#include <time.h>
#include <gtest/gtest.h>
#include <CommonUtils.h>

using namespace std;

#define STRFTIME_DATE_FORMAT "%Y%m%d"
#define SSCANF_DATE_FORMAT "%4d%2d%2d"
#define DATE_FORMAT_LENGTH 9

static void SignalDoWork(int signal)
{
    UNUSED(signal);
}

class CommonUtilsTest : public ::testing::Test
{
    protected:
        const char* m_path = "~test.test";
        const char* m_data = "`-=~!@#$%^&*()_+,./<>?'[]\{}| qwertyuiopasdfghjklzxcvbnm 1234567890 QWERTYUIOPASDFGHJKLZXCVBNM";
        const char* m_dataWithEol = "`-=~!@#$%^&*()_+,./<>?'[]\{}| qwertyuiopasdfghjklzxcvbnm 1234567890 QWERTYUIOPASDFGHJKLZXCVBNM\n";

        bool CreateTestFile(const char* path, const char* data)
        {
            bool result = false;

            if (nullptr != path)
            {
                ofstream ofs(path);
                if (nullptr != data)
                {
                    ofs << data;
                    result = true;
                }
                ofs.close();
            }

            return result;
        }

        bool Cleanup(const char* path)
        {
            if (nullptr != path)
            {
                if (remove(path))
                {
                    printf("CommonUtilsTest::Cleanup: cannot remove test file %s\n", path);
                    return false;
                }
            }

            return true;
        }
};

TEST_F(CommonUtilsTest, LoadStringFromFileInvalidArgument)
{
    EXPECT_STREQ(nullptr, LoadStringFromFile(nullptr, false));
}

TEST_F(CommonUtilsTest, LoadStringFromFile)
{
    EXPECT_TRUE(CreateTestFile(m_path, m_data));
    EXPECT_STREQ(m_data, LoadStringFromFile(m_path, true));
    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, LoadStringWithEolFromFile)
{
    EXPECT_TRUE(CreateTestFile(m_path, m_dataWithEol));
    EXPECT_STREQ(m_data, LoadStringFromFile(m_path, true));
    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, SavePayloadToFile)
{
    EXPECT_TRUE(SavePayloadToFile(m_path, m_data, strlen(m_data)));
    EXPECT_STREQ(m_data, LoadStringFromFile(m_path, true));
    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, SavePayloadWithEolToFile)
{
    EXPECT_TRUE(SavePayloadToFile(m_path, m_dataWithEol, strlen(m_dataWithEol)));
    EXPECT_STREQ(m_data, LoadStringFromFile(m_path, true));
    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, SavePayloadToFileInvalidArgument)
{
    EXPECT_FALSE(SavePayloadToFile(nullptr, m_data, sizeof(m_data)));
    EXPECT_FALSE(SavePayloadToFile(m_path, nullptr, sizeof(m_data)));
    EXPECT_FALSE(SavePayloadToFile(m_path, m_data, -1));
    EXPECT_FALSE(SavePayloadToFile(m_path, m_data, 0));
}

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResult)
{
    char* textResult = nullptr;

    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo test123", false, true, 0, 0, &textResult, nullptr, nullptr));
    // Echo appends an end of line character:
    EXPECT_STREQ("test123\n", textResult);

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResultAndTimeout)
{
    char* textResult = nullptr;

    signal(SIGUSR1, SignalDoWork);

    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo test123", false, true, 0, 10, &textResult, nullptr, nullptr));
    // Echo appends an end of line character:
    EXPECT_STREQ("test123\n", textResult);

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResultWithEolMapping)
{
    char* textResult = nullptr;

    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo test123", true, true, 0, 0, &textResult, nullptr, nullptr));
    // Echo appends an end of line character that's replaced with space:
    EXPECT_STREQ("test123 ", textResult);

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResultAndTruncation)
{
    char* textResult = nullptr;

    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo test123", false, true, 5, 0, &textResult, nullptr, nullptr));
    // Only first 5 characters including a null terminator are returned:
    EXPECT_STREQ("test", textResult);

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResultAndTruncationOfOne)
{
    char* textResult = nullptr;

    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo test123", false, true, 1, 0, &textResult, nullptr, nullptr));
    // Only the null terminator is returned, meaning empty string:
    EXPECT_STREQ("", textResult);

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResultAndTruncationOfEol)
{
    char* textResult = nullptr;

    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo test123", false, true, 8, 0, &textResult, nullptr, nullptr));
    // The EOL appended by echo is truncated from the result (replaced with the null terminator in this case):
    EXPECT_STREQ("test123", textResult);

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, ExecuteCommandWithSpecialCharactersInTextResult)
{
    char* textResult = nullptr;

    char specialCharacters[34] = {0};
    specialCharacters[0] = '\\';
    for (int i = 1; i < 32; i++)
    {
        specialCharacters[i] = (char)i;
    }
    specialCharacters[32] = 0x7f;
    specialCharacters[33] = 0;

    char command[50] = {0};
    sprintf(command, "echo \"%s\"", &specialCharacters[0]);

    // All special characters must be replaced with spaces:
    int expectedResultSize = sizeof(specialCharacters);
    char expectedResult[expectedResultSize + 1] = {0};
    for (int i = 0; i < expectedResultSize; i++)
    {
        expectedResult[i] = ' ';
    }

    EXPECT_EQ(0, ExecuteCommand(nullptr, command, true, true, strlen(command), 0, &textResult, nullptr, nullptr));
    EXPECT_STREQ(expectedResult, textResult);

    if (nullptr != textResult)
    {
        free(textResult);
        textResult = nullptr;
    }
}

TEST_F(CommonUtilsTest, ExecuteCommandWithoutTextResult)
{
    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo test456", false, true, 0, 0, nullptr, nullptr, nullptr));
    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo test456", false, false, 0, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommonUtilsTest, ExecuteCommandWithRedirectorCharacter)
{
    char* textResult = nullptr;
    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo test789 > testResultFile", false, true, 0, 0, &textResult, nullptr, nullptr));
    EXPECT_EQ(nullptr, textResult);
}

TEST_F(CommonUtilsTest, ExecuteCommandWithNullArgument)
{
    char* textResult = nullptr;
    EXPECT_EQ(-1, ExecuteCommand(nullptr, nullptr, false, true, 0, 0, &textResult, nullptr, nullptr));
    EXPECT_EQ(nullptr, textResult);

    EXPECT_EQ(-1, ExecuteCommand(nullptr, nullptr, false, false, 0, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommonUtilsTest, ExecuteCommandThatTimesOut)
{
    char* textResult = nullptr;

    signal(SIGUSR1, SignalDoWork);

    EXPECT_EQ(ETIME, ExecuteCommand(nullptr, "sleep 10", false, true, 0, 1, &textResult, nullptr, nullptr));

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

static int numberOfTimes = 0;

static int TestCommandCallback(void* context)
{
    printf("TestCommandCallback: context %p\n", context);

    numberOfTimes += 1;
    return (3 == numberOfTimes) ? 1 : 0;
}

TEST_F(CommonUtilsTest, CancelCommand)
{
    char* textResult = nullptr;

    signal(SIGUSR1, SignalDoWork);

    ::numberOfTimes = 0;

    EXPECT_EQ(ECANCELED, ExecuteCommand(nullptr, "sleep 20", false, true, 0, 120, &textResult, TestCommandCallback, nullptr));

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

class CallbackContext
{
public:
    CallbackContext()
    {
        printf("CallbackContext: new instance %p\n", this);
    }

    ~CallbackContext()
    {
        printf("CallbackContext: destroy instance %p\n", this);
    }

    static int TestCommandCallback(void* context)
    {
        EXPECT_NE(nullptr, context);
        return ::TestCommandCallback(context);
    }
};

TEST_F(CommonUtilsTest, CancelCommandWithContext)
{
    char* textResult = nullptr;

    signal(SIGUSR1, SignalDoWork);

    ::numberOfTimes = 0;

    CallbackContext context;

    EXPECT_EQ(ECANCELED, ExecuteCommand((void*)(&context), "sleep 30", false, true, 0, 120, &textResult, &(CallbackContext::TestCommandCallback), nullptr));

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResultWithAllCharacters)
{
    char* textResult = nullptr;

    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo 'abc\"123'", true, false, 0, 0, &textResult, nullptr, nullptr));
    EXPECT_STREQ("abc\"123 ", textResult);

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResultWithMappedJsonCharacters)
{
    char* textResult = nullptr;

    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo 'abc\"123'", true, true, 0, 0, &textResult, nullptr, nullptr));
    EXPECT_STREQ("abc 123 ", textResult);

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, ExecuteLongCommand)
{
    char* textResult = nullptr;

    size_t commandLength = 4000;
    char* command = (char*)malloc(commandLength);
    EXPECT_NE(nullptr, command);

    size_t expectedResultLength = commandLength + 1;
    char* expectedResult = (char*)malloc(expectedResultLength);
    EXPECT_NE(nullptr, expectedResult);

    if ((nullptr != command) && (nullptr != expectedResult))
    {
        snprintf(command, commandLength, "echo ");
        size_t echoLength = strlen(command);
        EXPECT_EQ(5, echoLength);

        for (size_t i = echoLength; i < (commandLength - 1); i++)
        {
            command[i] = (i % 2) ? '0' : '1';
        }
        command[commandLength - 1] = 0;

        snprintf(expectedResult, expectedResultLength - echoLength, "%s ", &(command[echoLength]));

        EXPECT_EQ(0, ExecuteCommand(nullptr, command, true, true, 0, 0, &textResult, nullptr, nullptr));
        EXPECT_STREQ(expectedResult, textResult);
    }

    if (nullptr != command)
    {
        free(command);
    }

    if (nullptr != expectedResult)
    {
        free(expectedResult);
    }

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, ExecuteTooLongCommand)
{
    char* textResult = nullptr;

    size_t commandLength = (size_t)(sysconf(_SC_ARG_MAX) + 1);
    char* command = (char*)malloc(commandLength);
    EXPECT_NE(nullptr, command);

    if (nullptr != command)
    {
        snprintf(command, commandLength, "echo ");
        size_t echoLength = strlen(command);
        EXPECT_EQ(5, echoLength);

        for (size_t i = echoLength; i < (commandLength - 1); i++)
        {
            command[i] = (i % 2) ? '0' : '1';
        }
        command[commandLength - 1] = 0;

        EXPECT_EQ(E2BIG, ExecuteCommand(nullptr, command, true, true, 0, 0, &textResult, nullptr, nullptr));
        EXPECT_EQ(nullptr, textResult);

        free(command);
    }

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

TEST_F(CommonUtilsTest, HashString)
{
    size_t dataHash = HashString(m_data);
    EXPECT_NE(0, dataHash);

    size_t dataWithEolHash = HashString(m_dataWithEol);
    EXPECT_NE(0, dataWithEolHash);
    EXPECT_NE(dataHash, dataWithEolHash);

    size_t sameDataHash = HashString(m_data);
    EXPECT_NE(0, sameDataHash);
    EXPECT_EQ(dataHash, sameDataHash);
}

TEST_F(CommonUtilsTest, RestrictFileAccess)
{
    EXPECT_TRUE(CreateTestFile(m_path, m_data));
    EXPECT_EQ(0, RestrictFileAccessToCurrentAccountOnly(m_path));
    EXPECT_NE(0, RestrictFileAccessToCurrentAccountOnly(nullptr));
    EXPECT_NE(0, RestrictFileAccessToCurrentAccountOnly(""));
    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, FileExists)
{
    EXPECT_TRUE(CreateTestFile(m_path, m_data));
    EXPECT_TRUE(FileExists(m_path));
    EXPECT_TRUE(Cleanup(m_path));
    EXPECT_FALSE(FileExists(m_path));
    EXPECT_FALSE(FileExists("This file does not exist"));
}

TEST_F(CommonUtilsTest, ValidClientName)
{
    std::list<std::string> validClientNames = {
        "Azure OSConfig 5;0.0.0.20210927",
        "Azure OSConfig 5;1.1.1.20210927",
        "Azure OSConfig 5;11.11.11.20210927",
        "Azure OSConfig 6;0.0.0.20210927",
        "Azure OSConfig 5;0.0.0.20210927abc123"
    };

    for (const auto& validClientName : validClientNames)
    {
        ASSERT_TRUE(IsValidClientName(validClientName.c_str()));
    }

    time_t t = time(0);
    char dateNow[DATE_FORMAT_LENGTH] = {0};
    strftime(dateNow, DATE_FORMAT_LENGTH, STRFTIME_DATE_FORMAT, localtime(&t));

    std::string clientNameWithCurrentDate = "Azure OSConfig 5;0.0.0." + std::string(dateNow);
    ASSERT_TRUE(IsValidClientName(clientNameWithCurrentDate.c_str()));
}

TEST_F(CommonUtilsTest, InvalidClientName)
{
    std::list<std::string> invalidClientNames = {
        "AzureOSConfig 5;0.0.0.20210927",
        "Azure OSConfig5;0.0.0.20210927",
        "azure osconfig 5;0.0.0.20210927",
        "AzureOSConfig 5;0.0.0.20210927",
        "Azure  OSConfig5;0.0.0.20210927",
        "Azure OSConfig  5;0.0.0.20210927",
        "Azure OSConfig 5:0.0.0.20210927",
        "Azure OSConfig 5;0,0,0,20210927",
        "Azure OSConfig 5;0.0.0.2021927",
        "Azure OSConfig -5;-1.-1.-1.20210927",
        "Azure OSConfig 1;0.0.0.20210927",
        "Azure OSConfig 2;0.0.0.20210927",
        "Azure OSConfig 3;0.0.0.20210927",
        "Azure OSConfig 4;0.0.0.20210927",
        "Azure OSConfig 5;0.0.0.20210827",
        "Azure OSConfig 5;0.0.0.20210926",
        "Azure OSConfig 5;0.0.0.20200927"
        "Azure OSConfig 5;0.0.0.20200927"
    };

    for (const auto& invalidClientName : invalidClientNames)
    {
        ASSERT_FALSE(IsValidClientName(invalidClientName.c_str()));
    }

    time_t t = time(0);
    char dateNow[DATE_FORMAT_LENGTH] = {0};
    strftime(dateNow, DATE_FORMAT_LENGTH, STRFTIME_DATE_FORMAT, localtime(&t));

    int yearNow, monthNow, dayNow;
    sscanf(dateNow, SSCANF_DATE_FORMAT, &yearNow, &monthNow, &dayNow);

    std::string clientNameWithYearAfterCurrentDate = "Azure OSConfig 5;0.0.0." + std::to_string(yearNow + 1) + std::to_string(monthNow) + std::to_string(dayNow);
    std::string clientNameWithMonthAfterCurrentDate = "Azure OSConfig 5;0.0.0." + std::to_string(yearNow) + std::to_string(monthNow + 1) + std::to_string(dayNow);
    std::string clientNameWithDayAfterCurrentDate = "Azure OSConfig 5;0.0.0." + std::to_string(yearNow) + std::to_string(monthNow) + std::to_string(dayNow + 1);

    ASSERT_FALSE(IsValidClientName(clientNameWithMonthAfterCurrentDate.c_str()));
    ASSERT_FALSE(IsValidClientName(clientNameWithDayAfterCurrentDate.c_str()));
    ASSERT_FALSE(IsValidClientName(clientNameWithYearAfterCurrentDate.c_str()));
}
