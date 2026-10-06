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
#include <limits.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define LOG_FILE "/var/log/osconfig_telemetry.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_telemetry.bak"

static int Transfer(void* buffer, size_t size, bool writing, OsConfigLogHandle log)
{
    unsigned char* bytes = NULL;
    size_t offset = 0;
    ssize_t count = 0;
    int error = 0;

    if ((NULL == buffer) && (0 != size))
    {
        OsConfigLogError(log, "Transfer called with invalid arguments");
        return EINVAL;
    }

    bytes = buffer;

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
            error = (0 == offset) ? ENODATA : EPROTO;
            break;
        }
        else if (EINTR != errno)
        {
            error = errno;
            break;
        }
    }

    if (ENODATA == error)
    {
        OsConfigLogInfo(log, "Transfer: connection closed while %s (transferred: %zu, requested: %zu)", writing ? "writing" : "reading", offset, size);
    }
    else if (0 != error)
    {
        OsConfigLogError(log, "Transfer: %s failed with %d (%s) (transferred: %zu, requested: %zu)", writing ? "write" : "read", error, strerror(error), offset, size);
    }

    return error;
}

static int SendReply(uint32_t operation, uint32_t sequence, int status, void* body, uint32_t size, OsConfigLogHandle log)
{
    TelemetryWorkerFrame reply = {0};
    int error = 0;

    reply.magic = TELEMETRY_WORKER_MAGIC;
    reply.version = TELEMETRY_WORKER_VERSION;
    reply.operation = operation;
    reply.sequence = sequence;
    reply.status = status;
    reply.size = (0 == status) ? size : 0;

    if (0 == (error = Transfer(&reply, sizeof(reply), true, log)))
    {
        error = Transfer(body, reply.size, true, log);
    }

    if (0 != error)
    {
        OsConfigLogError(log, "SendReply failed with %d (%s)", error, strerror(error));
    }

    return error;
}

static int CloseInheritedDescriptors(int logDescriptor, OsConfigLogHandle log)
{
    DIR* directory = opendir("/proc/self/fd");
    int ownDescriptor = 0;
    struct dirent* entry = NULL;
    char* end = NULL;
    long descriptor = 0;
    int status = 0;
    int closeStatus = 0;

    if (NULL == directory)
    {
        status = errno;
        OsConfigLogError(log, "CloseInheritedDescriptors: opendir failed with %d (%s)", status, strerror(status));
        return status;
    }

    ownDescriptor = dirfd(directory);

    if (ownDescriptor < 0)
    {
        status = errno;
        OsConfigLogError(log, "CloseInheritedDescriptors: dirfd failed with %d (%s)", status, strerror(status));

        if (0 != closedir(directory))
        {
            closeStatus = errno;
            OsConfigLogError(log, "CloseInheritedDescriptors: closedir failed with %d (%s)", closeStatus, strerror(closeStatus));
        }

        return status;
    }

    for (;;)
    {
        errno = 0;
        entry = readdir(directory);

        if (NULL == entry)
        {
            status = errno;

            if (0 != status)
            {
                OsConfigLogError(log, "CloseInheritedDescriptors: readdir failed with %d (%s)", status, strerror(status));
            }

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
            OsConfigLogError(log, "CloseInheritedDescriptors: invalid descriptor entry");
            break;
        }

        if ((descriptor > STDERR_FILENO) && (descriptor != ownDescriptor) && (descriptor != logDescriptor) &&
            (0 != close((int)descriptor)))
        {
            status = errno;
            OsConfigLogError(log, "CloseInheritedDescriptors: close(%ld) failed with %d (%s)", descriptor, status, strerror(status));
            break;
        }
    }

    if (0 != closedir(directory))
    {
        closeStatus = errno;
        OsConfigLogError(log, "CloseInheritedDescriptors: closedir failed with %d (%s)", closeStatus, strerror(closeStatus));

        if (0 == status)
        {
            status = closeStatus;
        }
    }

    return status;
}

static int ParseDeadline(const char* text, int64_t* deadline, OsConfigLogHandle log)
{
    char* end = NULL;
    long long value = 0;
    int status = 0;

    if ((text[0] < '0') || (text[0] > '9'))
    {
        OsConfigLogError(log, "ParseDeadline called with invalid arguments");
        return EINVAL;
    }

    errno = 0;
    value = strtoll(text, &end, 10);

    if ((0 != errno) || ('\0' != *end) || (value <= 0))
    {
        status = errno;
        OsConfigLogError(log, "ParseDeadline: expected a positive decimal deadline in range (conversion errno: %d)", status);
        return EINVAL;
    }

    *deadline = (int64_t)value;

    status = ((long long)*deadline == value) ? 0 : EOVERFLOW;

    if (status)
    {
        OsConfigLogError(log, "ParseDeadline failed with %d (%s)", status, strerror(status));
    }

    return status;
}

