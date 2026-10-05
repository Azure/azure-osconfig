// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryWorkerProtocol.h"
#include "TelemetryResolverProtocol.h"
#include "TelemetryDeadline.h"
#include "TelemetryEvent.h"
#include "TelemetryTransport.h"
#include "Keys.h"
#include <CommonUtils.h>
#include <Logging.h>

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define LOG_FILE "/var/log/osconfig_telemetry.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_telemetry.bak"

_Static_assert(sizeof(TelemetryWorkerFrame) == 32, "Unexpected worker frame layout");

static int Transfer(void* buffer, size_t size, bool writing)
{
    unsigned char* bytes = buffer;
    size_t offset = 0;
    ssize_t count = 0;

    while (offset < size)
    {
        count = writing ?
            send(STDIN_FILENO, bytes + offset, size - offset, MSG_NOSIGNAL) :
            recv(STDIN_FILENO, bytes + offset, size - offset, 0);
        if (count > 0)
        {
            offset += (size_t)count;
        }
        else if (0 == count)
        {
            return (0 == offset) ? ENODATA : EPROTO;
        }
        else if (EINTR != errno)
        {
            return errno;
        }
    }
    return 0;
}

static int SendReply(uint32_t operation, uint32_t sequence, int status, void* body, uint32_t size,
    OsConfigLogHandle log)
{
    TelemetryWorkerFrame reply = {0};
    int error = 0;

    reply.magic = TELEMETRY_WORKER_MAGIC;
    reply.version = TELEMETRY_WORKER_VERSION;
    reply.operation = operation;
    reply.sequence = sequence;
    reply.status = status;
    reply.size = (0 == status) ? size : 0;
    error = Transfer(&reply, sizeof(reply), true);
    if (0 == error)
    {
        error = Transfer(body, reply.size, true);
    }
    if ((0 != error) && (NULL != log))
    {
        OsConfigLogError(log, "OSConfigTelemetry: Cannot send reply (operation=%" PRIu32
            ", sequence=%" PRIu32 ", status=%d)", operation, sequence, error);
    }
    return error;
}

static int CloseInheritedDescriptors(int logDescriptor)
{
    // This Linux worker must not keep unrelated gc_worker descriptors alive.
    DIR* directory = opendir("/proc/self/fd");
    int ownDescriptor = 0;
    struct dirent* entry = NULL;
    char* end = NULL;
    long descriptor = 0;

    if (NULL == directory)
    {
        return errno;
    }
    ownDescriptor = dirfd(directory);
    if (ownDescriptor < 0)
    {
        int status = errno;
        closedir(directory);
        return status;
    }
    int status = 0;
    for (;;)
    {
        errno = 0;
        entry = readdir(directory);
        if (NULL == entry)
        {
            status = errno;
            break;
        }
        if ((entry->d_name[0] < '0') || (entry->d_name[0] > '9'))
        {
            continue;
        }
        end = NULL;
        descriptor = strtol(entry->d_name, &end, 10);
        if ((0 != errno) || ('\0' != *end) || (descriptor > INT_MAX))
        {
            status = EPROTO;
            break;
        }
        if ((descriptor > STDERR_FILENO) && (descriptor != ownDescriptor) && (descriptor != logDescriptor) &&
            (0 != close((int)descriptor)))
        {
            status = errno;
            break;
        }
    }
    if ((0 != closedir(directory)) && (0 == status))
    {
        status = errno;
    }
    return status;
}

static int ParseDeadline(const char* text, int64_t* deadline)
{
    char* end = NULL;
    long long value = 0;

    if ((text[0] < '0') || (text[0] > '9'))
    {
        return EINVAL;
    }
    errno = 0;
    value = strtoll(text, &end, 10);
    if ((0 != errno) || ('\0' != *end) || (value <= 0))
    {
        return EINVAL;
    }
    *deadline = (int64_t)value;
    return ((long long)*deadline == value) ? 0 : EOVERFLOW;
}

static int Initialize(int argc, char** argv, timer_t* timer, int64_t* lifetime)
{
    int64_t startup = 0;
    int status = 0;
    struct sigaction action = {0};
    sigset_t signals = {0};
    struct sigevent notification = {0};

    if ((4 != argc) || (0 != strcmp(argv[1], TELEMETRY_WORKER_ARGUMENT)))
    {
        return EINVAL;
    }
    if ((0 != (status = ParseDeadline(argv[2], lifetime))) ||
        (0 != (status = ParseDeadline(argv[3], &startup))))
    {
        return status;
    }
    if (startup > *lifetime)
    {
        return EINVAL;
    }
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    sigemptyset(&signals);
    sigaddset(&signals, SIGALRM);
    if ((0 != sigaction(SIGALRM, &action, NULL)) ||
        (0 != sigprocmask(SIG_UNBLOCK, &signals, NULL)))
    {
        return errno;
    }
    // After exec these dispositions belong only to this worker, never the host.
    action.sa_handler = SIG_IGN;
    if (0 != sigaction(SIGPIPE, &action, NULL))
    {
        return errno;
    }
    notification.sigev_notify = SIGEV_SIGNAL;
    notification.sigev_signo = SIGALRM;
    if (0 != timer_create(CLOCK_MONOTONIC, &notification, timer))
    {
        return errno;
    }
    return TelemetryArmDeadline(*timer, startup);
}

