// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <Telemetry.h>
#include <IpcInterface.h>
#include <EventPayload.h>
#include <CommonUtils.h>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <unistd.h>

// This suite uses a separately named copy of the real producer with mocked
// worker/platform calls. Other commontests suites retain the real functions.
// No network, chmod, or system-configuration writes. File reads use temporary fixtures.
struct TelemetryWorker {};
static TelemetryWorker workerInstance;
static int creates, destroys, sends, createStatus, preparationStatus;
static int lifetime, budget, operation;
static std::string eventName, workerPath;
static std::map<std::string, std::string> fields;
static std::string activityFilePath;
static int activityFileReads, preparationFileReads;

extern "C" char* TelemetryTestLoadStringFromFile(const char* path, bool stopAtEol, OsConfigLogHandle log)
{
    EXPECT_STREQ("/var/lib/GuestConfig/Configuration/activityId.txt", path);
    EXPECT_FALSE(stopAtEol);
    ++activityFileReads;
    return LoadStringFromFile(activityFilePath.c_str(), stopAtEol, log);
}

extern "C" int TelemetryWorkerCreate(const char* path, int lifetimeMs, int budgetMs,
    int operationMs, TelemetryWorker** worker, OsConfigLogHandle)
{
    ++creates;
    workerPath = path;
    lifetime = lifetimeMs;
    budget = budgetMs;
    operation = operationMs;

    if (!createStatus)
    {
        *worker = &workerInstance;
    }

    return createStatus;
}

extern "C" int TelemetryWorkerDestroy(TelemetryWorker** worker, OsConfigLogHandle)
{
    if (*worker)
    {
        ++destroys;
    }

    *worker = nullptr;

    return 0;
}

extern "C" int TelemetryWorkerSendEvent(TelemetryWorker*, const char* name,
    const TelemetryProperty* properties, size_t count, OsConfigLogHandle)
{
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {};
    unsigned char encoded[TELEMETRY_MAX_EVENT_SIZE] = {};
    size_t size = 0, encodedSize = 0;
    int64_t uploadTime = 0;
    size_t i = 0;

    ++sends;
    eventName = name;
    fields.clear();

    for (i = 0; i < count; ++i)
    {
        EXPECT_EQ(TelemetryPropertyString, properties[i].type);
        fields[properties[i].name] = properties[i].value.stringValue;
    }

    EXPECT_EQ(0, TelemetryPackEvent(name, properties, count, payload, sizeof(payload), &size, nullptr));
    EXPECT_EQ(0, TelemetryEncodePayload(payload, size, "o:fixture", "test-epoch", sends,
        encoded, &encodedSize, &uploadTime, nullptr));
    EXPECT_GT(encodedSize, 0U);

    return 0;
}

extern "C" int TelemetryWorkerAccountPreparation(TelemetryWorker*, int64_t started, OsConfigLogHandle)
{
    EXPECT_GT(started, 0);
    preparationFileReads = activityFileReads;

    return preparationStatus;
}

extern "C" int SetFileAccess(const char*, unsigned int, unsigned int, unsigned int, OsConfigLogHandle)
{
    return 0;
}

extern "C" char* GetOsPrettyName(OsConfigLogHandle)
{
    return strdup("Synthetic distro");
}

class TelemetryProducerTest : public ::testing::Test
{
protected:
    std::string directory, savedActivityId;
    bool hadActivityId = false;

    void SetUp() override
    {
        char temporary[] = "/tmp/osconfig-activity-id-XXXXXX";
        const char* original = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR);

