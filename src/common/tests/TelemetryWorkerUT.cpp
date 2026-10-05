// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <TelemetryWorker.h>
#include <TelemetryWorkerProtocol.h>

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <poll.h>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char** environ;

namespace
{
int64_t Now()
{
    struct timespec time = {};

    return (0 == clock_gettime(CLOCK_MONOTONIC, &time)) ?
        static_cast<int64_t>(time.tv_sec) * 1000000000 + time.tv_nsec : -1;
}

class RawWorker
{
public:
    int descriptor = -1;
    pid_t child = -1;
    int64_t lifetime = 0;

    ~RawWorker()
    {
        int status = 0;

        Disconnect();

        if (child > 0)
        {
            if ((!Wait(&status, 100)) && (child > 0))
            {
                kill(child, SIGKILL);

                while ((waitpid(child, NULL, 0) < 0) && (EINTR == errno))
                {
                }
            }
        }
    }

    int Start(const char* path, int lifetimeMs = 10000)
    {
        int sockets[2] = {-1, -1};
        posix_spawn_file_actions_t actions = {};
        int status = 0;
        int64_t now = 0;
        char deadline[32] = {};
        char* arguments[] = {const_cast<char*>(path), const_cast<char*>(TELEMETRY_WORKER_ARGUMENT),
            deadline, deadline, const_cast<char*>("--force-mintls"), NULL};

        if ((child > 0) || (descriptor >= 0))
        {
            return EALREADY;
        }

        if (0 != socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
        {
            return errno;
        }

        if (0 != (status = posix_spawn_file_actions_init(&actions)))
        {
            close(sockets[0]);
            close(sockets[1]);
            return status;
        }

        now = Now();

        if (now < 0)
        {
            status = EIO;
        }

        lifetime = now + static_cast<int64_t>(lifetimeMs) * 1000000;
        snprintf(deadline, sizeof(deadline), "%lld", static_cast<long long>(lifetime));

        if ((0 == status) &&
            (0 == (status = posix_spawn_file_actions_adddup2(&actions, sockets[1], STDIN_FILENO))) &&
            (0 == (status = posix_spawn_file_actions_addclose(&actions, sockets[0]))) &&
            (0 == (status = posix_spawn_file_actions_addclose(&actions, sockets[1]))))
        {
            status = posix_spawn(&child, path, &actions, NULL, arguments, environ);
        }

        posix_spawn_file_actions_destroy(&actions);
        close(sockets[1]);

        if (0 == status)
        {
            descriptor = sockets[0];
        }
        else
        {
            child = -1;
            close(sockets[0]);
        }

        return status;
    }

    void Disconnect()
    {
        if (descriptor >= 0)
        {
            close(descriptor);
            descriptor = -1;
        }
    }

    bool Write(const void* data, size_t size)
    {
        const char* bytes = static_cast<const char*>(data);
        ssize_t count = 0;

        while (size)
        {
            count = send(descriptor, bytes, size, MSG_NOSIGNAL);

            if (count > 0)
            {
                bytes += count;
                size -= static_cast<size_t>(count);
            }
            else if ((0 == count) || (EINTR != errno))
            {
                return false;
            }
        }

        return true;
    }

    bool Read(void* data, size_t size)
    {
        char* bytes = static_cast<char*>(data);
        int64_t start = Now();
        int64_t deadline = 0;
        int64_t now = 0;
        struct pollfd item = {};
        int polled = 0;
        ssize_t count = 0;

        if (start < 0)
        {
            return false;
        }

        deadline = start + 5000000000LL;

        while (size)
        {
            now = Now();

            if ((now < 0) || (now >= deadline))
            {
                return false;
            }

            item = {descriptor, POLLIN, 0};
            polled = poll(&item, 1, 20);

            if ((polled < 0) && (EINTR != errno))
            {
                return false;
            }

            if (polled <= 0)
            {
                continue;
            }

            count = recv(descriptor, bytes, size, 0);

            if (count > 0)
            {
                bytes += count;
                size -= static_cast<size_t>(count);
            }
            else if ((0 == count) || (EINTR != errno))
            {
                return false;
            }
        }

        return 0 == size;
    }

    bool Ready()
    {
        TelemetryWorkerFrame frame = {};

        return (Read(&frame, sizeof(frame))) && (TELEMETRY_WORKER_MAGIC == frame.magic) &&
            (TELEMETRY_WORKER_VERSION == frame.version) && (TELEMETRY_WORKER_READY == frame.operation) &&
            (0 == frame.sequence) && (0 == frame.status) && (0 == frame.size) && (0 == frame.deadline);
    }

    bool Wait(int* status, int timeoutMs = 5000)
    {
        int64_t start = 0;
        int64_t deadline = 0;
        int64_t now = 0;
        pid_t waited = 0;

        if (child <= 0)
        {
            return false;
        }

        start = Now();

        if (start < 0)
        {
            return false;
        }

        deadline = start + static_cast<int64_t>(timeoutMs) * 1000000;

        for (;;)
        {
            now = Now();

            if ((now < 0) || (now >= deadline))
            {
                return false;
            }

            waited = waitpid(child, status, WNOHANG);

            if (waited == child)
            {
                child = -1;
                return true;
            }

            if ((waited < 0) && (EINTR != errno))
            {
                if (ECHILD == errno)
                {
                    child = -1;
                }

                return false;
            }

            poll(NULL, 0, 5);
        }
    }
};

TelemetryWorkerFrame Request(const RawWorker& worker, uint32_t size)
{
    TelemetryWorkerFrame request = {};

    request.magic = TELEMETRY_WORKER_MAGIC;
    request.version = TELEMETRY_WORKER_VERSION;
    request.operation = TELEMETRY_WORKER_RESOLVE;
    request.sequence = 1;
    request.size = size;
    request.deadline = worker.lifetime;

    return request;
}
}

class TelemetryWorkerTest : public ::testing::Test
{
protected:
    TelemetryWorker* worker = NULL;
    TelemetryResolvedHost result = {};

