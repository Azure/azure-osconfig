// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <iostream>
#include <cstdio>
#include <string>
#include <list>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <CommonUtils.h>

using namespace std;

#define STRFTIME_DATE_FORMAT "%Y%m%d"
#define SSCANF_DATE_FORMAT "%4d%2d%2d"
#define DATE_FORMAT_LENGTH 9

class CommonUtilsTest : public ::testing::Test
{
    protected:
        const char* m_path = "~test.test";
        const char* m_data = "`-=~!@#$%^&*()_+,./<>?'[]\\{}| qwertyuiopasdfghjklzxcvbnm 1234567890 QWERTYUIOPASDFGHJKLZXCVBNM";
        const char* m_dataWithEol = "`-=~!@#$%^&*()_+,./<>?'[]\\{}| qwertyuiopasdfghjklzxcvbnm 1234567890 QWERTYUIOPASDFGHJKLZXCVBNM\n";

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
    EXPECT_STREQ(nullptr, LoadStringFromFile(nullptr, false, nullptr));
}

TEST_F(CommonUtilsTest, LoadStringFromFile)
{
    EXPECT_TRUE(CreateTestFile(m_path, m_data));
    EXPECT_STREQ(m_data, LoadStringFromFile(m_path, true, nullptr));
    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, LoadStringWithEolFromFile)
{
    EXPECT_TRUE(CreateTestFile(m_path, m_dataWithEol));
    EXPECT_STREQ(m_data, LoadStringFromFile(m_path, true, nullptr));
    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, SavePayloadToFile)
{
    EXPECT_TRUE(SavePayloadToFile(m_path, m_data, strlen(m_data), nullptr));
    EXPECT_STREQ(m_data, LoadStringFromFile(m_path, true, nullptr));
    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, SavePayloadWithEolToFile)
{
    EXPECT_TRUE(SavePayloadToFile(m_path, m_dataWithEol, strlen(m_dataWithEol), nullptr));
    EXPECT_STREQ(m_data, LoadStringFromFile(m_path, true, nullptr));
    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, SavePayloadToFileInvalidArgument)
{
    EXPECT_FALSE(SavePayloadToFile(nullptr, m_data, sizeof(m_data), nullptr));
    EXPECT_FALSE(SavePayloadToFile(m_path, nullptr, sizeof(m_data), nullptr));
    EXPECT_FALSE(SavePayloadToFile(m_path, m_data, -1, nullptr));
    EXPECT_FALSE(SavePayloadToFile(m_path, m_data, 0, nullptr));
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

TEST_F(CommonUtilsTest, ExecuteCommandWithStdErrOutput)
{
    char* textResult = nullptr;

    EXPECT_EQ(127, ExecuteCommand(nullptr, "hh", false, true, 100, 0, &textResult, nullptr, nullptr));
    EXPECT_NE(nullptr, strstr(textResult, "sh: 1: hh: not found\n"));

    if (nullptr != textResult)
    {
        free(textResult);
    }

    EXPECT_EQ(127, ExecuteCommand(nullptr, "blah", true, true, 100, 0, &textResult, nullptr, nullptr));
    EXPECT_NE(nullptr, strstr(textResult, "sh: 1: blah: not found "));

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

void* TestTimeoutCommand(void*)
{
    char* textResult = nullptr;

    EXPECT_EQ(ETIME, ExecuteCommand(nullptr, "sleep 10", false, true, 0, 1, &textResult, nullptr, nullptr));

    if (nullptr != textResult)
    {
        free(textResult);
    }

    return nullptr;
}

TEST_F(CommonUtilsTest, ExecuteCommandThatTimesOutOnWorkerThread)
{
    pthread_t tid = 0;
    
    EXPECT_EQ(0, pthread_create(&tid, NULL, &TestTimeoutCommand, NULL));
    
    // Wait for the worker thread to finish so test errors will be captured for this test case
    EXPECT_EQ(0, pthread_join(tid, NULL));
}

TEST_F(CommonUtilsTest, ExecuteCommandThatTimesOut)
{
    char* textResult = nullptr;

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
        return ::TestCommandCallback(context);
    }
};

void* TestCancelCommand(void*)
{
    char* textResult = nullptr;

    EXPECT_EQ(ECANCELED, ExecuteCommand(nullptr, "sleep 20", false, true, 0, 120, &textResult, &(CallbackContext::TestCommandCallback), nullptr));

    if (nullptr != textResult)
    {
        free(textResult);
    }

    return nullptr;
}

TEST_F(CommonUtilsTest, CancelCommandOnWorkerThread)
{
    pthread_t tid = 0;

    ::numberOfTimes = 0;

    EXPECT_EQ(0, pthread_create(&tid, NULL, &TestCancelCommand, NULL));
    
    // Wait for the worker thread to finish so test errors will be captured for this test case
    EXPECT_EQ(0, pthread_join(tid, NULL));
}

TEST_F(CommonUtilsTest, CancelCommand)
{
    char* textResult = nullptr;

    EXPECT_EQ(ECANCELED, ExecuteCommand(nullptr, "sleep 20", false, true, 0, 120, &textResult, &(CallbackContext::TestCommandCallback), nullptr));

    if (nullptr != textResult)
    {
        free(textResult);
    }
}

void* TestCancelCommandWithContext(void*)
{
    CallbackContext context;

    char* textResult = nullptr;

    EXPECT_EQ(ECANCELED, ExecuteCommand((void*)(&context), "sleep 30", false, true, 0, 120, &textResult, &(CallbackContext::TestCommandCallback), nullptr));

    if (nullptr != textResult)
    {
        free(textResult);
    }

    return nullptr;
}

TEST_F(CommonUtilsTest, CancelCommandWithContextOnWorkerThread)
{
    pthread_t tid = 0;

    ::numberOfTimes = 0;

    EXPECT_EQ(0, pthread_create(&tid, NULL, &TestCancelCommandWithContext, NULL));
    
    // Wait for the worker thread to finish so test errors will be captured for this test case
    EXPECT_EQ(0, pthread_join(tid, NULL));
}

TEST_F(CommonUtilsTest, CancelCommandWithContext)
{
    CallbackContext context;

    char* textResult = nullptr;

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
        "Azure OSConfig 10;0.0.0.20210927abc123"
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

TEST_F(CommonUtilsTest, ValidateMimObjectPayload)
{
    // Valid payloads
    const char stringPayload[] = R"""("string")""";
    const char integerPayload[] = R"""(1)""";
    const char booleanPayload[] = R"""(true)""";
    const char objectPayload[] = R"""({
            "string": "value",
            "integer": 1,
            "boolean": true,
            "integerEnum": 1,
            "stringArray": ["value1", "value2"],
            "integerArray": [1, 2],
            "stringMap": {"key1": "value1", "key2": "value2"},
            "integerMap": {"key1": 1, "key2": 2}
        })""";
    const char arrayObjectPayload[] = R"""([
        {
            "string": "value",
            "integer": 1,
            "boolean": true,
            "integerEnum": 1,
            "stringArray": ["value1", "value2"],
            "integerArray": [1, 2],
            "stringMap": {"key1": "value1", "key2": "value2"},
            "integerMap": {"key1": 1, "key2": 2}
        },
        {
            "string": "value",
            "integer": 1,
            "boolean": true,
            "integerEnum": 1,
            "stringArray": ["value1", "value2"],
            "integerArray": [1, 2],
            "stringMap": {"key1": "value1", "key2": "value2"},
            "integerMap": {"key1": 1, "key2": 2}
        }
    ])""";
    const char stringArrayPayload[] = R"""(["value1", "value2"])""";
    const char integerArrayPayload[] = R"""([1, 2])""";
    const char stringMap[] = R"""({"key1": "value1", "key2" : "value2", "key3": null})""";
    const char integerMap[] = R"""({"key1": 1, "key2" : 2, "key3": null})""";

    ASSERT_TRUE(IsValidMimObjectPayload(stringPayload, sizeof(stringPayload), nullptr));
    ASSERT_TRUE(IsValidMimObjectPayload(integerPayload, sizeof(integerPayload), nullptr));
    ASSERT_TRUE(IsValidMimObjectPayload(booleanPayload, sizeof(booleanPayload), nullptr));
    ASSERT_TRUE(IsValidMimObjectPayload(objectPayload, sizeof(objectPayload), nullptr));
    ASSERT_TRUE(IsValidMimObjectPayload(arrayObjectPayload, sizeof(arrayObjectPayload), nullptr));
    ASSERT_TRUE(IsValidMimObjectPayload(stringArrayPayload, sizeof(stringArrayPayload), nullptr));
    ASSERT_TRUE(IsValidMimObjectPayload(integerArrayPayload, sizeof(integerArrayPayload), nullptr));
    ASSERT_TRUE(IsValidMimObjectPayload(stringMap, sizeof(stringMap), nullptr));
    ASSERT_TRUE(IsValidMimObjectPayload(integerMap, sizeof(integerMap), nullptr));

    // Invalid payloads
    const char invalidJson[] = R"""(invalid)""";
    const char invalidstringArrayPayload[] = R"""({"stringArray": ["value1", 1]})""";
    const char invalidIntegerArrayPayload[] = R"""({"integerArray": [1, "value1"]})""";
    const char invalidStringMapPayload[] = R"""({"stringMap": {"key1": "value1", "key2": 1}})""";
    const char invalidIntegerMapPayload[] = R"""({"integerMap": {"key1": 1, "key2": "value1"}})""";

    ASSERT_FALSE(IsValidMimObjectPayload(nullptr, 0, nullptr));
    ASSERT_FALSE(IsValidMimObjectPayload(invalidJson, sizeof(invalidJson), nullptr));
    ASSERT_FALSE(IsValidMimObjectPayload(invalidstringArrayPayload, sizeof(invalidstringArrayPayload), nullptr));
    ASSERT_FALSE(IsValidMimObjectPayload(invalidIntegerArrayPayload, sizeof(invalidIntegerArrayPayload), nullptr));
    ASSERT_FALSE(IsValidMimObjectPayload(invalidStringMapPayload, sizeof(invalidStringMapPayload), nullptr));
    ASSERT_FALSE(IsValidMimObjectPayload(invalidIntegerMapPayload, sizeof(invalidIntegerMapPayload), nullptr));
}

struct HttpProxyOptions
{
    const char* data;
    const char* hostAddress;
    int port;
    const char* username;
    const char* password;
};

TEST_F(CommonUtilsTest, ValidHttpProxyData)
{
    char* hostAddress = nullptr;
    int port = 0;
    char* username = nullptr;
    char* password = nullptr;

    HttpProxyOptions validOptions[] = {
        { "http://0123456789!abcdefghIjklmn\\opqrstuvwxyz$_-.ABCD\\@mail.foo:p\\@ssw\\@rd@EFGHIJKLMNOPQRSTUVWXYZ:100", "EFGHIJKLMNOPQRSTUVWXYZ", 100, "0123456789!abcdefghIjklmn\\opqrstuvwxyz$_-.ABCD@mail.foo", "p@ssw@rd" },
        { "HTTP://0123456789\\opqrstuvwxyz$_-.ABCD\\@!abcdefghIjk.lmn:p\\@ssw\\@rd@EFGHIJKLMNOPQRSTUVWXYZ:8080", "EFGHIJKLMNOPQRSTUVWXYZ", 8080, "0123456789\\opqrstuvwxyz$_-.ABCD@!abcdefghIjk.lmn", "p@ssw@rd" },
        { "http://0123456789!abcdefghIjklmnopqrstuvwxyz$_-.ABCDEFGHIJKLMNOPQRSTUVWXYZEFGHIJKLMNOPQRSTUVWXYZ:101", "0123456789!abcdefghIjklmnopqrstuvwxyz$_-.ABCDEFGHIJKLMNOPQRSTUVWXYZEFGHIJKLMNOPQRSTUVWXYZ", 101, nullptr, nullptr },
        { "http://fooname:foo$pass!word@wwww.foo.org:7070", "wwww.foo.org", 7070, "fooname", "foo$pass!word" },
        { "http://fooname:foo$pass!word@wwww.foo.org:8070//", "wwww.foo.org", 8070, "fooname", "foo$pass!word" },
        { "http://a\\b:c@d:1", "d", 1, "a\\b", "c" },
        { "http://a\\@b:c@d:1", "d", 1, "a@b", "c" },
        { "http://a:b@c:1", "c", 1, "a", "b" },
        { "http://a:1", "a", 1, nullptr, nullptr },
        { "http://1:a", "1", 0, nullptr, nullptr }
    };

    int validOptionsSize = ARRAY_SIZE(validOptions);

    for (int i = 0; i < validOptionsSize; i++)
    {
        EXPECT_TRUE(ParseHttpProxyData(validOptions[i].data, &hostAddress, &port, &username, &password, nullptr));
        EXPECT_STREQ(hostAddress, validOptions[i].hostAddress);
        EXPECT_EQ(port, validOptions[i].port);
        EXPECT_STREQ(username, validOptions[i].username);
        EXPECT_STREQ(password, validOptions[i].password);

        FREE_MEMORY(hostAddress);
        FREE_MEMORY(username);
        FREE_MEMORY(password);
    }
}

TEST_F(CommonUtilsTest, InvalidHttpProxyData)
{
    char* hostAddress = nullptr;
    int port = 0;
    char* username = nullptr;
    char* password = nullptr;

    const char* badOptions[] = {
        "some random text",
        "http://blah",
        "http://blah oh",
        "123",
        "http://abc",
        "wwww.foo.org:1010",
        "11.22.22.44:2020",
        "//wwww.foo.org:3030",
        "https://wwww.foo.org:40",
        "HTTPS://wwww.foo.org:5050",
        "http://foo`name:foopassword@wwww.foo.org:6060",
        "http://fooname:foo=password@wwww.foo.org:6060",
        "http://foo~name:foopassword@wwww.foo.org:6060",
        "http://foo#name:foopassword@wwww.foo.org:6060",
        "http://foo%name:foopassword@wwww.foo.org:6060",
        "http://fooname:foo^password@wwww.foo.org:6060",
        "http://fooname:foo&password@wwww.foo.org:6060",
        "http://foo*name:foopassword@wwww.foo.org:6060",
        "http://fooname:foo(password@wwww.foo.org:6060",
        "http://foo)name:foopassword@wwww.foo.org:6060",
        "http://fooname:foo+password@wwww.foo.org:6060",
        "http://foo,name:foopassword@wwww.foo.org:6060",
        "http://fooname:foo<password@wwww.foo.org:6060",
        "http://foo>name:foopassword@wwww.foo.org:6060",
        "http://fooname:foo?password@wwww.foo.org:6060",
        "http://foo'name:foopassword@wwww.foo.org:6060",
        "http://fooname:foo[password@wwww.foo.org:6060",
        "http://foo]name:foopassword@wwww.foo.org:6060",
        "http://fooname:foo{password@wwww.foo.org:6060",
        "http://foo}name:foopassword@wwww.foo.org:6060",
        "http://fooname:foo password@wwww.foo.org:6060",
        "http://foo|name:foopassword@wwww.foo.org:6060",
        "http://fooname:foopassword@@wwww.foo.org:7070",
        "http://foo:name:foo:password@@wwww.foo.org:8080"
        "http://fooname:foopassword@wwww.foo.org:***",
        "http://fooname:foo\"password@wwww.foo.org:9090"
    };

    int badOptionsSize = ARRAY_SIZE(badOptions);

    for (int i = 0; i < badOptionsSize; i++)
    {
        EXPECT_FALSE(ParseHttpProxyData(badOptions[i], &hostAddress, &port, &username, &password, nullptr));

        FREE_MEMORY(hostAddress);
        FREE_MEMORY(username);
        FREE_MEMORY(password);
    }
}

TEST_F(CommonUtilsTest, InvalidArgumentsHttpProxyDataParsing)
{
    char* hostAddress = nullptr;
    int port = 0;

    EXPECT_FALSE(ParseHttpProxyData(nullptr, &hostAddress, &port, nullptr, nullptr, nullptr));
    EXPECT_FALSE(ParseHttpProxyData("http://a:1", nullptr, &port, nullptr, nullptr, nullptr));
    EXPECT_FALSE(ParseHttpProxyData("http://a:1", &hostAddress, nullptr, nullptr, nullptr, nullptr));
}

TEST_F(CommonUtilsTest, OsProperties)
{
    char* osName = NULL;
    char* osVersion = NULL;
    char* cpuType = NULL;
    char* cpuVendor = NULL;
    char* cpuModel = NULL;
    long totalMemory = 0;
    long freeMemory = 0;
    char* kernelName = NULL;
    char* kernelVersion = NULL;
    char* kernelRelease = NULL;

    EXPECT_NE(nullptr, osName = GetOsName(nullptr));
    EXPECT_NE(nullptr, osVersion = GetOsVersion(nullptr));
    EXPECT_NE(nullptr, cpuType = GetCpuType(nullptr));
    EXPECT_NE(nullptr, cpuVendor = GetCpuType(nullptr));
    EXPECT_NE(nullptr, cpuModel = GetCpuType(nullptr));
    EXPECT_NE(0, totalMemory = GetTotalMemory(nullptr));
    EXPECT_NE(0, freeMemory = GetTotalMemory(nullptr));
    EXPECT_NE(nullptr, kernelName = GetOsKernelName(nullptr));
    EXPECT_NE(nullptr, kernelVersion = GetOsKernelVersion(nullptr));
    EXPECT_NE(nullptr, kernelRelease = GetOsKernelRelease(nullptr));

    FREE_MEMORY(osName);
    FREE_MEMORY(osVersion);
    FREE_MEMORY(cpuType);
    FREE_MEMORY(cpuVendor);
    FREE_MEMORY(cpuModel);
    FREE_MEMORY(kernelName);
    FREE_MEMORY(kernelVersion);
    FREE_MEMORY(kernelRelease);
}

char* AllocateAndCopyTestString(const char* source)
{
    char* output = nullptr;
    int length = 0;

    EXPECT_NE(nullptr, source);
    EXPECT_NE(0, length = (int)strlen(source));
    EXPECT_NE(nullptr, output = (char*)malloc(length + 1));

    if (nullptr != output)
    {
        memcpy(output, source, length);
        output[length] = 0;
    }

    return output;
}

TEST_F(CommonUtilsTest, RemovePrefixBlanks)
{
    const char* targets[] = {
        "Test",
        " Test",
        "  Test",
        "   Test",
        "    Test",
        "     Test",
        "      Test",
        "       Test",
        "        Test",
        "                            Test"
    };

    int numTargets = ARRAY_SIZE(targets);
    char expected[] = "Test";
    char* testString = nullptr;

    for (int i = 0; i < numTargets; i++)
    {
        EXPECT_NE(nullptr, testString = AllocateAndCopyTestString(targets[i]));
        RemovePrefixBlanks(testString);
        EXPECT_STREQ(testString, expected);
        FREE_MEMORY(testString);
    }
}

TEST_F(CommonUtilsTest, RemoveTrailingBlanks)
{
    const char* targets[] = {
        "Test",
        "Test ",
        "Test  ",
        "Test   ",
        "Test    ",
        "Test      ",
        "Test       ",
        "Test        ",
        "Test           ",
        "Test                       "
    };

    int numTargets = ARRAY_SIZE(targets);
    char expected[] = "Test";
    char* testString = nullptr;

    for (int i = 0; i < numTargets; i++)
    {
        EXPECT_NE(nullptr, testString = AllocateAndCopyTestString(targets[i]));
        RemoveTrailingBlanks(testString);
        EXPECT_STREQ(testString, expected);
        FREE_MEMORY(testString);
    }
}

struct MarkedTestTargets
{
    const char* target;
    char marker;
};

TEST_F(CommonUtilsTest, RemovePrefixUpTo)
{
    MarkedTestTargets targets[] = {
        { "Test", '&' },
        { "123=Test", '=' },
        { "jshsaHGFsajhgksajge27u313987yhjsA,NSQ.I3U21P903PUDSJQ#Test", '#' },
        { "1$Test", '$' },
        { "Test$Test=Test", '=' },
        { "@Test", '@' },
        { "123456789Test", '9' },
        { "!@!#@$#$^%^^%&^*&()(_)(+-Test", '-' }
    };

    int numTargets = ARRAY_SIZE(targets);
    char expected[] = "Test";
    char* testString = nullptr;

    for (int i = 0; i < numTargets; i++)
    {
        EXPECT_NE(nullptr, testString = AllocateAndCopyTestString(targets[i].target));
        RemovePrefixUpTo(testString, targets[i].marker);
        EXPECT_STREQ(testString, expected);
        FREE_MEMORY(testString);
    }
}

TEST_F(CommonUtilsTest, TruncateAtFirst)
{
    MarkedTestTargets targets[] = {
        { "Test", '&' },
        { "Test=123", '=' },
        { "Test#jshsaHGFsajhgksajge27u313987yhjsA,NSQ.I3U21P903PUDSJQ", '#' },
        { "Test$1$Test", '$' },
        { "Test=$Test=Test", '=' },
        { "Test@", '@' },
        { "Test123456789Test", '1' },
        { "Test!@!#@$#$^%^^%&^*&()(_)(+-Test", '!' }
    };

    int numTargets = ARRAY_SIZE(targets);
    char expected[] = "Test";
    char* testString = nullptr;

    for (int i = 0; i < numTargets; i++)
    {
        EXPECT_NE(nullptr, testString = AllocateAndCopyTestString(targets[i].target));
        TruncateAtFirst(testString, targets[i].marker);
        EXPECT_STREQ(testString, expected);
        FREE_MEMORY(testString);
    }
}

struct UrlEncoding
{
    const char* decoded;
    const char* encoded;
};

TEST_F(CommonUtilsTest, UrlEncodeDecode)
{
    UrlEncoding testUrls[] = {
        { "+", "%2B" },
        { " ", "%20" },
        { "\n", "%0A" },
        { "abcABC123", "abcABC123" },
        { "~abcd~EFGH-123_456", "~abcd~EFGH-123_456" },
        { "name=value", "name%3Dvalue" },
        { "\"name\"=\"value\"", "%22name%22%3D%22value%22" },
        { "(\"name1\"=\"value1\"&\"name2\"=\"value2\")", "%28%22name1%22%3D%22value1%22%26%22name2%22%3D%22value2%22%29" },
        { "Azure OSConfig 5;1.0.1.20220308 (\"os_name\"=\"Ubuntu\"&os_version\"=\"20.04.4\"&\"cpu_architecture\"=\"x86_64\"&"
        "\"kernel_name\"=\"Linux\"&\"kernel_release\"=\"5.13.0-30-generic\"&\"kernel_version\"=\"#33~20.04.1-Ubuntu SMP Mon "
        "Feb 7 14:25:10 UTC 2022\"&\"product_vendor\"=\"Acme Inc.\"&\"product_name\"=\"Foo 123\")",
        "Azure%20OSConfig%205%3B1.0.1.20220308%20%28%22os_name%22%3D%22Ubuntu%22%26os_version%22%3D%2220.04.4%22%26%22cpu_"
        "architecture%22%3D%22x86_64%22%26%22kernel_name%22%3D%22Linux%22%26%22kernel_release%22%3D%225.13.0-30-generic%22%26"
        "%22kernel_version%22%3D%22%2333~20.04.1-Ubuntu%20SMP%20Mon%20Feb%207%2014%3A25%3A10%20UTC%202022%22%26%22"
        "product_vendor%22%3D%22Acme%20Inc.%22%26%22product_name%22%3D%22Foo%20123%22%29" },
        { "`-=~!@#$%^&*()_+,./<>?'[]{}| qwertyuiopasdfghjklzxcvbnm 1234567890 QWERTYUIOPASDFGHJKLZXCVBNM\n",
        "%60-%3D~%21%40%23%24%25%5E%26%2A%28%29_%2B%2C.%2F%3C%3E%3F%27%5B%5D%7B%7D%7C%20qwertyuiopasdfghjklzxcvbnm%201234567890%20QWERTYUIOPASDFGHJKLZXCVBNM%0A" }
    };

    int testUrlsSize = ARRAY_SIZE(testUrls);

    char* url = nullptr;

    for (int i = 0; i < testUrlsSize; i++)
    {
        EXPECT_NE(nullptr, url = UrlEncode((char*)testUrls[i].decoded));
        EXPECT_STREQ(url, testUrls[i].encoded);
        FREE_MEMORY(url);

        EXPECT_NE(nullptr, url = UrlDecode((char*)testUrls[i].encoded));
        EXPECT_STREQ(url, testUrls[i].decoded);
        FREE_MEMORY(url);
    }

    EXPECT_EQ(nullptr, url = UrlEncode(nullptr));
    EXPECT_EQ(nullptr, url = UrlDecode(nullptr));
}

TEST_F(CommonUtilsTest, LockUnlockFile)
{
    FILE* testFile = nullptr;

    EXPECT_TRUE(CreateTestFile(m_path, m_data));
    EXPECT_NE(nullptr, testFile = fopen(m_path, "r"));
    EXPECT_TRUE(LockFile(testFile, nullptr));
    EXPECT_EQ(nullptr, LoadStringFromFile(m_path, true, nullptr));
    EXPECT_TRUE(UnlockFile(testFile, nullptr));
    EXPECT_STREQ(m_data, LoadStringFromFile(m_path, true, nullptr));
    fclose(testFile);
    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, DuplicateString)
{
    char* duplicate = nullptr;
    EXPECT_EQ(nullptr, duplicate = DuplicateString(nullptr));
    EXPECT_NE(nullptr, duplicate = DuplicateString(m_data));
    EXPECT_STREQ(m_data, duplicate);
    FREE_MEMORY(duplicate);
}

TEST_F(CommonUtilsTest, HashCommand)
{
    EXPECT_EQ(nullptr, HashCommand(nullptr, nullptr));

    char* hashOne = nullptr;
    char* hashTwo = nullptr;
    char* hashThree = nullptr;

    const char testOne[] = "echo \"This is a test 1234567890\"";
    const char testTwo[] = "echo \"This is a test 1234567890 test\"";

    EXPECT_NE(nullptr, hashOne = HashCommand(testOne, nullptr));
    EXPECT_NE(nullptr, hashTwo = HashCommand(testTwo, nullptr));
    EXPECT_NE(nullptr, hashThree = HashCommand(testOne, nullptr));

    EXPECT_NE(hashOne, hashTwo);
    EXPECT_STREQ(hashOne, hashThree);

    FREE_MEMORY(hashOne);
    FREE_MEMORY(hashTwo);
    FREE_MEMORY(hashThree);
}

struct TestHttpHeader
{
    const char* httpRequest;
    const char* expectedUri;
    int expectedHttpStatus;
    int expectedHttpContentLength;
};

TEST_F(CommonUtilsTest, ReadtHttpHeaderInfoFromSocket)
{
    const char* testPath = "~socket.test";
    
    TestHttpHeader testHttpHeaders[] = {
        { "POST /foo/ HTTP/1.1\r\nblah blah\r\n\r\n\"", "foo", 404, 0 },
        { "HTTP/1.1 301\r\ntest 123\r\n\r\n\"", NULL, 301, 0 },
        { "POST /blah HTTP/1.1 402 something \r\ntest 123\r\n\r\n\"", "blah", 402, 0 },
        { "PUT /MpiOpen/ HTTP/1.1\r\nContent-Length: 2\r\n here 123\r\n\r\n\"12\"", NULL, 404, 2 },
        { "POST /MpiGetReported/ HTTP/1.1\r\ntest test test\r\nContent-Length: 10\r\n\r\n\"1234567890\"", "MpiGetReported", 404, 10 },
        { "POST /MpiSetDesired HTTP/1.1 400 Boom! \r\test abc\r\nContent-Length: 1\r\n\r\n\"1\"", "MpiSetDesired", 400, 1 },
        { "POST /mpi HTTP/1.1\r\nHost: osconfig\r\nUser-Agent: osconfig\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: 12\r\n\r\n\"{1234567890}\"", "mpi", 404, 12},
        { "HTTP/1.1 200 OK\r\nHost: osconfig\r\nUser-Agent: osconfig\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: 5\r\n\r\n\"{123}\"", NULL, 200, 5 }
    };

    int testHttpHeadersSize = ARRAY_SIZE(testHttpHeaders);

    int fileDescriptor = -1;

    char* uri = NULL;

    int i = 0;

    for (i = 0; i < testHttpHeadersSize; i++)
    {
        EXPECT_TRUE(CreateTestFile(testPath, testHttpHeaders[i].httpRequest));
        EXPECT_STREQ(testHttpHeaders[i].httpRequest, LoadStringFromFile(testPath, false, nullptr));
        EXPECT_NE(-1, fileDescriptor = open(testPath, O_RDONLY));
        EXPECT_EQ(testHttpHeaders[i].expectedHttpStatus, ReadHttpStatusFromSocket(fileDescriptor, nullptr));
        EXPECT_EQ(testHttpHeaders[i].expectedHttpContentLength, ReadHttpContentLengthFromSocket(fileDescriptor, nullptr));
        EXPECT_EQ(0, close(fileDescriptor));
        EXPECT_TRUE(Cleanup(testPath));
    }

    for (i = 0; i < testHttpHeadersSize; i++)
    {
        EXPECT_TRUE(CreateTestFile(testPath, testHttpHeaders[i].httpRequest));
        EXPECT_STREQ(testHttpHeaders[i].httpRequest, LoadStringFromFile(testPath, false, nullptr));
        EXPECT_NE(-1, fileDescriptor = open(testPath, O_RDONLY));
        if (NULL == testHttpHeaders[i].expectedUri)
        {
            EXPECT_EQ(testHttpHeaders[i].expectedUri, ReadUriFromSocket(fileDescriptor, nullptr));
        }
        else
        {
            EXPECT_STREQ(testHttpHeaders[i].expectedUri, uri = ReadUriFromSocket(fileDescriptor, nullptr));
            FREE_MEMORY(uri);
        }
        EXPECT_EQ(0, close(fileDescriptor));
        EXPECT_TRUE(Cleanup(testPath));
    }
}

TEST_F(CommonUtilsTest, Sleep)
{
    long validValue = 100;
    long negativeValue = -100;
    long tooBigValue = 999999999 + 1;

    EXPECT_EQ(0, SleepMilliseconds(validValue));
    EXPECT_EQ(EINVAL, SleepMilliseconds(negativeValue));
    EXPECT_EQ(EINVAL, SleepMilliseconds(tooBigValue));
}