// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <TelemetryTls.h>
#include <TelemetryDeadline.h>
#include <TelemetryTransport.h>
#include <TelemetryWorker.h>

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char** environ;

namespace
{
int64_t Deadline(int milliseconds = 5000)
{
    int64_t now = 0;
    if (0 != TelemetryMonotonicTime(&now)) return 0;
    return now + static_cast<int64_t>(milliseconds) * 1000000;
}

class Peer
{
public:
    TelemetryTls* tls = NULL;
    int descriptor = -1;
    pid_t child = -1;
    std::string directory;

    ~Peer()
    {
        TelemetryTlsDestroy(&tls, NULL);
        if (descriptor >= 0) close(descriptor);
        if (child > 0)
        {
            pid_t waited;
            do { waited = waitpid(child, NULL, WNOHANG); } while (waited < 0 && errno == EINTR);
            if (waited == 0)
            {
                kill(child, SIGKILL);
                do { waited = waitpid(child, NULL, 0); } while (waited < 0 && errno == EINTR);
            }
        }
        if (!directory.empty())
        {
            for (const char* file : {"server-key.pem", "server.pem", "root-key.pem", "root.pem", "openssl.cnf"})
            {
                unlink((directory + "/" + file).c_str());
            }
            rmdir(directory.c_str());
        }
    }

    int Wait()
    {
        if (child <= 0) return ECHILD;
        int64_t deadline = Deadline(15000);
        for (;;)
        {
            int result = 0;
            pid_t done = waitpid(child, &result, WNOHANG);
            if (done == child)
            {
                child = -1;
                return WIFEXITED(result) && WEXITSTATUS(result) == 0 ? 0 : EIO;
            }
            if (done < 0 && errno != EINTR) return errno;
            int remaining = 0;
            int status = TelemetryDeadlineRemaining(deadline, &remaining);
            if (status) return status;
            poll(NULL, 0, 5);
        }
    }

    int Start(const char* mode, bool tlsSocket = true)
    {
        char temporary[] = "/tmp/osconfig-tls-test-XXXXXX";
        if (NULL == mkdtemp(temporary)) return errno;
        directory = temporary;
        int output[2] = {-1, -1};
        if (0 != pipe(output)) return errno;
        posix_spawn_file_actions_t actions;
        int status = posix_spawn_file_actions_init(&actions);
        if (status)
        {
            close(output[0]);
            close(output[1]);
            return status;
        }
        char* arguments[] = {
            const_cast<char*>(TELEMETRY_TLS_PYTHON),
            const_cast<char*>(TELEMETRY_TLS_PEER_SCRIPT),
            const_cast<char*>(TELEMETRY_TLS_OPENSSL),
            const_cast<char*>(directory.c_str()), const_cast<char*>(mode), NULL
        };
        if ((0 == (status = posix_spawn_file_actions_adddup2(&actions, output[1], STDOUT_FILENO))) &&
            (0 == (status = posix_spawn_file_actions_addclose(&actions, output[0]))) &&
            (0 == (status = posix_spawn_file_actions_addclose(&actions, output[1]))))
        {
            status = posix_spawn(&child, TELEMETRY_TLS_PYTHON, &actions, NULL, arguments, environ);
        }
        posix_spawn_file_actions_destroy(&actions);
        close(output[1]);
        if (status)
        {
            child = -1;
            close(output[0]);
            return status;
        }
        char port[8] = {};
        size_t size = 0;
        int64_t deadline = Deadline(25000);
        for (;;)
        {
            int remaining = 0;
            status = TelemetryDeadlineRemaining(deadline, &remaining);
            if (status) break;
            struct pollfd item = {output[0], POLLIN, 0};
            int ready = poll(&item, 1, remaining);
            if (ready < 0 && errno == EINTR) continue;
            if (ready <= 0)
            {
                status = ready ? errno : ETIMEDOUT;
                break;
            }
            char byte = 0;
            ssize_t count = read(output[0], &byte, 1);
            if (count < 0 && errno == EINTR) continue;
            if (count != 1) { status = EPIPE; break; }
            if (byte == '\n') break;
            if (byte < '0' || byte > '9' || size + 1 >= sizeof(port)) { status = EPROTO; break; }
            port[size++] = byte;
        }
        close(output[0]);
        if (status) return status;
        long number = strtol(port, NULL, 10);
        if (number <= 0 || number > 65535) return EPROTO;
        if (setenv("SSL_CERT_FILE", (directory + "/root.pem").c_str(), 1) ||
            setenv("SSL_CERT_DIR", directory.c_str(), 1)) return errno;
        if (!tlsSocket)
        {
            for (const char* variable : {"https_proxy", "HTTPS_PROXY", "all_proxy", "ALL_PROXY", "no_proxy", "NO_PROXY"})
            {
                if (unsetenv(variable)) return errno;
            }
            const std::string credentials = strcmp(mode, "aria-auth") == 0 ? "user:p%40ss@" : "";
            const std::string proxy = "http://" + credentials + "127.0.0.1:" + port;
            return setenv("https_proxy", proxy.c_str(), 1) ? errno : 0;
        }
        descriptor = socket(AF_INET, SOCK_STREAM, 0);
        if (descriptor < 0) return errno;
        struct sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(number));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (connect(descriptor, reinterpret_cast<struct sockaddr*>(&address), sizeof(address))) return errno;
        int flags = fcntl(descriptor, F_GETFL);
        if (flags < 0 || fcntl(descriptor, F_SETFL, flags | O_NONBLOCK)) return errno;
        return TelemetryTlsCreate(&tls, Deadline(), NULL);
    }
};