    void TearDown() override
    {
        if (NULL != worker)
        {
            TelemetryWorkerDestroy(&worker, NULL);
        }
    }

    int Create(const char* path = TELEMETRY_WORKER_LOOKUP_PATH,
        int lifetime = 30000, int budget = 10000, int operation = 5000)
    {
        return TelemetryWorkerCreate(path, lifetime, budget, operation, &worker, true, NULL);
    }

    uint32_t Pid()
    {
        struct sockaddr_in6 address = {};

        memcpy(&address, &result.addresses[0], sizeof(address));

        return address.sin6_scope_id;
    }
};

TEST_F(TelemetryWorkerTest, ReusesOneProcessAcrossRequestsAndLookupErrors)
{
    uint32_t first = 0;

    ASSERT_EQ(0, Create());
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    first = Pid();
    ASSERT_GT(first, 0U);
    EXPECT_EQ(EHOSTUNREACH, TelemetryWorkerResolve(worker, "lookup-failure", &result, NULL));
    EXPECT_EQ(0U, result.count);
    EXPECT_EQ(EACCES, TelemetryWorkerResolve(worker, "system-failure", &result, NULL));
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    EXPECT_EQ(first, Pid());
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "many", &result, NULL));
    EXPECT_EQ(static_cast<size_t>(TELEMETRY_MAX_RESOLVED_ADDRESSES), result.count);
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "ipv6", &result, NULL));
    EXPECT_EQ(7U, Pid());
    EXPECT_EQ(0, TelemetryWorkerDestroy(&worker, NULL));
    EXPECT_EQ(nullptr, worker);
    EXPECT_EQ(0, TelemetryWorkerDestroy(&worker, NULL));
}

