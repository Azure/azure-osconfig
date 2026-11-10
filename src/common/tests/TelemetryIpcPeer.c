// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include <IpcInterface.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Offline caller-test peer. It never encodes events or opens network connections.
static int Transfer(void* buffer, size_t size, bool writing, bool split)
{
    size_t offset = 0;
    size_t count = 0;
    ssize_t done = 0;
    int status = 0;

    while ((0 == status) && (offset < size))
    {
        count = split ? 1 : size - offset;
        done = writing ?
            send(STDIN_FILENO, (char*)buffer + offset, count, MSG_NOSIGNAL) :
            recv(STDIN_FILENO, (char*)buffer + offset, count, 0);
        if (done > 0)
        {
            offset += (size_t)done;
        }
        else if ((0 == done) || (EINTR != errno))
        {
            status = EPIPE;
        }
    }

    return status;
}

int main(void)
{
    TelemetryWorkerFrame response = {0};
    TelemetryWorkerFrame request = {0};
    TelemetryWorkerSendReply reply = {0};
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {0};
    unsigned char packet[sizeof(response) + sizeof(reply)] = {0};
    uint32_t sequence = 0;
    const char* event = NULL;
    const char* readyVersion = getenv("OSCONFIG_TEST_READY_VERSION");
    int status = 0;

    response.magic = TELEMETRY_WORKER_MAGIC;
    response.version = (NULL == readyVersion) ? TELEMETRY_WORKER_VERSION : (uint32_t)atoi(readyVersion);
    response.operation = TELEMETRY_WORKER_READY;
    status = Transfer(&response, sizeof(response), true, false);

    while (0 == status)
    {
        status = Transfer(&request, sizeof(request), false, false);
        if (0 != status)
        {
            // Closing the parent's connection is normal teardown.
            status = 0;
            break;
        }

        if ((TELEMETRY_WORKER_MAGIC != request.magic) || (TELEMETRY_WORKER_VERSION != request.version) ||
            (TELEMETRY_WORKER_SEND != request.operation) || (request.sequence != ++sequence) ||
            (request.size <= sizeof(uint32_t)) || (request.size > sizeof(payload)) ||
            (0 != Transfer(payload, request.size, false, false)))
        {
            status = EPROTO;
            break;
        }

        event = (const char*)payload + sizeof(uint32_t);
        if (NULL == memchr(event, '\0', request.size - sizeof(uint32_t)))
        {
            status = EPROTO;
            break;
        }

        if (0 == strcmp(event, "hang"))
        {
            for (;;)
            {
                pause();
            }
        }
        if (0 == strcmp(event, "disconnect"))
        {
            break;
        }

        response = (TelemetryWorkerFrame){0};
        response.magic = TELEMETRY_WORKER_MAGIC;
        response.version = TELEMETRY_WORKER_VERSION;
        response.operation = request.operation;
        response.sequence = request.sequence;
        response.size = sizeof(reply);
        reply = (TelemetryWorkerSendReply){0};

        if ((0 == strcmp(event, "must-reuse")) && (sequence < 2))
        {
            reply.status = EPROTO;
        }
        if (0 == strcmp(event, "reject"))
        {
            reply.status = EIO;
        }
        if (0 == strcmp(event, "suppress"))
        {
            reply.status = ECANCELED;
            reply.suppressed = 1;
        }
        if (0 == strcmp(event, "bad-version"))
        {
            ++response.version;
        }
        if (0 == strcmp(event, "bad-sequence"))
        {
            ++response.sequence;
        }
        if (0 == strcmp(event, "negative-status"))
        {
            reply.status = -1;
        }
        if (0 == strcmp(event, "bad-suppression"))
        {
            reply.suppressed = 2;
        }

        memcpy(packet, &response, sizeof(response));
        memcpy(packet + sizeof(response), &reply, sizeof(reply));
        status = Transfer(packet, sizeof(packet), true, 0 == strcmp(event, "split"));
    }

    return (0 == status) ? EXIT_SUCCESS : EXIT_FAILURE;
}
