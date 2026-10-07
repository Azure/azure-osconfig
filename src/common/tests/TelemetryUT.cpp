// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <Telemetry.h>
#include <TelemetryWorker.h>
#include <TelemetryEvent.h>
#include <CommonUtils.h>
#include <cerrno>
#include <cstdlib>
#include <map>
#include <string>

// This executable compiles the real producer with worker/platform entry points
// substituted at compile time. No network, chmod, or machine-file writes.
struct TelemetryWorker {};
static TelemetryWorker workerInstance;
static int creates, destroys, sends, createStatus, preparationStatus;
static int lifetime, budget, operation;
static std::string eventName, workerPath;
static std::map<std::string, std::string> fields;

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

extern "C" int TelemetryWorkerSend(TelemetryWorker*, const char* name,
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
    void SetUp() override
    {
        TelemetryCleanup(nullptr);
        creates = destroys = sends = createStatus = preparationStatus = 0;
        fields.clear();
    }

    void TearDown() override
    {
        TelemetryCleanup(nullptr);
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
    EXPECT_EQ(EALREADY, TelemetryInitializeInternal(nullptr));
    EXPECT_EQ(1, creates);
    TelemetryCleanup(nullptr);
    EXPECT_EQ(1, destroys);
    TelemetryCleanup(nullptr);
    EXPECT_EQ(1, destroys);
}

TEST_F(TelemetryProducerTest, PreservesAllFourNamedStringSchemasAndSpecialCharacters)
{
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryBaselineRun("baseline\"\\\n", "Audit", 1.25);
    EXPECT_EQ("BaselineRun", eventName);
    EXPECT_EQ("baseline\"\\\n", fields["BaselineName"]);
    EXPECT_EQ("1.25", fields["DurationSeconds"]);
    EXPECT_EQ("Synthetic distro", fields["DistroName"]);
    EXPECT_FALSE(fields["Timestamp"].empty());
    OSConfigTelemetryRuleComplete("component", "object", 5, INT64_C(4294967296));
    EXPECT_EQ("RuleComplete", eventName);
    EXPECT_EQ("4294967296", fields["Microseconds"]);
    OSConfigTelemetryStatusTrace("function", EIO);
    EXPECT_EQ("StatusTrace", eventName);
    EXPECT_EQ(std::to_string(EIO), fields["ResultCode"]);
    OSConfigTelemetryCrashDetected("quote\" slash\\ newline\ncrash");
    EXPECT_EQ("CrashDetected", eventName);
    EXPECT_EQ("quote\" slash\\ newline\ncrash", fields["CrashInfo"]);
    EXPECT_EQ(4, sends);
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
    EXPECT_EQ(EALREADY, TelemetryInitializeInternal(nullptr));
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
}

TEST_F(TelemetryProducerTest, ExhaustedPreparationBudgetDoesNotBeginSend)
{
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    preparationStatus = ETIMEDOUT;
    OSConfigTelemetryCrashDetected("dropped");
    EXPECT_EQ(0, sends);
}