        hadActivityId = nullptr != original;
        savedActivityId = hadActivityId ? original : "";
        ASSERT_EQ(0, unsetenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR));
        TelemetryCleanup(nullptr);
        creates = destroys = sends = createStatus = preparationStatus = 0;
        activityFileReads = preparationFileReads = 0;
        activityFilePath.clear();
        fields.clear();
        ASSERT_NE(nullptr, mkdtemp(temporary));
        directory = temporary;
        activityFilePath = directory + "/activityId.txt";
    }

    void TearDown() override
    {
        TelemetryCleanup(nullptr);
        EXPECT_EQ(0, hadActivityId ?
            setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, savedActivityId.c_str(), 1) :
            unsetenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR));
        if (!directory.empty())
        {
            if ((0 != unlink(activityFilePath.c_str())) && (ENOENT != errno))
            {
                ADD_FAILURE() << "Cannot remove activity ID fixture: " << strerror(errno);
            }
            EXPECT_EQ(0, rmdir(directory.c_str()));
        }
        activityFilePath.clear();
    }

    void WriteActivityId(const std::string& value)
    {
        std::ofstream output(activityFilePath, std::ios::binary);
        ASSERT_TRUE(output.is_open());
        output << value;
        output.close();
        ASSERT_FALSE(output.fail());
    }
};

TEST_F(TelemetryProducerTest, PreservesLegacyInitializeEmitCleanupCalls)
{
    void (*initialize)(OsConfigLogHandle) = TelemetryInitialize;

    initialize(nullptr);
    EXPECT_EQ(1, creates);
    OSConfigTelemetryCrashDetected("previous crash");
    EXPECT_EQ("CrashDetected", eventName);
    EXPECT_EQ(1, sends);
    TelemetryCleanup(nullptr);
    EXPECT_EQ(1, destroys);
}

TEST_F(TelemetryProducerTest, UsesApprovedLimitsAndDoesNotResetActiveInvocation)
{
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    EXPECT_EQ(600000, lifetime);
    EXPECT_GT(budget, 0);
    EXPECT_LE(budget, 500);
    EXPECT_EQ(500, operation);
    EXPECT_EQ('/', workerPath[0]);
    EXPECT_NE(std::string::npos, workerPath.find("/OSConfigTelemetry"));
    EXPECT_EQ(0, TelemetryInitializeInternal(nullptr));
    EXPECT_EQ(1, creates);
    EXPECT_EQ(0, destroys);
    OSConfigTelemetryCrashDetected("existing invocation");
    EXPECT_EQ(1, sends);
    TelemetryCleanup(nullptr);
    EXPECT_EQ(1, destroys);
    TelemetryCleanup(nullptr);
    EXPECT_EQ(1, destroys);
}

TEST_F(TelemetryProducerTest, PreservesAllFourNamedStringSchemasAndSpecialCharacters)
{
    WriteActivityId("file-activity-id");
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryBaselineRun("baseline\"\\\n", "Audit", 1.25);
    EXPECT_EQ("BaselineRun", eventName);
    EXPECT_EQ("baseline\"\\\n", fields["BaselineName"]);
    EXPECT_EQ("1.25", fields["DurationSeconds"]);
    EXPECT_EQ("Synthetic distro", fields["DistroName"]);
    EXPECT_EQ("file-activity-id", fields["CorrelationId"]);
    EXPECT_FALSE(fields["Timestamp"].empty());
    OSConfigTelemetryRuleComplete("component", "object", 5, INT64_C(4294967296));
    EXPECT_EQ("RuleComplete", eventName);
    EXPECT_EQ("4294967296", fields["Microseconds"]);
    EXPECT_EQ("file-activity-id", fields["CorrelationId"]);
    OSConfigTelemetryStatusTrace("function", EIO);
    EXPECT_EQ("StatusTrace", eventName);
    EXPECT_EQ(std::to_string(EIO), fields["ResultCode"]);
    EXPECT_EQ("file-activity-id", fields["CorrelationId"]);
    OSConfigTelemetryCrashDetected("quote\" slash\\ newline\ncrash");
    EXPECT_EQ("CrashDetected", eventName);
    EXPECT_EQ("quote\" slash\\ newline\ncrash", fields["CrashInfo"]);
    EXPECT_EQ("file-activity-id", fields["CorrelationId"]);
    EXPECT_EQ(4, sends);
    EXPECT_EQ(4, activityFileReads);
}

