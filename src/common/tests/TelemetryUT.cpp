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
// No network or system-configuration writes. File reads use temporary fixtures.
struct TelemetryWorker {};
static TelemetryWorker workerInstance;
static int creates, destroys, sends, createStatus;
static std::string eventName, workerPath;
static std::map<std::string, std::string> fields;
static std::string activityFilePath;
static int activityFileReads;
static int activityFileAccessStatus, activityFileReadStatus;
static int executableStatus, executableChecks;

extern "C" char* TelemetryTestLoadStringFromFile(const char* path, bool stopAtEol, OsConfigLogHandle log)
{
    EXPECT_STREQ("/var/lib/GuestConfig/Configuration/activityId.txt", path);
    EXPECT_FALSE(stopAtEol);
    ++activityFileReads;
    if (activityFileReadStatus)
    {
        errno = activityFileReadStatus;
        return nullptr;
    }
    return LoadStringFromFile(activityFilePath.c_str(), stopAtEol, log);
}

extern "C" int Initialize(const char* path, TelemetryWorker** worker, OsConfigLogHandle)
{
    ++creates;
    workerPath = path;

    if (!createStatus)
    {
        *worker = &workerInstance;
    }

    return createStatus;
}

extern "C" int Shutdown(TelemetryWorker** worker, OsConfigLogHandle)
{
    if (*worker)
    {
        ++destroys;
    }

    *worker = nullptr;

    return 0;
}

extern "C" int SendEvent(TelemetryWorker*, const char* name,
    const TelemetryProperty* properties, size_t count, OsConfigLogHandle)
{
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {};
    size_t size = 0;
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
    EXPECT_GT(size, 0U);

    return 0;
}

extern "C" int TelemetryTestAccess(const char* path, int mode)
{
    if (0 == strcmp(path, "/var/lib/GuestConfig/Configuration/activityId.txt"))
    {
        EXPECT_EQ(F_OK, mode);
        if (activityFileAccessStatus)
        {
            errno = activityFileAccessStatus;
            return -1;
        }
        return access(activityFilePath.c_str(), mode);
    }
    ++executableChecks;
    EXPECT_NE(nullptr, strstr(path, "/OSConfigTelemetry"));
    EXPECT_EQ(F_OK, mode);
    errno = executableStatus;
    return executableStatus ? -1 : 0;
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
        creates = destroys = sends = createStatus = 0;
        executableStatus = executableChecks = 0;
        activityFileReads = 0;
        activityFileAccessStatus = activityFileReadStatus = 0;
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

TEST_F(TelemetryProducerTest, DoesNotResetActiveInvocation)
{
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
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

TEST_F(TelemetryProducerTest, PreservesNamedStringSchemasAndSpecialCharacters)
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
    OSConfigTelemetryCrashDetected("quote\" slash\\ newline\ncrash");
    EXPECT_EQ("CrashDetected", eventName);
    EXPECT_EQ("quote\" slash\\ newline\ncrash", fields["CrashInfo"]);
    EXPECT_EQ("file-activity-id", fields["CorrelationId"]);
    EXPECT_EQ(2, sends);
    EXPECT_EQ(2, activityFileReads);
}

TEST_F(TelemetryProducerTest, FileCorrelationIdTakesPrecedenceOverEnvironment)
{
    WriteActivityId("file-activity-id");
    ASSERT_EQ(0, setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, "environment-activity-id", 1));
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("fixture");
    EXPECT_EQ("file-activity-id", fields["CorrelationId"]);
    EXPECT_EQ(1, activityFileReads);
}

TEST_F(TelemetryProducerTest, FileCorrelationIdIsUsedWithEmptyEnvironment)
{
    WriteActivityId("file-activity-id");
    ASSERT_EQ(0, setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, "", 1));
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("fixture");
    EXPECT_EQ("file-activity-id", fields["CorrelationId"]);
    EXPECT_EQ(1, activityFileReads);
}

TEST_F(TelemetryProducerTest, MissingCorrelationIdFileFallsBackToEnvironment)
{
    ASSERT_EQ(0, setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, "environment-activity-id", 1));
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("missing file");
    EXPECT_EQ("environment-activity-id", fields["CorrelationId"]);
    EXPECT_EQ(0, activityFileReads);
    ASSERT_EQ(0, setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, "", 1));
    OSConfigTelemetryCrashDetected("missing file and empty environment");
    EXPECT_EQ("N/A", fields["CorrelationId"]);
}