TEST_F(TelemetryWorkerTest, ResolvesNumericAddressesWithoutExternalDns)
{
    struct sockaddr_in ipv4 = {};
    struct sockaddr_in6 ipv6 = {};

    ASSERT_EQ(0, Create(TELEMETRY_WORKER_PATH));
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "127.0.0.1", &result, NULL));
    memcpy(&ipv4, &result.addresses[0], sizeof(ipv4));
    EXPECT_EQ(AF_INET, ipv4.sin_family);
    EXPECT_EQ(INADDR_LOOPBACK, ntohl(ipv4.sin_addr.s_addr));
    EXPECT_EQ(0, ipv4.sin_port);
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "::1", &result, NULL));
    memcpy(&ipv6, &result.addresses[0], sizeof(ipv6));
    EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(&ipv6.sin6_addr));
    EXPECT_EQ(0, ipv6.sin6_port);
}

TEST_F(TelemetryWorkerTest, RejectsInvalidArgumentsAndDoesNotResetAnActiveContext)
{
    TelemetryWorker* original = NULL;
    const std::string oversized(254, 'x');
    const std::string maximum(253, 'x');

    EXPECT_EQ(EINVAL, TelemetryWorkerCreate(NULL, 1, 1, 1, &worker, true, NULL));
    EXPECT_EQ(EINVAL, Create("relative"));
    EXPECT_EQ(EINVAL, Create(TELEMETRY_WORKER_PATH, 0));
    EXPECT_EQ(EINVAL, Create(TELEMETRY_WORKER_PATH, 1, 0));
    EXPECT_EQ(EINVAL, Create(TELEMETRY_WORKER_PATH, 1, 1, 0));
    EXPECT_EQ(EINVAL, TelemetryWorkerCreate(TELEMETRY_WORKER_PATH, 1, 1, 1, NULL, true, NULL));
    EXPECT_EQ(EINVAL, TelemetryWorkerResolve(NULL, "127.0.0.1", &result, NULL));
    EXPECT_EQ(EINVAL, TelemetryWorkerDestroy(NULL, NULL));
    ASSERT_EQ(nullptr, worker);
    ASSERT_EQ(0, Create());
    original = worker;
    EXPECT_EQ(EALREADY, Create());
    EXPECT_EQ(original, worker);
    EXPECT_EQ(EINVAL, TelemetryWorkerResolve(worker, NULL, &result, NULL));
    EXPECT_EQ(EINVAL, TelemetryWorkerResolve(worker, "", &result, NULL));
    EXPECT_EQ(EINVAL, TelemetryWorkerResolve(worker, "x", NULL, NULL));
    EXPECT_EQ(EINVAL, TelemetryWorkerResolve(worker, oversized.c_str(), &result, NULL));
    EXPECT_EQ(0, TelemetryWorkerResolve(worker, maximum.c_str(), &result, NULL));
}

TEST_F(TelemetryWorkerTest, ReportsMissingExecutableWithoutLosingTheInvocation)
{
    const std::string missing = std::string(TELEMETRY_WORKER_PATH) + ".nonexistent";

    ASSERT_EQ(0, Create(missing.c_str()));
    EXPECT_EQ(ENOENT, TelemetryWorkerResolve(worker, "127.0.0.1", &result, NULL));
    EXPECT_EQ(0U, result.count);
    EXPECT_EQ(ENOENT, TelemetryWorkerResolve(worker, "127.0.0.1", &result, NULL));
    EXPECT_EQ(0, TelemetryWorkerDestroy(&worker, NULL));
}

TEST_F(TelemetryWorkerTest, SendStartupFailureDoesNotConsumeCollectorSuppressionState)
{
    const std::string missing = std::string(TELEMETRY_WORKER_PATH) + ".nonexistent";
    TelemetryProperty property = {};

    ASSERT_EQ(0, Create(missing.c_str()));
    property.name = "CrashInfo";
    property.type = TelemetryPropertyString;
    property.value.stringValue = "fixture";
    EXPECT_EQ(EINVAL, TelemetryWorkerSend(nullptr, "CrashDetected", &property, 1, nullptr));
    EXPECT_EQ(ENOENT, TelemetryWorkerSend(worker, "CrashDetected", &property, 1, nullptr));
    EXPECT_EQ(ENOENT, TelemetryWorkerSend(worker, "CrashDetected", &property, 1, nullptr));
}

