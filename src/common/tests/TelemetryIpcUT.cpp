// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <IpcInterface.h>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

class TelemetryIpcTest : public ::testing::Test
{
protected:
    TelemetryWorker* worker = nullptr;

    int Create(const char* path = TELEMETRY_IPC_PEER_PATH)
    {
        return Initialize(path, &worker, nullptr);
    }

    int Send(const char* event, OsConfigLogHandle log = nullptr)
    {
        TelemetryProperty property = {};
        property.name = "CrashInfo";
        property.value.stringValue = "offline fixture";
        return SendEvent(worker, event, &property, 1, log);
    }

    void TearDown() override
    {
        EXPECT_EQ(0, Shutdown(&worker, nullptr));
        EXPECT_EQ(nullptr, worker);
    }
};

TEST_F(TelemetryIpcTest, ReusesExternalPeerAndAcceptsSplitReplies)
{
    ASSERT_EQ(0, Create());
    EXPECT_EQ(0, Send("split"));
    EXPECT_EQ(0, Send("must_reuse"));
    EXPECT_EQ(0, Shutdown(&worker, nullptr));
    EXPECT_EQ(0, Shutdown(&worker, nullptr));
}

TEST_F(TelemetryIpcTest, PreservesWorkerOutcomeAndSuppression)
{
    ASSERT_EQ(0, Create());
    EXPECT_EQ(EIO, Send("reject"));
    EXPECT_EQ(0, Send("must_reuse"));
    EXPECT_EQ(ECANCELED, Send("suppress"));
    EXPECT_EQ(ECANCELED, Send("must_reuse"));
}

TEST_F(TelemetryIpcTest, CompleteReplyRemainsAcceptedWhenPeerExits)
{
    ASSERT_EQ(0, Create());
    EXPECT_EQ(0, Send("accepted_exit"));
}

TEST_F(TelemetryIpcTest, BrokenPipeDoesNotDeliverSigpipeToTheHost)
{
    ASSERT_EQ(0, Create());
    ASSERT_EQ(0, Send("close_input"));
    EXPECT_EQ(EPIPE, Send("fixture"));
    EXPECT_EQ(ECANCELED, Send("fixture"));
}

TEST_F(TelemetryIpcTest, RustWorkerIntegration)
{
    const char* path = getenv("OSCONFIG_TEST_RUST_WORKER");
    if (nullptr == path) GTEST_SKIP() << "Requires an explicitly configured offline TLS collector fixture";
    const char* key = getenv("OsConfigTelemetryApiKey");
    const char* proxy = getenv("https_proxy");
    ASSERT_TRUE(nullptr != key && 0 == strcmp(key, "offline-test-key"));
    ASSERT_TRUE(nullptr != proxy && 0 == strncmp(proxy, "http://127.0.0.1:", 17));
    ASSERT_TRUE(nullptr == getenv("no_proxy") || '\0' == getenv("no_proxy")[0]);
    ASSERT_TRUE(nullptr == getenv("NO_PROXY") || '\0' == getenv("NO_PROXY")[0]);
    ASSERT_EQ(0, Create(path));
    EXPECT_EQ(0, Send("Fixture.Event"));
    EXPECT_EQ(0, Send("Fixture.Event"));
}

TEST_F(TelemetryIpcTest, DropsMalformedRepliesWithoutReplaying)
{
    const char* events[] = {"bad_version", "bad_sequence", "negative_status", "bad_suppression"};
    for (const char* event : events)
    {
        ASSERT_EQ(0, Create());
        EXPECT_EQ(EPROTO, Send(event)) << event;
        EXPECT_EQ(ECANCELED, Send("must_reuse"));
        EXPECT_EQ(0, Shutdown(&worker, nullptr));
    }
}

TEST_F(TelemetryIpcTest, ReapsPeerAfterDisconnectOrTimeout)
{
    ASSERT_EQ(0, Create());
    EXPECT_EQ(EPIPE, Send("disconnect"));
    EXPECT_EQ(ECANCELED, Send("must_reuse"));
    EXPECT_EQ(0, Shutdown(&worker, nullptr));

    ASSERT_EQ(0, Create());
    EXPECT_EQ(ETIMEDOUT, Send("hang"));
    EXPECT_EQ(ECANCELED, Send("must_reuse"));
}

TEST_F(TelemetryIpcTest, RejectsMismatchedStartupVersion)
{
    const char* previous = getenv("OSCONFIG_TEST_READY_VERSION");
    const bool hadPrevious = nullptr != previous;
    const std::string saved = hadPrevious ? previous : "";
    ASSERT_EQ(0, setenv("OSCONFIG_TEST_READY_VERSION", "999", 1));
    EXPECT_EQ(EPROTO, Create());
    EXPECT_EQ(nullptr, worker);
    EXPECT_EQ(0, hadPrevious ?
        setenv("OSCONFIG_TEST_READY_VERSION", saved.c_str(), 1) :
        unsetenv("OSCONFIG_TEST_READY_VERSION"));
    ASSERT_EQ(0, Create());
    EXPECT_EQ(0, Send("fixture"));
}

TEST_F(TelemetryIpcTest, MissingExecutableFailsInitializeAndRequiresExplicitRetry)
{
    char temporary[] = "/tmp/osconfig-ipc-XXXXXX";
    ASSERT_NE(nullptr, mkdtemp(temporary));
    const std::string directory = temporary;
    const std::string executable = directory + "/OSConfigTelemetry";
    EXPECT_EQ(ENOENT, Create(executable.c_str()));
    EXPECT_EQ(nullptr, worker);
    EXPECT_EQ(0, symlink(TELEMETRY_IPC_PEER_PATH, executable.c_str()));
    EXPECT_EQ(EINVAL, Send("fixture"));
    EXPECT_EQ(0, Create(executable.c_str()));
    EXPECT_EQ(0, Send("fixture"));
    EXPECT_EQ(0, Shutdown(&worker, nullptr));
    EXPECT_EQ(0, unlink(executable.c_str()));
    EXPECT_EQ(0, rmdir(directory.c_str()));
}

TEST_F(TelemetryIpcTest, ShutsDownInitializedPeerWithoutSendingAnEvent)
{
    ASSERT_EQ(0, Create());
    EXPECT_EQ(0, Shutdown(&worker, nullptr));
    EXPECT_EQ(nullptr, worker);
}

TEST_F(TelemetryIpcTest, Sends100EventsThroughOnePeerBeyondFormerBudget)
{
    ASSERT_EQ(0, Create());
    EXPECT_EQ(0, Send("delayed"));
    usleep(600000);
    for (int event = 1; event < 100; ++event)
    {
        ASSERT_EQ(0, Send("must_reuse"));
    }
}

TEST_F(TelemetryIpcTest, InvalidArgumentsDoNotReplaceAnActiveWorker)
{
    EXPECT_EQ(EINVAL, Create("relative-path"));
    EXPECT_EQ(nullptr, worker);
    ASSERT_EQ(0, Create());
    EXPECT_EQ(EALREADY, Create());
    EXPECT_EQ(0, Send("fixture"));
}
