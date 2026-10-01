// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <TelemetryResolver.h>

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <poll.h>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char** environ;

class TelemetryResolverTest : public ::testing::Test
{
protected:
    TelemetryResolvedHost result = {};

    static int64_t MonotonicMilliseconds()
    {
        struct timespec now = {};
        if (0 != clock_gettime(CLOCK_MONOTONIC, &now))
        {
            return -1;
        }
        return static_cast<int64_t>(now.tv_sec) * 1000 + now.tv_nsec / 1000000;
    }
};

TEST_F(TelemetryResolverTest, ResolvesNumericIpv4WithoutExternalDns)
{
    ASSERT_EQ(0, TelemetryResolveHost(TELEMETRY_RESOLVER_WORKER_PATH, "127.0.0.1", 5000, &result, NULL));
    ASSERT_GT(result.count, 0U);
    ASSERT_EQ(sizeof(struct sockaddr_in), result.lengths[0]);
    struct sockaddr_in address = {};
    memcpy(&address, &result.addresses[0], sizeof(address));
    EXPECT_EQ(AF_INET, address.sin_family);
    EXPECT_EQ(INADDR_LOOPBACK, ntohl(address.sin_addr.s_addr));
    EXPECT_EQ(0, address.sin_port);
}

TEST_F(TelemetryResolverTest, ResolvesNumericIpv6WithoutExternalDns)
{
    ASSERT_EQ(0, TelemetryResolveHost(TELEMETRY_RESOLVER_WORKER_PATH, "::1", 5000, &result, NULL));
    ASSERT_GT(result.count, 0U);
    struct sockaddr_in6 address = {};
    memcpy(&address, &result.addresses[0], sizeof(address));
    EXPECT_EQ(AF_INET6, address.sin6_family);
    EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(&address.sin6_addr));
    EXPECT_EQ(0, address.sin6_port);
}

TEST_F(TelemetryResolverTest, PreservesIpv6ScopeAndCapsAddressCount)
{
    ASSERT_EQ(0, TelemetryResolveHost(TELEMETRY_RESOLVER_LOOKUP_WORKER_PATH, "ipv6", 5000, &result, NULL));
    ASSERT_EQ(1U, result.count);
    struct sockaddr_in6 address = {};
    memcpy(&address, &result.addresses[0], sizeof(address));
    EXPECT_EQ(7U, address.sin6_scope_id);
    EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(&address.sin6_addr));

    ASSERT_EQ(0, TelemetryResolveHost(TELEMETRY_RESOLVER_LOOKUP_WORKER_PATH, "many", 5000, &result, NULL));
    EXPECT_EQ(static_cast<size_t>(TELEMETRY_MAX_RESOLVED_ADDRESSES), result.count);
}

TEST_F(TelemetryResolverTest, ReportsResolverErrorsWithoutOutput)
{
    EXPECT_EQ(EHOSTUNREACH, TelemetryResolveHost(TELEMETRY_RESOLVER_LOOKUP_WORKER_PATH,
        "lookup-failure", 5000, &result, NULL));
    EXPECT_EQ(0U, result.count);
    EXPECT_EQ(EACCES, TelemetryResolveHost(TELEMETRY_RESOLVER_LOOKUP_WORKER_PATH,
        "system-failure", 5000, &result, NULL));
    EXPECT_EQ(0U, result.count);
    EXPECT_EQ(EHOSTUNREACH, TelemetryResolveHost(TELEMETRY_RESOLVER_LOOKUP_WORKER_PATH,
        "unsupported", 5000, &result, NULL));
    EXPECT_EQ(0U, result.count);
}