TEST_F(TelemetryWorkerTest, RejectsMismatchedStartupProtocol)
{
    ASSERT_EQ(0, Create(TELEMETRY_WORKER_BAD_READY_PATH));
    EXPECT_EQ(EPROTO, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    EXPECT_EQ(0U, result.count);
    EXPECT_EQ(EPROTO, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    EXPECT_EQ(0, TelemetryWorkerDestroy(&worker, NULL));
}

TEST_F(TelemetryWorkerTest, DropsTimedOutOperationAndRestartsOnlyForTheNextCall)
{
    uint32_t first = 0;

    ASSERT_EQ(0, Create(TELEMETRY_WORKER_LOOKUP_PATH, 30000, 10000, 500));
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    first = Pid();
    EXPECT_EQ(ETIMEDOUT, TelemetryWorkerResolve(worker, "hang", &result, NULL));
    EXPECT_EQ(0U, result.count);
    errno = 0;
    EXPECT_EQ(-1, waitpid(static_cast<pid_t>(first), NULL, WNOHANG));
    EXPECT_EQ(ECHILD, errno);
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    EXPECT_NE(first, Pid());
}

TEST_F(TelemetryWorkerTest, RestartDoesNotResetTelemetryBudget)
{
    int i = 0;

    ASSERT_EQ(0, Create(TELEMETRY_WORKER_FAULT_PATH, 30000, 300, 100));

    for (i = 0; i < 4; ++i)
    {
        EXPECT_EQ(ETIMEDOUT, TelemetryWorkerResolve(worker, "hang", &result, NULL));
    }

    EXPECT_EQ(ETIMEDOUT, TelemetryWorkerResolve(worker, "split", &result, NULL));
    EXPECT_EQ(0U, result.count);
}

TEST_F(TelemetryWorkerTest, AuditTimeBetweenCallsDoesNotConsumeTelemetryBudget)
{
    uint32_t first = 0;

    ASSERT_EQ(0, Create(TELEMETRY_WORKER_LOOKUP_PATH, 30000, 1000, 500));
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    first = Pid();
    poll(NULL, 0, 1100);
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    EXPECT_EQ(first, Pid());
}

TEST_F(TelemetryWorkerTest, PreparationConsumesTheSameInvocationBudget)
{
    int64_t started = 0;

    ASSERT_EQ(0, Create(TELEMETRY_WORKER_LOOKUP_PATH, 600000, 500, 500));
    EXPECT_EQ(EINVAL, TelemetryWorkerAccountPreparation(worker, 0, nullptr));
    EXPECT_EQ(EINVAL, TelemetryWorkerAccountPreparation(nullptr, Now(), nullptr));
    started = Now();
    poll(nullptr, 0, 550);
    EXPECT_EQ(ETIMEDOUT, TelemetryWorkerAccountPreparation(worker, started, nullptr));
    EXPECT_EQ(ETIMEDOUT, TelemetryWorkerResolve(worker, "worker-pid", &result, nullptr));
}

TEST_F(TelemetryWorkerTest, InvocationLifetimeIsNotResetByRequests)
{
    ASSERT_EQ(0, Create(TELEMETRY_WORKER_LOOKUP_PATH, 1000, 10000, 5000));
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    poll(NULL, 0, 1100);
    EXPECT_EQ(ETIMEDOUT, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    EXPECT_EQ(0U, result.count);
    EXPECT_EQ(ETIMEDOUT, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
}

TEST_F(TelemetryWorkerTest, RejectsMalformedRepliesAndHandlesSplitFrames)
{
    const unsigned char empty[sizeof(result)] = {};

    ASSERT_EQ(0, Create(TELEMETRY_WORKER_FAULT_PATH));

    for (const char* mode : {"wrong-version", "wrong-operation", "wrong-sequence", "huge",
        "negative-status", "bad-count", "bad-family", "short", "extra"})
    {
        SCOPED_TRACE(mode);
        memset(&result, 0xA5, sizeof(result));
        EXPECT_EQ(EPROTO, TelemetryWorkerResolve(worker, mode, &result, NULL));
        EXPECT_EQ(0, memcmp(empty, &result, sizeof(result)));
    }

    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "split", &result, NULL));
    EXPECT_EQ(1U, result.count);
}

TEST_F(TelemetryWorkerTest, TerminatesWorkersThatStopFollowingTheProtocol)
{
    ASSERT_EQ(0, Create(TELEMETRY_WORKER_FAULT_PATH, 30000, 10000, 1000));
    EXPECT_EQ(EPIPE, TelemetryWorkerResolve(worker, "exit", &result, NULL));
    EXPECT_EQ(EPIPE, TelemetryWorkerResolve(worker, "close-hang", &result, NULL));
    EXPECT_EQ(ETIMEDOUT, TelemetryWorkerResolve(worker, "short-hang", &result, NULL));
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "reply-hang", &result, NULL));
    EXPECT_EQ(ETIMEDOUT, TelemetryWorkerDestroy(&worker, NULL));
    EXPECT_EQ(nullptr, worker);
}