static int SendEvent(TelemetryTransport** transport, char* epoch,
    const unsigned char* payload, size_t size, uint32_t sequence, int64_t deadline,
    OsConfigLogHandle log)
{
    // The package's key is compiled only into the owned executable. A runtime
    // override also permits offline tests with a deliberately fake token.
    const char* token = getenv("OsConfigTelemetryApiKey");
    char headers[TELEMETRY_HTTP_HEADER_LIMIT + 1] = {0};
    size_t headerSize = 0;
    int status = 0;
    size_t tenantLength = 0;
    char iKey[TELEMETRY_HTTP_TOKEN_LIMIT + 3] = "o:";
    unsigned char bytes[TELEMETRY_MAX_EVENT_SIZE] = {0};
    size_t encodedSize = 0;
    int64_t uploadTime = 0;
    TelemetryHttpResponse response = {0};

    if (!token)
    {
        token = API_KEY;
    }
    status = TelemetryHttpBuildRequest(token, TELEMETRY_CLIENT_VERSION, 0, 1,
        headers, sizeof(headers), &headerSize, log);
    if (!status)
    {
        tenantLength = strcspn(token, "-");
        if (!tenantLength || token[tenantLength] != '-')
        {
            OsConfigLogError(log, "OSConfigTelemetry: Ingestion key lacks tenant prefix");
            status = EINVAL;
        }
        else
        {
            memcpy(iKey + 2, token, tenantLength);
            if (!epoch[0])
            {
                status = TelemetryCreateEpoch(epoch, log);
            }
        }
    }
    if (!status)
    {
        status = TelemetryEncodePayload(payload, size, iKey, epoch, sequence, bytes,
            &encodedSize, &uploadTime, log);
    }
    if (!status && !*transport)
    {
        status = TelemetryTransportCreate(transport, log);
    }
    if (!status)
    {
        status = TelemetryTransportSend(*transport, token, TELEMETRY_CLIENT_VERSION,
            uploadTime, bytes, encodedSize, deadline, &response, log);
    }
    if (!status && response.acceptance != TelemetryAccepted)
    {
        status = response.acceptance == TelemetryRejected ? ECANCELED : EPROTO;
        OsConfigLogInfo(log, "OSConfigTelemetry: Event not accepted (http=%u, acceptance=%d)",
            response.status, (int)response.acceptance);
    }
    return status;
}