static int InitializeTelemetry(int argc, char** argv, timer_t* timer, int64_t* lifetime, bool* forceMinTls, OsConfigLogHandle log)
{
    int64_t startup = 0;
    int status = 0;
    struct sigaction action = {0};
    sigset_t signals = {0};
    struct sigevent notification = {0};
    FILE* logFile = NULL;
    int logDescriptor = -1;

    if (((4 != argc) && (5 != argc)) || (0 != strcmp(argv[1], TELEMETRY_WORKER_ARGUMENT)) || ((5 == argc) && (0 != strcmp(argv[4], "--force-mintls"))))
    {
        OsConfigLogError(log, "InitializeTelemetry called with invalid arguments");
        return EINVAL;
    }

    *forceMinTls = 5 == argc;

    if (0 != (status = ParseDeadline(argv[2], lifetime, log)))
    {
        OsConfigLogError(log, "InitializeTelemetry: cannot parse lifetime deadline, status: %d (%s)", status, strerror(status));
        return status;
    }

    if (0 != (status = ParseDeadline(argv[3], &startup, log)))
    {
        OsConfigLogError(log, "InitializeTelemetry: cannot parse startup deadline, status: %d (%s)", status, strerror(status));
        return status;
    }

    if (startup > *lifetime)
    {
        OsConfigLogError(log, "InitializeTelemetry: startup deadline exceeds lifetime deadline");
        return EINVAL;
    }

    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    sigemptyset(&signals);
    sigaddset(&signals, SIGALRM);

    if (0 != sigaction(SIGALRM, &action, NULL))
    {
        status = errno;
        OsConfigLogError(log, "InitializeTelemetry: sigaction(SIGALRM) failed with %d (%s)", status, strerror(status));
        return status;
    }

    if (0 != sigprocmask(SIG_UNBLOCK, &signals, NULL))
    {
        status = errno;
        OsConfigLogError(log, "InitializeTelemetry: sigprocmask(SIG_UNBLOCK) failed with %d (%s)", status, strerror(status));
        return status;
    }

    // After exec these dispositions belong only to this worker, never the host.
    action.sa_handler = SIG_IGN;

    if (0 != sigaction(SIGPIPE, &action, NULL))
    {
        status = errno;
        OsConfigLogError(log, "InitializeTelemetry: sigaction(SIGPIPE) failed with %d (%s)", status, strerror(status));
        return status;
    }

    notification.sigev_notify = SIGEV_SIGNAL;
    notification.sigev_signo = SIGALRM;

    if (0 != timer_create(CLOCK_MONOTONIC, &notification, timer))
    {
        status = errno;
        OsConfigLogError(log, "InitializeTelemetry: timer_create failed with %d (%s)", status, strerror(status));
        return status;
    }

    if (0 != (status = TelemetryArmDeadline(*timer, startup)))
    {
        OsConfigLogError(log, "InitializeTelemetry: arming startup deadline failed with %d (%s)", status, strerror(status));
        return status;
    }

    logFile = GetLogFile(log);

    if (NULL != logFile)
    {
        if ((logDescriptor = fileno(logFile)) < 0)
        {
            status = errno;
            OsConfigLogError(log, "InitializeTelemetry: fileno(logFile) failed with %d (%s)", status, strerror(status));
            return status;
        }
    }

    if (0 != (status = CloseInheritedDescriptors(logDescriptor, log)))
    {
        OsConfigLogError(log, "InitializeTelemetry: CloseInheritedDescriptors failed with %d (%s)", status, strerror(status));
    }

    return status;
}