TEST_F(TelemetryWorkerTest, RejectsAutoReapingAndPreservesHostSignalDisposition)
{
    struct sigaction ignored = {}, original = {}, observed = {}, pipeBefore = {}, pipeAfter = {};
    int status = 0;
    int observeStatus = 0;
    int restoreStatus = 0;

    ASSERT_EQ(0, Create());
    ASSERT_EQ(0, sigaction(SIGPIPE, NULL, &pipeBefore));
    ignored.sa_handler = SIG_IGN;
    sigemptyset(&ignored.sa_mask);
    ASSERT_EQ(0, sigaction(SIGCHLD, &ignored, &original));
    status = TelemetryWorkerResolve(worker, "worker-pid", &result, NULL);
    observeStatus = sigaction(SIGCHLD, NULL, &observed);
    restoreStatus = sigaction(SIGCHLD, &original, NULL);
    EXPECT_EQ(ENOTSUP, status);
    EXPECT_EQ(0, observeStatus);
    EXPECT_EQ(SIG_IGN, observed.sa_handler);
    ASSERT_EQ(0, restoreStatus);
    ASSERT_EQ(0, TelemetryWorkerResolve(worker, "worker-pid", &result, NULL));
    ASSERT_EQ(0, sigaction(SIGPIPE, NULL, &pipeAfter));
    EXPECT_EQ(pipeBefore.sa_handler, pipeAfter.sa_handler);
}

TEST(TelemetryWorkerProtocolTest, ParentDisconnectEndsIdleWorker)
{
    RawWorker worker;
    int status = 0;

    ASSERT_EQ(0, worker.Start(TELEMETRY_WORKER_PATH));
    ASSERT_TRUE(worker.Ready());
    worker.Disconnect();
    ASSERT_TRUE(worker.Wait(&status));
    ASSERT_TRUE(WIFEXITED(status));
    EXPECT_EQ(0, WEXITSTATUS(status));
}

TEST(TelemetryWorkerProtocolTest, SelfTimeoutSurvivesInheritedIgnoredAndBlockedAlarm)
{
    RawWorker worker;
    struct sigaction ignored = {}, original = {};
    sigset_t blocked = {}, originalMask = {};
    int maskStatus = 0;
    int spawnStatus = 0;
    int status = 0;

    ignored.sa_handler = SIG_IGN;
    sigemptyset(&ignored.sa_mask);
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGALRM);
    ASSERT_EQ(0, sigaction(SIGALRM, &ignored, &original));

    if (0 != (maskStatus = sigprocmask(SIG_BLOCK, &blocked, &originalMask)))
    {
        sigaction(SIGALRM, &original, NULL);
        FAIL() << "Cannot set inherited alarm mask";
    }

    spawnStatus = worker.Start(TELEMETRY_WORKER_PATH, 1000);
    EXPECT_EQ(0, sigaction(SIGALRM, &original, NULL));
    EXPECT_EQ(0, sigprocmask(SIG_SETMASK, &originalMask, NULL));
    ASSERT_EQ(0, spawnStatus);
    ASSERT_TRUE(worker.Ready());
    ASSERT_TRUE(worker.Wait(&status));
    ASSERT_TRUE(WIFSIGNALED(status));
    EXPECT_EQ(SIGALRM, WTERMSIG(status));
}