void IgnorePipe()
{
    struct sigaction action = {};
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    ASSERT_EQ(0, sigaction(SIGPIPE, &action, NULL));
    action.sa_handler = SIG_DFL;
    ASSERT_EQ(0, sigaction(SIGALRM, &action, NULL));
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGALRM);
    ASSERT_EQ(0, sigprocmask(SIG_UNBLOCK, &signals, NULL));
    alarm(40);
}

void ReadExact(Peer& peer, const std::string& expected)
{
    std::string received;
    const int64_t deadline = Deadline();
    while (received.size() < expected.size())
    {
        char bytes[32] = {};
        size_t count = 0;
        bool eof = false;
        ASSERT_EQ(0, TelemetryTlsRead(peer.tls, bytes, sizeof(bytes), &count, &eof, deadline, NULL));
        ASSERT_FALSE(eof);
        ASSERT_GT(count, 0U);
        received.append(bytes, count);
    }
    EXPECT_EQ(expected, received);
}

void Exchange(const char* identity)
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start(strcmp(identity, "127.0.0.1") == 0 ? "ip" : "exchange"));
    ASSERT_EQ(0, TelemetryTlsHandshake(peer.tls, peer.descriptor, identity, Deadline(), NULL));
    TelemetryTls* original = peer.tls;
    EXPECT_EQ(EALREADY, TelemetryTlsCreate(&peer.tls, Deadline(), NULL));
    EXPECT_EQ(original, peer.tls);
    EXPECT_EQ(EALREADY, TelemetryTlsHandshake(peer.tls, peer.descriptor, identity, Deadline(), NULL));
    for (int i = 0; i < 2; ++i)
    {
        ASSERT_EQ(0, TelemetryTlsWrite(peer.tls, "ping", 4, Deadline(), NULL));
        ReadExact(peer, "pong");
        ASSERT_FALSE(::testing::Test::HasFailure());
    }
    char byte = 0;
    size_t count = 99;
    bool eof = false;
    ASSERT_EQ(0, TelemetryTlsRead(peer.tls, &byte, 1, &count, &eof, Deadline(), NULL));
    EXPECT_TRUE(eof);
    EXPECT_EQ(0U, count);
    EXPECT_EQ(EINVAL, TelemetryTlsWrite(peer.tls, "x", 1, Deadline(), NULL));
    TelemetryTlsDestroy(&peer.tls, NULL);
    EXPECT_EQ(nullptr, peer.tls);
    EXPECT_GE(fcntl(peer.descriptor, F_GETFL), 0);
    TelemetryTlsDestroy(&peer.tls, NULL);
}

void HandshakeFailure(const char* mode, const char* identity, int expected)
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start(mode));
    int status = TelemetryTlsHandshake(peer.tls, peer.descriptor, identity,
        Deadline(strcmp(mode, "silent-handshake") == 0 ? 500 : 5000), NULL);
    if (expected) EXPECT_EQ(expected, status);
    else EXPECT_NE(0, status);
    EXPECT_EQ(EINVAL, TelemetryTlsWrite(peer.tls, "x", 1, Deadline(), NULL));
    EXPECT_GE(fcntl(peer.descriptor, F_GETFL), 0);
}

void ReadFailure(const char* mode)
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start(mode));
    ASSERT_EQ(0, TelemetryTlsHandshake(peer.tls, peer.descriptor, "localhost", Deadline(), NULL));
    if (strcmp(mode, "abrupt") == 0)
    {
        ReadExact(peer, "x");
        ASSERT_FALSE(::testing::Test::HasFailure());
    }
    char byte = 0;
    size_t size = 99;
    bool eof = true;
    int expected = strcmp(mode, "abrupt") == 0 ? EPROTO : ETIMEDOUT;
    EXPECT_EQ(expected, TelemetryTlsRead(peer.tls, &byte, 1, &size, &eof, Deadline(500), NULL));
    EXPECT_EQ(0U, size);
    EXPECT_FALSE(eof);
    EXPECT_EQ(EINVAL, TelemetryTlsWrite(peer.tls, "x", 1, Deadline(), NULL));
}

