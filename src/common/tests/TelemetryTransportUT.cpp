// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <AriaInterface.h>
#include <Utils.h>
#include <OpenSslAdapter.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <time.h>
#include <vector>

namespace
{
// A separately named transport uses a deterministic clock and connect results.
// Real socket descriptors and cleanup are retained; no external traffic is sent.
int64_t now = 0, lookupTime = 0, tlsDeadline = 0, connectedAt = 0;
std::vector<int> families, outcomes, descriptors, waits;
size_t attempts = 0;
int socketError = 0;
bool interruptPoll = false, finishPoll = false, reachedTls = false;
bool clockFails = false, timerFails = false;
int clockError = 0, timerError = 0, clockCalls = 0, timerCalls = 0, timerFlags = 0;
timer_t armedTimer = {};
struct itimerspec armedExpiration = {};
const char* proxyVariables[] = {"https_proxy", "HTTPS_PROXY", "all_proxy", "ALL_PROXY", "no_proxy", "NO_PROXY"};
}

extern "C" int clock_gettime(clockid_t clock, struct timespec* value) noexcept
{
    EXPECT_EQ(CLOCK_MONOTONIC, clock);
    ++clockCalls;
    if (clockFails)
    {
        errno = clockError;
        return -1;
    }
    value->tv_sec = static_cast<time_t>(now / 1000000000);
    value->tv_nsec = static_cast<long>(now % 1000000000);
    return 0;
}

extern "C" int timer_settime(timer_t timer, int flags, const struct itimerspec* value, struct itimerspec* previous) noexcept
{
    EXPECT_EQ(nullptr, previous);
    ++timerCalls;
    armedTimer = timer;
    timerFlags = flags;
    armedExpiration = *value;
    if (timerFails)
    {
        errno = timerError;
        return -1;
    }
    return 0;
}

extern "C" void TelemetryLookupHost(const char*, TelemetryResolverReply* reply)
{
    *reply = {};
    reply->version = TELEMETRY_RESOLVER_PROTOCOL_VERSION;
    reply->count = static_cast<uint32_t>(families.size());
    for (size_t i = 0; i < families.size(); ++i)
    {
        reply->addresses[i].family = static_cast<uint32_t>(families[i]);
        reply->addresses[i].bytes[AF_INET == families[i] ? 3 : 15] = 1;
        if (AF_INET == families[i])
        {
            reply->addresses[i].bytes[0] = 127;
        }
    }
    now += lookupTime;
}

extern "C" int connect(int descriptor, const struct sockaddr* address, socklen_t size)
{
    EXPECT_LT(attempts, outcomes.size());
    EXPECT_EQ(families.at(attempts), address->sa_family);
    EXPECT_EQ(AF_INET == address->sa_family ? sizeof(sockaddr_in) : sizeof(sockaddr_in6), size);
    descriptors.push_back(descriptor);
    const int result = outcomes.at(attempts++);
    errno = result;
    return result ? -1 : 0;
}

extern "C" int TelemetryTransportTestPoll(struct pollfd* items, nfds_t count, int milliseconds)
{
    EXPECT_EQ(1U, count);
    EXPECT_EQ(POLLOUT, items[0].events);
    EXPECT_GT(milliseconds, 0);
    waits.push_back(milliseconds);
    if (interruptPoll)
    {
        interruptPoll = false;
        now += 10000000;
        errno = EINTR;
        return -1;
    }
    if (finishPoll)
    {
        finishPoll = false;
        items[0].revents = POLLOUT;
        return 1;
    }
    now += static_cast<int64_t>(milliseconds) * 1000000;
    return 0;
}

extern "C" int getsockopt(int, int level, int option, void* value, socklen_t* size) noexcept
{
    EXPECT_EQ(SOL_SOCKET, level);
    EXPECT_EQ(SO_ERROR, option);
    EXPECT_EQ(sizeof(int), *size);
    memcpy(value, &socketError, sizeof(socketError));
    return 0;
}

extern "C" int TelemetryTlsCreate(TelemetryTls**, int64_t deadline, OsConfigLogHandle)
{
    reachedTls = true;
    connectedAt = now;
    tlsDeadline = deadline;
    // Stop after TCP selection; real TLS/HTTP exchanges have their own peer suite.
    return ENOTSUP;
}