TEST(TelemetryWorkerProtocolTest, ParentDisconnectDuringBlockedDnsStillSelfTerminates)
{
    RawWorker worker;
    char host[] = "hang";
    TelemetryWorkerFrame request = {};
    int status = 0;

    ASSERT_EQ(0, worker.Start(TELEMETRY_WORKER_LOOKUP_PATH));
    ASSERT_TRUE(worker.Ready());
    request = Request(worker, sizeof(host));
    request.deadline = Now() + 1000000000;
    ASSERT_TRUE(worker.Write(&request, sizeof(request)));
    ASSERT_TRUE(worker.Write(host, sizeof(host)));
    worker.Disconnect();
    ASSERT_TRUE(worker.Wait(&status));
    ASSERT_TRUE(WIFSIGNALED(status));
    EXPECT_EQ(SIGALRM, WTERMSIG(status));
}

TEST(TelemetryWorkerProtocolTest, RejectsMalformedAndMismatchedRequests)
{
    int mode = 0;
    TelemetryWorkerFrame request = {};
    TelemetryWorkerFrame response = {};
    int status = 0;

    for (mode = 0; mode < 8; ++mode)
    {
        RawWorker worker;

        SCOPED_TRACE(mode);
        ASSERT_EQ(0, worker.Start(TELEMETRY_WORKER_PATH));
        ASSERT_TRUE(worker.Ready());
        request = Request(worker, 2);

        switch (mode)
        {
        case 0:
            ++request.magic;
            break;

        case 1:
            ++request.version;
            break;

        case 2:
            request.operation = UINT32_MAX;
            break;

        case 3:
            request.sequence = 0;
            break;

        case 4:
            request.size = UINT32_MAX;
            break;

        case 5:
            request.deadline = INT64_MAX;
            break;

        case 6:
            request.status = -1;
            break;

        case 7:
            request.deadline = 0;
            break;
        }

        ASSERT_TRUE(worker.Write(&request, sizeof(request)));
        response = {};
        ASSERT_TRUE(worker.Read(&response, sizeof(response)));
        EXPECT_EQ(EPROTO, response.status);
        EXPECT_EQ(0U, response.size);
        status = 0;
        ASSERT_TRUE(worker.Wait(&status));
        ASSERT_TRUE(WIFEXITED(status));
        EXPECT_NE(0, WEXITSTATUS(status));
    }
}

TEST(TelemetryWorkerProtocolTest, SelfTerminatesDuringIncompleteRequestBody)
{
    RawWorker worker;
    TelemetryWorkerFrame request = {};
    int status = 0;

    ASSERT_EQ(0, worker.Start(TELEMETRY_WORKER_PATH));
    ASSERT_TRUE(worker.Ready());
    request = Request(worker, 4);
    request.deadline = Now() + 1000000000;
    ASSERT_TRUE(worker.Write(&request, sizeof(request)));
    ASSERT_TRUE(worker.Write("a", 1));
    ASSERT_TRUE(worker.Wait(&status));
    ASSERT_TRUE(WIFSIGNALED(status));
    EXPECT_EQ(SIGALRM, WTERMSIG(status));
}