void LargeWrite()
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start("large-write"));
    int size = 4096;
    ASSERT_EQ(0, setsockopt(peer.descriptor, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)));
    ASSERT_EQ(0, TelemetryTlsHandshake(peer.tls, peer.descriptor, "localhost", Deadline(), NULL));
    std::vector<unsigned char> bytes(512 * 1024);
    for (size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<unsigned char>(i % 251);
    ASSERT_EQ(0, TelemetryTlsWrite(peer.tls, bytes.data(), bytes.size(), Deadline(), NULL));
    ReadExact(peer, "ok");
}

void InvalidArguments()
{
    IgnorePipe();
    TelemetryTls* tls = NULL;
    EXPECT_EQ(EINVAL, TelemetryTlsCreate(NULL, Deadline(), NULL));
    EXPECT_EQ(ETIMEDOUT, TelemetryTlsCreate(&tls, 0, NULL));
    EXPECT_EQ(nullptr, tls);
    EXPECT_EQ(EINVAL, TelemetryTlsHandshake(NULL, -1, NULL, Deadline(), NULL));
    EXPECT_EQ(EINVAL, TelemetryTlsWrite(NULL, NULL, 1, Deadline(), NULL));
    char byte = 0;
    size_t size = 99;
    bool eof = true;
    EXPECT_EQ(EINVAL, TelemetryTlsRead(NULL, &byte, 1, &size, &eof, Deadline(), NULL));
    EXPECT_EQ(0U, size);
    EXPECT_FALSE(eof);
    TelemetryTlsDestroy(NULL, NULL);
    TelemetryTlsDestroy(&tls, NULL);

    Peer peer;
    ASSERT_EQ(0, peer.Start("silent-handshake"));
    int flags = fcntl(peer.descriptor, F_GETFL);
    ASSERT_GE(flags, 0);
    ASSERT_EQ(0, fcntl(peer.descriptor, F_SETFL, flags & ~O_NONBLOCK));
    EXPECT_EQ(EINVAL, TelemetryTlsHandshake(peer.tls, peer.descriptor, "localhost", Deadline(), NULL));
    ASSERT_EQ(0, fcntl(peer.descriptor, F_SETFL, flags));
    EXPECT_EQ(EINVAL, TelemetryTlsHandshake(peer.tls, peer.descriptor, "bad\nname", Deadline(), NULL));
    EXPECT_EQ(ETIMEDOUT, TelemetryTlsHandshake(peer.tls, peer.descriptor, "localhost", 0, NULL));
}

void PreservesSignalDisposition()
{
    struct sigaction action = {}, after = {};
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    ASSERT_EQ(0, sigaction(SIGPIPE, &action, NULL));
    TelemetryTls* tls = NULL;
    EXPECT_EQ(ENOTSUP, TelemetryTlsCreate(&tls, Deadline(), NULL));
    EXPECT_EQ(nullptr, tls);
    ASSERT_EQ(0, sigaction(SIGPIPE, NULL, &after));
    EXPECT_EQ(SIG_DFL, after.sa_handler);
}

void ExpiredWrite()
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start("silent-read"));
    ASSERT_EQ(0, TelemetryTlsHandshake(peer.tls, peer.descriptor, "localhost", Deadline(), NULL));
    EXPECT_EQ(ETIMEDOUT, TelemetryTlsWrite(peer.tls, "x", 1, 0, NULL));
    EXPECT_EQ(EINVAL, TelemetryTlsWrite(peer.tls, "x", 1, Deadline(), NULL));
}

void WriteTimeout()
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start("silent-write"));
    int size = 4096;
    ASSERT_EQ(0, setsockopt(peer.descriptor, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)));
    ASSERT_EQ(0, TelemetryTlsHandshake(peer.tls, peer.descriptor, "localhost", Deadline(), NULL));
    std::vector<unsigned char> bytes(512 * 1024, 'x');
    int64_t start = 0, end = 0;
    ASSERT_EQ(0, TelemetryMonotonicTime(&start));
    EXPECT_EQ(ETIMEDOUT, TelemetryTlsWrite(peer.tls, bytes.data(), bytes.size(), start + 500000000, NULL));
    ASSERT_EQ(0, TelemetryMonotonicTime(&end));
    EXPECT_GE(end - start, 500000000);
    EXPECT_LT(end - start, INT64_C(2000000000));
    EXPECT_EQ(EINVAL, TelemetryTlsWrite(peer.tls, "x", 1, Deadline(), NULL));
}