TEST_F(TelemetryProducerTest, EnvironmentCorrelationIdTakesPrecedenceWithoutReadingFile)
{
    WriteActivityId("file-activity-id");
    ASSERT_EQ(0, setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, "environment-activity-id", 1));
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("fixture");
    EXPECT_EQ("environment-activity-id", fields["CorrelationId"]);
    EXPECT_EQ(0, activityFileReads);
}

TEST_F(TelemetryProducerTest, EmptyEnvironmentCorrelationIdFallsBackToFile)
{
    WriteActivityId("file-activity-id");
    ASSERT_EQ(0, setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, "", 1));
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("fixture");
    EXPECT_EQ("file-activity-id", fields["CorrelationId"]);
    EXPECT_EQ(1, activityFileReads);
}

TEST_F(TelemetryProducerTest, MissingOrEmptyCorrelationIdFileRetainsPlaceholder)
{
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("missing file");
    EXPECT_EQ("N/A", fields["CorrelationId"]);
    WriteActivityId("");
    OSConfigTelemetryCrashDetected("empty file");
    EXPECT_EQ("N/A", fields["CorrelationId"]);
    EXPECT_EQ(2, activityFileReads);
    EXPECT_EQ(2, sends);
}

TEST_F(TelemetryProducerTest, CorrelationIdIsResolvedAgainForEachEvent)
{
    WriteActivityId("first-file-id");
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("first");
    EXPECT_EQ("first-file-id", fields["CorrelationId"]);
    WriteActivityId("second-file-id");
    OSConfigTelemetryBaselineRun("baseline", "Audit", 0);
    EXPECT_EQ("second-file-id", fields["CorrelationId"]);
    ASSERT_EQ(0, setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, "environment-id", 1));
    OSConfigTelemetryRuleComplete("component", "object", 0, 0);
    EXPECT_EQ("environment-id", fields["CorrelationId"]);
    EXPECT_EQ(2, activityFileReads);
    ASSERT_EQ(0, unsetenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR));
    OSConfigTelemetryCrashDetected("file again");
    EXPECT_EQ("second-file-id", fields["CorrelationId"]);
    EXPECT_EQ(3, activityFileReads);
}

TEST_F(TelemetryProducerTest, NullOptionalValuesUseExplicitPlaceholder)
{
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryBaselineRun(nullptr, nullptr, 0);
    EXPECT_EQ("N/A", fields["BaselineName"]);
    EXPECT_EQ("N/A", fields["Mode"]);
    OSConfigTelemetryCrashDetected(nullptr);
    EXPECT_EQ("N/A", fields["CrashInfo"]);
}

TEST_F(TelemetryProducerTest, InitializationFailureDoesNotRetryOrSendUntilCleanup)
{
    createStatus = ENOMEM;
    EXPECT_EQ(ENOMEM, TelemetryInitializeInternal(nullptr));
    EXPECT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("dropped");
    EXPECT_EQ(0, sends);
    EXPECT_EQ(1, creates);
    TelemetryCleanup(nullptr);
    createStatus = 0;
    EXPECT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("new invocation");
    EXPECT_EQ(1, sends);
}

TEST_F(TelemetryProducerTest, CallsOutsideInvocationDoNotStartAWorker)
{
    OSConfigTelemetryCrashDetected("outside");
    EXPECT_EQ(0, creates);
    EXPECT_EQ(0, sends);
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    TelemetryCleanup(nullptr);
    OSConfigTelemetryCrashDetected("after cleanup");
    EXPECT_EQ(1, creates);
    EXPECT_EQ(0, sends);
    EXPECT_EQ(0, activityFileReads);
}

TEST_F(TelemetryProducerTest, ExhaustedPreparationBudgetDoesNotBeginSend)
{
    WriteActivityId("file-activity-id");
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    preparationStatus = ETIMEDOUT;
    OSConfigTelemetryCrashDetected("dropped");
    EXPECT_EQ(0, sends);
    EXPECT_EQ(1, activityFileReads);
    EXPECT_EQ(1, preparationFileReads);
}