class TelemetryTransportConnectTest : public ::testing::Test
{
protected:
    TelemetryTransport* transport = nullptr;
    std::map<std::string, std::string> environment;
    const int64_t start = INT64_C(1000000000);
    const int64_t deadline = INT64_C(1500000000);

    void SetUp() override
    {
        for (const char* name : proxyVariables)
        {
            const char* value = getenv(name);
            if (nullptr != value)
            {
                environment[name] = value;
            }
            ASSERT_EQ(0, unsetenv(name));
        }
        now = start;
        clockFails = timerFails = false;
        lookupTime = tlsDeadline = connectedAt = 0;
        families = {AF_INET, AF_INET};
        outcomes = {EINPROGRESS, 0};
        descriptors.clear();
        waits.clear();
        attempts = 0;
        socketError = 0;
        interruptPoll = finishPoll = reachedTls = false;
        ASSERT_EQ(0, TelemetryTransportCreate(&transport, nullptr));
    }

    void TearDown() override
    {
        TelemetryTransportDestroy(&transport, nullptr);
        for (const char* name : proxyVariables)
        {
            const auto found = environment.find(name);
            EXPECT_EQ(0, found == environment.end() ? unsetenv(name) : setenv(name, found->second.c_str(), 1));
        }
        for (const int descriptor : descriptors)
        {
            errno = 0;
            EXPECT_EQ(-1, fcntl(descriptor, F_GETFD));
            EXPECT_EQ(EBADF, errno);
        }
    }

    int Send()
    {
        TelemetryHttpResponse response = {};
        return TelemetryTransportSend(transport, "fixture-token", "OSConfig-C/test", 1, "x", 1,
            deadline, &response, nullptr);
    }
};

TEST_F(TelemetryTransportConnectTest, BlackholedAddressLeavesHalfDeadlineForFallback)
{
    EXPECT_EQ(ENOTSUP, Send());
    EXPECT_EQ(2U, attempts);
    ASSERT_EQ(1U, waits.size());
    EXPECT_EQ(250, waits[0]);
    EXPECT_TRUE(reachedTls);
    EXPECT_EQ(start + 250000000, connectedAt);
    EXPECT_EQ(deadline, tlsDeadline);
}

TEST_F(TelemetryTransportConnectTest, CanFallBackFromIpv6ToIpv4)
{
    families = {AF_INET6, AF_INET};
    EXPECT_EQ(ENOTSUP, Send());
    EXPECT_EQ(2U, attempts);
    EXPECT_TRUE(reachedTls);
    EXPECT_EQ(start + 250000000, connectedAt);
}

TEST_F(TelemetryTransportConnectTest, DnsTimeIsNotAddedBackToDeadline)
{
    lookupTime = 100000000;
    EXPECT_EQ(ENOTSUP, Send());
    ASSERT_EQ(1U, waits.size());
    EXPECT_EQ(200, waits[0]);
    EXPECT_EQ(start + 300000000, connectedAt);
    EXPECT_EQ(deadline, tlsDeadline);
}

TEST_F(TelemetryTransportConnectTest, InterruptDoesNotResetAddressAllowance)
{
    interruptPoll = true;
    EXPECT_EQ(ENOTSUP, Send());
    ASSERT_EQ(2U, waits.size());
    EXPECT_EQ(250, waits[0]);
    EXPECT_EQ(240, waits[1]);
    EXPECT_EQ(start + 250000000, connectedAt);
}

TEST_F(TelemetryTransportConnectTest, ImmediateFailureLeavesRemainingTimeUnused)
{
    outcomes = {ECONNREFUSED, 0};
    EXPECT_EQ(ENOTSUP, Send());
    EXPECT_TRUE(waits.empty());
    EXPECT_EQ(start, connectedAt);
    EXPECT_EQ(deadline, tlsDeadline);
}

TEST_F(TelemetryTransportConnectTest, CompletedFailedConnectTriesNextAddress)
{
    finishPoll = true;
    socketError = ECONNREFUSED;
    EXPECT_EQ(ENOTSUP, Send());
    EXPECT_EQ(2U, attempts);
    EXPECT_EQ(start, connectedAt);
}