class Transport
{
public:
    TelemetryTransport* value = NULL;
    ~Transport() { TelemetryTransportDestroy(&value, NULL); }
    int Send(TelemetryHttpResponse* response, int milliseconds = 5000)
    {
        return TelemetryTransportSend(value, "fixture-token", "OSConfig-C/test", 1,
            "encoded-fixture", strlen("encoded-fixture"), Deadline(milliseconds), response, NULL);
    }
};

void TransportExchange(const char* mode)
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start(mode, false));
    Transport transport;
    ASSERT_EQ(0, TelemetryTransportCreate(&transport.value, NULL));
    // The invocation keeps its selected route even if the environment changes.
    ASSERT_EQ(0, setenv("https_proxy", "unsupported://must-not-use.invalid", 1));
    TelemetryHttpResponse response;
    if (strcmp(mode, "aria-recover") == 0)
    {
        EXPECT_NE(0, transport.Send(&response));
        EXPECT_EQ(TelemetryUnconfirmed, response.acceptance);
    }
    int count = strcmp(mode, "aria-recover") == 0 ? 1 : 2;
    for (int i = 0; i < count; ++i)
    {
        ASSERT_EQ(0, transport.Send(&response));
        EXPECT_TRUE(response.complete);
        EXPECT_EQ(TelemetryAccepted, response.acceptance);
        EXPECT_FALSE(TelemetryTransportSuppressed(transport.value));
    }
    TelemetryTransportDestroy(&transport.value, NULL);
    EXPECT_EQ(0, peer.Wait());
}

void TransportFailure(const char* mode, int expected, bool slow = false)
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start(mode, false));
    Transport transport;
    ASSERT_EQ(0, TelemetryTransportCreate(&transport.value, NULL));
    TelemetryHttpResponse response;
    int64_t begin = 0, end = 0;
    ASSERT_EQ(0, TelemetryMonotonicTime(&begin));
    EXPECT_EQ(expected, transport.Send(&response, slow ? 300 : 5000));
    ASSERT_EQ(0, TelemetryMonotonicTime(&end));
    EXPECT_EQ(TelemetryUnconfirmed, response.acceptance);
    TelemetryTransportDestroy(&transport.value, NULL);
    if (slow)
    {
        EXPECT_GE(end - begin, INT64_C(300000000));
        EXPECT_LT(end - begin, INT64_C(2000000000));
    }
    else EXPECT_EQ(0, peer.Wait());
}

void TransportResponse(const char* mode, TelemetryAcceptance expected, bool suppressed)
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start(mode, false));
    Transport transport;
    ASSERT_EQ(0, TelemetryTransportCreate(&transport.value, NULL));
    TelemetryHttpResponse response;
    ASSERT_EQ(0, transport.Send(&response));
    EXPECT_EQ(expected, response.acceptance);
    EXPECT_EQ(suppressed, TelemetryTransportSuppressed(transport.value));
    if (suppressed)
    {
        EXPECT_EQ(ECANCELED, transport.Send(&response));
        EXPECT_EQ(TelemetryUnconfirmed, response.acceptance);
    }
    TelemetryTransportDestroy(&transport.value, NULL);
    EXPECT_EQ(0, peer.Wait());
}

void TransportArguments()
{
    IgnorePipe();
    for (const char* name : {"no_proxy", "NO_PROXY"}) ASSERT_EQ(0, unsetenv(name));
    Transport transport;
    EXPECT_EQ(EINVAL, TelemetryTransportCreate(NULL, NULL));
    ASSERT_EQ(0, setenv("https_proxy", "https://proxy.invalid", 1));
    EXPECT_EQ(ENOTSUP, TelemetryTransportCreate(&transport.value, NULL));
    EXPECT_EQ(nullptr, transport.value);
    ASSERT_EQ(0, setenv("https_proxy", "http://127.0.0.1:9", 1));
    ASSERT_EQ(0, TelemetryTransportCreate(&transport.value, NULL));
    TelemetryTransport* original = transport.value;
    EXPECT_EQ(EALREADY, TelemetryTransportCreate(&transport.value, NULL));
    EXPECT_EQ(original, transport.value);
    TelemetryHttpResponse response;
    EXPECT_EQ(EINVAL, TelemetryTransportSend(NULL, "fixture-token", "test", 1, "x", 1, Deadline(), &response, NULL));
    EXPECT_EQ(EINVAL, TelemetryTransportSend(transport.value, NULL, "test", 1, "x", 1, Deadline(), &response, NULL));
    EXPECT_EQ(EINVAL, TelemetryTransportSend(transport.value, "fixture-token", "test", 1, NULL, 1, Deadline(), &response, NULL));
    EXPECT_EQ(EINVAL, TelemetryTransportSend(transport.value, "fixture-token", "test", 1, "x", 1, Deadline(), NULL, NULL));
    EXPECT_EQ(ETIMEDOUT, TelemetryTransportSend(transport.value, "fixture-token", "test", 1, "x", 1, 0, &response, NULL));
}

