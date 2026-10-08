// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 * How to run this
 *
 * From ~/azure-osconfig/build on Linux:
 *
 * sudo --preserve-env=OsConfigTelemetryApiKey cmake --build . --target telemetryariatest
 * sudo --preserve-env=https_proxy,HTTPS_PROXY,all_proxy,ALL_PROXY,no_proxy,NO_PROXY ./common/telemetry/telemetryariatest --send-status-trace-10000
 *
 * Sends 10,000 synthetic StatusTrace events to Aria using the compiled ingestion key.
 * Save the printed correlation ID and final counts to verify the run in Aria.
 * This exercises the encoder/transport directly, not WorkerMain.c or worker IPC.
 */

#define _POSIX_C_SOURCE 200809L

#include "Encoder.h"
#include "Event.h"
#include "Transport.h"
#include "Deadline.h"
#include "Keys.h"
#include <version.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_COUNT 10000
#define TEST_RESULT "731001"
#define TEST_MARKER "*** Distilled 1DS SDK test No. 66 ***"
#define TEST_CLIENT "OSConfig-C/0.1"

static int InitializeTimer(timer_t* timer)
{
    struct sigaction action = {0};
    sigset_t signals = {0};
    struct sigevent notification = {0};
    int status = 0;

    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    sigemptyset(&signals);
    sigaddset(&signals, SIGALRM);

    if ((0 != sigaction(SIGALRM, &action, NULL)) || (0 != sigprocmask(SIG_UNBLOCK, &signals, NULL)))
    {
        status = (0 != errno) ? errno : EIO;
    }
    else
    {
        action.sa_handler = SIG_IGN;

        if (0 != sigaction(SIGPIPE, &action, NULL))
        {
            status = (0 != errno) ? errno : EIO;
        }
        else
        {
            notification.sigev_notify = SIGEV_SIGNAL;
            notification.sigev_signo = SIGALRM;
            status = (0 != timer_create(CLOCK_MONOTONIC, &notification, timer)) ? ((0 != errno) ? errno : EIO) : 0;
        }
    }

    return status;
}