TEST_F(TelemetryResolverTest, RejectsInvalidArgumentsAndMissingExecutable)
{
    EXPECT_EQ(EINVAL, TelemetryResolveHost(NULL, "127.0.0.1", 5000, &result, NULL));
    EXPECT_EQ(EINVAL, TelemetryResolveHost("relative-worker", "127.0.0.1", 5000, &result, NULL));
    EXPECT_EQ(EINVAL, TelemetryResolveHost(TELEMETRY_RESOLVER_WORKER_PATH, NULL, 5000, &result, NULL));
    EXPECT_EQ(EINVAL, TelemetryResolveHost(TELEMETRY_RESOLVER_WORKER_PATH, "", 5000, &result, NULL));
    EXPECT_EQ(EINVAL, TelemetryResolveHost(TELEMETRY_RESOLVER_WORKER_PATH, "127.0.0.1", 0, &result, NULL));
    EXPECT_EQ(EINVAL, TelemetryResolveHost(TELEMETRY_RESOLVER_WORKER_PATH, "127.0.0.1", -1, &result, NULL));
    EXPECT_EQ(EINVAL, TelemetryResolveHost(TELEMETRY_RESOLVER_WORKER_PATH, "127.0.0.1", 5000, NULL, NULL));
    const std::string oversized(254, 'x');
    EXPECT_EQ(EINVAL, TelemetryResolveHost(TELEMETRY_RESOLVER_WORKER_PATH, oversized.c_str(), 5000, &result, NULL));
    const std::string missing = std::string(TELEMETRY_RESOLVER_WORKER_PATH) + ".nonexistent";
    EXPECT_EQ(ENOENT, TelemetryResolveHost(missing.c_str(), "127.0.0.1", 5000, &result, NULL));
    EXPECT_EQ(0U, result.count);
}

TEST_F(TelemetryResolverTest, RejectsMalformedAndIncompleteReplies)
{
    for (const char* mode : {"wrong-version", "bad-count", "bad-family", "truncated", "oversized"})
    {
        SCOPED_TRACE(mode);
        memset(&result, 0xA5, sizeof(result));
        EXPECT_EQ(EPROTO, TelemetryResolveHost(TELEMETRY_RESOLVER_FAULT_WORKER_PATH, mode, 5000, &result, NULL));
        const unsigned char empty[sizeof(result)] = {};
        EXPECT_EQ(0, memcmp(empty, &result, sizeof(result)));
    }
    EXPECT_EQ(EIO, TelemetryResolveHost(TELEMETRY_RESOLVER_FAULT_WORKER_PATH,
        "nonzero-exit", 5000, &result, NULL));
}

TEST_F(TelemetryResolverTest, TimesOutHangingResolverAndUncooperativeWorkers)
{
    EXPECT_EQ(ETIMEDOUT, TelemetryResolveHost(TELEMETRY_RESOLVER_LOOKUP_WORKER_PATH,
        "hang", 200, &result, NULL));
    for (const char* mode : {"close-then-hang", "reply-then-hang"})
    {
        SCOPED_TRACE(mode);
        EXPECT_EQ(ETIMEDOUT, TelemetryResolveHost(TELEMETRY_RESOLVER_FAULT_WORKER_PATH,
            mode, 200, &result, NULL));
        EXPECT_EQ(0U, result.count);
    }
}

TEST_F(TelemetryResolverTest, RejectsAutoReapingWithoutChangingHostDisposition)
{
    struct sigaction ignored = {};
    struct sigaction original = {};
    struct sigaction observed = {};
    ignored.sa_handler = SIG_IGN;
    sigemptyset(&ignored.sa_mask);
    ASSERT_EQ(0, sigaction(SIGCHLD, &ignored, &original));
    int status = TelemetryResolveHost(TELEMETRY_RESOLVER_WORKER_PATH, "127.0.0.1", 5000, &result, NULL);
    int observedStatus = sigaction(SIGCHLD, NULL, &observed);
    int restoredStatus = sigaction(SIGCHLD, &original, NULL);
    EXPECT_EQ(ENOTSUP, status);
    EXPECT_EQ(0, observedStatus);
    EXPECT_EQ(SIG_IGN, observed.sa_handler);
    EXPECT_EQ(0, restoredStatus);
}