class LiveTestProcess
{
public:
    pid_t child = -1;
    int output = -1;
    ~LiveTestProcess()
    {
        if (output >= 0) close(output);
        if (child > 0)
        {
            pid_t waited;
            do { waited = waitpid(child, NULL, WNOHANG); } while (waited < 0 && errno == EINTR);
            if (waited == 0)
            {
                kill(child, SIGKILL);
                do { waited = waitpid(child, NULL, 0); } while (waited < 0 && errno == EINTR);
            }
        }
    }
};

void LiveEvents(const char* mode, const char* expected, int exitCode)
{
    IgnorePipe();
    const bool fullRun = strcmp(mode, "aria-live") == 0;
    if (fullRun) alarm(1200);
    Peer peer;
    ASSERT_EQ(0, peer.Start(mode, false));
    ASSERT_EQ(0, setenv("OsConfigTelemetryApiKey", "fixture-token", 1));
    LiveTestProcess process;
    int output[2];
    ASSERT_EQ(0, pipe(output));
    process.output = output[0];
    posix_spawn_file_actions_t actions;
    int status = posix_spawn_file_actions_init(&actions);
    if (status) { close(output[1]); FAIL() << "spawn action initialization failed"; }
    char* arguments[] = {const_cast<char*>(TELEMETRY_ARIA_TEST_PATH),
        const_cast<char*>("--send-status-trace-10000"), NULL};
    if ((0 == (status = posix_spawn_file_actions_adddup2(&actions, output[1], STDOUT_FILENO))) &&
        (0 == (status = posix_spawn_file_actions_addclose(&actions, output[0]))) &&
        (0 == (status = posix_spawn_file_actions_addclose(&actions, output[1]))))
    {
        status = posix_spawn(&process.child, TELEMETRY_ARIA_TEST_PATH, &actions, NULL, arguments, environ);
    }
    posix_spawn_file_actions_destroy(&actions);
    close(output[1]);
    if (status) process.child = -1;
    ASSERT_EQ(0, status);
    std::string text;
    int64_t deadline = Deadline(fullRun ? 1200000 : 15000);
    for (;;)
    {
        int remaining = 0;
        ASSERT_EQ(0, TelemetryDeadlineRemaining(deadline, &remaining));
        struct pollfd item = {process.output, POLLIN, 0};
        int ready = poll(&item, 1, remaining);
        if (ready < 0 && errno == EINTR) continue;
        ASSERT_GT(ready, 0);
        char bytes[1024];
        ssize_t size = read(process.output, bytes, sizeof(bytes));
        if (size < 0 && errno == EINTR) continue;
        ASSERT_GE(size, 0);
        if (!size) break;
        text.append(bytes, static_cast<size_t>(size));
        ASSERT_LE(text.size(), 2048U);
    }
    int result = 0;
    for (;;)
    {
        pid_t done = waitpid(process.child, &result, WNOHANG);
        if (done == process.child) { process.child = -1; break; }
        if (done < 0 && errno == EINTR) continue;
        ASSERT_GE(done, 0);
        int remaining = 0;
        ASSERT_EQ(0, TelemetryDeadlineRemaining(deadline, &remaining));
        poll(NULL, 0, 5);
    }
    ASSERT_TRUE(WIFEXITED(result)) << text;
    EXPECT_EQ(exitCode, WEXITSTATUS(result)) << text;
    EXPECT_NE(std::string::npos, text.find(expected)) << text;
    EXPECT_NE(std::string::npos, text.find("ResultCode=731001"));
    EXPECT_NE(std::string::npos, text.find("*** Distilled 1DS SDK test No. 2 ***"));
    EXPECT_EQ(std::string::npos, text.find("fixture-token"));
    EXPECT_EQ(std::string::npos, text.find("event="));
    EXPECT_EQ(0, peer.Wait());
}

class WorkerOwner
{
public:
    TelemetryWorker* value = nullptr;
    ~WorkerOwner() { TelemetryWorkerDestroy(&value, nullptr); }
};

