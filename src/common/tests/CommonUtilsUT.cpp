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
    char* kernelName = NULL;
    char* kernelVersion = NULL;
    char* kernelRelease = NULL;

    EXPECT_NE(nullptr, osName = GetOsName(nullptr));
    EXPECT_NE(nullptr, osVersion = GetOsVersion(nullptr));
    EXPECT_NE(nullptr, cpuType = GetCpu(nullptr));
    EXPECT_NE(nullptr, kernelName = GetOsKernelName(nullptr));
    EXPECT_NE(nullptr, kernelVersion = GetOsKernelVersion(nullptr));
    EXPECT_NE(nullptr, kernelRelease = GetOsKernelRelease(nullptr));

    FREE_MEMORY(osName);
    FREE_MEMORY(osVersion);
    FREE_MEMORY(cpuType);
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

TEST_F(CommonUtilsTest, PersistentHashString)
{
    EXPECT_EQ(nullptr, PersistentHashString(nullptr, nullptr));

    char* hashOne = nullptr;
    char* hashTwo = nullptr;
    char* hashThree = nullptr;
    
    const char testOne[] = "This is a test 1234567890";
    const char testTwo[] = "This is a test 1234567890 test";

    EXPECT_NE(nullptr, hashOne = PersistentHashString(testOne, nullptr));
    EXPECT_NE(nullptr, hashTwo = PersistentHashString(testTwo, nullptr));
    EXPECT_NE(nullptr, hashThree = PersistentHashString(testOne, nullptr));

    EXPECT_NE(hashOne, hashTwo);
    EXPECT_STREQ(hashOne, hashThree);

    FREE_MEMORY(hashOne);
    FREE_MEMORY(hashTwo);
    FREE_MEMORY(hashThree);
}