static int SendEvent(TelemetryTransport** transport, char* epoch, const unsigned char* payload, size_t size, uint32_t sequence, int64_t deadline, bool forceMinTls, OsConfigLogHandle log)
{
    const char* token = getenv("OsConfigTelemetryApiKey");
    char headers[TELEMETRY_HTTP_HEADER_LIMIT + 1] = {0};
    size_t headerSize = 0;
    size_t tenantLength = 0;
    char iKey[TELEMETRY_HTTP_TOKEN_LIMIT + 3] = "o:";
    unsigned char bytes[TELEMETRY_MAX_EVENT_SIZE] = {0};
    size_t encodedSize = 0;
    int64_t uploadTime = 0;
    TelemetryHttpResponse response = {0};
    int status = 0;

    if (!token)
    {
        token = API_KEY;
    }

    if (0 == (status = TelemetryHttpBuildRequest(token, TELEMETRY_CLIENT_VERSION, 0, 1, headers, sizeof(headers), &headerSize, log)))
    {
        tenantLength = strcspn(token, "-");

        if ((0 == tenantLength) || ('-' != token[tenantLength]))
        {
            OsConfigLogError(log, "SendEvent: Ingestion key lacks tenant prefix");
            status = EINVAL;
        }
        else
        {
            memcpy(iKey + 2, token, tenantLength);

            if (!epoch[0])
            {
                if (0 != (status = TelemetryCreateEpoch(epoch, log)))
                {
                    OsConfigLogError(log, "SendEvent: TelemetryCreateEpoch failed with %d (%s)", status, strerror(status));
                }
            }
        }
    }

    if (!status)
    {
        if (0 != (status = TelemetryEncodePayload(payload, size, iKey, epoch, sequence, bytes, &encodedSize, &uploadTime, log)))
        {
            OsConfigLogError(log, "SendEvent: TelemetryEncodePayload failed with %d (%s)", status, strerror(status));
        }
    }

    if ((!status) && (!*transport))
    {
        if (0 != (status = TelemetryTransportCreate(transport, forceMinTls, log)))
        {
            OsConfigLogError(log, "SendEvent: TelemetryTransportCreate failed with %d (%s)", status, strerror(status));
        }
    }

    if (0 == status)
    {
        if (0 != (status = TelemetryTransportSend(*transport, token, TELEMETRY_CLIENT_VERSION, uploadTime, bytes, encodedSize, deadline, &response, log)))
        {
            OsConfigLogError(log, "SendEvent: TelemetryTransportSend failed with %d (%s)", status, strerror(status));
        }
    }

    if ((!status) && (TelemetryAccepted != response.acceptance))
    {
        status = TelemetryRejected == response.acceptance ? ECANCELED : EPROTO;
        OsConfigLogInfo(log, "SendEvent: event not accepted (http: %u, acceptance: %d)", response.status, (int)response.acceptance);
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
    TelemetryWorkerFrame request = {0};
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {0};
    TelemetryResolverReply reply = {0};
    TelemetryWorkerSendReply sent = {0};
    bool forceMinTls = false;

    log = OpenLog(LOG_FILE, ROLLED_LOG_FILE);

    OsConfigLogInfo(log, "OSConfigTelemetry starting (PID: %ld, PPID: %ld)", (long)getpid(), (long)getppid());

    if (0 != (status = InitializeTelemetry(argc, argv, &timer, &lifetime, &forceMinTls, log)))
    {
        OsConfigLogError(log, "OSConfigTelemetry: InitializeTelemetry failed with %d (%s)", status, strerror(status));
        SendReply(TELEMETRY_WORKER_READY, 0, status, NULL, 0, log);
    }
    else if (0 != (status = SendReply(TELEMETRY_WORKER_READY, 0, 0, NULL, 0, log)))
    {
        OsConfigLogError(log, "OSConfigTelemetry: SendReply failed with %d (%s)", status, strerror(status));
    }
    else
    {
        OsConfigLogInfo(log, "OSConfigTelemetry: ready");

        for (;;)
        {
            request = (TelemetryWorkerFrame){0};
            reply = (TelemetryResolverReply){0};

            if (0 != (status = TelemetryArmDeadline(timer, lifetime)))
            {
                OsConfigLogError(log, "OSConfigTelemetry: TelemetryArmDeadline failed with %d (%s)", status, strerror(status));
                break;
            }

            if (0 != (status = Transfer(&request, sizeof(request), false, log)))
            {
                if (ENODATA == status)
                {
                    OsConfigLogInfo(log, "OSConfigTelemetry: Transfer failed with ENODATA, parent disconnected");
                    status = 0;
                }
                else
                {
                    OsConfigLogError(log, "OSConfigTelemetry: Transfer failed with %d (%s)", status, strerror(status));
                }

                break;
            }

            if ((TELEMETRY_WORKER_MAGIC != request.magic) ||
                (TELEMETRY_WORKER_VERSION != request.version) ||
                ((TELEMETRY_WORKER_RESOLVE != request.operation) && (TELEMETRY_WORKER_SEND != request.operation)) ||
                (0 != request.status) ||
                (UINT32_MAX == sequence) || (request.sequence != sequence + 1) ||
                (request.size < 2) ||
                (request.size > sizeof(payload)) ||
                ((TELEMETRY_WORKER_RESOLVE == request.operation) && (request.size > TELEMETRY_RESOLVER_HOST_LIMIT + 1)) ||
                (request.deadline <= 0) ||
                (request.deadline > lifetime))
            {
                status = EPROTO;

                OsConfigLogError(log, "OSConfigTelemetry: invalid request (operation: %u, sequence: %u)", (unsigned int)request.operation, (unsigned int)request.sequence);
                SendReply(request.operation, request.sequence, status, NULL, 0, log);
                break;
            }

            sequence = request.sequence;

            if (0 == (status = TelemetryArmDeadline(timer, request.deadline)))
            {
                OsConfigLogDebug(log, "OSConfigTelemetry: receiving request (sequence: %u)", (unsigned int)sequence);
                status = Transfer(payload, request.size, false, log);
            }
            else
            {
                OsConfigLogError(log, "OSConfigTelemetry: arming request deadline failed with %d (%s) (sequence: %u)", status, strerror(status), (unsigned int)sequence);
            }

            if ((0 == status) && (TELEMETRY_WORKER_RESOLVE == request.operation) && (strnlen((const char*)payload, request.size) != request.size - 1))
            {
                OsConfigLogError(log, "OSConfigTelemetry: invalid hostname payload, expected one trailing NUL and no embedded NUL (sequence: %u)", (unsigned int)sequence);
                status = EINVAL;
            }

            if (0 != status)
            {
                OsConfigLogError(log, "OSConfigTelemetry: request deadline/body failed with %d (%s) (sequence: %u)", status, strerror(status), (unsigned int)sequence);
                SendReply(request.operation, sequence, status, NULL, 0, log);
                break;
            }

            if (TELEMETRY_WORKER_SEND == request.operation)
            {
                sent = (TelemetryWorkerSendReply){0};
                sent.status = SendEvent(&transport, epoch, payload, request.size, sequence, request.deadline, forceMinTls, log);
                sent.suppressed = TelemetryTransportSuppressed(transport) ? 1 : 0;

                if (sent.status)
                {
                    OsConfigLogError(log, "OSConfigTelemetry: SEND failed with %d (%s) (sequence: %u)", sent.status, strerror(sent.status), (unsigned int)sequence);
                }

                if (0 != (status = SendReply(request.operation, sequence, 0, &sent, sizeof(sent), log)))
                {
                    OsConfigLogError(log, "OSConfigTelemetry: SendReply failed with %d (%s)", status, strerror(status));
                    break;
                }

                continue;
            }

            OsConfigLogInfo(log, "OSConfigTelemetry: resolving request (sequence: %u)", (unsigned int)sequence);
            TelemetryLookupHost((const char*)payload, &reply);

            if (0 != reply.error)
            {
                OsConfigLogError(log, "OSConfigTelemetry: lookup failed with %d (%s), resolver: %d (%s), sequence: %u",
                    (int)reply.error, strerror(reply.error), (int)reply.lookupError,
                    (0 != reply.lookupError) ? gai_strerror(reply.lookupError) : "no resolver error", (unsigned int)sequence);
            }
            else
            {
                OsConfigLogInfo(log, "OSConfigTelemetry: lookup complete (sequence: %u, reply count: %u)", (unsigned int)sequence, (unsigned int)reply.count);
            }

            if (0 != (status = SendReply(request.operation, sequence, 0, &reply, sizeof(reply), log)))
            {
                OsConfigLogError(log, "OSConfigTelemetry: SendReply failed with %d (%s)", status, strerror(status));
                break;
            }

            // Keep the operation timer armed through the reply; the next loop restores the lifetime timer while waiting for the caller's next event.
        }
    }

    if (NULL != transport)
    {
        TelemetryTransportDestroy(&transport, log);
    }

    OsConfigLogInfo(log, "OSConfigTelemetry: exiting with status: %d (%s)", status, strerror(status));

    CloseLog(&log);

    _exit((0 == status) ? EXIT_SUCCESS : EXIT_FAILURE);
}