void WorkerEvents(const char* mode, int expected, bool suppressed, bool second)
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start(mode, false));
    ASSERT_EQ(0, setenv("OsConfigTelemetryApiKey", "fixture-token", 1));
    WorkerOwner worker;
    const bool timeout = strcmp(mode, "aria-worker-timeout") == 0;
    ASSERT_EQ(0, TelemetryWorkerCreate(TELEMETRY_WORKER_PATH, 600000,
        timeout ? 500 : 10000, timeout ? 500 : 5000, &worker.value, nullptr));
    const char* names[] = {"DistroName", "CorrelationId", "Version", "Timestamp", "CrashInfo"};
    TelemetryProperty properties[5] = {};
    for (size_t i = 0; i < 5; ++i)
    {
        properties[i].name = names[i];
        properties[i].type = TelemetryPropertyString;
        properties[i].value.stringValue = "worker-fixture";
    }
    int64_t before = 0, after = 0;
    ASSERT_EQ(0, TelemetryMonotonicTime(&before));
    int status = TelemetryWorkerSend(worker.value, "CrashDetected", properties, 5, nullptr);
    ASSERT_EQ(0, TelemetryMonotonicTime(&after));
    if (timeout)
    {
        // The independent worker timer can close IPC just before parent timeout.
        EXPECT_TRUE(status == ETIMEDOUT || status == EPIPE);
        EXPECT_GE(after - before, INT64_C(400000000));
        EXPECT_LT(after - before, INT64_C(1000000000));
    }
    else EXPECT_EQ(expected, status);
    if (second || suppressed || timeout)
    {
        properties[4].value.stringValue = "worker-fixture-next";
        EXPECT_EQ((suppressed || timeout) ? ECANCELED : 0,
            TelemetryWorkerSend(worker.value, "CrashDetected", properties, 5, nullptr));
    }
    TelemetryWorkerDestroy(&worker.value, nullptr);
    EXPECT_EQ(nullptr, worker.value);
    EXPECT_EQ(0, peer.Wait());
}

void MalformedKill()
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start("aria-bad-kill", false));
    Transport transport;
    ASSERT_EQ(0, TelemetryTransportCreate(&transport.value, nullptr));
    TelemetryHttpResponse response;
    EXPECT_EQ(EPROTO, transport.Send(&response));
    EXPECT_TRUE(TelemetryTransportSuppressed(transport.value));
    EXPECT_EQ(ECANCELED, transport.Send(&response));
    TelemetryTransportDestroy(&transport.value, nullptr);
    EXPECT_EQ(0, peer.Wait());
}

void WorkerRejectsBeforeNetwork()
{
    IgnorePipe();
    ASSERT_EQ(0, setenv("https_proxy", "http://127.0.0.1:9", 1));
    ASSERT_EQ(0, setenv("OsConfigTelemetryApiKey", "", 1));
    WorkerOwner worker;
    ASSERT_EQ(0, TelemetryWorkerCreate(TELEMETRY_WORKER_PATH, 10000, 5000, 2000, &worker.value, nullptr));
    const char* names[] = {"DistroName", "CorrelationId", "Version", "Timestamp", "CrashInfo"};
    TelemetryProperty properties[5] = {};
    for (size_t i = 0; i < 5; ++i)
    {
        properties[i].name = names[i];
        properties[i].type = TelemetryPropertyString;
        properties[i].value.stringValue = "worker-fixture";
    }
    EXPECT_EQ(EINVAL, TelemetryWorkerSend(worker.value, "CrashDetected", properties, 5, nullptr));
    TelemetryWorkerDestroy(&worker.value, nullptr);
    ASSERT_EQ(0, setenv("OsConfigTelemetryApiKey", "fixture-token", 1));
    ASSERT_EQ(0, TelemetryWorkerCreate(TELEMETRY_WORKER_PATH, 10000, 5000, 2000, &worker.value, nullptr));
    // Valid token, invalid named schema: must fail before contacting the route.
    EXPECT_EQ(EINVAL, TelemetryWorkerSend(worker.value, "CrashDetected", properties, 1, nullptr));
    EXPECT_EQ(EINVAL, TelemetryWorkerSend(worker.value, "Unknown", properties, 5, nullptr));
}

#ifdef TELEMETRY_PRODUCER_PROBE_PATH
void ProducerEvents()
{
    IgnorePipe();
    Peer peer;
    ASSERT_EQ(0, peer.Start("aria-producer", false));
    ASSERT_EQ(0, setenv("OsConfigTelemetryApiKey", "fixture-token", 1));
    LiveTestProcess process;
    char* arguments[] = {const_cast<char*>(TELEMETRY_PRODUCER_PROBE_PATH), nullptr};
    ASSERT_EQ(0, posix_spawn(&process.child, TELEMETRY_PRODUCER_PROBE_PATH, nullptr, nullptr, arguments, environ));
    int status = 0;
    pid_t waited;
    do { waited = waitpid(process.child, &status, 0); } while (waited < 0 && errno == EINTR);
    ASSERT_EQ(process.child, waited);
    process.child = -1;
    ASSERT_TRUE(WIFEXITED(status));
    EXPECT_EQ(0, WEXITSTATUS(status));
    // The peer requires both the real CrashDetected and BaselineRun; the
    // producer must fit startup and both synchronous sends inside 500 ms.
    EXPECT_EQ(0, peer.Wait());
}
#endif
}