TEST_F(TelemetryResolverTest, WorkerSelfTerminatesWithoutParentTimeoutSignal)
{
    struct timespec deadline = {};
    ASSERT_EQ(0, clock_gettime(CLOCK_MONOTONIC, &deadline));
    ++deadline.tv_sec;
    char seconds[32];
    char nanoseconds[16];
    snprintf(seconds, sizeof(seconds), "%ld", static_cast<long>(deadline.tv_sec));
    snprintf(nanoseconds, sizeof(nanoseconds), "%ld", deadline.tv_nsec);
    char path[] = TELEMETRY_RESOLVER_LOOKUP_WORKER_PATH;
    char host[] = "hang";
    char* arguments[] = {path, host, seconds, nanoseconds, NULL};
    pid_t child = -1;
    struct sigaction ignored = {};
    struct sigaction original = {};
    sigset_t blocked;
    sigset_t originalMask;
    ignored.sa_handler = SIG_IGN;
    sigemptyset(&ignored.sa_mask);
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGALRM);
    ASSERT_EQ(0, sigaction(SIGALRM, &ignored, &original));
    int maskStatus = sigprocmask(SIG_BLOCK, &blocked, &originalMask);
    if (0 != maskStatus)
    {
        sigaction(SIGALRM, &original, NULL);
        FAIL() << "Cannot prepare inherited timer signal mask";
    }
    int spawnStatus = posix_spawn(&child, path, NULL, NULL, arguments, environ);
    EXPECT_EQ(0, sigaction(SIGALRM, &original, NULL));
    EXPECT_EQ(0, sigprocmask(SIG_SETMASK, &originalMask, NULL));
    ASSERT_EQ(0, spawnStatus);
    const int64_t start = MonotonicMilliseconds();
    int status = 0;
    pid_t waited = 0;
    while (0 == waited)
    {
        waited = waitpid(child, &status, WNOHANG);
        if ((waited < 0) && (EINTR == errno))
        {
            waited = 0;
        }
        const int64_t now = MonotonicMilliseconds();
        if ((now < 0) || (start < 0) || (now - start >= 5000))
        {
            break;
        }
        if (0 == waited)
        {
            poll(NULL, 0, 10);
        }
    }
    if (0 == waited)
    {
        kill(child, SIGKILL);
        while ((waitpid(child, &status, 0) < 0) && (EINTR == errno))
        {
        }
        FAIL() << "Worker did not enforce its own deadline";
    }
    ASSERT_EQ(child, waited);
    ASSERT_TRUE(WIFSIGNALED(status));
    EXPECT_EQ(SIGALRM, WTERMSIG(status));
}

TEST(TelemetryResolverDeathTest, ReapsEveryOwnedChildAndLeavesOtherChildrenAlone)
{
    // The isolated test process owns no unrelated harness children.
    EXPECT_EXIT(
        {
            TelemetryResolvedHost result = {};
            for (const char* mode : {"wrong-version", "truncated", "close-then-hang", "reply-then-hang"})
            {
                if (0 == TelemetryResolveHost(TELEMETRY_RESOLVER_FAULT_WORKER_PATH, mode, 100, &result, NULL))
                {
                    _exit(1);
                }
                errno = 0;
                if ((-1 != waitpid(-1, NULL, WNOHANG)) || (ECHILD != errno))
                {
                    _exit(2);
                }
            }
            pid_t other = fork();
            if (0 == other)
            {
                _exit(37);
            }
            if (other < 0)
            {
                _exit(3);
            }
            int resolveStatus = TelemetryResolveHost(TELEMETRY_RESOLVER_WORKER_PATH,
                "127.0.0.1", 5000, &result, NULL);
            int status = 0;
            pid_t waited;
            do
            {
                waited = waitpid(other, &status, 0);
            } while ((waited < 0) && (EINTR == errno));
            if ((waited != other) || !WIFEXITED(status) || (37 != WEXITSTATUS(status)))
            {
                _exit(4);
            }
            if (0 != resolveStatus)
            {
                _exit(3);
            }
            errno = 0;
            if ((-1 != waitpid(-1, NULL, WNOHANG)) || (ECHILD != errno))
            {
                _exit(5);
            }
            _exit(0);
        },
        ::testing::ExitedWithCode(0), "");
}