TEST_F(CommonUtilsTest, LargeTextPersistentHashString)
{
    char* hash = nullptr;

    const char largeText[] = "accountsservice (=0.6.55-0ubuntu12~20.04.5) adduser (=3.118ubuntu2) alsa-topology-conf (=1.2.2-1) alsa-ucm-conf (=1.2.2-1ubuntu0.11) apparmor (=2.13.3-7ubuntu5.1) apport "
        "(=2.20.11-0ubuntu27.21) apport-symptoms (=0.23) apt (=2.0.6) apt-transport-https (=2.0.6) apt-utils (=2.0.6) at (=3.1.23-1ubuntu1) autoconf (=2.69-11.1) automake (=1:1.16.1-4ubuntu6) autopoint "
        "(=0.19.8.1-10build1) autotools-dev (=20180224.1) aziot-edge (=1.2.7-1) aziot-identity-service (=1.2.5-1) azure-cli (=2.0.81+ds-4ubuntu0.2) babeltrace (=1.5.8-1build1) base-files (=11ubuntu5.4) "
        "base-passwd (=3.5.47) bash (=5.0-6ubuntu1.1) bash-completion (=1:2.10-1ubuntu1) bc (=1.07.1-2build1) bcache-tools (=1.0.8-3ubuntu0.1) bind9-dnsutils (=1:9.16.1-0ubuntu2.10) bind9-host "
        "(=1:9.16.1-0ubuntu2.10) bind9-libs (=1:9.16.1-0ubuntu2.10) binutils (=2.34-6ubuntu1.3) binutils-aarch64-linux-gnu (=2.34-6ubuntu1.3) binutils-common (=2.34-6ubuntu1.3) bison (=2:3.5.1+dfsg-1) "
        "bolt (=0.8-4ubuntu1) bsdmainutils (=11.1.2ubuntu3) bsdutils (=1:2.34-0.1ubuntu9.3) btrfs-progs (=5.4.1-2) build-essential (=12.8ubuntu1.1) busybox-initramfs (=1:1.30.1-4ubuntu6.4) busybox-static "
        "(=1:1.30.1-4ubuntu6.4) byobu (=5.133-0ubuntu1) bzip2 (=1.0.8-2) ca-certificates (=20210119~20.04.2) ccze (=0.2.1-4build1) cloud-guest-utils (=0.31-7-gd99b2d76-0ubuntu1) cloud-init "
        "(=21.4-0ubuntu1~20.04.1) cloud-initramfs-copymods (=0.45ubuntu2) cloud-initramfs-dyn-netconf (=0.45ubuntu2) cmake (=3.16.3-1ubuntu1) cmake-data (=3.16.3-1ubuntu1) command-not-found (=20.04.4) "
        "console-setup (=1.194ubuntu3) console-setup-linux (=1.194ubuntu3) coreutils (=8.30-3ubuntu2) cpio (=2.13+dfsg-2ubuntu0.3) cpp (=4:9.3.0-1ubuntu2) cpp-9 (=9.4.0-1ubuntu1~20.04.1) crda (=3.18-1build1) "
        "cron (=3.0pl1-136ubuntu1) cryptsetup (=2:2.2.2-3ubuntu2.4) cryptsetup-bin (=2:2.2.2-3ubuntu2.4) cryptsetup-initramfs (=2:2.2.2-3ubuntu2.4) cryptsetup-run (=2:2.2.2-3ubuntu2.4) curl (=7.68.0-1ubuntu2.7) "
        "dash (=0.5.10.2-6) dbus (=1.12.16-2ubuntu2.1) dbus-user-session (=1.12.16-2ubuntu2.1) dconf-gsettings-backend (=0.36.0-1) dconf-service (=0.36.0-1) debconf (=1.5.73) debconf-i18n (=1.5.73) "
        "debhelper (=12.10ubuntu1) debianutils (=4.9.1) device-tree-compiler (=1.5.1-1) devio (=1.2-1.2) dh-autoreconf (=19) dh-strip-nondeterminism (=1.7.0-1) diffutils (=1:3.7-3) dirmngr (=2.2.19-3ubuntu2.1) "
        "discover (=2.1.2-8) discover-data (=2.2013.01.11ubuntu1) distro-info (=0.23ubuntu1) distro-info-data (=0.43ubuntu1.9) dmeventd (=2:1.02.167-1ubuntu1) dmidecode (=3.2-3) dmsetup (=2:1.02.167-1ubuntu1) "
        "dosfstools (=4.1-2) dpkg (=1.19.7ubuntu3) dpkg-dev (=1.19.7ubuntu3) dwz (=0.13-5) e2fsprogs (=1.45.5-2ubuntu1) eatmydata (=105-7) ed (=1.16-1) eject (=2.1.5+deb1+cvs20081104-14) ethtool (=1:5.4-1) "
        "fakeroot (=1.24-1) fdisk (=2.34-0.1ubuntu9.3) file (=1:5.38-4) finalrd (=6~ubuntu20.04.1) findutils (=4.7.0-1ubuntu1) flash-kernel (=3.103ubuntu1~20.04.3) fonts-lato (=2.0-2) fonts-ubuntu-console "
        "(=0.83-4ubuntu1) fortune-mod (=1:1.99.1-7build1) fortunes-min (=1:1.99.1-7build1) friendly-recovery (=0.2.41ubuntu0.20.04.1) ftp (=0.17-34.1) fuse (=2.9.9-3) fwupd (=1.5.11-0ubuntu1~20.04.2) "
        "fwupd-signed (=1.27.1ubuntu5+1.5.11-0ubuntu1~20.04.2) g++ (=4:9.3.0-1ubuntu2) g++-9 (=9.4.0-1ubuntu1~20.04.1) gawk (=1:5.0.1+dfsg-1) gcc (=4:9.3.0-1ubuntu2) gcc-10-base (=10.3.0-1ubuntu1~20.04) "
        "gcc-9 (=9.4.0-1ubuntu1~20.04.1) gcc-9-base (=9.4.0-1ubuntu1~20.04.1) gdb (=9.2-0ubuntu1~20.04) gdbserver (=9.2-0ubuntu1~20.04) gdisk (=1.0.5-1) gettext (=0.19.8.1-10build1) gettext-base "
        "(=0.19.8.1-10build1) gir1.2-glib-2.0 (=1.64.1-1~ubuntu20.04.1) gir1.2-packagekitglib-1.0 (=1.1.13-2ubuntu1.1) git (=1:2.25.1-1ubuntu3.2) git-man (=1:2.25.1-1ubuntu3.2) glib-networking "
        "(=2.64.2-1ubuntu0.1) glib-networking-common (=2.64.2-1ubuntu0.1) glib-networking-services (=2.64.2-1ubuntu0.1) gnupg (=2.2.19-3ubuntu2.1) gnupg-l10n (=2.2.19-3ubuntu2.1) gnupg-utils "
        "(=2.2.19-3ubuntu2.1) googletest (=1.10.0-2) gpg (=2.2.19-3ubuntu2.1) gpg-agent (=2.2.19-3ubuntu2.1) gpg-wks-client (=2.2.19-3ubuntu2.1) gpg-wks-server (=2.2.19-3ubuntu2.1) gpgconf "
        "(=2.2.19-3ubuntu2.1) gpgsm (=2.2.19-3ubuntu2.1) gpgv (=2.2.19-3ubuntu2.1) grep (=3.4-1) groff-base (=1.22.4-4build1) gsettings-desktop-schemas (=3.36.0-1ubuntu1) gzip (=1.10-0ubuntu4) hdparm "
        "(=9.58+ds-4) hostname (=3.23) htop (=2.2.0-2build1) info (=6.7.0.dfsg.2-5) init (=1.57) init-system-helpers (=1.57) initramfs-tools (=0.136ubuntu6.6) initramfs-tools-bin (=0.136ubuntu6.6) "
        "initramfs-tools-core (=0.136ubuntu6.6) install-info (=6.7.0.dfsg.2-5) intltool-debian (=0.35.0+20060710.5) iotop (=0.6-24-g733f3f8-1) iproute2 (=5.5.0-1ubuntu1) iptables (=1.8.4-3ubuntu2) "
        "iputils-ping (=3:20190709-3) iputils-tracepath (=3:20190709-3) irqbalance (=1.6.0-3ubuntu1) isc-dhcp-client (=4.4.1-2.1ubuntu5.20.04.2) isc-dhcp-common (=4.4.1-2.1ubuntu5.20.04.2) iso-codes "
        "(=4.4-1) iw (=5.4-1) javascript-common (=11) jq (=1.6-1ubuntu0.20.04.1) kbd (=2.0.4-4ubuntu2) keyboard-configuration (=1.194ubuntu3) klibc-utils (=2.0.7-1ubuntu5) kmod (=27-1ubuntu2) kpartx "
        "(=0.8.3-1ubuntu2) krb5-locales (=1.17-6ubuntu4.1) landscape-common (=19.12-0ubuntu4.2) language-selector-common (=0.204.2) less (=551-1ubuntu0.1) libaccountsservice0 (=0.6.55-0ubuntu12~20.04.5) "
        "libacl1 (=2.2.53-6) libaio1 (=0.3.112-5) libalgorithm-diff-perl (=1.19.03-2) libalgorithm-diff-xs-perl (=0.04-6) libalgorithm-merge-perl (=0.08-3) libapparmor1 (=2.13.3-7ubuntu5.1) libappstream4 "
        "(=0.12.10-2) libapt-pkg6.0 (=2.0.6) libarchive-cpio-perl (=0.10-1) libarchive-zip-perl (=1.67-2) libarchive13 (=3.4.0-2ubuntu1.1) libargon2-1 (=0~20171227-0.2) libasan5 (=9.4.0-1ubuntu1~20.04.1) "
        "libasn1-8-heimdal (=7.7.0+dfsg-1ubuntu1) libasound2 (=1.2.2-2.1ubuntu2.5) libasound2-data (=1.2.2-2.1ubuntu2.5) libassuan0 (=2.5.3-7ubuntu2) libatasmart4 (=0.19-5) libatm1 (=1:2.5.1-4) libatomic1 "
        "(=10.3.0-1ubuntu1~20.04) libattr1 (=1:2.4.48-5) libaudit-common (=1:2.8.5-2ubuntu6) libaudit1 (=1:2.8.5-2ubuntu6) libbabeltrace1 (=1.5.8-1build1) libbinutils (=2.34-6ubuntu1.3) libblkid1 "
        "(=2.34-0.1ubuntu9.3) libblockdev-crypto2 (=2.23-2ubuntu3) libblockdev-fs2 (=2.23-2ubuntu3) libblockdev-loop2 (=2.23-2ubuntu3) libblockdev-part-err2 (=2.23-2ubuntu3) libblockdev-part2 (=2.23-2ubuntu3) "
        "libblockdev-swap2 (=2.23-2ubuntu3) libblockdev-utils2 (=2.23-2ubuntu3) libblockdev2 (=2.23-2ubuntu3) libbrotli1 (=1.0.7-6ubuntu0.1) libbsd0 (=0.10.0-1) libbz2-1.0 (=1.0.8-2) libc-bin (=2.31-0ubuntu9.7) "
        "libc-dev-bin (=2.31-0ubuntu9.7) libc6 (=2.31-0ubuntu9.7) libc6-dbg (=2.31-0ubuntu9.7) libc6-dev (=2.31-0ubuntu9.7) libcanberra0 (=0.30-7ubuntu1) libcap-ng0 (=0.7.9-2.1build1) libcap2 (=1:2.32-1) "
        "libcap2-bin (=1:2.32-1) libcbor0.6 (=0.6.0-0ubuntu1) libcc1-0 (=10.3.0-1ubuntu1~20.04) libcom-err2 (=1.45.5-2ubuntu1) libcroco3 (=0.6.13-1) libcrypt-dev (=1:4.4.10-10ubuntu4) libcrypt1 "
        "(=1:4.4.10-10ubuntu4) libcryptsetup12 (=2:2.2.2-3ubuntu2.4) libctf-nobfd0 (=2.34-6ubuntu1.3) libctf0 (=2.34-6ubuntu1.3) libcurl3-gnutls (=7.68.0-1ubuntu2.7) libcurl4 (=7.68.0-1ubuntu2.7) "
        "libcurl4-gnutls-dev (=7.68.0-1ubuntu2.7) libdb5.3 (=5.3.28+dfsg1-0.6ubuntu2) libdbd-mariadb-perl (=1.11-3ubuntu2) libdbi-perl (=1.643-1ubuntu0.1) libdbus-1-3 (=1.12.16-2ubuntu2.1) libdconf1 "
        "(=0.36.0-1) libdebconfclient0 (=0.251ubuntu1) libdebhelper-perl (=12.10ubuntu1) libdevmapper-event1.02.1 (=2:1.02.167-1ubuntu1) libdevmapper1.02.1 (=2:1.02.167-1ubuntu1) libdiscover2 (=2.1.2-8) "
        "libdns-export1109 (=1:9.11.16+dfsg-3~ubuntu1) libdpkg-perl (=1.19.7ubuntu3) libdrm-common (=2.4.107-8ubuntu1~20.04.2) libdrm2 (=2.4.107-8ubuntu1~20.04.2) libdw1 (=0.176-1.1build1) libeatmydata1 "
        "(=105-7) libedit2 (=3.1-20191231-1) libefiboot1 (=37-2ubuntu2.2) libefivar1 (=37-2ubuntu2.2) libelf1 (=0.176-1.1build1) liberror-perl (=0.17029-1) libestr0 (=0.1.10-2.1) libevent-2.1-7 "
        "(=2.1.11-stable-1) libexpat1 (=2.2.9-1ubuntu0.4) libexpat1-dev (=2.2.9-1ubuntu0.4) libext2fs2 (=1.45.5-2ubuntu1) libfakeroot (=1.24-1) libfastjson4 (=0.99.8-2) libfdisk1 (=2.34-0.1ubuntu9.3) "
        "libfdt1 (=1.5.1-1) libffi7 (=3.3-4) libfido2-1 (=1.3.1-1ubuntu2) libfile-fcntllock-perl (=0.22-3build4) libfile-stripnondeterminism-perl (=1.7.0-1) libfl2 (=2.6.4-6.2) libfribidi0 (=1.0.8-2) "
        "libfuse2 (=2.9.9-3) libfwupd2 (=1.5.11-0ubuntu1~20.04.2) libfwupdplugin1 (=1.5.11-0ubuntu1~20.04.2) libgcab-1.0-0 (=1.4-1) libgcc-9-dev (=9.4.0-1ubuntu1~20.04.1) libgcc-s1 (=10.3.0-1ubuntu1~20.04) "
        "libgcrypt20 (=1.8.5-5ubuntu1.1) libgdbm-compat4 (=1.18.1-5) libgdbm6 (=1.18.1-5) libgirepository-1.0-1 (=1.64.1-1~ubuntu20.04.1) libglib2.0-0 (=2.64.6-1~ubuntu20.04.4) libglib2.0-bin "
        "(=2.64.6-1~ubuntu20.04.4) libglib2.0-data (=2.64.6-1~ubuntu20.04.4) libgmock-dev (=1.10.0-2) libgmp-dev (=2:6.2.0+dfsg-4) libgmp10 (=2:6.2.0+dfsg-4) libgmpxx4ldbl (=2:6.2.0+dfsg-4) libgnutls30 "
        "(=3.6.13-2ubuntu1.6) libgomp1 (=10.3.0-1ubuntu1~20.04) libgpg-error0 (=1.37-1) libgpgme11 (=1.13.1-7ubuntu2) libgpm2 (=1.20.7-5) libgssapi-krb5-2 (=1.17-6ubuntu4.1) libgssapi3-heimdal "
        "(=7.7.0+dfsg-1ubuntu1) libgstreamer1.0-0 (=1.16.2-2) libgtest-dev (=1.10.0-2) libgudev-1.0-0 (=1:233-1) libgusb2 (=0.3.4-0.1) libhcrypto4-heimdal (=7.7.0+dfsg-1ubuntu1) libheimbase1-heimdal "
        "(=7.7.0+dfsg-1ubuntu1) libheimntlm0-heimdal (=7.7.0+dfsg-1ubuntu1) libhogweed5 (=3.5.1+really3.5.1-2ubuntu0.2) libhx509-5-heimdal (=7.7.0+dfsg-1ubuntu1) libicu66 (=66.1-2ubuntu2.1) libidn2-0 "
        "(=2.2.0-2) libip4tc2 (=1.8.4-3ubuntu2) libip6tc2 (=1.8.4-3ubuntu2) libisc-export1105 (=1:9.11.16+dfsg-3~ubuntu1) libisl22 (=0.22.1-1) libisns0 (=0.97-3) libitm1 (=10.3.0-1ubuntu1~20.04) libjcat1 "
        "(=0.1.3-2~ubuntu20.04.1) libjq1 (=1.6-1ubuntu0.20.04.1) libjs-jquery (=3.3.1~dfsg-3) libjs-sphinxdoc (=1.8.5-7ubuntu3) libjs-underscore (=1.9.1~dfsg-1ubuntu0.20.04.1) libjson-c4 "
        "(=0.13.1+dfsg-7ubuntu0.3) libjson-glib-1.0-0 (=1.4.4-2ubuntu2) libjson-glib-1.0-common (=1.4.4-2ubuntu2) libjsoncpp1 (=1.7.4-3.1ubuntu2) libk5crypto3 (=1.17-6ubuntu4.1) libkeyutils1 "
        "(=1.6-6ubuntu1) libklibc (=2.0.7-1ubuntu5) libkmod2 (=27-1ubuntu2) libkrb5-26-heimdal (=7.7.0+dfsg-1ubuntu1) libkrb5-3 (=1.17-6ubuntu4.1) libkrb5support0 (=1.17-6ubuntu4.1) libksba8 (=1.3.5-2) "
        "libldap-2.4-2 (=2.4.49+dfsg-2ubuntu1.8) libldap-common (=2.4.49+dfsg-2ubuntu1.8) liblmdb0 (=0.9.24-1) liblocale-gettext-perl (=1.07-4) liblsan0 (=10.3.0-1ubuntu1~20.04) libltdl-dev (=2.4.6-14) "
        "libltdl7 (=2.4.6-14) liblttng-ctl0 (=2.11.2-1build1) liblttng-ust-ctl4 (=2.11.0-1) liblttng-ust-dev (=2.11.0-1) liblttng-ust-python-agent0 (=2.11.0-1) liblttng-ust0 (=2.11.0-1) liblvm2cmd2.03 "
        "(=2.03.07-1ubuntu1) liblz4-1 (=1.9.2-2ubuntu0.20.04.1) liblzma5 (=5.2.4-1ubuntu1) liblzo2-2 (=2.10-2) libmagic-mgc (=1:5.38-4) libmagic1 (=1:5.38-4) libmail-sendmail-perl (=0.80-1) libmariadb3 "
        "(=1:10.5.15+maria~focal) libmaxminddb0 (=1.4.2-0ubuntu1.20.04.1) libmnl0 (=1.0.4-2) libmount1 (=2.34-0.1ubuntu9.3) libmpc3 (=1.1.0-1) libmpdec2 (=2.4.2-3) libmpfr6 (=4.0.2-1) libmysqlclient21 "
        "(=8.0.28-0ubuntu0.20.04.3) libncurses6 (=6.2-0ubuntu2) libncursesw6 (=6.2-0ubuntu2) libnetfilter-conntrack3 (=1.0.7-2) libnetplan0 (=0.103-0ubuntu5~20.04.5) libnettle7 (=3.5.1+really3.5.1-2ubuntu0.2) "
        "libnewt0.52 (=0.52.21-4ubuntu2) libnfnetlink0 (=1.0.1-3build1) libnftnl11 (=1.1.5-1) libnghttp2-14 (=1.40.0-1build1) libnl-3-200 (=3.4.0-1) libnl-genl-3-200 (=3.4.0-1) libnl-route-3-200 (=3.4.0-1) "
        "libnpth0 (=1.6-1) libnspr4 (=2:4.25-1) libnss-systemd (=245.4-4ubuntu3.15) libnss3 (=2:3.49.1-1ubuntu1.6) libntfs-3g883 (=1:2017.3.23AR.3-3ubuntu1.1) libnuma1 (=2.0.12-1) libogg0 (=1.3.4-0ubuntu1) "
        "libonig5 (=6.9.4-1) libp11-kit0 (=0.23.20-1ubuntu0.1) libpackagekit-glib2-18 (=1.1.13-2ubuntu1.1) libpam-cap (=1:2.32-1) libpam-modules (=1.3.1-5ubuntu4.3) libpam-modules-bin (=1.3.1-5ubuntu4.3) "
        "libpam-runtime (=1.3.1-5ubuntu4.3) libpam-systemd (=245.4-4ubuntu3.15) libpam0g (=1.3.1-5ubuntu4.3) libparted-fs-resize0 (=3.3-4ubuntu0.20.04.1) libparted2 (=3.3-4ubuntu0.20.04.1) libpcap0.8 (=1.9.1-3) "
        "libpci3 (=1:3.6.4-1ubuntu0.20.04.1) libpcre2-8-0 (=10.34-7) libpcre3 (=2:8.39-12build1) libpcsclite1 (=1.8.26-3) libperl5.30 (=5.30.0-9ubuntu0.2) libpipeline1 (=1.5.2-2build1) libplymouth5 "
        "(=0.9.4git20200323-0ubuntu6.2) libpng16-16 (=1.6.37-2) libpolkit-agent-1-0 (=0.105-26ubuntu1.3) libpolkit-gobject-1-0 (=0.105-26ubuntu1.3) libpopt0 (=1.16-14) libprocps8 (=2:3.3.16-1ubuntu2.3) "
        "libproxy1v5 (=0.4.15-10ubuntu1.2) libpsl5 (=0.21.0-1ubuntu1) libpython3-dev (=3.8.2-0ubuntu2) libpython3-stdlib (=3.8.2-0ubuntu2) libpython3.8 (=3.8.10-0ubuntu1~20.04.4) libpython3.8-dev "
        "(=3.8.10-0ubuntu1~20.04.4) libpython3.8-minimal (=3.8.10-0ubuntu1~20.04.4) libpython3.8-stdlib (=3.8.10-0ubuntu1~20.04.4) libraspberrypi-bin (=0~20200520+git2fe4ca3-0ubuntu3~20.04) libraspberrypi0 "
        "(=0~20200520+git2fe4ca3-0ubuntu3~20.04) libreadline5 (=5.2+dfsg-3build3) libreadline8 (=8.0-4) librecode0 (=3.6-24) librhash0 (=1.3.9-1) libroken18-heimdal (=7.7.0+dfsg-1ubuntu1) librtmp1 "
        "(=2.4+20151223.gitfa8646d.1-2build1) libruby2.7 (=2.7.0-5ubuntu1.6) libsasl2-2 (=2.1.27+dfsg-2ubuntu0.1) libsasl2-modules (=2.1.27+dfsg-2ubuntu0.1) libsasl2-modules-db (=2.1.27+dfsg-2ubuntu0.1) "
        "libseccomp2 (=2.5.1-1ubuntu1~20.04.2) libselinux1 (=3.0-1build2) libsemanage-common (=3.0-1build2) libsemanage1 (=3.0-1build2) libsepol1 (=3.0-1) libsgutils2-2 (=1.44-1ubuntu2) libsigsegv2 (=2.12-2) "
        "libslang2 (=2.3.2-4) libsmartcols1 (=2.34-0.1ubuntu9.3) libsodium23 (=1.0.18-1) libsoup2.4-1 (=2.70.0-1) libsqlite3-0 (=3.31.1-4ubuntu0.2) libss2 (=1.45.5-2ubuntu1) libssh-4 (=0.9.3-2ubuntu2.2) "
        "libssh2-1 (=1.8.0-2.1build1) libssl-dev (=1.1.1f-1ubuntu2.12) libssl1.1 (=1.1.1f-1ubuntu2.12) libstdc++-9-dev (=9.4.0-1ubuntu1~20.04.1) libstdc++6 (=10.3.0-1ubuntu1~20.04) libstemmer0d (=0+svn585-2) "
        "libsub-override-perl (=0.09-2) libsys-hostname-long-perl (=1.5-1) libsystemd0 (=245.4-4ubuntu3.15) libtasn1-6 (=4.16.0-2) libtdb1 (=1.4.3-0ubuntu0.20.04.1) libterm-readkey-perl (=2.38-1build1) "
        "libtext-charwidth-perl (=0.04-10) libtext-iconv-perl (=1.7-7) libtext-wrapi18n-perl (=0.06-9) libtinfo6 (=6.2-0ubuntu2) libtool (=2.4.6-14) libtsan0 (=10.3.0-1ubuntu1~20.04) libtss2-esys0 (=2.3.2-1) "
        "libubootenv-tool (=0.2-1) libubootenv0.1 (=0.2-1) libubsan1 (=10.3.0-1ubuntu1~20.04) libuchardet0 (=0.0.6-3build1) libudev1 (=245.4-4ubuntu3.15) libudisks2-0 (=2.8.4-1ubuntu2) libunistring2 (=0.9.10-2) "
        "liburcu-dev (=0.11.1-2) liburcu6 (=0.11.1-2) libusb-0.1-4 (=2:0.1.12-32) libusb-1.0-0 (=2:1.0.23-2build1) libutempter0 (=1.1.6-4) libuuid1 (=2.34-0.1ubuntu9.3) libuv1 (=1.34.2-1ubuntu1.3) "
        "libvolume-key1 (=0.3.12-3.1) libvorbis0a (=1.3.6-2ubuntu1) libvorbisfile3 (=1.3.6-2ubuntu1) libwind0-heimdal (=7.7.0+dfsg-1ubuntu1) libwrap0 (=7.6.q-30) libx11-6 (=2:1.6.9-2ubuntu1.2) libx11-data "
        "(=2:1.6.9-2ubuntu1.2) libxau6 (=1:1.0.9-0ubuntu1) libxcb1 (=1.14-2) libxdmcp6 (=1:1.1.3-0ubuntu1) libxext6 (=2:1.3.4-0ubuntu1) libxml2 (=2.9.10+dfsg-5ubuntu0.20.04.2) libxmlb1 (=0.1.15-2ubuntu1~20.04.1) "
        "libxmuu1 (=2:1.1.3-0ubuntu1) libxtables12 (=1.8.4-3ubuntu2) libyaml-0-2 (=0.2.2-1) libzstd1 (=1.4.4+dfsg-3ubuntu0.1) linux-base (=4.5ubuntu3.7) linux-firmware (=1.187.20) linux-firmware-raspi2 "
        "(=4-0ubuntu0~20.04.1) linux-headers-5.4.0-1056-raspi (=5.4.0-1056.63) linux-headers-5.4.0-1058-raspi (=5.4.0-1058.65) linux-headers-raspi (=5.4.0.1058.92) linux-image-5.4.0-1042-raspi (=5.4.0-1042.46) "
        "linux-image-5.4.0-1046-raspi (=5.4.0-1046.50) linux-image-5.4.0-1047-raspi (=5.4.0-1047.52) linux-image-5.4.0-1048-raspi (=5.4.0-1048.53) linux-image-5.4.0-1050-raspi (=5.4.0-1050.56) "
        "linux-image-5.4.0-1052-raspi (=5.4.0-1052.58) linux-image-5.4.0-1053-raspi (=5.4.0-1053.60) linux-image-5.4.0-1055-raspi (=5.4.0-1055.62) linux-image-5.4.0-1056-raspi (=5.4.0-1056.63) "
        "linux-image-5.4.0-1058-raspi (=5.4.0-1058.65) linux-image-raspi (=5.4.0.1058.92) linux-libc-dev (=5.4.0-107.121) linux-modules-5.4.0-1042-raspi (=5.4.0-1042.46) linux-modules-5.4.0-1046-raspi "
        "(=5.4.0-1046.50) linux-modules-5.4.0-1047-raspi (=5.4.0-1047.52) linux-modules-5.4.0-1048-raspi (=5.4.0-1048.53) linux-modules-5.4.0-1050-raspi (=5.4.0-1050.56) linux-modules-5.4.0-1052-raspi "
        "(=5.4.0-1052.58) linux-modules-5.4.0-1053-raspi (=5.4.0-1053.60) linux-modules-5.4.0-1055-raspi (=5.4.0-1055.62) linux-modules-5.4.0-1056-raspi (=5.4.0-1056.63) linux-modules-5.4.0-1058-raspi "
        "(=5.4.0-1058.65) linux-raspi (=5.4.0.1058.92) linux-raspi-headers-5.4.0-1056 (=5.4.0-1056.63) linux-raspi-headers-5.4.0-1058 (=5.4.0-1058.65) locales (=2.31-0ubuntu9.7) login (=1:4.8.1-1ubuntu5.20.04.1) "
        "logrotate (=3.14.0-4ubuntu3) logsave (=1.45.5-2ubuntu1) lolcat (=42.0.99-1) lsb-base (=11.1.0ubuntu2) lsb-release (=11.1.0ubuntu2) lshw (=02.18.85-0.3ubuntu2.20.04.1) lsof (=4.93.2+dfsg-1ubuntu0.20.04.1) "
        "ltrace (=0.7.3-6.1ubuntu1) lttng-tools (=2.11.2-1build1) lvm2 (=2.03.07-1ubuntu1) lxd-agent-loader (=0.4) lz4 (=1.9.2-2ubuntu0.20.04.1) m4 (=1.4.18-4) make (=4.2.1-1.2) man-db (=2.9.1-1) manpages "
        "(=5.05-1) manpages-dev (=5.05-1) mariadb-client (=1:10.5.15+maria~focal) mariadb-client-10.5 (=1:10.5.15+maria~focal) mariadb-client-core-10.5 (=1:10.5.15+maria~focal) mariadb-common "
        "(=1:10.5.15+maria~focal) mawk (=1.3.4.20200120-2) mc (=3:4.8.24-2ubuntu1) mc-data (=3:4.8.24-2ubuntu1) mdadm (=4.1-5ubuntu1.2) mime-support (=3.64ubuntu1) moby-buildx (=0.7.1+azure-1) moby-cli "
        "(=20.10.11+azure-2) moby-containerd (=1.4.12+azure-1) moby-engine (=20.10.11+azure-2) moby-runc (=1.0.2+azure-1) motd-news-config (=11ubuntu5.4) mount (=2.34-0.1ubuntu9.3) mtd-utils (=1:2.1.1-1ubuntu1) "
        "mtr-tiny (=0.93-1) multipath-tools (=0.8.3-1ubuntu2) mysql-common (=1:10.5.15+maria~focal) nano (=4.8-1ubuntu1) ncdu (=1.14.1-1) ncurses-base (=6.2-0ubuntu2) ncurses-bin (=6.2-0ubuntu2) ncurses-term "
        "(=6.2-0ubuntu2) neofetch (=7.0.0-1) net-tools (=1.60+git20180626.aebd88e-1ubuntu1) netbase (=6.1) netcat-openbsd (=1.206-1ubuntu1) netplan.io (=0.103-0ubuntu5~20.04.5) networkd-dispatcher "
        "(=2.1-2~ubuntu20.04.1) ntfs-3g (=1:2017.3.23AR.3-3ubuntu1.1) open-iscsi (=2.0.874-7.1ubuntu6.2) openssh-client (=1:8.2p1-4ubuntu0.3) openssh-server (=1:8.2p1-4ubuntu0.3) openssh-sftp-server "
        "(=1:8.2p1-4ubuntu0.3) openssl (=1.1.1f-1ubuntu2.12) overlayroot (=0.45ubuntu2) packagekit (=1.1.13-2ubuntu1.1) packagekit-tools (=1.1.13-2ubuntu1.1) packages-microsoft-prod (=1.0-ubuntu20.04.1) "
        "parted (=3.3-4ubuntu0.20.04.1) passwd (=1:4.8.1-1ubuntu5.20.04.1) pastebinit (=1.5.1-1) patch (=2.7.6-6) pci.ids (=0.0~2020.03.20-1) pciutils (=1:3.6.4-1ubuntu0.20.04.1) perl (=5.30.0-9ubuntu0.2) "
        "perl-base (=5.30.0-9ubuntu0.2) perl-modules-5.30 (=5.30.0-9ubuntu0.2) pigz (=2.4-1) pinentry-curses (=1.1.0-3build1) plymouth (=0.9.4git20200323-0ubuntu6.2) plymouth-theme-ubuntu-text "
        "(=0.9.4git20200323-0ubuntu6.2) po-debconf (=1.0.21) policykit-1 (=0.105-26ubuntu1.3) pollinate (=4.33-3ubuntu1.20.04.1) popularity-contest (=1.69ubuntu1) powermgmt-base (=1.36) procps "
        "(=2:3.3.16-1ubuntu2.3) psmisc (=23.3-1) publicsuffix (=20200303.0012-1) python-apt-common (=2.0.0ubuntu0.20.04.6) python-pip-whl (=20.0.2-5ubuntu1.6) python-pkginfo-doc (=1.4.2-3) python3 "
        "(=3.8.2-0ubuntu2) python3-adal (=1.2.2-1) python3-aiohttp (=3.6.2-1build1) python3-appdirs (=1.4.3-2.1) python3-applicationinsights (=0.11.9-2) python3-apport (=2.20.11-0ubuntu27.21) python3-apt "
        "(=2.0.0ubuntu0.20.04.6) python3-argcomplete (=1.8.1-1.3ubuntu1) python3-async-timeout (=3.0.1-1) python3-attr (=19.3.0-2) python3-automat (=0.8.0-1ubuntu1) python3-azext-devops (=0.17.0-5) "
        "python3-azure (=20200130+git-2) python3-azure-cli (=2.0.81+ds-4ubuntu0.2) python3-azure-cli-core (=2.0.81+ds-4ubuntu0.2) python3-azure-cli-telemetry (=2.0.81+ds-4ubuntu0.2) python3-azure-cosmos "
        "(=3.1.1-3) python3-azure-cosmosdb-table (=1.0.5+git20191025-5) python3-azure-datalake-store (=0.0.48-3) python3-azure-functions-devops-build (=0.0.22-4) python3-azure-multiapi-storage (=0.2.4-3) "
        "python3-azure-storage (=20181109+git-2) python3-bcrypt (=3.1.7-2ubuntu1) python3-blinker (=1.4+dfsg1-0.3ubuntu1) python3-certifi (=2019.11.28-1) python3-cffi (=1.14.0-1build1) python3-cffi-backend "
        "(=1.14.0-1build1) python3-chardet (=3.0.4-4build1) python3-click (=7.0-3) python3-colorama (=0.4.3-1build1) python3-commandnotfound (=20.04.4) python3-configobj (=5.0.6-4) python3-constantly "
        "(=15.1.0-1build1) python3-cryptography (=2.8-3ubuntu0.1) python3-dateutil (=2.7.3-3ubuntu1) python3-dbus (=1.2.16-1build1) python3-debconf (=1.5.73) python3-debian (=0.1.36ubuntu1) python3-dev "
        "(=3.8.2-0ubuntu2) python3-distlib (=0.3.0-1) python3-distro (=1.4.0-1) python3-distro-info (=0.23ubuntu1) python3-distupgrade (=1:20.04.36) python3-distutils (=3.8.10-0ubuntu1~20.04) python3-entrypoints "
        "(=0.3-2ubuntu1) python3-fabric (=2.5.0-0.2) python3-filelock (=3.0.12-2) python3-gdbm (=3.8.10-0ubuntu1~20.04) python3-gi (=3.36.0-1) python3-hamcrest (=1.9.0-3) python3-httplib2 (=0.14.0-1ubuntu1) "
        "python3-humanfriendly (=4.18-2) python3-hyperlink (=19.0.0-1) python3-idna (=2.8-1) python3-importlib-metadata (=1.5.0-1) python3-incremental (=16.10.1-3.2) python3-invoke (=1.3.0+ds-0.1) "
        "python3-isodate (=0.6.0-2) python3-javaproperties (=0.6.0-2) python3-jinja2 (=2.10.1-2) python3-jmespath (=0.9.4-2ubuntu1) python3-jsmin (=2.2.2-2) python3-json-pointer (=2.0-0ubuntu1) "
        "python3-jsondiff (=1.1.1-4) python3-jsonpatch (=1.23-3) python3-jsonschema (=3.2.0-0ubuntu2) python3-jwt (=1.7.1-2ubuntu2) python3-keyring (=18.0.1-2ubuntu1) python3-knack (=0.6.3-2) "
        "python3-launchpadlib (=1.10.13-1) python3-lazr.restfulclient (=0.14.2-2build1) python3-lazr.uri (=1.0.3-4build1) python3-lib2to3 (=3.8.10-0ubuntu1~20.04) python3-markupsafe (=1.1.0-1build2) "
        "python3-minimal (=3.8.2-0ubuntu2) python3-mock (=3.0.5-1build1) python3-more-itertools (=4.2.0-1build1) python3-msal (=1.1.0-3) python3-msal-extensions (=0.1.3-2) python3-msrest (=0.6.10-1) "
        "python3-msrestazure (=0.6.2-1) python3-multidict (=4.7.3-1build1) python3-nacl (=1.3.0-5) python3-netifaces (=0.10.4-1ubuntu4) python3-newt (=0.52.21-4ubuntu2) python3-oauthlib (=3.1.0-1ubuntu2) "
        "python3-openssl (=19.0.0-1build1) python3-paramiko (=2.6.0-2ubuntu0.1) python3-pbr (=5.4.5-0ubuntu1) python3-pexpect (=4.6.0-1build1) python3-pip (=20.0.2-5ubuntu1.6) python3-pkg-resources (=45.2.0-1) "
        "python3-pkginfo (=1.4.2-3) python3-ply (=3.11-3ubuntu0.1) python3-portalocker (=1.5.1-1) python3-problem-report (=2.20.11-0ubuntu27.21) python3-psutil (=5.5.1-1ubuntu4) python3-ptyprocess "
        "(=0.6.0-1ubuntu1) python3-pyasn1 (=0.4.2-3build1) python3-pyasn1-modules (=0.2.1-0.2build1) python3-pycparser (=2.19-1ubuntu1) python3-pygments (=2.3.1+dfsg-1ubuntu2.2) python3-pymacaroons (=0.13.0-3) "
        "python3-pyrsistent (=0.15.5-1build1) python3-requests (=2.22.0-2ubuntu1) python3-requests-oauthlib (=1.0.0-1.1build1) python3-requests-unixsocket (=0.2.0-2) python3-scp (=0.13.0-2) python3-secretstorage "
        "(=2.3.1-2ubuntu1) python3-serial (=3.4-5.1) python3-service-identity (=18.1.0-5build1) python3-setuptools (=45.2.0-1) python3-simplejson (=3.16.0-2ubuntu2) python3-six (=1.14.0-2) "
        "python3-software-properties (=0.99.9.8) python3-sshtunnel (=0.1.4-2) python3-systemd (=234-3build2) python3-tabulate (=0.8.6-0ubuntu2) python3-twisted (=18.9.0-11ubuntu0.20.04.2) python3-twisted-bin "
        "(=18.9.0-11ubuntu0.20.04.2) python3-tz (=2019.3-1) python3-uamqp (=1.2.6-4) python3-update-manager (=1:20.04.10.9) python3-urllib3 (=1.25.8-2ubuntu0.1) python3-virtualenv (=20.0.17-1ubuntu0.4) "
        "python3-vsts-cd-manager (=1.0.2-2) python3-wadllib (=1.3.3-3build1) python3-websocket (=0.53.0-2ubuntu1) python3-wheel (=0.34.2-1) python3-xmltodict (=0.12.0-1) python3-yaml (=5.3.1-1ubuntu0.1) "
        "python3-yarl (=1.4.2-2) python3-zipp (=1.0.0-1) python3-zope.interface (=4.7.1-1) python3.8 (=3.8.10-0ubuntu1~20.04.4) python3.8-dev (=3.8.10-0ubuntu1~20.04.4) python3.8-minimal "
        "(=3.8.10-0ubuntu1~20.04.4) rake (=13.0.1-4) rapidjson-dev (=1.1.0+dfsg2-5ubuntu1) readline-common (=8.0-4) rsync (=3.1.3-8ubuntu0.3) rsyslog (=8.2001.0-1ubuntu1.1) ruby (=1:2.7+1) ruby-dev "
        "(=1:2.7+1) ruby-minitest (=5.13.0-1) ruby-net-telnet (=0.1.1-2) ruby-paint (=0.8.6-2) ruby-power-assert (=1.1.7-1) ruby-test-unit (=3.3.5-1) ruby-trollop (=2.0-2) ruby-xmlrpc (=0.3.0-2) ruby2.7 "
        "(=2.7.0-5ubuntu1.6) ruby2.7-dev (=2.7.0-5ubuntu1.6) ruby2.7-doc (=2.7.0-5ubuntu1.6) rubygems-integration (=1.16) run-one (=1.17-0ubuntu1) screen (=4.8.0-1ubuntu0.1) sed (=4.7-1) sensible-utils "
        "(=0.0.12+nmu1) sg3-utils (=1.44-1ubuntu2) sg3-utils-udev (=1.44-1ubuntu2) shared-mime-info (=1.15-1) snapd (=2.54.3+20.04.1ubuntu0.2) software-properties-common (=0.99.9.8) sosreport "
        "(=4.1-1ubuntu0.20.04.3) sound-theme-freedesktop (=0.8-2ubuntu1) squashfs-tools (=1:4.4-1ubuntu0.3) ssh-import-id (=5.10-0ubuntu1) strace (=5.5-3ubuntu1) sudo (=1.8.31-1ubuntu1.2) systemd "
        "(=245.4-4ubuntu3.15) systemd-sysv (=245.4-4ubuntu3.15) systemd-timesyncd (=245.4-4ubuntu3.15) sysvinit-utils (=2.96-2.1ubuntu1) tar (=1.30+dfsg-7ubuntu0.20.04.2) tcpdump (=4.9.3-4) telnet "
        "(=0.17-41.2build1) thin-provisioning-tools (=0.8.5-4build1) time (=1.7-25.1build1) tmux (=3.0a-2ubuntu0.3) tpm-udev (=0.4) tzdata (=2022a-0ubuntu0.20.04) u-boot-rpi (=2021.01+dfsg-3ubuntu0~20.04.3) "
        "u-boot-tools (=2021.01+dfsg-3ubuntu0~20.04.3) ubuntu-advantage-tools (=27.4.2~20.04.1) ubuntu-keyring (=2020.02.11.4) ubuntu-minimal (=1.450.2) ubuntu-release-upgrader-core (=1:20.04.36) "
        "ubuntu-server (=1.450.2) ubuntu-standard (=1.450.2) ucf (=3.0038+nmu1) udev (=245.4-4ubuntu3.15) udisks2 (=2.8.4-1ubuntu2) ufw (=0.36-6ubuntu1) unattended-upgrades (=2.3ubuntu0.1) unzip "
        "(=6.0-25ubuntu1) update-manager-core (=1:20.04.10.9) update-notifier-common (=3.192.30.9) usb.ids (=2020.03.19-1) usbutils (=1:012-2) util-linux (=2.34-0.1ubuntu9.3) uuid-dev (=2.34-0.1ubuntu9.3) "
        "uuid-runtime (=2.34-0.1ubuntu9.3) vim (=2:8.1.2269-1ubuntu5.7) vim-common (=2:8.1.2269-1ubuntu5.7) vim-runtime (=2:8.1.2269-1ubuntu5.7) vim-tiny (=2:8.1.2269-1ubuntu5.7) wget (=1.20.3-1ubuntu2) "
        "whiptail (=0.52.21-4ubuntu2) wireless-regdb (=2021.08.28-0ubuntu1~20.04.1) wpasupplicant (=2:2.9-1ubuntu4.3) xauth (=1:1.1-0ubuntu1) xdg-user-dirs (=0.17-2ubuntu1) xfsprogs (=5.3.0-1ubuntu2) xkb-data "
        "(=2.29-2) xxd (=2:8.1.2269-1ubuntu5.7) xz-utils (=5.2.4-1ubuntu1) zip (=3.0-11build1) zlib1g (=1:1.2.11.dfsg-2ubuntu1.3) zlib1g-dev (=1:1.2.11.dfsg-2ubuntu1.3)";

    EXPECT_NE(nullptr, hash = PersistentHashString(largeText, nullptr));
    EXPECT_NE(0, strlen(hash));
    
    FREE_MEMORY(hash);
}