// Each case has a fresh owned process: provider residency and environment/signal
// changes never enter the policy host or another test's OpenSSL instance.
#define TLS_CASE(statement) EXPECT_EXIT({ statement; _exit(::testing::Test::HasFailure() ? 1 : 0); }, \
    ::testing::ExitedWithCode(0), "")

TEST(TelemetryTlsDeathTest, VerifiesDnsAndReusesConnection) { TLS_CASE(Exchange("localhost")); }
TEST(TelemetryTlsDeathTest, VerifiesIpWithoutSendingSni) { TLS_CASE(Exchange("127.0.0.1")); }
TEST(TelemetryTlsDeathTest, RejectsWrongDnsIdentity) { TLS_CASE(HandshakeFailure("wrong-name", "localhost", EACCES)); }
TEST(TelemetryTlsDeathTest, RejectsWrongIpIdentity) { TLS_CASE(HandshakeFailure("wrong-ip", "127.0.0.2", EACCES)); }
TEST(TelemetryTlsDeathTest, RejectsUntrustedCertificate) { TLS_CASE(HandshakeFailure("untrusted", "localhost", EACCES)); }
TEST(TelemetryTlsDeathTest, RejectsTls10) { TLS_CASE(HandshakeFailure("tls10", "localhost", 0)); }
TEST(TelemetryTlsDeathTest, TimesOutSilentHandshake) { TLS_CASE(HandshakeFailure("silent-handshake", "localhost", ETIMEDOUT)); }
TEST(TelemetryTlsDeathTest, HandlesSocketResetWithoutSigpipe) { TLS_CASE(HandshakeFailure("reset", "localhost", 0)); }
TEST(TelemetryTlsDeathTest, RejectsUnauthenticatedEof) { TLS_CASE(ReadFailure("abrupt")); }
TEST(TelemetryTlsDeathTest, TimesOutSilentRead) { TLS_CASE(ReadFailure("silent-read")); }
TEST(TelemetryTlsDeathTest, ContinuesBackpressuredWriteWithoutReplay) { TLS_CASE(LargeWrite()); }
TEST(TelemetryTlsDeathTest, RejectsInvalidArgumentsAndBlockingSockets) { TLS_CASE(InvalidArguments()); }
TEST(TelemetryTlsDeathTest, DoesNotChangeHostSignalDisposition) { TLS_CASE(PreservesSignalDisposition()); }
TEST(TelemetryTlsDeathTest, DoesNotWriteAfterDeadline) { TLS_CASE(ExpiredWrite()); }
TEST(TelemetryTlsDeathTest, TimesOutBackpressuredWriteWithoutResettingDeadline) { TLS_CASE(WriteTimeout()); }

