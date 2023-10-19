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
#include <UserUtils.h>

using namespace std;

#define STRFTIME_DATE_FORMAT "%Y%m%d"
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

struct ExecuteCommandOptions
{
    const char* command;
    bool replaceEol;
    bool forJson;
    unsigned int maxTextResultBytes;
    unsigned int timeoutSeconds;
    const char* expectedTextResult;
};

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResult)
{
    char* textResult = nullptr;

    ExecuteCommandOptions options[] = {
        { "echo test123", false, true, 0, 0, "test123\n" },
        { "echo test123", false, true, 0, 10, "test123\n" },
        { "echo test123", true, true, 0, 0, "test123 "},
        { "echo test123", false, true, 5, 0, "test"},
        { "echo test123", false, true, 1, 0, "" },
        { "echo test123", false, true, 8, 0, "test123" }
    };

    int optionsSize = ARRAY_SIZE(options);

    for (int i = 0; i < optionsSize; i++)
    {
        EXPECT_EQ(0, ExecuteCommand(nullptr, options[i].command, options[i].replaceEol, options[i].forJson, options[i].maxTextResultBytes, options[i].timeoutSeconds, &textResult, nullptr, nullptr));
        EXPECT_STREQ(textResult, options[i].expectedTextResult);
        FREE_MEMORY(textResult);
    }
}

struct ExecuteTwoCommandsOptions
{
    const char* command;
    bool replaceEol;
    bool forJson;
    unsigned int maxTextResultBytes;
    unsigned int timeoutSeconds;
    const char* expectedTextResultOne;
    const char* expectedTextResultTwo;
};

TEST_F(CommonUtilsTest, ExecuteMultipleCommandsAsOneCommandWithTextResult)
{
    char* textResult = nullptr;

    ExecuteTwoCommandsOptions options[] = {
        { "echo alpha; echo beta", true, true, 0, 0, "alpha ", "beta " },
        { "((echo alpha1); (echo beta1))", true, true, 0, 0, "alpha1 ", "beta1 " },
        { "((echo alpha12); echo beta12)", true, true, 0, 0, "alpha12 ", "beta12 " },
        { "echo alpha123 && echo beta123", true, true, 0, 0, "beta123 ", "alpha123 " },
        { "((echo alpha1234)&&(echo beta1234))", true, true, 0, 0, "beta1234 ", "alpha1234 " },
        { "((echo alpha12345) && echo beta12345)", true, true, 0, 0, "beta12345 ", "alpha12345 " },
        { "echo alpha123456 > null; echo beta123456", true, true, 0, 0, "beta123456 ", nullptr },
        { "echo alpha1234567 > null && echo beta1234567", true, true, 0, 0, "beta1234567 ", nullptr }
    };

    int optionsSize = ARRAY_SIZE(options);

    for (int i = 0; i < optionsSize; i++)
    {
        EXPECT_EQ(0, ExecuteCommand(nullptr, options[i].command, options[i].replaceEol, options[i].forJson, options[i].maxTextResultBytes, options[i].timeoutSeconds, &textResult, nullptr, nullptr));
        EXPECT_NE(nullptr, textResult);
        EXPECT_NE(nullptr, strstr(textResult, options[i].expectedTextResultOne));
        
        if (nullptr != options[i].expectedTextResultTwo)
        {
            EXPECT_NE(nullptr, strstr(textResult, options[i].expectedTextResultTwo));
            EXPECT_EQ(strlen(textResult), strlen(options[i].expectedTextResultOne) + strlen(options[i].expectedTextResultTwo));
        }
        else
        {
            EXPECT_EQ(strlen(textResult), strlen(options[i].expectedTextResultOne));
        }

        FREE_MEMORY(textResult);
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

    FREE_MEMORY(textResult);
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

    FREE_MEMORY(textResult);
}

void* TestTimeoutCommand(void*)
{
    char* textResult = nullptr;

    EXPECT_EQ(ETIME, ExecuteCommand(nullptr, "sleep 10", false, true, 0, 1, &textResult, nullptr, nullptr));

    FREE_MEMORY(textResult);

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

    FREE_MEMORY(textResult);
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

    FREE_MEMORY(textResult);

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

    FREE_MEMORY(textResult);
}

void* TestCancelCommandWithContext(void*)
{
    CallbackContext context;

    char* textResult = nullptr;

    EXPECT_EQ(ECANCELED, ExecuteCommand((void*)(&context), "sleep 30", false, true, 0, 120, &textResult, &(CallbackContext::TestCommandCallback), nullptr));

    FREE_MEMORY(textResult);

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

    FREE_MEMORY(textResult);
}

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResultWithAllCharacters)
{
    char* textResult = nullptr;

    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo 'abc\"123'", true, false, 0, 0, &textResult, nullptr, nullptr));
    EXPECT_STREQ("abc\"123 ", textResult);

    FREE_MEMORY(textResult);
}