TEST_F(TelemetryProducerTest, EmptyOrUnreadableCorrelationIdFileDoesNotFallBackToEnvironment)
{
    ASSERT_EQ(0, setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, "environment-activity-id", 1));
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    WriteActivityId("");
    OSConfigTelemetryCrashDetected("empty file");
    EXPECT_EQ("N/A", fields["CorrelationId"]);
    WriteActivityId("file-activity-id");
    activityFileReadStatus = EACCES;
    OSConfigTelemetryCrashDetected("unreadable file");
    EXPECT_EQ("N/A", fields["CorrelationId"]);
    EXPECT_EQ(2, activityFileReads);
}

TEST_F(TelemetryProducerTest, CorrelationIdFileAccessErrorDoesNotFallBackToEnvironment)
{
    ASSERT_EQ(0, setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, "environment-activity-id", 1));
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    for (int error : {EACCES, EIO, ENOTDIR})
    {
        activityFileAccessStatus = error;
        OSConfigTelemetryCrashDetected("file access failed");
        EXPECT_EQ("N/A", fields["CorrelationId"]);
    }
    EXPECT_EQ(0, activityFileReads);
}

TEST_F(TelemetryProducerTest, MissingOrEmptyCorrelationIdFileRetainsPlaceholder)
{
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("missing file");
    EXPECT_EQ("N/A", fields["CorrelationId"]);
    WriteActivityId("");
    OSConfigTelemetryCrashDetected("empty file");
    EXPECT_EQ("N/A", fields["CorrelationId"]);
    EXPECT_EQ(1, activityFileReads);
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
    OSConfigTelemetryBaselineRun("baseline", "Audit", 0);
    EXPECT_EQ("second-file-id", fields["CorrelationId"]);
    EXPECT_EQ(3, activityFileReads);
    ASSERT_EQ(0, unlink(activityFilePath.c_str()));
    OSConfigTelemetryBaselineRun("baseline", "Audit", 0);
    EXPECT_EQ("environment-id", fields["CorrelationId"]);
    EXPECT_EQ(3, activityFileReads);
    WriteActivityId("third-file-id");
    OSConfigTelemetryCrashDetected("file again");
    EXPECT_EQ("third-file-id", fields["CorrelationId"]);
    EXPECT_EQ(4, activityFileReads);
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

TEST_F(TelemetryProducerTest, SendsAll100EventsThroughOneInvocation)
{
    WriteActivityId("file-activity-id");
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    for (int event = 0; event < 100; ++event)
    {
        OSConfigTelemetryCrashDetected("fixture");
    }
    EXPECT_EQ(100, sends);
    EXPECT_EQ(100, activityFileReads);
    EXPECT_EQ(1, creates);
}

TEST_F(TelemetryProducerTest, MissingExecutableSilentlyDisablesEventsAcrossInvocations)
{
    std::string logPath = directory + "/telemetry.log";
    std::string backupPath = logPath + ".bak";
    OsConfigLogHandle log = OpenLog(logPath.c_str(), backupPath.c_str());
    ASSERT_NE(nullptr, log);
    executableStatus = ENOENT;

    for (int invocation = 0; invocation < 3; ++invocation)
    {
        EXPECT_EQ(0, TelemetryInitializeInternal(log));
        EXPECT_EQ(0, TelemetryInitializeInternal(log));
        OSConfigTelemetryCrashDetected("ignored");
        OSConfigTelemetryBaselineRun("baseline", "Audit", 0);
        TelemetryCleanup(log);
    }

    EXPECT_EQ(3, executableChecks);
    EXPECT_EQ(0, creates);
    EXPECT_EQ(0, sends);
    EXPECT_EQ(0, activityFileReads);
    CloseLog(&log);
    std::ifstream input(logPath, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    EXPECT_EQ(std::ifstream::traits_type::eof(), input.peek());
    input.close();
    EXPECT_EQ(0, unlink(logPath.c_str()));

    executableStatus = 0;
    ASSERT_EQ(0, TelemetryInitializeInternal(nullptr));
    EXPECT_EQ(1, creates);
}

TEST_F(TelemetryProducerTest, InaccessibleExecutableDoesNotStartWorker)
{
    executableStatus = EACCES;
    EXPECT_EQ(EACCES, TelemetryInitializeInternal(nullptr));
    OSConfigTelemetryCrashDetected("ignored");
    EXPECT_EQ(0, creates);
    EXPECT_EQ(0, sends);
}
