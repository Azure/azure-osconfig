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
 * This exercises the encoder/transport directly, not TelemetryWorkerMain.c or worker IPC.
 */

#define _POSIX_C_SOURCE 200809L

#include "TelemetryEncoder.h"
#include "TelemetryEvent.h"
#include "TelemetryTransport.h"
#include "TelemetryDeadline.h"
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
#define TEST_MARKER "*** Distilled 1DS SDK test No. 3 ***"
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

    if ((sigaction(SIGALRM, &action, NULL)) || (sigprocmask(SIG_UNBLOCK, &signals, NULL)))
    {
        status = errno ? errno : EIO;
    }
    else
    {
        action.sa_handler = SIG_IGN;

        if (sigaction(SIGPIPE, &action, NULL))
        {
            status = errno ? errno : EIO;
        }
        else
        {
            notification.sigev_notify = SIGEV_SIGNAL;
            notification.sigev_signo = SIGALRM;
            status = timer_create(CLOCK_MONOTONIC, &notification, timer) ? (errno ? errno : EIO) : 0;
        }
    }

    return status;
}

static int Encode(const char* iKey, const char* correlation, unsigned int sequence,
    unsigned char* bytes, size_t* size, int64_t* milliseconds, OsConfigLogHandle log)
{
    struct timespec now = {0, 0};
    struct tm utc = {0};
    char timestamp[40] = {0};
    char line[16] = {0};
    const char* names[] = {"DistroName", "CorrelationId", "Version", "Timestamp", "FileName",
        "LineNumber", "ScenarioName", "FunctionName", "RuleCodename", "CallingFunctionName",
        "Microseconds", "ResultCode", "ResultString"};
    TelemetryProperty properties[ARRAY_SIZE(names)] = {0};
    size_t i = 0;
    TelemetryEvent event = {0};

    if ((clock_gettime(CLOCK_REALTIME, &now)) || (!gmtime_r(&now.tv_sec, &utc)))
    {
        OsConfigLogError(log, "TelemetryAriaTest: Cannot read UTC clock");
        return EIO;
    }

    if ((now.tv_sec < 0) || ((uint64_t)now.tv_sec > UINT64_C(253402300799)) ||
        (!strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S+0000", &utc)))
    {
        OsConfigLogError(log, "TelemetryAriaTest: Invalid UTC clock");
        return EOVERFLOW;
    }

    snprintf(line, sizeof(line), "%u", sequence);
    *milliseconds = (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
    const char* values[] = {"Synthetic Linux test", correlation, OSCONFIG_VERSION, timestamp,
        "TelemetryAriaTest.c", line, "DistilledTelemetryTest", "TelemetryAriaTest",
        "DistilledTelemetryTest", "TelemetryTransportSend", "0", TEST_RESULT, TEST_MARKER};

    for (i = 0; i < ARRAY_SIZE(names); ++i)
    {
        properties[i].name = names[i];
        properties[i].type = TelemetryPropertyString;
        properties[i].value.stringValue = values[i];
    }

    event.name = "StatusTrace";
    event.iKey = iKey;
    event.time = INT64_C(621355968000000000) + (int64_t)now.tv_sec * 10000000 + now.tv_nsec / 100;
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

    if ((2 != argc) || (strcmp(argv[1], "--send-status-trace-10000")))
    {
        fprintf(stderr, "Explicit live test only: %s --send-status-trace-10000\n"
            "Sends 10000 synthetic StatusTrace events to Aria using the build-time ingestion key.\n"
            "OsConfigTelemetryApiKey in the runtime environment overrides that key.\n"
            "Honors inherited HTTPS proxy settings. Never scheduled by ctest.\n", argv[0]);
        return 2;
    }

    if (0 == (status = InitializeTimer(&timer)))
    {
        status = TelemetryMonotonicTime(&started);
    }

    if ((!status) && (started > INT64_MAX - INT64_C(15000000000)))
    {
        status = EOVERFLOW;
    }

    if (!status)
    {
        status = TelemetryArmDeadline(timer, started + INT64_C(15000000000));
    }

    if (status)
    {
        fprintf(stderr, "Live test timer initialization failed (status=%d).\n", status);
        return 1;
    }

    SetConsoleLoggingEnabled(false);
    log = OpenLog("/var/log/osconfig_telemetry.log", "/var/log/osconfig_telemetry.bak");

    if ((!log) || (!GetLogFile(log)))
    {
        fprintf(stderr, "Cannot open /var/log/osconfig_telemetry.log; no events sent.\n");
        CloseLog(&log);

        return 1;
    }

    token = getenv("OsConfigTelemetryApiKey");

    if (!token)
    {
        token = API_KEY;
    }

    if ((!token) || (!*token))
    {
        status = EINVAL;
        OsConfigLogError(log, "TelemetryAriaTest: No ingestion key configured");
        fprintf(stderr, "Configure the build with OsConfigTelemetryApiKey or supply a nonempty runtime override; no events sent.\n");
        goto cleanup;
    }

    if (0 != (status = TelemetryHttpBuildRequest(token, TEST_CLIENT, 0, 1, headers, sizeof(headers), &headerSize, log)))
    {
        goto cleanup;
    }

    tenantLength = strcspn(token, "-");

    if ((!tenantLength) || ('-' != token[tenantLength]))
    {
        status = EINVAL;
        OsConfigLogError(log, "TelemetryAriaTest: Ingestion token lacks tenant prefix");
        goto cleanup;
    }

    memcpy(iKey, "o:", 2);
    memcpy(iKey + 2, token, tenantLength);

    if (0 == (status = TelemetryCreateEpoch(correlation, log)))
    {
        status = TelemetryTransportCreate(&transport, true, log);
    }

    if (status)
    {
        goto cleanup;
    }

    printf("LIVE StatusTrace test: requested=%d ResultCode=%s\n"
        "ResultString=%s\nCorrelationId=%s\n"
        "TLS mode: mintls only (OS OpenSSL discovery disabled)\n"
        "Acceptance is collector acknowledgment, not proof of downstream Aria visibility.\n",
        TEST_COUNT, TEST_RESULT, TEST_MARKER, correlation);
    fflush(stdout);

    for (i = 0; i < TEST_COUNT; ++i)
    {
        now = 0;
        uploadTime = 0;

        if (0 != (status = TelemetryMonotonicTime(&now)))
        {
            break;
        }

        if (now > INT64_MAX - INT64_C(15000000000))
        {
            status = EOVERFLOW;
            break;
        }

        const int64_t deadline = now + INT64_C(15000000000);

        if (0 != (status = TelemetryArmDeadline(timer, deadline)))
        {
            break;
        }

        size = 0;

        if (0 != (status = Encode(iKey, correlation, i + 1, bytes, &size, &uploadTime, log)))
        {
            break;
        }

        ++attempted;

        if ((0 == (status = TelemetryTransportSend(transport, token, TEST_CLIENT, uploadTime, bytes, size,
            deadline, &response, log))) && (TelemetryAccepted == response.acceptance))
        {
            ++accepted;
        }
        else if ((!status) && (TelemetryRejected == response.acceptance))
        {
            ++rejected;
        }
        else
        {
            ++unconfirmed;
        }

        if ((status) || (TelemetryAccepted != response.acceptance) || (TelemetryTransportSuppressed(transport)))
        {
            fprintf(stderr, "event=%u http=%u status=%d controls=%zu\n",
                i + 1, response.status, status, response.controlCount);

            if (!status)
            {
                status = ECANCELED;
            }

            fprintf(stderr, "Stopped without replay. Inspect the log and collector controls before another pass.\n");
            break;
        }
    }

cleanup:
    if (status)
    {
        OsConfigLogError(log, "TelemetryAriaTest: Run stopped (status=%d)", status);
    }

    TelemetryTransportDestroy(&transport, log);
    // Keep the self-timer armed through log/runtime cleanup; process exit reclaims it.
    printf("requested=%d attempted=%u accepted=%u rejected=%u unconfirmed=%u unsent=%u status=%d\n"
        "CorrelationId=%s\n",
        TEST_COUNT, attempted, accepted, rejected, unconfirmed, TEST_COUNT - attempted, status, correlation);

    if (status)
    {
        fprintf(stderr, "See /var/log/osconfig_telemetry.log; do not print the ingestion token.\n");
    }

    CloseLog(&log);

    return ((!status) && (TEST_COUNT == accepted)) ? 0 : 1;
}
