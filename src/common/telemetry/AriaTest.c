// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Explicit live test only: telemetryariatest --send-status-trace /path/to/OSConfigTelemetry
// Uses the real C adapter and pipe v3 worker with unchanged production time limits.

#define _GNU_SOURCE

#include <CommonUtils.h>
#include <IpcInterface.h>
#include <version.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_MARKER "*** OSConfig C adapter test No. 73 (IPC to Ubuntu 14 Rust worker) ***"

int main(int argc, char** argv)
{
    TelemetryWorker* worker = NULL;
    char* path = NULL;
    char* distro = NULL;
    FILE* uuidFile = NULL;
    char correlation[38] = {0};
    char timestamp[40] = {0};
    const char* key = getenv("OsConfigTelemetryApiKey");
    const char* operation = "API key environment check";
    const char* names[] = {"DistroName", "CorrelationId", "Version", "Timestamp", "FileName", "LineNumber",
        "ScenarioName", "FunctionName", "RuleCodename", "CallingFunctionName", "Microseconds", "ResultCode", "ResultString"};
    const char* values[] = {NULL, correlation, OSCONFIG_VERSION, timestamp, "AriaTest.c", "0",
        "DistilledTelemetryTest", "TelemetryAriaTest", "DistilledTelemetryTest", "TelemetryWorkerSendEvent",
        "0", "731001", TEST_MARKER};
    TelemetryProperty properties[ARRAY_SIZE(names)];
    int status = 0;
    int cleanupStatus = 0;
    int64_t started = 0;
    unsigned int attempted = 0;
    unsigned int accepted = 0;
    size_t i = 0;

    if ((3 != argc) || (0 != strcmp(argv[1], "--send-status-trace")))
    {
        fprintf(stderr, "Explicit live test: %s --send-status-trace /path/to/OSConfigTelemetry\n"
            "Sends one synthetic StatusTrace event over IPC using OsConfigTelemetryApiKey.\n"
            "The production 500 ms cumulative work budget is unchanged. No retries.\n", argv[0]);
        return 2;
    }

    SetConsoleLoggingEnabled(true);
    if ((NULL == key) || ('\0' == key[0]))
    {
        status = EINVAL;
    }
    else
    {
        operation = "resolve worker path";
        path = realpath(argv[2], NULL);
        if (NULL == path)
        {
            status = (0 != errno) ? errno : ENOENT;
        }
    }

    if (0 == status)
    {
        operation = "TelemetryWorkerCreate";
        status = TelemetryWorkerCreate(path, 600000, 500, 500, &worker, NULL);
    }
    if (0 == status)
    {
        operation = "preparation start time";
        TelemetryMonotonicTime(&started, status);
    }
    if (0 == status)
    {
        operation = "read kernel-generated correlation ID";
        uuidFile = fopen("/proc/sys/kernel/random/uuid", "r");
        if (NULL == uuidFile)
        {
            status = (0 != errno) ? errno : EIO;
        }
        else
        {
            if (NULL == fgets(correlation, sizeof(correlation), uuidFile))
            {
                status = EIO;
            }
            correlation[strcspn(correlation, "\r\n")] = '\0';
            if ((0 == status) && (36 != strlen(correlation)))
            {
                status = EINVAL;
            }
            if ((0 != fclose(uuidFile)) && (0 == status))
            {
                status = (0 != errno) ? errno : EIO;
            }
        }
    }
    if (0 == status)
    {
        operation = "GetOsPrettyName";
        distro = GetOsPrettyName(NULL);
        if ((NULL == distro) || ('\0' == distro[0]))
        {
            status = ENODATA;
        }
    }
    if (0 == status)
    {
        operation = "prepare UTC timestamp";
        const char* now = GetFormattedTime();
        int length = (NULL != now) ? snprintf(timestamp, sizeof(timestamp), "%s", now) : -1;
        if ((length < 0) || ((size_t)length >= sizeof(timestamp)))
        {
            status = EOVERFLOW;
        }
    }
    if (0 == status)
    {
        values[0] = distro;
        for (i = 0; i < ARRAY_SIZE(properties); ++i)
        {
            properties[i].name = names[i];
            properties[i].type = TelemetryPropertyString;
            properties[i].value.stringValue = values[i];
        }
        printf("%s\nCorrelationId=%s\n", TEST_MARKER, correlation);
        fflush(stdout);
        operation = "TelemetryWorkerAccountPreparation";
        status = TelemetryWorkerAccountPreparation(worker, started, NULL);
    }
    if (0 == status)
    {
        operation = "TelemetryWorkerSendEvent";
        attempted = 1;
        status = TelemetryWorkerSendEvent(worker, "StatusTrace", properties, ARRAY_SIZE(properties), NULL);
        accepted = (0 == status) ? 1 : 0;
    }
    if (0 != status)
    {
        OsConfigLogInfo(NULL, "AriaTest: %s failed with %d (%s)", operation, status, strerror(status));
    }

    cleanupStatus = TelemetryWorkerDestroy(&worker, NULL);
    if (0 != cleanupStatus)
    {
        OsConfigLogInfo(NULL, "AriaTest: TelemetryWorkerDestroy failed with %d (%s)", cleanupStatus, strerror(cleanupStatus));
        if (0 == status)
        {
            status = cleanupStatus;
        }
    }
    free(distro);
    free(path);
    printf("Attempted: %u; accepted: %u; planned: 1\n", attempted, accepted);
    return (0 == status) ? 0 : 1;
}