int main(int argc, char** argv)
{
    timer_t timer = 0;
    OsConfigLogHandle log = NULL;
    int64_t lifetime = 0;
    uint32_t sequence = 0;
    TelemetryTransport* transport = NULL;
    char epoch[TELEMETRY_EPOCH_SIZE] = {0};
    int status = 0;
    char* configuration = NULL;
    TelemetryWorkerFrame request = {0};
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {0};
    TelemetryResolverReply reply = {0};
    TelemetryWorkerSendReply sent = {0};

    SetConsoleLoggingEnabled(false);
    status = Initialize(argc, argv, &timer, &lifetime);
    if (0 == status)
    {
        log = OpenLog(LOG_FILE, ROLLED_LOG_FILE);
        if (NULL == log)
        {
            status = ENOMEM;
        }
        else if (NULL == GetLogFile(log))
        {
            status = EIO;
            CloseLog(&log);
        }
        else
        {
            OsConfigLogInfo(log, "OSConfigTelemetry: Starting (PID=%ld, PPID=%ld)",
                (long)getpid(), (long)getppid());
            status = CloseInheritedDescriptors(fileno(GetLogFile(log)));
            if (!status && FileExists("/etc/osconfig/osconfig.json"))
            {
                configuration = LoadStringFromFile("/etc/osconfig/osconfig.json", false, log);
                if (!configuration)
                {
                    status = EIO;
                }
                else
                {
                    SetLoggingLevel(GetLoggingLevelFromJsonConfig(configuration, log));
                    SetMaxLogSize(GetMaxLogSizeFromJsonConfig(configuration, log));
                    SetMaxLogSizeDebugMultiplier(GetMaxLogSizeDebugMultiplierFromJsonConfig(configuration, log));
                    free(configuration);
                }
            }
        }
    }
    if (0 != status)
    {
        if (NULL != log)
        {
            OsConfigLogError(log, "OSConfigTelemetry: Startup failed (status=%d)", status);
        }
        SendReply(TELEMETRY_WORKER_READY, 0, status, NULL, 0, log);
        goto cleanup;
    }
    if (0 != (status = SendReply(TELEMETRY_WORKER_READY, 0, 0, NULL, 0, log)))
    {
        goto cleanup;
    }
    OsConfigLogInfo(log, "OSConfigTelemetry: Ready");

    for (;;)
    {
        request = (TelemetryWorkerFrame){0};
        reply = (TelemetryResolverReply){0};
        if (0 != (status = TelemetryArmDeadline(timer, lifetime)))
        {
            OsConfigLogError(log, "OSConfigTelemetry: Cannot arm lifetime timer (status=%d)", status);
            break;
        }
        status = Transfer(&request, sizeof(request), false);
        if (0 != status)
        {
            if (ENODATA == status)
            {
                OsConfigLogInfo(log, "OSConfigTelemetry: Parent disconnected");
                status = 0;
            }
            else
            {
                OsConfigLogError(log, "OSConfigTelemetry: Cannot read request (status=%d)", status);
            }
            break;
        }
        if ((request.magic != TELEMETRY_WORKER_MAGIC) || (request.version != TELEMETRY_WORKER_VERSION) ||
            ((request.operation != TELEMETRY_WORKER_RESOLVE) &&
                (request.operation != TELEMETRY_WORKER_SEND)) || (request.status != 0) ||
            (UINT32_MAX == sequence) || (request.sequence != sequence + 1) ||
            (request.size < 2) || (request.size > sizeof(payload)) ||
            ((request.operation == TELEMETRY_WORKER_RESOLVE) &&
                (request.size > TELEMETRY_RESOLVER_HOST_LIMIT + 1)) ||
            (request.deadline <= 0) || (request.deadline > lifetime))
        {
            status = EPROTO;
            OsConfigLogError(log, "OSConfigTelemetry: Invalid request frame (operation=%" PRIu32
                ", sequence=%" PRIu32 ")", request.operation, request.sequence);
            SendReply(request.operation, request.sequence, status, NULL, 0, log);
            break;
        }
        sequence = request.sequence;
        status = TelemetryArmDeadline(timer, request.deadline);
        if (0 == status)
        {
            OsConfigLogDebug(log, "OSConfigTelemetry: Receiving request (sequence=%" PRIu32 ")", sequence);
            status = Transfer(payload, request.size, false);
        }
        if ((0 == status) && (request.operation == TELEMETRY_WORKER_RESOLVE) &&
            (strnlen((const char*)payload, request.size) != request.size - 1))
        {
            status = EINVAL;
        }
        if (0 != status)
        {
            OsConfigLogError(log, "OSConfigTelemetry: Request deadline/body failed (sequence=%" PRIu32
                ", status=%d)", sequence, status);
            SendReply(request.operation, sequence, status, NULL, 0, log);
            break;
        }

        if (request.operation == TELEMETRY_WORKER_SEND)
        {
            sent = (TelemetryWorkerSendReply){0};
            sent.status = SendEvent(&transport, epoch, payload, request.size, sequence, request.deadline, log);
            sent.suppressed = TelemetryTransportSuppressed(transport) ? 1 : 0;
            if (sent.status)
            {
                OsConfigLogInfo(log, "OSConfigTelemetry: SEND failed (sequence=%" PRIu32 ", status=%d)",
                    sequence, sent.status);
            }
            if (0 != (status = SendReply(request.operation, sequence, 0, &sent, sizeof(sent), log)))
            {
                break;
            }
            continue;
        }
        OsConfigLogInfo(log, "OSConfigTelemetry: Resolving request (sequence=%" PRIu32 ")", sequence);
        TelemetryLookupHost((const char*)payload, &reply);
        if (0 != reply.error)
        {
            OsConfigLogError(log, "OSConfigTelemetry: Lookup failed (sequence=%" PRIu32
                ", status=%" PRId32 ", lookup=%" PRId32 ")", sequence, reply.error, reply.lookupError);
        }
        else
        {
            OsConfigLogInfo(log, "OSConfigTelemetry: Lookup complete (sequence=%" PRIu32
                ", addresses=%" PRIu32 ")", sequence, reply.count);
        }
        if (0 != (status = SendReply(request.operation, sequence, 0, &reply, sizeof(reply), log)))
        {
            break;
        }
        // Keep the operation timer armed through the reply; the next loop
        // restores the lifetime timer while waiting for the caller's next event.
    }

cleanup:
    if (NULL != transport)
    {
        TelemetryTransportDestroy(&transport, log);
    }
    if (NULL != log)
    {
        OsConfigLogInfo(log, "OSConfigTelemetry: Exiting (status=%d)", status);
        CloseLog(&log);
    }
    _exit((0 == status) ? EXIT_SUCCESS : EXIT_FAILURE);
}
