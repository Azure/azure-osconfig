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
    const char stringMap[] = R"""({"key1": "value1", "key2" : "value2"})""";
    const char integerMap[] = R"""({"key1": 1, "key2" : 2})""";

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

TEST_F(CommonUtilsTest, ValidateHttpProxyDataParsing)
{
    char* hostAddress = NULL;
    int port = 0;
    char* username = NULL;
    char* password = NULL;

    HttpProxyOptions validOptions[] = {
        { "http://wwww.foo.org:123", "wwww.foo.org", 123, nullptr, nullptr },
        { "http://11.22.33.44:123", "11.22.33.44", 123, nullptr, nullptr },
        { "http://user:password@wwww.foo.org:123", "wwww.foo.org", 123, "user", "password" },
        { "http://user:password@11.22.33.44:123", "11.22.33.44", 123, "user", "password" },
        { "http://user:password@wwww.foo.org:123/", "wwww.foo.org", 123, "user", "password" },
        { "http://user:password@11.22.33.44.55:123/", "11.22.33.44.55", 123, "user", "password" },
        { "http://user:password@wwww.foo.org:123//", "wwww.foo.org", 123, "user", "password" },
        { "HTTP://wwww.foo.org:123", "wwww.foo.org", 123, nullptr, nullptr },
        { "HTTP://11.22.33.44:123", "11.22.33.44", 123, nullptr, nullptr },
        { "HTTP://user:password@wwww.foo.org:123", "wwww.foo.org", 123, "user", "password" },
        { "HTTP://user:password@11.22.33.44.55:123", "11.22.33.44.55", 123, "user", "password" },
        { "HTTP://user:password@wwww.foo.org:123/", "wwww.foo.org", 123, "user", "password" },
        { "HTTP://user:password@11.22.33.44.55:123/", "11.22.33.44.55", 123, "user", "password" },
        { "HTTP://boom_user:boom-password@www.boom.org:666/", "www.boom.org", 666, "boom_user", "boom-password" },
        { "HTTP://user\\@foomail.org:passw\\@rd@wwww.foo.org:123//", "wwww.foo.org", 123, "user@foomail.org", "passw@rd" },
        { "http://user\\@blah:p\\@\\@ssword@11.22.33.44.55:123", "11.22.33.44.55", 123, "user@blah", "p@@ssword" },
        { "HTTP://foo_domain\\username:p\\@ssw\\@rd@wwww.foo.org:123//", "wwww.foo.org", 123, "foo_domain\\username", "p@ssw@rd" },
        { "http://proxyuser:password@10.0.0.2:8080", "10.0.0.2", 8080, "proxyuser", "password" },
        { "http://10.0.0.2:8080", "10.0.0.2", 8080, nullptr, nullptr },
        { "HTTP://foodomain\\user:pass\\@word@11.22.33.44.55:123/", "11.22.33.44.55", 123, "foodomain\\user", "pass@word" }
    };

    int validOptionsSize = ARRAY_SIZE(validOptions);
    EXPECT_EQ(validOptionsSize, 20);

    EXPECT_EQ(sizeof(validOptions[1]), sizeof(validOptions[3]));

    const char* badOptions[] = {
        "//wwww.foo.org:123",
        "https://wwww.foo.org:123",
        "11.22.22.44:123",
        "//wwww.foo.org:123@@",
        "user:password@wwww.foo.org:123/",
        "HTTPS://user:password@wwww.foo.org:123",
        "some text",
        "http://some text",
        "123",
        "http://abc"
        "HTTP://user:pass#word@wwww.foo.org:123//",
        "http://us$er:pass#word@wwww.foo.org:123",
        "http://wwww.foo!.org:123",
        "http://a`:1",
        "@//wwww.foo.org:123",
        "http://wwww.^foo.org:123",
        "http://wwww.fo$o.org:123",
        "http://wwww.fo%o.org:123",
        "http://wwww.&oo.org:123",
        "http://wwww.foo?org:abc",
        "http://www*.foo.org:abc",
        "http://user:(password)@wwww.foo.org:123",
        "http://user:pass+word@wwww.foo.org:123",
        "http://user:pass=word@wwww.foo.org:123",
        "http://user:[password]@wwww.foo.org:123",
        "http://user:{password}@wwww.foo.org:123",
        "http://wwww.;foo.org:123",
        "HTTP://user@foomail.org:password@wwww.foo.org:123",
        "http://user:<password>@wwww.foo.org:123",
        "http://user:pass,word@wwww.foo.org:123",
        "http://user:pass|word@wwww.foo.org:123",
        "http://10,0,10,10:8080",
        "http://10`0`10`10:8080",
        "http://proxyuser:password@10'0'0'2:8080"
    };

    int badOptionsSize = ARRAY_SIZE(badOptions);
    EXPECT_EQ(badOptionsSize, 33);

    for (int i = 0; i < validOptionsSize; i++)
    {
        EXPECT_TRUE(ParseHttpProxyData(validOptions[i].data, &hostAddress, &port, &username, &password, nullptr));
        EXPECT_STREQ(hostAddress, validOptions[i].hostAddress);
        EXPECT_EQ(port, validOptions[i].port);
        
        if (nullptr != validOptions[i].username)
        {
            EXPECT_STREQ(username, validOptions[i].username);
        }
        else 
        {
            EXPECT_EQ(nullptr, username);
        }

        if (nullptr != validOptions[i].password)
        {
            EXPECT_STREQ(password, validOptions[i].password);
        }
        else 
        {
            EXPECT_EQ(nullptr, password);
        }    

        if (nullptr != hostAddress)
        {
            free(hostAddress);
        }

        if (nullptr != username)
        {
            free(username);
        }

        if (nullptr != password)
        {
            free(password);
        }
    }

    for (int i = 0; i < badOptionsSize; i++)
    {
        EXPECT_FALSE(ParseHttpProxyData(badOptions[i], &hostAddress, &port, &username, &password, nullptr));

        if (nullptr != hostAddress)
        {
            free(hostAddress);
        }

        if (nullptr != username)
        {
            free(username);
        }

        if (nullptr != password)
        {
            free(password);
        }
    }
}