static int Encode(const char* iKey, const char* correlation, unsigned int sequence, unsigned char* bytes, size_t* size, int64_t* milliseconds, OsConfigLogHandle log)
{
    struct timespec now = {0, 0};
    struct tm utc = {0};
    char timestamp[40] = {0};
    char line[16] = {0};
    const char* names[] = {"DistroName", "CorrelationId", "Version", "Timestamp", "FileName", "LineNumber", "ScenarioName", "FunctionName",
        "RuleCodename", "CallingFunctionName", "Microseconds", "ResultCode", "ResultString"};
    TelemetryProperty properties[ARRAY_SIZE(names)] = {0};
    size_t i = 0;
    TelemetryEvent event = {0};
    int status = 0;

    if (0 != clock_gettime(CLOCK_REALTIME, &now))
    {
        status = errno;
        OsConfigLogError(log, "Encode: clock_gettime(CLOCK_REALTIME) failed with %d (%s)", status, strerror(status));
        return EIO;
    }

    if (NULL == gmtime_r(&now.tv_sec, &utc))
    {
        status = errno;
        OsConfigLogError(log, "Encode: gmtime_r failed with %d (%s)", status, strerror(status));
        return EIO;
    }

    if ((now.tv_sec < 0) || ((uint64_t)now.tv_sec > UINT64_C(253402300799)))
    {
        OsConfigLogError(log, "Encode: UTC timestamp range check failed with %d (%s)", EOVERFLOW, strerror(EOVERFLOW));
        return EOVERFLOW;
    }

    if (0 == strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S+0000", &utc))
    {
        OsConfigLogError(log, "Encode: strftime failed with %d (%s)", EOVERFLOW, strerror(EOVERFLOW));
        return EOVERFLOW;
    }

    snprintf(line, sizeof(line), "%u", sequence);
    *milliseconds = ((int64_t)now.tv_sec * 1000) + (now.tv_nsec / 1000000);
    const char* values[] = {"Synthetic Linux test", correlation, OSCONFIG_VERSION, timestamp, "AriaTest.c", line, "DistilledTelemetryTest", "TelemetryAriaTest",
        "DistilledTelemetryTest", "TelemetryTransportSend", "0", TEST_RESULT, TEST_MARKER};

    for (i = 0; i < ARRAY_SIZE(names); ++i)
    {
        properties[i].name = names[i];
        properties[i].type = TelemetryPropertyString;
        properties[i].value.stringValue = values[i];
    }

    event.name = "StatusTrace";
    event.iKey = iKey;
    event.time = INT64_C(621355968000000000) + ((int64_t)now.tv_sec * 10000000) + (now.tv_nsec / 100);
    event.flags = 0x0101;
    event.sdkVersion = TEST_CLIENT;
    event.sdkEpoch = correlation;
    event.sequence = sequence;
    event.properties = properties;
    event.propertyCount = ARRAY_SIZE(properties);

    return TelemetryEncodeEvent(&event, bytes, TELEMETRY_MAX_EVENT_SIZE, size, log);
}

int main(int argc, char** argv)
{
    timer_t timer = 0;
    int status = 0;
    int64_t started = 0;
    OsConfigLogHandle log = NULL;
    TelemetryTransport* transport = NULL;
    unsigned int attempted = 0;
    unsigned int accepted = 0;
    unsigned int rejected = 0;
    unsigned int unconfirmed = 0;
    char correlation[37] = {0};
    char iKey[TELEMETRY_HTTP_TOKEN_LIMIT + 3] = {0};
    const char* token = NULL;
    char headers[TELEMETRY_HTTP_HEADER_LIMIT + 1] = {0};
    size_t headerSize = 0;
    size_t tenantLength = 0;
    unsigned int i = 0;
    int64_t now = 0;
    int64_t uploadTime = 0;
    unsigned char bytes[TELEMETRY_MAX_EVENT_SIZE] = {0};
    size_t size = 0;
    TelemetryHttpResponse response = {0};
    const char* operation = "InitializeTimer";

    if ((2 != argc) || (0 != strcmp(argv[1], "--send-status-trace-10000")))
    {
        fprintf(stderr, "Explicit live test only: %s --send-status-trace-10000\n"
            "Sends 10000 synthetic StatusTrace events to Aria using the build-time ingestion key.\n"
            "OsConfigTelemetryApiKey in the runtime environment overrides that key.\n"
            "Honors inherited HTTPS proxy settings. Never scheduled by ctest.\n", argv[0]);
        return 2;
    }

    if (0 == (status = InitializeTimer(&timer)))
    {
        operation = "TelemetryMonotonicTime";
        status = TelemetryMonotonicTime(&started);
    }

    if ((0 == status) && (started > (INT64_MAX - INT64_C(15000000000))))
    {
        operation = "deadline range check";
        status = EOVERFLOW;
    }

    if (0 == status)
    {
        operation = "TelemetryArmDeadline";
        status = TelemetryArmDeadline(timer, started + INT64_C(15000000000));
    }

    if (0 != status)
    {
        fprintf(stderr, "main: %s failed with %d (%s)\n", operation, status, strerror(status));
        return 1;
    }

    SetConsoleLoggingEnabled(false);
    log = OpenLog("/var/log/osconfig_telemetry.log", "/var/log/osconfig_telemetry.bak");

    if ((NULL == log) || (NULL == GetLogFile(log)))
    {
        fprintf(stderr, "Cannot open /var/log/osconfig_telemetry.log; no events sent.\n");
        CloseLog(&log);

        return 1;
    }

    token = getenv("OsConfigTelemetryApiKey");

    if (NULL == token)
    {
        token = API_KEY;
    }

    if ((NULL == token) || ('\0' == *token))
    {
        operation = "ingestion key validation";
        status = EINVAL;
        OsConfigLogError(log, "main: ingestion key validation failed with %d (%s); missing key", status, strerror(status));
        fprintf(stderr, "Configure the build with OsConfigTelemetryApiKey or supply a nonempty runtime override; no events sent.\n");
        goto cleanup;
    }

    operation = "TelemetryHttpBuildRequest";
    if (0 != (status = TelemetryHttpBuildRequest(token, TEST_CLIENT, 0, 1, headers, sizeof(headers), &headerSize, log)))
    {
        goto cleanup;
    }

    tenantLength = strcspn(token, "-");

    if ((0 == tenantLength) || ('-' != token[tenantLength]))
    {
        operation = "ingestion key tenant prefix validation";
        status = EINVAL;
        OsConfigLogError(log, "main: ingestion key tenant prefix validation failed with %d (%s)", status, strerror(status));
        goto cleanup;
    }

    memcpy(iKey, "o:", 2);
    memcpy(iKey + 2, token, tenantLength);

    operation = "TelemetryCreateEpoch";
    if (0 == (status = TelemetryCreateEpoch(correlation, log)))
    {
        operation = "TelemetryTransportCreate";
        status = TelemetryTransportCreate(&transport, log);
    }

    if (0 != status)
    {
        goto cleanup;
    }

    printf("LIVE StatusTrace test: requested=%d ResultCode=%s\n"
        "ResultString=%s\nCorrelationId=%s\n"
        "TLS mode: system OpenSSL (3, 1.1, then 1.0.2)\n"
        "Acceptance is collector acknowledgment, not proof of downstream Aria visibility.\n",
        TEST_COUNT, TEST_RESULT, TEST_MARKER, correlation);
    fflush(stdout);

    for (i = 0; i < TEST_COUNT; ++i)
    {
        now = 0;
        uploadTime = 0;
        operation = "TelemetryMonotonicTime";

        if (0 != (status = TelemetryMonotonicTime(&now)))
        {
            break;
        }

        if (now > (INT64_MAX - INT64_C(15000000000)))
        {
            operation = "deadline range check";
            status = EOVERFLOW;
            break;
        }

        const int64_t deadline = now + INT64_C(15000000000);

        operation = "TelemetryArmDeadline";
        if (0 != (status = TelemetryArmDeadline(timer, deadline)))
        {
            break;
        }

        size = 0;
        operation = "Encode";

        if (0 != (status = Encode(iKey, correlation, i + 1, bytes, &size, &uploadTime, log)))
        {
            break;
        }

        ++attempted;
        operation = "TelemetryTransportSend";

        if ((0 == (status = TelemetryTransportSend(transport, token, TEST_CLIENT, uploadTime, bytes, size, deadline, &response, log))) && (TelemetryAccepted == response.acceptance))
        {
            ++accepted;
        }
        else if ((0 == status) && (TelemetryRejected == response.acceptance))
        {
            ++rejected;
        }
        else
        {
            ++unconfirmed;
        }

        if ((0 != status) || (TelemetryAccepted != response.acceptance) || TelemetryTransportSuppressed(transport))
        {
            fprintf(stderr, "main: event %u delivery failed with %d (%s); HTTP status %u, acceptance %d, controls %zu\n",
                i + 1, (0 != status) ? status : ECANCELED, strerror((0 != status) ? status : ECANCELED),
                response.status, (int)response.acceptance, response.controlCount);

            if (0 == status)
            {
                operation = "collector acceptance/suppression check";
                status = ECANCELED;
            }

            fprintf(stderr, "Stopped without replay. Inspect the log and collector controls before another pass.\n");
            break;
        }
    }

cleanup:
    if (0 != status)
    {
        OsConfigLogError(log, "main: %s failed with %d (%s)", operation, status, strerror(status));
    }

    TelemetryTransportDestroy(&transport, log);
    // Keep the self-timer armed through log/runtime cleanup; process exit reclaims it.
    printf("requested=%d attempted=%u accepted=%u rejected=%u unconfirmed=%u unsent=%u status=%d\n"
        "CorrelationId=%s\n",
        TEST_COUNT, attempted, accepted, rejected, unconfirmed, TEST_COUNT - attempted, status, correlation);

    if (0 != status)
    {
        fprintf(stderr, "See /var/log/osconfig_telemetry.log; do not print the ingestion token.\n");
    }

    CloseLog(&log);

    return ((0 == status) && (TEST_COUNT == accepted)) ? 0 : 1;
}