TEST_F(CommonUtilsTest, ExecuteCommandWithTextResultWithMappedJsonCharacters)
{
    char* textResult = nullptr;

    EXPECT_EQ(0, ExecuteCommand(nullptr, "echo 'abc\"123'", true, true, 0, 0, &textResult, nullptr, nullptr));
    EXPECT_STREQ("abc 123 ", textResult);

    FREE_MEMORY(textResult);
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

    FREE_MEMORY(command);
    FREE_MEMORY(expectedResult);
    FREE_MEMORY(textResult);
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

    FREE_MEMORY(textResult);
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

TEST_F(CommonUtilsTest, CheckFileExists)
{
    EXPECT_TRUE(CreateTestFile(m_path, m_data));
    EXPECT_EQ(0, CheckFileExists(m_path, nullptr));
    EXPECT_TRUE(Cleanup(m_path));
    EXPECT_EQ(EEXIST, CheckFileExists(m_path, nullptr));
    EXPECT_EQ(EEXIST, CheckFileExists("This file does not exist", nullptr));
}

TEST_F(CommonUtilsTest, DirectoryExists)
{
    EXPECT_TRUE(CreateTestFile(m_path, m_data));
    EXPECT_FALSE(DirectoryExists(m_path));
    EXPECT_TRUE(Cleanup(m_path));
    EXPECT_FALSE(DirectoryExists(m_path));
    EXPECT_FALSE(DirectoryExists("This directory does not exist"));
    EXPECT_TRUE(DirectoryExists("/etc"));
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
    const char* sscanfDateFormat = "%4d%2d%2d";

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
    sscanf(dateNow, sscanfDateFormat, &yearNow, &monthNow, &dayNow);

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
    char* cpuFlags = NULL;
    long totalMemory = 0;
    long freeMemory = 0;
    char* kernelName = NULL;
    char* kernelVersion = NULL;
    char* kernelRelease = NULL;
    char* umask = NULL;

    EXPECT_NE(nullptr, osName = GetOsName(nullptr));
    EXPECT_NE(nullptr, osVersion = GetOsVersion(nullptr));
    EXPECT_NE(nullptr, cpuType = GetCpuType(nullptr));
    EXPECT_NE(nullptr, cpuVendor = GetCpuType(nullptr));
    EXPECT_NE(nullptr, cpuModel = GetCpuType(nullptr));
    EXPECT_NE(nullptr, cpuFlags = GetCpuFlags(nullptr));
    EXPECT_EQ(true, IsCpuFlagSupported("fpu", nullptr));
    EXPECT_EQ(false, IsCpuFlagSupported("test123", nullptr));
    EXPECT_EQ(false, IsCpuFlagSupported(nullptr, nullptr));
    EXPECT_NE(0, totalMemory = GetTotalMemory(nullptr));
    EXPECT_NE(0, freeMemory = GetTotalMemory(nullptr));
    EXPECT_NE(nullptr, kernelName = GetOsKernelName(nullptr));
    EXPECT_NE(nullptr, kernelVersion = GetOsKernelVersion(nullptr));
    EXPECT_NE(nullptr, kernelRelease = GetOsKernelRelease(nullptr));
    EXPECT_NE(nullptr, umask = GetLoginUmask(nullptr));

    FREE_MEMORY(osName);
    FREE_MEMORY(osVersion);
    FREE_MEMORY(cpuType);
    FREE_MEMORY(cpuVendor);
    FREE_MEMORY(cpuModel);
    FREE_MEMORY(cpuFlags);
    FREE_MEMORY(kernelName);
    FREE_MEMORY(kernelVersion);
    FREE_MEMORY(kernelRelease);
    FREE_MEMORY(umask);
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

TEST_F(CommonUtilsTest, FormatAllocateString)
{
    char* formattted = nullptr;
    EXPECT_EQ(nullptr, formattted = FormatAllocateString(nullptr));
    EXPECT_STREQ(m_data, formattted = FormatAllocateString(m_data));
    FREE_MEMORY(formattted);
    EXPECT_STREQ("Test ABC 123", formattted = FormatAllocateString("Test %s %d", "ABC", 123));
    FREE_MEMORY(formattted);
}

TEST_F(CommonUtilsTest, DuplicateAndFormatAllocateString)
{
    char* reason = NULL;
    char* temp = NULL;

    EXPECT_NE(nullptr, reason = FormatAllocateString("'%s': %d", "Test 123", 456));
    EXPECT_STREQ(reason, "'Test 123': 456");
    EXPECT_NE(nullptr, temp = DuplicateString(reason));
    EXPECT_STREQ(temp, reason);
    FREE_MEMORY(reason);
    EXPECT_NE(nullptr, reason = FormatAllocateString("%s, and also '%s' is %d", temp, "Test B", 789));
    FREE_MEMORY(temp);
    EXPECT_STREQ(reason, "'Test 123': 456, and also 'Test B' is 789");
    FREE_MEMORY(reason);
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

TEST_F(CommonUtilsTest, MillisecondsSleep)
{
    long validValue = 100;
    long negativeValue = -100;
    long tooBigValue = 999999999 + 1;

    EXPECT_EQ(0, SleepMilliseconds(validValue));
    EXPECT_EQ(EINVAL, SleepMilliseconds(negativeValue));
    EXPECT_EQ(EINVAL, SleepMilliseconds(tooBigValue));
}

TEST_F(CommonUtilsTest, LoadConfiguration)
{
    const char* configuration = 
        "{"
          "\"CommandLogging\": 0,"
          "\"FullLogging\": 1,"
          "\"GitManagement\": 1,"
          "\"GitRepositoryUrl\": \"https://USERNAME:PASSWORD@github.com/Azure/azure-osconfig\","
          "\"GitBranch\": \"foo/test\","
          "\"LocalManagement\": 3,"
          "\"ModelVersion\": 11,"
          "\"IotHubProtocol\": 2,"
          "\"Reported\": ["
          "  {"
          "    \"ComponentName\": \"DeviceInfo\","
          "    \"ObjectName\": \"osName\""
          "  },"
          "  {"
          "    \"ComponentName\": \"TestABC\","
          "    \"ObjectName\": \"TestVa12lue\""
          "  }"
          "],"
          "\"ReportingIntervalSeconds\": 30"
        "}";

    REPORTED_PROPERTY* reportedProperties = nullptr;

    char* value = nullptr;
    
    EXPECT_FALSE(IsCommandLoggingEnabledInJsonConfig(configuration));
    EXPECT_TRUE(IsFullLoggingEnabledInJsonConfig(configuration));
    EXPECT_EQ(30, GetReportingIntervalFromJsonConfig(configuration, nullptr));
    EXPECT_EQ(11, GetModelVersionFromJsonConfig(configuration, nullptr));
    EXPECT_EQ(2, GetIotHubProtocolFromJsonConfig(configuration, nullptr));

    // The value of 3 is too big, shall be changed to 1
    EXPECT_EQ(1, GetLocalManagementFromJsonConfig(configuration, nullptr));

    EXPECT_EQ(2, LoadReportedFromJsonConfig(configuration, &reportedProperties, nullptr));
    EXPECT_STREQ("DeviceInfo", reportedProperties[0].componentName);
    EXPECT_STREQ("osName", reportedProperties[0].propertyName);
    EXPECT_STREQ("TestABC", reportedProperties[1].componentName);
    EXPECT_STREQ("TestVa12lue", reportedProperties[1].propertyName);

    EXPECT_EQ(1, GetGitManagementFromJsonConfig(configuration, nullptr));

    EXPECT_STREQ("https://USERNAME:PASSWORD@github.com/Azure/azure-osconfig", value = GetGitRepositoryUrlFromJsonConfig(configuration, nullptr));
    FREE_MEMORY(value);

    EXPECT_STREQ("foo/test", value = GetGitBranchFromJsonConfig(configuration, nullptr));
    FREE_MEMORY(value);

    FREE_MEMORY(reportedProperties);
}

TEST_F(CommonUtilsTest, SetAndCheckFileAccess)
{
    unsigned int testModes[] = { 0, 600, 601, 640, 644, 650, 700, 710, 750, 777 };
    int numTestModes = ARRAY_SIZE(testModes);
    
    EXPECT_TRUE(CreateTestFile(m_path, m_data));
    for (int i = 0; i < numTestModes; i++)
    {
        EXPECT_EQ(0, SetFileAccess(m_path, 0, 0, testModes[i], nullptr));
        EXPECT_EQ(0, CheckFileAccess(m_path, 0, 0, testModes[i], nullptr, nullptr));
    }

    EXPECT_TRUE(Cleanup(m_path));
    
    EXPECT_EQ(EINVAL, SetFileAccess(nullptr, 0, 0, 777, nullptr));
    EXPECT_EQ(EINVAL, CheckFileAccess(nullptr, 0, 0, 777, nullptr, nullptr));
}

TEST_F(CommonUtilsTest, SetAndCheckDirectoryAccess)
{
    unsigned int testModes[] = { 0, 600, 601, 640, 644, 650, 700, 710, 750, 777 };
    int numTestModes = ARRAY_SIZE(testModes);

    EXPECT_EQ(0, ExecuteCommand(nullptr, "mkdir ~test", false, false, 0, 0, nullptr, nullptr, nullptr));
    for (int i = 0; i < numTestModes; i++)
    {
        EXPECT_EQ(0, SetDirectoryAccess("~test", 0, 0, testModes[i], nullptr));
        EXPECT_EQ(0, CheckDirectoryAccess("~test", 0, 0, testModes[i], false, nullptr, nullptr));
    }
    EXPECT_EQ(0, ExecuteCommand(nullptr, "rm -r ~test", false, false, 0, 0, nullptr, nullptr, nullptr));

    EXPECT_EQ(EINVAL, SetDirectoryAccess(nullptr, 0, 0, 777, nullptr));
    EXPECT_EQ(EINVAL, CheckDirectoryAccess(nullptr, 0, 0, 777, false, nullptr, nullptr));
}

TEST_F(CommonUtilsTest, CheckFileSystemMountingOption)
{
    const char* testFstab = 
        "# /etc/fstab: static file system information.\n"
        "#\n"
        "# Use 'blkid' to print the universally unique identifier for a\n"
        "# device; this may be used with UUID= as a more robust way to name devices\n"
        "# that works even if disks are added and removed. See fstab(5).\n"
        "#\n"
        "# <file system> <mount point>   <type>  <options>       <dump>  <pass>\n"
        "# / was on /dev/sda1 during installation\n"
        "Test2 /home/test/home               ext6    noauto,123            0                 0\n"
        "/dev/scd1  /media/dvdrom0  udf,iso9660  user,noauto,noexec,utf8  0  0\n"
        "UUID=test123 /test/media/home               ext6,iso9660    123,noexec            0                 0\n"
        "UUID=blah /test/root               ext6    123            0       0\n"
        "UUID=62fd6763-f758-4417-98be-0cf4d82d5c1b /               ext4    errors=remount-ro 0       0\n"
        "# / was on /dev/sda5 during installation\n"
        "        UUID = b65913ad - db15 - 4ba0 - 8c8b - d24f6fcb2825 / ext4    errors=remount-ro 0       0\n"
        "# /boot/efi was on /dev/sda1 during installation\n"
        "UUID=6BBC-4153/boot/efi       vfat    umask=0077,nosuid      0       1\n"
        "/dev/scd0  /media/cdrom0  udf,iso9660  user,noauto,noexec,utf8,nosuid  0  0\n"
        "/swapfile                                 none            swap    sw              0       0";

    EXPECT_TRUE(CreateTestFile(m_path, testFstab));

    EXPECT_EQ(EINVAL, CheckFileSystemMountingOption(m_path, "none", "swap", nullptr, nullptr, nullptr));
    EXPECT_EQ(EINVAL, CheckFileSystemMountingOption(m_path, "none", nullptr, nullptr, nullptr, nullptr));
    EXPECT_EQ(EINVAL, CheckFileSystemMountingOption(nullptr, "none", "swap", "sw", nullptr, nullptr));

    EXPECT_EQ(ENOENT, CheckFileSystemMountingOption(m_path, "none", "swap", "does_not_exist", nullptr, nullptr));
    EXPECT_EQ(ENOENT, CheckFileSystemMountingOption(m_path, "none", nullptr, "this_neither", nullptr, nullptr));
    EXPECT_EQ(ENOENT, CheckFileSystemMountingOption(m_path, nullptr, "swap", "also_not_this", nullptr, nullptr));
    EXPECT_EQ(ENOENT, CheckFileSystemMountingOption(m_path, "doesnotexist", nullptr, "doesnotexist", nullptr, nullptr));
    EXPECT_EQ(ENOENT, CheckFileSystemMountingOption(m_path, nullptr, "doesnotexist", "doesnotexist", nullptr, nullptr));

    // The requested option is present in all matching mounting points
    EXPECT_EQ(0, CheckFileSystemMountingOption(m_path, "/media", "udf", "noexec", nullptr, nullptr));
    EXPECT_EQ(0, CheckFileSystemMountingOption(m_path, nullptr, "udf", "noexec", nullptr, nullptr));
    EXPECT_EQ(0, CheckFileSystemMountingOption(m_path, "/media", nullptr, "noexec", nullptr, nullptr));

    // The requested option is missing from one of the matching mounting points
    EXPECT_EQ(ENOENT, CheckFileSystemMountingOption(m_path, "/media", "iso9660", "noauto", nullptr, nullptr));
    EXPECT_EQ(ENOENT, CheckFileSystemMountingOption(m_path, nullptr, "iso9660", "noauto", nullptr, nullptr));
    EXPECT_EQ(ENOENT, CheckFileSystemMountingOption(m_path, "/media", nullptr, "noauto", nullptr, nullptr));

    // The requested option is present in all matching mounting points
    EXPECT_EQ(0, CheckFileSystemMountingOption(m_path, "/test", "ext6", "123", nullptr, nullptr));
    EXPECT_EQ(0, CheckFileSystemMountingOption(m_path, nullptr, "ext6", "123", nullptr, nullptr));
    EXPECT_EQ(0, CheckFileSystemMountingOption(m_path, "/test", nullptr, "123", nullptr, nullptr));
    EXPECT_EQ(0, CheckFileSystemMountingOption(m_path, "/test/root", nullptr, "123", nullptr, nullptr));
    EXPECT_EQ(0, CheckFileSystemMountingOption(m_path, "/root", nullptr, "123", nullptr, nullptr));

    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, CheckInstallUninstallPackage)
{
    EXPECT_EQ(EINVAL, CheckPackageInstalled(nullptr, nullptr));
    EXPECT_EQ(EINVAL, InstallPackage(nullptr, nullptr));
    EXPECT_EQ(EINVAL, UninstallPackage(nullptr, nullptr));

    EXPECT_EQ(EINVAL, CheckPackageInstalled("", nullptr));
    EXPECT_EQ(EINVAL, InstallPackage("", nullptr));
    EXPECT_EQ(EINVAL, UninstallPackage("", nullptr));

    EXPECT_NE(0, CheckPackageInstalled("~package_that_does_not_exist", nullptr));
    EXPECT_NE(0, InstallPackage("~package_that_does_not_exist", nullptr));
    
    // Nothing to uninstall
    EXPECT_EQ(0, UninstallPackage("~package_that_does_not_exist", nullptr));

    EXPECT_NE(0, CheckPackageInstalled("*~package_that_does_not_exist", nullptr));
    EXPECT_NE(0, CheckPackageInstalled("~package_that_does_not_exist*", nullptr));
    EXPECT_NE(0, CheckPackageInstalled("*~package_that_does_not_exist*", nullptr));
    
    EXPECT_EQ(0, CheckPackageInstalled("apt", nullptr));
    EXPECT_EQ(0, CheckPackageInstalled("ap*", nullptr));

    EXPECT_EQ(0, InstallPackage("rolldice", nullptr));
    EXPECT_EQ(0, CheckPackageInstalled("rolldice", nullptr));
    EXPECT_EQ(0, UninstallPackage("rolldice", nullptr));
}

TEST_F(CommonUtilsTest, GetNumberOfLinesInFile)
{
    EXPECT_EQ(0, GetNumberOfLinesInFile(nullptr));
    EXPECT_EQ(0, GetNumberOfLinesInFile("~file_that_does_not_exist"));
    EXPECT_EQ(GetNumberOfLinesInFile("/etc/passwd"), GetNumberOfLinesInFile("/etc/shadow"));
}

TEST_F(CommonUtilsTest, CharacterFoundInFile)
{
    EXPECT_FALSE(CharacterFoundInFile(nullptr, 0));
    EXPECT_FALSE(CharacterFoundInFile("~file_that_does_not_exist", 0));
    EXPECT_TRUE(CharacterFoundInFile("/etc/passwd", ':'));
}

TEST_F(CommonUtilsTest, EnumerateUsersAndTheirGroups)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;

    struct SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;

    EXPECT_EQ(0, EnumerateUsers(&userList, &userListSize, nullptr));
    EXPECT_EQ(userListSize, GetNumberOfLinesInFile("/etc/passwd"));
    EXPECT_NE(nullptr, userList);

    for (unsigned int i = 0; i < userListSize; i++)
    {
        EXPECT_NE(nullptr, userList[i].username);
        
        EXPECT_EQ(0, EnumerateUserGroups(&userList[i], &groupList, &groupListSize, nullptr));
        EXPECT_NE(nullptr, groupList);
        
        for (unsigned int j = 0; j < groupListSize; j++)
        {
            EXPECT_NE(nullptr, groupList[j].groupName);
        }
        
        FreeGroupList(&groupList, groupListSize);
        EXPECT_EQ(nullptr, groupList);
    }
    
    FreeUsersList(&userList, userListSize);
    EXPECT_EQ(nullptr, userList);
}

TEST_F(CommonUtilsTest, EnumerateAllGroups)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;

    EXPECT_EQ(0, EnumerateAllGroups(&groupList, &groupListSize, nullptr));
    EXPECT_EQ(groupListSize, GetNumberOfLinesInFile("/etc/group"));
    EXPECT_NE(nullptr, groupList);

    for (unsigned int i = 0; i < groupListSize; i++)
    {
        EXPECT_NE(nullptr, groupList[i].groupName);
    }

    FreeGroupList(&groupList, groupListSize);
    EXPECT_EQ(nullptr, groupList);
}

TEST_F(CommonUtilsTest, CheckUsersAndGroups)
{
    EXPECT_EQ(0, CheckAllEtcPasswdGroupsExistInEtcGroup(nullptr, nullptr));
    EXPECT_EQ(0, CheckSystemAccountsAreNonLogin(nullptr, nullptr));
    EXPECT_EQ(0, CheckShadowGroupIsEmpty(nullptr, nullptr));
}

TEST_F(CommonUtilsTest, CheckNoDuplicateUsersGroups)
{
    EXPECT_EQ(0, CheckNoDuplicateUidsExist(nullptr, nullptr));
    EXPECT_EQ(0, CheckNoDuplicateGidsExist(nullptr, nullptr));
    EXPECT_EQ(0, CheckNoDuplicateUserNamesExist(nullptr, nullptr));
    EXPECT_EQ(0, CheckNoDuplicateGroupsExist(nullptr, nullptr));
}

TEST_F(CommonUtilsTest, CheckNoPlusEntriesInFile)
{
    const char* testPath = "~plusentries";
    
    const char* goodTestFileContents[] = {
        "hplip:*:18858:0:99999:7:::\nwhoopsie:*:18858:0:99999:7:::\ncolord:*:18858:0:99999:7:::\ngeoclue:*:18858:0:99999:7:::",
        "gnats:x:41:41:Gnats Bug-Reporting System (admin):/var/lib/gnats:/usr/sbin/nologin\nnobody:x:65534:65534:nobody:/nonexistent:/usr/sbin/nologin",
        "whoopsie:x:127:\ncolord:x:128:\ngeoclue:x:129:"
    };

    const char* badTestFileContents[] = {
        "+:!:::::::\nhplip:*:18858:0:99999:7:::\nwhoopsie:*:18858:0:99999:7:::\ncolord:*:18858:0:99999:7:::\ngeoclue:*:18858:0:99999:7:::",
        "gnats:x:41:41:Gnats Bug-Reporting System (admin):/var/lib/gnats:/usr/sbin/nologin\n+:!:::::::\nnobody:x:65534:65534:nobody:/nonexistent:/usr/sbin/nologin",
        "whoopsie:x:127:\ncolord:x:128:\n+:!:::::::\ngeoclue:x:129:",
        "hplip:*:18858:0:99999:7:::\nwhoopsie:*:18858:0:99999:7:::\ncolord:*:18858:0:99999:7:::\ngeoclue:*:18858:0:99999:7:::+",
        "gnats:x:41:41:Gnats Bug-Reporting System (admin):/var/lib/gnats:/usr/sbin/nologin\nnobody:x:65+534:65534:nobody:/nonexistent:/usr/sbin/nologin",
        "whoo+++psie:x:127:\ncolord:x:128:\ngeoclue:x:129:"
    };

    int goodTestFileContentsSize = ARRAY_SIZE(goodTestFileContents);
    int badTestFileContentsSize = ARRAY_SIZE(badTestFileContents);

    int i = 0;

    for (i = 0; i < goodTestFileContentsSize; i++)
    {
        EXPECT_TRUE(CreateTestFile(testPath, goodTestFileContents[i]));
        EXPECT_EQ(0, CheckNoLegacyPlusEntriesInFile(testPath, nullptr));
        EXPECT_TRUE(Cleanup(testPath));
    }

    for (i = 0; i < badTestFileContentsSize; i++)
    {
        EXPECT_TRUE(CreateTestFile(testPath, badTestFileContents[i]));
        EXPECT_NE(0, CheckNoLegacyPlusEntriesInFile(testPath, nullptr));
        EXPECT_TRUE(Cleanup(testPath));
    }
}

TEST_F(CommonUtilsTest, CheckRootUserAndGroup)
{
    EXPECT_EQ(0, CheckRootGroupExists(nullptr, nullptr));
    EXPECT_EQ(0, CheckDefaultRootAccountGroupIsGidZero(nullptr, nullptr));
    EXPECT_EQ(0, CheckRootIsOnlyUidZeroAccount(nullptr, nullptr));
    
    // Optional:
    CheckRootPasswordForSingleUserMode(nullptr, nullptr);
}

TEST_F(CommonUtilsTest, CheckUsersHavePasswords)
{
    // Optional:
    CheckAllUsersHavePasswordsSet(nullptr, nullptr);
    CheckUsersRecordedPasswordChangeDates(nullptr, nullptr);
    CheckMinDaysBetweenPasswordChanges(0, nullptr, nullptr);
    CheckMaxDaysBetweenPasswordChanges(99999, nullptr, nullptr);
    CheckPasswordExpirationWarning(0, nullptr, nullptr);
    CheckPasswordExpirationLessThan(99999, nullptr, nullptr);
}

TEST_F(CommonUtilsTest, CheckUserHomeDirectories)
{
    EXPECT_EQ(0, CheckAllUsersHomeDirectoriesExist(nullptr, nullptr));
    
    //Optional:
    CheckUsersOwnTheirHomeDirectories(nullptr, nullptr);
}

TEST_F(CommonUtilsTest, CheckOrEnsureUsersDontHaveDotFiles)
{
    EXPECT_EQ(EINVAL, CheckOrEnsureUsersDontHaveDotFiles(nullptr, false, nullptr, nullptr));
    EXPECT_EQ(0, CheckOrEnsureUsersDontHaveDotFiles("foo", false, nullptr, nullptr));
    EXPECT_EQ(0, CheckOrEnsureUsersDontHaveDotFiles("blah", false, nullptr, nullptr));
    EXPECT_EQ(0, CheckOrEnsureUsersDontHaveDotFiles("test123", false, nullptr, nullptr));
    EXPECT_EQ(0, CheckOrEnsureUsersDontHaveDotFiles("foo", true, nullptr, nullptr));
    EXPECT_EQ(0, CheckOrEnsureUsersDontHaveDotFiles("blah", true, nullptr, nullptr));
    EXPECT_EQ(0, CheckOrEnsureUsersDontHaveDotFiles("test123", true, nullptr, nullptr));
}

TEST_F(CommonUtilsTest, FindTextInFile)
{
    const char* test = "This is a text with options /1 /2 \\3 \\zoo -34!";

    EXPECT_TRUE(CreateTestFile(m_path, test));

    EXPECT_EQ(EINVAL, FindTextInFile(nullptr, test, nullptr));
    EXPECT_EQ(EINVAL, FindTextInFile(m_path, "", nullptr));
    EXPECT_EQ(EINVAL, FindTextInFile(m_path, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindTextInFile(nullptr, nullptr, nullptr));

    EXPECT_EQ(ENOENT, FindTextInFile("~~DoesNotExist", "text", nullptr));
    
    EXPECT_EQ(0, FindTextInFile(m_path, "text", nullptr));
    EXPECT_EQ(0, FindTextInFile(m_path, "/1", nullptr));
    EXPECT_EQ(0, FindTextInFile(m_path, "\\3", nullptr));
    EXPECT_EQ(0, FindTextInFile(m_path, "\\z", nullptr));
    EXPECT_EQ(0, FindTextInFile(m_path, "34", nullptr));
    EXPECT_NE(0, FindTextInFile(m_path, "not found", nullptr));
    EXPECT_NE(0, FindTextInFile(m_path, "\\m", nullptr));

    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, FindMarkedTextInFile)
{
    const char* test = "Test \n FOO=test:/123:!abcdef.123:/test.d TEST1; TEST2/..TEST3:Blah=0";

    EXPECT_TRUE(CreateTestFile(m_path, test));

    EXPECT_EQ(EINVAL, FindMarkedTextInFile(nullptr, nullptr, nullptr, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindMarkedTextInFile(m_path, nullptr, nullptr, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindMarkedTextInFile(m_path, "FOO", nullptr, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindMarkedTextInFile(m_path, nullptr, ";", nullptr, nullptr));

    EXPECT_EQ(EINVAL, FindMarkedTextInFile(m_path, "", "", nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindMarkedTextInFile(m_path, "FOO", "", nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindMarkedTextInFile(m_path, "", ";", nullptr, nullptr));

    EXPECT_EQ(EINVAL, FindMarkedTextInFile("~~DoesNotExist", "FOO", ";", nullptr, nullptr));

    EXPECT_EQ(0, FindMarkedTextInFile(m_path, "FOO", ".", nullptr, nullptr));
    
    EXPECT_EQ(ENOENT, FindMarkedTextInFile(m_path, "FOO", "!", nullptr, nullptr));
    
    EXPECT_EQ(0, FindMarkedTextInFile(m_path, "FOO", ";", nullptr, nullptr));
    EXPECT_EQ(0, FindMarkedTextInFile(m_path, "FOO", "..", nullptr, nullptr));

    EXPECT_EQ(0, FindMarkedTextInFile(m_path, "TEST1", ";", nullptr, nullptr));
    EXPECT_EQ(0, FindMarkedTextInFile(m_path, "TEST1", ".", nullptr, nullptr));
    EXPECT_EQ(0, FindMarkedTextInFile(m_path, "TEST1", "..", nullptr, nullptr));

    EXPECT_EQ(0, FindMarkedTextInFile(m_path, "TEST2", ".", nullptr, nullptr));
    EXPECT_EQ(0, FindMarkedTextInFile(m_path, "TEST2", "..", nullptr, nullptr));

    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, FindTextInEnvironmentVariable)
{
    EXPECT_EQ(EINVAL, FindTextInEnvironmentVariable(nullptr, "/", false, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindTextInEnvironmentVariable("PATH", "", false, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindTextInEnvironmentVariable("", "/", false, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindTextInEnvironmentVariable("PATH", nullptr, false, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindTextInEnvironmentVariable(nullptr, nullptr, false, nullptr, nullptr));

    EXPECT_EQ(0, FindTextInEnvironmentVariable("PATH", ":", false, nullptr, nullptr));

    EXPECT_EQ(0, setenv("TESTVAR", "0", 1));
    EXPECT_EQ(0, FindTextInEnvironmentVariable("TESTVAR", "0", false, nullptr, nullptr));
    EXPECT_EQ(0, FindTextInEnvironmentVariable("TESTVAR", "0 ", true, nullptr, nullptr));
    EXPECT_EQ(ENOENT, FindTextInEnvironmentVariable("TESTVAR", "1", true, nullptr, nullptr));
    EXPECT_EQ(0, unsetenv("TESTVAR"));
}

TEST_F(CommonUtilsTest, CompareFileContents)
{
    EXPECT_EQ(EINVAL, CompareFileContents(nullptr, "2", nullptr));
    EXPECT_EQ(EINVAL, CompareFileContents(nullptr, nullptr, nullptr));
    EXPECT_EQ(EINVAL, CompareFileContents(m_path, nullptr, nullptr));
    EXPECT_EQ(EINVAL, CompareFileContents(m_path, "", nullptr));
    EXPECT_EQ(EINVAL, CompareFileContents("", "2", nullptr));

    const char* test[] = {"2", "ABC", "~1", "This is a test", "One line\nAnd another\n"};
    size_t sizeOfTest = ARRAY_SIZE(test);

    for (size_t i = 0; i < sizeOfTest; i++)
    {
        EXPECT_TRUE(CreateTestFile(m_path, test[i]));
        EXPECT_EQ(0, CompareFileContents(m_path, test[i], nullptr));
        EXPECT_TRUE(Cleanup(m_path));
    }
}

TEST_F(CommonUtilsTest, OtherOptionalTests)
{
    CheckOsAndKernelMatchDistro(nullptr, nullptr);
}

TEST_F(CommonUtilsTest, FindTextInFolder)
{
    EXPECT_EQ(EINVAL, FindTextInFolder(nullptr, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindTextInFolder(nullptr, "a", nullptr));
    EXPECT_EQ(EINVAL, FindTextInFolder("/etc", nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindTextInFolder("/foo/does_not_exist", "test", nullptr));
    
    FindTextInFolder("/etc/modprobe.d", "ac97", nullptr);
}

TEST_F(CommonUtilsTest, CheckLineNotFoundOrCommentedOut)
{
    const char* testFile =
        "# Test 123 commented\n"
        " Test 123 uncommented\n"
        "345 Test 345 Test # 345 Test\n"
        "ABC!DEF # Test 678 1234567890\n"
        "   ##           Foo    \n"
        "   #  Example of a line    \n"
        "          Example of a line    \n"
        "   ! @  Blah 3    \n";


    EXPECT_TRUE(CreateTestFile(m_path, testFile));

    EXPECT_EQ(EINVAL, CheckLineNotFoundOrCommentedOut(m_path, '#', nullptr, nullptr));
    EXPECT_EQ(EINVAL, CheckLineNotFoundOrCommentedOut(nullptr, '#', "test", nullptr));
    EXPECT_EQ(EINVAL, CheckLineNotFoundOrCommentedOut(nullptr, '#', nullptr, nullptr));

    EXPECT_EQ(0, CheckLineNotFoundOrCommentedOut("/foo/does_not_exist", '#', "Test", nullptr));

    EXPECT_EQ(0, CheckLineNotFoundOrCommentedOut(m_path, '#', "does-not__exist123", nullptr));
    EXPECT_EQ(0, CheckLineNotFoundOrCommentedOut(m_path, '#', "9876543210", nullptr));
    EXPECT_EQ(0, CheckLineNotFoundOrCommentedOut(m_path, '#', "Test 123 not really commented", nullptr));

    EXPECT_EQ(0, CheckLineNotFoundOrCommentedOut(m_path, '#', "Test 123 commented", nullptr));
    EXPECT_EQ(EEXIST, CheckLineNotFoundOrCommentedOut(m_path, '#', "Test 123", nullptr));
    EXPECT_EQ(EEXIST, CheckLineNotFoundOrCommentedOut(m_path, '#', "Test 123 uncommented", nullptr));
    
    EXPECT_EQ(EEXIST, CheckLineNotFoundOrCommentedOut(m_path, '#', "345 Test 345 Test # 345 Test", nullptr));
    EXPECT_EQ(EEXIST, CheckLineNotFoundOrCommentedOut(m_path, '#', "345 Test", nullptr));
    EXPECT_EQ(EEXIST, CheckLineNotFoundOrCommentedOut(m_path, '#', "345", nullptr));
    
    EXPECT_EQ(EEXIST, CheckLineNotFoundOrCommentedOut(m_path, '#', "ABC!DEF # Test 678 1234567890", nullptr));
    EXPECT_EQ(0, CheckLineNotFoundOrCommentedOut(m_path, '#', "Test 678 1234567890", nullptr));
    
    EXPECT_EQ(0, CheckLineNotFoundOrCommentedOut(m_path, '#', "Foo", nullptr));
    
    EXPECT_EQ(EEXIST, CheckLineNotFoundOrCommentedOut(m_path, '#', "Example of a line", nullptr));
    EXPECT_EQ(EEXIST, CheckLineNotFoundOrCommentedOut(m_path, '#', "Example", nullptr));
    EXPECT_EQ(EEXIST, CheckLineNotFoundOrCommentedOut(m_path, '#', " of a ", nullptr));
    
    EXPECT_EQ(0, CheckLineNotFoundOrCommentedOut(m_path, '@', "Blah 3", nullptr));
    EXPECT_EQ(0, CheckLineNotFoundOrCommentedOut(m_path, '!', "Blah 3", nullptr));
    EXPECT_EQ(EEXIST, CheckLineNotFoundOrCommentedOut(m_path, '#', "Blah 3", nullptr));

    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, FindTextInCommandOutput)
{
    EXPECT_EQ(EINVAL, FindTextInCommandOutput(nullptr, nullptr, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindTextInCommandOutput("echo Test123", nullptr, nullptr, nullptr));
    EXPECT_EQ(EINVAL, FindTextInCommandOutput(nullptr, "Test", nullptr, nullptr));
    EXPECT_NE(0, FindTextInCommandOutput("echo Test", "~does_not_exist", nullptr, nullptr));
    EXPECT_NE(0, FindTextInCommandOutput("blah", "Test", nullptr, nullptr));
    EXPECT_EQ(0, FindTextInCommandOutput("echo Test123", "Test", nullptr, nullptr));
    EXPECT_EQ(0, FindTextInCommandOutput("echo Test123", "123", nullptr, nullptr));
    EXPECT_EQ(0, FindTextInCommandOutput("echo Test123", "2", nullptr, nullptr));
}

TEST_F(CommonUtilsTest, GetOptionFromFile)
{
    const char* testFile =
        "FooEntry1:test\n"
        " Test1=abc foo=123  \n"
        "FooEntry2  =     234\n"
        "FooEntry3 :     2 3 4\n"
        "abc Test4 0456 # rt 4 $"
        "Test2:     12 $!    test test\n"
        "password [success=1 default=ignore] pam_unix.so obscure sha512 remember=5\n"
        "password [success=1 default=ignore] pam_unix.so obscure sha512 remembering   = -1";
    
    char* value = nullptr;

    EXPECT_TRUE(CreateTestFile(m_path, testFile));

    EXPECT_EQ(nullptr, GetStringOptionFromFile(nullptr, nullptr, ':', nullptr));
    EXPECT_EQ(-999, GetIntegerOptionFromFile(nullptr, nullptr, ':', nullptr));
    EXPECT_EQ(nullptr, GetStringOptionFromFile(m_path, nullptr, ':', nullptr));
    EXPECT_EQ(-999, GetIntegerOptionFromFile(m_path, nullptr, ':', nullptr));
    EXPECT_EQ(nullptr, GetStringOptionFromFile(nullptr, "Test1", ':', nullptr));
    EXPECT_EQ(-999, GetIntegerOptionFromFile(nullptr, "Test1", ':', nullptr));
    EXPECT_EQ(nullptr, GetStringOptionFromFile("~does_not_exist", "Test", '=', nullptr));
    EXPECT_EQ(-999, GetIntegerOptionFromFile("~does_not_exist", "Test", '=', nullptr));
    
    EXPECT_STREQ("test", value = GetStringOptionFromFile(m_path, "FooEntry1:", ':', nullptr));
    FREE_MEMORY(value);
    EXPECT_STREQ("test", value = GetStringOptionFromFile(m_path, "FooEntry1", ':', nullptr));
    FREE_MEMORY(value);

    EXPECT_STREQ("abc", value = GetStringOptionFromFile(m_path, "Test1=", '=', nullptr));
    FREE_MEMORY(value);
    EXPECT_STREQ("abc", value = GetStringOptionFromFile(m_path, "Test1", '=', nullptr));
    FREE_MEMORY(value);

    EXPECT_STREQ("234", value = GetStringOptionFromFile(m_path, "FooEntry2", '=', nullptr));
    FREE_MEMORY(value);
    EXPECT_EQ(234, GetIntegerOptionFromFile(m_path, "FooEntry2", '=', nullptr));

    EXPECT_STREQ("2", value = GetStringOptionFromFile(m_path, "FooEntry3 :", ':', nullptr));
    FREE_MEMORY(value);
    EXPECT_STREQ("2", value = GetStringOptionFromFile(m_path, "FooEntry3", ':', nullptr));
    FREE_MEMORY(value);
    EXPECT_EQ(2, GetIntegerOptionFromFile(m_path, "FooEntry3 :", ':', nullptr));
    EXPECT_EQ(2, GetIntegerOptionFromFile(m_path, "FooEntry3", ':', nullptr));

    EXPECT_STREQ("0456", value = GetStringOptionFromFile(m_path, "Test4", ' ', nullptr));
    FREE_MEMORY(value);
    EXPECT_EQ(456, GetIntegerOptionFromFile(m_path, "Test4", ' ', nullptr));

    EXPECT_STREQ("12", value = GetStringOptionFromFile(m_path, "Test2:", ':', nullptr));
    FREE_MEMORY(value);
    EXPECT_STREQ("12", value = GetStringOptionFromFile(m_path, "Test2", ':', nullptr));
    FREE_MEMORY(value);
    EXPECT_EQ(12, GetIntegerOptionFromFile(m_path, "Test2:", ':', nullptr));
    EXPECT_EQ(12, GetIntegerOptionFromFile(m_path, "Test2", ':', nullptr));

    EXPECT_STREQ("5", value = GetStringOptionFromFile(m_path, "remember=", '=', nullptr));
    FREE_MEMORY(value);
    EXPECT_STREQ("5", value = GetStringOptionFromFile(m_path, "remember", '=', nullptr));
    FREE_MEMORY(value);
    EXPECT_EQ(5, GetIntegerOptionFromFile(m_path, "remember=", '=', nullptr));
    EXPECT_EQ(5, GetIntegerOptionFromFile(m_path, "remember", '=', nullptr));

    EXPECT_STREQ("-1", value = GetStringOptionFromFile(m_path, "remembering", '=', nullptr));
    FREE_MEMORY(value);
    EXPECT_EQ(-1, GetIntegerOptionFromFile(m_path, "remembering", '=', nullptr));

    EXPECT_TRUE(Cleanup(m_path));
}

TEST_F(CommonUtilsTest, CheckLockoutForFailedPasswordAttempts)
{
    const char* goodTestFileContents[] = {
        "auth required pam_tally2.so file=/var/log/tallylog deny=1 unlock_time=1000",
        "auth required pam_tally2.so file=/var/log/tallylog unlock_time=2000 deny=2",
        "auth required pam_tally2.so file=/var/log/tallylog deny=3 even_deny_root unlock_time=1000",
        "auth required pam_tally2.so   file=/var/log/tallylog test deny=3 even_deny_root 123 unlock_time=1000 456",
        "auth        required      pam_tally2.so  file=/var/log/tallylog deny=3  unlock_time=100",
        "auth required      pam_tally2.so  file=/var/log/tallylog deny=1 unlock_time=10",
        "auth                   required pam_tally2.so       file=/var/log/tallylog    deny=5  unlock_time=2000",
        "This is a positive test\nauth required pam_tally2.so file=/var/log/tallylog deny=3 unlock_time=123",
        "This is a positive test\nAnother one with auth test\nauth required pam_tally2.so file=/var/log/tallylog deny=3 unlock_time=123",
        "auth	[success=1 default=ignore]	pam_unix.so nullok\n"
        "# here's the fallback if no module succeeds\n"
        "auth	requisite			pam_deny.so\n"
        "# prime the stack with a positive return value if there isn't one already;\n"
        "# this avoids us returning an error just because nothing sets a success code\n"
        "# since the modules above will each just jump around\n"
        "auth	required			pam_permit.so\n"
        "auth required pam_tally2.so file=/var/log/tallylog deny=3 unlock_time=888\n"
        "# and here are more per-package modules (the Additional block)\n"
        "auth	optional			pam_cap.so\n" 
        "# end of pam-auth-update config"
    };

    const char* badTestFileContents[] = {
        "auth optional pam_tally2.so file=/var/log/tallylog deny=2 even_deny_root unlock_time=1000",
        "auth        required      pam_tally2.so  file=/var/log/foolog deny=3 even_deny_root unlock_time=100",
        "auth required  pam_tally.so  file=/var/log/tallylog deny=1 even_deny_root unlock_time=10",
        "auth required pam_tally2.so  deny=5 even_deny_root unlock_time=2000",
        "auth required  pam_tally.so  file=/var/log/tallylog deny=1 even_deny_root unlock_time=10",
        "auth required  pam_tally2.so file=/var/log/tallylog deny=1 unlock_time=-1",
        "auth required  pam_tally2.so file=/var/log/tallylog deny=-1 unlock_time=-1",
        "auth required pam_tally2.so file=/var/log/tallylog deny=2 unlock_time=0",
        "auth required pam_tally2.so file=/var/log/tallylog deny=0 unlock_time=0",
        "auth required pam_tally2.so file=/var/log/tallylog deny=2 unlock_time=",
        "auth required pam_tally2.so file=/var/log/tallylog",
        "This is a negative auth test",
        "This is a negative test",
        "auth	[success=1 default=ignore]	pam_unix.so nullok\n"
        "# here's the fallback if no module succeeds\n"
        "auth	requisite			pam_deny.so\n"
        "# prime the stack with a positive return value if there isn't one already;\n"
        "# this avoids us returning an error just because nothing sets a success code\n"
        "# since the modules above will each just jump around\n"
        "auth	required			pam_permit.so\n"
        "auth required pam_tally2.so file=/var/log/tallylog deny=0 unlock_time=888\n"
        "# and here are more per-package modules (the Additional block)\n"
        "auth	optional			pam_cap.so\n" 
        "# end of pam-auth-update config"
    };

    int goodTestFileContentsSize = ARRAY_SIZE(goodTestFileContents);
    int badTestFileContentsSize = ARRAY_SIZE(badTestFileContents);

    int i = 0;

    EXPECT_NE(0, CheckLockoutForFailedPasswordAttempts(nullptr, nullptr));
    EXPECT_NE(0, CheckLockoutForFailedPasswordAttempts("~file_that_does_not_exist", nullptr));

    for (i = 0; i < goodTestFileContentsSize; i++)
    {
        EXPECT_TRUE(CreateTestFile(m_path, goodTestFileContents[i]));
        EXPECT_EQ(0, CheckLockoutForFailedPasswordAttempts(m_path, nullptr));
        EXPECT_TRUE(Cleanup(m_path));
    }

    for (i = 0; i < badTestFileContentsSize; i++)
    {
        EXPECT_TRUE(CreateTestFile(m_path, badTestFileContents[i]));
        EXPECT_NE(0, CheckLockoutForFailedPasswordAttempts(m_path, nullptr));
        EXPECT_TRUE(Cleanup(m_path));
    }
}