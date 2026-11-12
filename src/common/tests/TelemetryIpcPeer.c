// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include <IpcInterface.h>
#include <parson.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
            write(STDOUT_FILENO, (char*)buffer + offset, count) :
            read(STDIN_FILENO, (char*)buffer + offset, count);
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

int main(int argc, char** argv)
{
    TelemetryWorkerFrame response = {0};
    TelemetryWorkerFrame request = {0};
    TelemetryWorkerSendReply reply = {0};
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE + 1] = {0};
    unsigned char packet[TELEMETRY_WORKER_HEADER_SIZE + 8] = {0};
    unsigned char header[TELEMETRY_WORKER_HEADER_SIZE] = {0};
    JSON_Value* parsed = NULL;
    uint32_t sequence = 0;
    const char* event = NULL;
    const char* readyVersion = getenv("OSCONFIG_TEST_READY_VERSION");
    int status = 0;

    if ((argc != 6) || (0 != strcmp(argv[1], "--pipe-v4")) ||
        (0 != strcmp(argv[2], "--api-key-env")) || (0 != strcmp(argv[3], "OsConfigTelemetryApiKey")) ||
        (0 != strcmp(argv[4], "--sdk-version")) || (0 != strcmp(argv[5], "OSConfig-C/0.1")))
    {
        return EXIT_FAILURE;
    }
    response.magic = TELEMETRY_WORKER_MAGIC;
    response.version = (NULL == readyVersion) ? TELEMETRY_WORKER_VERSION : (uint32_t)atoi(readyVersion);
    response.operation = TELEMETRY_WORKER_READY;
    TelemetryEncodeFrame(&response, header);
    status = Transfer(header, sizeof(header), true, false);

    while (0 == status)
    {
        status = Transfer(header, sizeof(header), false, false);
        if (0 != status)
        {
            // Closing the parent's connection is normal teardown.
            status = 0;
            break;
        }

        TelemetryDecodeFrame(header, &request);
        if ((TELEMETRY_WORKER_MAGIC != request.magic) || (TELEMETRY_WORKER_VERSION != request.version) ||
            (TELEMETRY_WORKER_SEND != request.operation) || (request.sequence != ++sequence) ||
            (request.size == 0) || (request.size > TELEMETRY_MAX_EVENT_SIZE) ||
            (request.reserved != 0) || (request.status != 0) ||
            (0 != Transfer(payload, request.size, false, false)))
        {
            status = EPROTO;
            break;
        }

        payload[request.size] = '\0';
        json_value_free(parsed);
        parsed = json_parse_string((const char*)payload);
        event = json_object_get_string(json_value_get_object(parsed), "EventName");
        if (NULL == event)
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
        if (0 == strcmp(event, "delayed"))
        {
            struct timespec delay = {0, 600000000};
            while ((0 != nanosleep(&delay, &delay)) && (EINTR == errno)) {}
        }

        response = (TelemetryWorkerFrame){0};
        response.magic = TELEMETRY_WORKER_MAGIC;
        response.version = TELEMETRY_WORKER_VERSION;
        response.operation = request.operation;
        response.sequence = request.sequence;
        response.size = sizeof(reply);
        reply = (TelemetryWorkerSendReply){0};

        if ((0 == strcmp(event, "must_reuse")) && (sequence < 2))
        {
            reply.status = 3;
        }
        if (0 == strcmp(event, "reject"))
        {
            reply.status = 5;
        }
        if (0 == strcmp(event, "suppress"))
        {
            reply.status = 7;
            reply.suppressed = 1;
        }
        if (0 == strcmp(event, "bad_version"))
        {
            ++response.version;
        }
        if (0 == strcmp(event, "bad_sequence"))
        {
            ++response.sequence;
        }
        if (0 == strcmp(event, "negative_status"))
        {
            reply.status = -1;
        }
        if (0 == strcmp(event, "bad_suppression"))
        {
            reply.suppressed = 2;
        }

        TelemetryEncodeFrame(&response, packet);
        TelemetryWriteWord(packet + TELEMETRY_WORKER_HEADER_SIZE, reply.status);
        TelemetryWriteWord(packet + TELEMETRY_WORKER_HEADER_SIZE + 4, reply.suppressed);
        if (0 == strcmp(event, "close_input")) close(STDIN_FILENO);
        status = Transfer(packet, sizeof(packet), true, 0 == strcmp(event, "split"));
        if (0 == strcmp(event, "close_input")) for (;;) pause();
        if (0 == strcmp(event, "accepted_exit")) break;
    }

    json_value_free(parsed);
    return (0 == status) ? EXIT_SUCCESS : EXIT_FAILURE;
}