TEST_F(TelemetryTransportConnectTest, AllBlackholedAddressesShareOriginalDeadline)
{
    families.assign(TELEMETRY_RESOLVER_ADDRESS_LIMIT, AF_INET);
    outcomes.assign(TELEMETRY_RESOLVER_ADDRESS_LIMIT, EINPROGRESS);
    EXPECT_EQ(ETIMEDOUT, Send());
    EXPECT_EQ(families.size(), attempts);
    EXPECT_FALSE(reachedTls);
    EXPECT_EQ(deadline, now);
}

TEST_F(TelemetryTransportConnectTest, SingleAddressRetainsFullAllowance)
{
    families = {AF_INET};
    outcomes = {EINPROGRESS};
    EXPECT_EQ(ETIMEDOUT, Send());
    ASSERT_EQ(1U, waits.size());
    EXPECT_EQ(500, waits[0]);
    EXPECT_EQ(deadline, now);
}

TEST_F(TelemetryTransportConnectTest, TlsFailureDoesNotTryAnotherAddress)
{
    outcomes = {0, 0};
    EXPECT_EQ(ENOTSUP, Send());
    EXPECT_EQ(1U, attempts);
    EXPECT_TRUE(reachedTls);
    EXPECT_EQ(start, connectedAt);
}

class TelemetryDeadlineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        now = INT64_C(1000000000);
        clockFails = timerFails = false;
        clockError = timerError = clockCalls = timerCalls = timerFlags = 0;
        armedTimer = {};
        armedExpiration = {};
    }

    void TearDown() override
    {
        clockFails = timerFails = false;
    }
};

TEST_F(TelemetryDeadlineTest, MonotonicTimeIsOneStatementAndEvaluatesArgumentsOnce)
{
    int64_t values[] = {-1, -1};
    int statuses[] = {-1, -1};
    size_t valueIndex = 0, statusIndex = 0;

    if (true)
        TelemetryMonotonicTime(&values[valueIndex++], statuses[statusIndex++]);
    else
        FAIL() << "Macro changed the enclosing conditional";

    EXPECT_EQ(1U, valueIndex);
    EXPECT_EQ(1U, statusIndex);
    EXPECT_EQ(now, values[0]);
    EXPECT_EQ(-1, values[1]);
    EXPECT_EQ(0, statuses[0]);
    EXPECT_EQ(-1, statuses[1]);
    EXPECT_EQ(1, clockCalls);
}

TEST_F(TelemetryDeadlineTest, RemainingTimeEvaluatesArgumentsOnceAndRoundsUp)
{
    const int64_t deadlines[] = {now + 1000001, 0};
    int remaining[] = {-1, -1}, statuses[] = {-1, -1};
    size_t deadlineIndex = 0, remainingIndex = 0, statusIndex = 0;

    TelemetryDeadlineRemaining(deadlines[deadlineIndex++], &remaining[remainingIndex++], statuses[statusIndex++]);

    EXPECT_EQ(1U, deadlineIndex);
    EXPECT_EQ(1U, remainingIndex);
    EXPECT_EQ(1U, statusIndex);
    EXPECT_EQ(2, remaining[0]);
    EXPECT_EQ(-1, remaining[1]);
    EXPECT_EQ(0, statuses[0]);
    EXPECT_EQ(-1, statuses[1]);
    EXPECT_EQ(1, clockCalls);
}

TEST_F(TelemetryDeadlineTest, RemainingTimePreservesExpiredOutputAndClampsLargeIntervals)
{
    const int64_t deadlines[] = {now - 1, now, now + 1, now + 1000000, INT64_MAX};
    const int expectedStatus[] = {ETIMEDOUT, ETIMEDOUT, 0, 0, 0};
    const int expectedRemaining[] = {-1, -1, 1, 1, INT_MAX};
    int remaining = 0, status = 0;

    for (size_t i = 0; i < ARRAY_SIZE(deadlines); ++i)
    {
        remaining = -1;
        TelemetryDeadlineRemaining(deadlines[i], &remaining, status);
        EXPECT_EQ(expectedStatus[i], status);
        EXPECT_EQ(expectedRemaining[i], remaining);
    }
}