TEST(TelemetryWorkerProtocolTest, RejectsEmbeddedNullAndUnterminatedHost)
{
    TelemetryWorkerFrame request = {};
    TelemetryWorkerFrame response = {};

    for (const std::string& host : {std::string("a\0b\0", 4), std::string("abc", 3)})
    {
        RawWorker worker;

        ASSERT_EQ(0, worker.Start(TELEMETRY_WORKER_PATH));
        ASSERT_TRUE(worker.Ready());
        request = Request(worker, static_cast<uint32_t>(host.size()));
        ASSERT_TRUE(worker.Write(&request, sizeof(request)));
        ASSERT_TRUE(worker.Write(host.data(), host.size()));
        response = {};
        ASSERT_TRUE(worker.Read(&response, sizeof(response)));
        EXPECT_EQ(EINVAL, response.status);
    }
}

TEST(TelemetryWorkerProtocolTest, DoesNotRetainUnrelatedInheritedDescriptors)
{
    int descriptors[2] = {-1, -1};
    RawWorker worker;
    int status = 0;
    bool ready = false;
    struct pollfd item = {};
    int polled = 0;
    char byte = 0;
    ssize_t count = 0;

    ASSERT_EQ(0, pipe(descriptors));
    status = worker.Start(TELEMETRY_WORKER_PATH);
    close(descriptors[1]);
    ready = (0 == status) && (worker.Ready());
    item = {descriptors[0], POLLIN, 0};
    polled = poll(&item, 1, 1000);
    count = (polled > 0) ? read(descriptors[0], &byte, 1) : -1;
    close(descriptors[0]);
    ASSERT_EQ(0, status);
    ASSERT_TRUE(ready);
    ASSERT_GT(polled, 0);
    EXPECT_EQ(0, count);
}

TEST(TelemetryWorkerDeathTest, ReapsOwnedChildrenAndPreservesOtherChildren)
{
    EXPECT_EXIT(
        {
            pid_t other = -1;
            TelemetryWorker* worker = NULL;
            TelemetryResolvedHost result = {};
            int status = 0;
            pid_t waited = 0;

            other = fork();

            if (0 == other)
            {
                _exit(37);
            }

            if (other < 0)
            {
                _exit(1);
            }

            if (0 != TelemetryWorkerCreate(TELEMETRY_WORKER_FAULT_PATH, 30000, 10000, 100, &worker, true, NULL))
            {
                _exit(2);
            }

            for (const char* mode : {"wrong-version", "short", "close-hang", "hang"})
            {
                if (0 == TelemetryWorkerResolve(worker, mode, &result, NULL))
                {
                    _exit(3);
                }
            }

            if (0 != TelemetryWorkerDestroy(&worker, NULL))
            {
                _exit(4);
            }

            do
            {
                waited = waitpid(other, &status, 0);
            } while ((waited < 0) && (EINTR == errno));

            if ((waited != other) || (!WIFEXITED(status)) || (37 != WEXITSTATUS(status)))
            {
                _exit(5);
            }

            errno = 0;

            if ((-1 != waitpid(-1, NULL, WNOHANG)) || (ECHILD != errno))
            {
                _exit(6);
            }

            _exit(0);
        }, ::testing::ExitedWithCode(0), "");
}

TEST(TelemetryWorkerDeathTest, HandlesClosedStandardDescriptors)
{
    EXPECT_EXIT(
        {
            TelemetryWorker* worker = NULL;
            TelemetryResolvedHost result = {};

            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

            if (0 != TelemetryWorkerCreate(TELEMETRY_WORKER_LOOKUP_PATH, 30000, 10000, 5000, &worker, true, NULL))
            {
                _exit(1);
            }

            if (0 != TelemetryWorkerResolve(worker, "many", &result, NULL))
            {
                _exit(2);
            }

            if (0 != TelemetryWorkerDestroy(&worker, NULL))
            {
                _exit(3);
            }

            errno = 0;

            if ((-1 != waitpid(-1, NULL, WNOHANG)) || (ECHILD != errno))
            {
                _exit(4);
            }

            _exit(0);
        }, ::testing::ExitedWithCode(0), "");
}