TEST(TelemetryTransportDeathTest, TunnelsAndReusesVerifiedConnection) { TLS_CASE(TransportExchange("aria-reuse")); }
TEST(TelemetryTransportDeathTest, KeepsProxyCredentialsOutOfCollectorRequest) { TLS_CASE(TransportExchange("aria-auth")); }
TEST(TelemetryTransportDeathTest, ReconnectsAfterConnectionClose) { TLS_CASE(TransportExchange("aria-close")); }
TEST(TelemetryTransportDeathTest, DropsFailedEventAndReconnectsOnlyForNextCall) { TLS_CASE(TransportExchange("aria-recover")); }
TEST(TelemetryTransportDeathTest, RejectsProxyAuthenticationFailure) { TLS_CASE(TransportFailure("aria-407", EACCES)); }
TEST(TelemetryTransportDeathTest, DoesNotFollowProxyRedirect) { TLS_CASE(TransportFailure("aria-redirect", ECONNREFUSED)); }
TEST(TelemetryTransportDeathTest, RejectsMalformedConnectHeaders) { TLS_CASE(TransportFailure("aria-bad-connect", EPROTO)); }
TEST(TelemetryTransportDeathTest, BoundsConnectLine) { TLS_CASE(TransportFailure("aria-long-connect", EMSGSIZE)); }
TEST(TelemetryTransportDeathTest, RejectsTruncatedConnectResponse) { TLS_CASE(TransportFailure("aria-truncated-connect", EPROTO)); }
TEST(TelemetryTransportDeathTest, RejectsConnectUpgrade) { TLS_CASE(TransportFailure("aria-upgrade", ENOTSUP)); }
TEST(TelemetryTransportDeathTest, BoundsConnectInformationalResponses) { TLS_CASE(TransportFailure("aria-many-interim", EMSGSIZE)); }
TEST(TelemetryTransportDeathTest, VerifiesCollectorIdentityThroughProxy) { TLS_CASE(TransportFailure("aria-wrong-name", EACCES)); }
TEST(TelemetryTransportDeathTest, TimesOutSilentProxy) { TLS_CASE(TransportFailure("aria-proxy-timeout", ETIMEDOUT, true)); }
TEST(TelemetryTransportDeathTest, TimesOutCollectorResponse) { TLS_CASE(TransportFailure("aria-read-timeout", ETIMEDOUT, true)); }
TEST(TelemetryTransportDeathTest, RejectsTruncatedTlsWithoutReplay) { TLS_CASE(TransportFailure("aria-drop", EPROTO)); }
TEST(TelemetryTransportDeathTest, ReportsCollectorRejection) { TLS_CASE(TransportResponse("aria-reject", TelemetryRejected, false)); }
TEST(TelemetryTransportDeathTest, EmptySuccessIsUnconfirmed) { TLS_CASE(TransportResponse("aria-empty", TelemetryUnconfirmed, false)); }
TEST(TelemetryTransportDeathTest, SuppressesFurtherSendsOnThrottling) { TLS_CASE(TransportResponse("aria-throttle", TelemetryRejected, true)); }
TEST(TelemetryTransportDeathTest, SuppressesFurtherSendsOnKillDirective) { TLS_CASE(TransportResponse("aria-kill", TelemetryAccepted, true)); }
TEST(TelemetryTransportDeathTest, PreservesKillDirectiveWhenBodyIsMalformed) { TLS_CASE(MalformedKill()); }
TEST(TelemetryTransportDeathTest, RejectsInvalidArgumentsAndUnsupportedRoutes) { TLS_CASE(TransportArguments()); }
TEST(TelemetryTransportDeathTest, LiveTestExecutableSendsTenThousandEventsToLoopbackOnly)
{
    TLS_CASE(LiveEvents("aria-live",
        "requested=10000 attempted=10000 accepted=10000 rejected=0 unconfirmed=0 unsent=0 status=0", 0));
}
TEST(TelemetryTransportDeathTest, LiveTestStopsOnThrottling)
{
    TLS_CASE(LiveEvents("aria-live-throttle",
        "requested=10000 attempted=1 accepted=0 rejected=1 unconfirmed=0 unsent=9999", 1));
}
TEST(TelemetryTransportDeathTest, LiveTestStopsOnExplicitRejection)
{
    TLS_CASE(LiveEvents("aria-live-reject",
        "requested=10000 attempted=1 accepted=0 rejected=1 unconfirmed=0 unsent=9999", 1));
}
TEST(TelemetryTransportDeathTest, LiveTestDoesNotTreatEmpty200AsAcceptance)
{
    TLS_CASE(LiveEvents("aria-live-empty",
        "requested=10000 attempted=1 accepted=0 rejected=0 unconfirmed=1 unsent=9999", 1));
}
TEST(TelemetryTransportDeathTest, LiveTestDoesNotReplayAmbiguousDelivery)
{
    TLS_CASE(LiveEvents("aria-live-drop",
        "requested=10000 attempted=1 accepted=0 rejected=0 unconfirmed=1 unsent=9999", 1));
}

TEST(TelemetryWorkerSendDeathTest, SendsAndReusesActualWorker) { TLS_CASE(WorkerEvents("aria-worker", 0, false, true)); }
TEST(TelemetryWorkerSendDeathTest, ReportsRejection) { TLS_CASE(WorkerEvents("aria-worker-reject", ECANCELED, false, false)); }
TEST(TelemetryWorkerSendDeathTest, ReportsUnconfirmedDelivery) { TLS_CASE(WorkerEvents("aria-worker-empty", EPROTO, false, false)); }
TEST(TelemetryWorkerSendDeathTest, PreservesThrottleInParent) { TLS_CASE(WorkerEvents("aria-worker-throttle", ECANCELED, true, false)); }
TEST(TelemetryWorkerSendDeathTest, PreservesKillInParent) { TLS_CASE(WorkerEvents("aria-worker-kill", 0, true, false)); }
TEST(TelemetryWorkerSendDeathTest, PreservesKillOnMalformedResponse) { TLS_CASE(WorkerEvents("aria-worker-bad-kill", EPROTO, true, false)); }
TEST(TelemetryWorkerSendDeathTest, ReconnectsOnlyForNextEvent) { TLS_CASE(WorkerEvents("aria-worker-recover", EPROTO, false, true)); }
TEST(TelemetryWorkerSendDeathTest, EnforcesFiveHundredMillisecondBudget) { TLS_CASE(WorkerEvents("aria-worker-timeout", ETIMEDOUT, true, false)); }
TEST(TelemetryWorkerSendDeathTest, RejectsMissingKeyAndInvalidSchemaBeforeNetwork) { TLS_CASE(WorkerRejectsBeforeNetwork()); }
#ifdef TELEMETRY_PRODUCER_PROBE_PATH
TEST(TelemetryWorkerSendDeathTest, RealProducersAndPackagedWorkerShareFiveHundredMilliseconds) { TLS_CASE(ProducerEvents()); }
#endif