TEST_F(TelemetryDeadlineTest, ClockFailuresPreserveOutputsAndPropagateThroughNestedMacros)
{
    int64_t value = -1;
    int remaining = -1, status = 0;
    timer_t timer = {};
    clockFails = true;

    for (int error : {EACCES, 0})
    {
        clockError = error;
        const int expected = error ? error : EIO;
        TelemetryMonotonicTime(&value, status);
        EXPECT_EQ(expected, status);
        EXPECT_EQ(-1, value);
        TelemetryDeadlineRemaining(now + 1, &remaining, status);
        EXPECT_EQ(expected, status);
        EXPECT_EQ(-1, remaining);
        TelemetryArmDeadline(timer, now + 1, status);
        EXPECT_EQ(expected, status);
    }
    EXPECT_EQ(0, timerCalls);
}

TEST_F(TelemetryDeadlineTest, RejectsUnrepresentableMonotonicTime)
{
    int64_t value = -1;
    int status = 0;
    now = -INT64_C(1000000000);
    TelemetryMonotonicTime(&value, status);
    EXPECT_EQ(EOVERFLOW, status);
    EXPECT_EQ(-1, value);

    if (sizeof(time_t) >= sizeof(int64_t))
    {
        now = INT64_MAX;
        TelemetryMonotonicTime(&value, status);
        EXPECT_EQ(EOVERFLOW, status);
        EXPECT_EQ(-1, value);
    }
}

TEST_F(TelemetryDeadlineTest, ArmsAbsoluteTimerWithSingleArgumentEvaluation)
{
    timer_t timers[2] = {};
    const int64_t deadlines[] = {now + 123456789, 0};
    int statuses[] = {-1, -1};
    size_t timerIndex = 0, deadlineIndex = 0, statusIndex = 0;

    TelemetryArmDeadline(timers[timerIndex++], deadlines[deadlineIndex++], statuses[statusIndex++]);

    EXPECT_EQ(1U, timerIndex);
    EXPECT_EQ(1U, deadlineIndex);
    EXPECT_EQ(1U, statusIndex);
    EXPECT_EQ(0, statuses[0]);
    EXPECT_EQ(-1, statuses[1]);
    EXPECT_EQ(1, clockCalls);
    EXPECT_EQ(1, timerCalls);
    EXPECT_EQ(timers[0], armedTimer);
    EXPECT_EQ(TIMER_ABSTIME, timerFlags);
    EXPECT_EQ(1, armedExpiration.it_value.tv_sec);
    EXPECT_EQ(123456789, armedExpiration.it_value.tv_nsec);
    EXPECT_EQ(0, armedExpiration.it_interval.tv_sec);
    EXPECT_EQ(0, armedExpiration.it_interval.tv_nsec);
}

TEST_F(TelemetryDeadlineTest, RejectsInvalidOrExpiredTimersWithoutArming)
{
    int status = 0;
    timer_t timer = {};

    TelemetryArmDeadline(timer, 0, status);
    EXPECT_EQ(EINVAL, status);
    TelemetryArmDeadline(timer, -1, status);
    EXPECT_EQ(EINVAL, status);
    EXPECT_EQ(0, clockCalls);
    TelemetryArmDeadline(timer, now, status);
    EXPECT_EQ(ETIMEDOUT, status);
    if (sizeof(time_t) < sizeof(int64_t))
    {
        TelemetryArmDeadline(timer, INT64_MAX, status);
        EXPECT_EQ(EINVAL, status);
    }
    EXPECT_EQ(0, timerCalls);
}

TEST_F(TelemetryDeadlineTest, ReportsTimerFailureWithErrnoFallback)
{
    int status = 0;
    timer_t timer = {};
    timerFails = true;

    for (int error : {EACCES, 0})
    {
        timerError = error;
        TelemetryArmDeadline(timer, now + 1, status);
        EXPECT_EQ(error ? error : EIO, status);
    }
    EXPECT_EQ(2, timerCalls);
}
