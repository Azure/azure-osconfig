// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Sends 100 events sequentially over IPC through one Rust worker; no retries.
// Build from azure-osconfig/build: cmake --build . --target AriaTestC
// Use a worker with a compiled key, or set OsConfigTelemetryApiKey securely.
// Run from that directory: ./common/telemetry/AriaTestC --send-status-trace "$PWD/../src/common/telemetry/OSConfigTelemetry"
// Both samples share a test number; increment TEST_MARKER for the next test.

#define _GNU_SOURCE

#include <CommonUtils.h>
#include <IpcInterface.h>
#include <version.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_MARKER "***Universal Rust Telemetry Client test (C adapter) #2***"

int main(int argc, char** argv)
{
    TelemetryWorker* worker = NULL;
    char* path = NULL;
    char* distro = NULL;
    FILE* uuidFile = NULL;
    char correlation[38] = {0};
    char timestamp[40] = {0};
    const char* operation = "resolve worker path";
    const char* names[] = {"DistroName", "CorrelationId", "Version", "Timestamp", "FileName", "LineNumber",
        "ScenarioName", "FunctionName", "RuleCodename", "CallingFunctionName", "Microseconds", "ResultCode", "ResultString"};
    const char* values[] = {NULL, correlation, OSCONFIG_VERSION, timestamp, "AriaTest.c", "0",
        "DistilledTelemetryTest", "TelemetryAriaTest", "DistilledTelemetryTest", "SendEvent",
        "0", "731001", TEST_MARKER};
    TelemetryProperty properties[ARRAY_SIZE(names)];
    int status = 0;
    int cleanupStatus = 0;
    unsigned int attempted = 0;
    unsigned int accepted = 0;
    size_t i = 0;

    if ((3 != argc) || (0 != strcmp(argv[1], "--send-status-trace")))
    {
        OsConfigLogError(NULL, "Explicit live test: %s --send-status-trace /path/to/OSConfigTelemetry", argv[0]);
        OsConfigLogInfo(NULL, "Sends 100 synthetic StatusTrace events over one worker's IPC using a compiled key or OsConfigTelemetryApiKey.");
        OsConfigLogInfo(NULL, "No invocation budget, batching, delays, or retries.");
        return 2;
    }

    path = realpath(argv[2], NULL);
    if (NULL == path)
    {
        status = (0 != errno) ? errno : ENOENT;
    }

    if (0 == status)
    {
        operation = "Initialize";
        status = Initialize(path, &worker, NULL);
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
        OsConfigLogInfo(NULL, "%s", TEST_MARKER);
        OsConfigLogInfo(NULL, "CorrelationId=%s", correlation);
        fflush(stdout);
    }
    while ((0 == status) && (attempted < 100))
    {
        operation = "SendEvent";
        ++attempted;
        status = SendEvent(worker, "StatusTrace", properties, ARRAY_SIZE(properties), NULL);
        if (0 == status)
        {
            ++accepted;
        }
    }
    if (0 != status)
    {
        OsConfigLogInfo(NULL, "AriaTest: %s failed with %d (%s)", operation, status, strerror(status));
    }

    cleanupStatus = Shutdown(&worker, NULL);
    if (0 != cleanupStatus)
    {
        OsConfigLogInfo(NULL, "AriaTest: Shutdown failed with %d (%s)", cleanupStatus, strerror(cleanupStatus));
        if (0 == status)
        {
            status = cleanupStatus;
        }
    }
    free(distro);
    free(path);
    OsConfigLogInfo(NULL, "Attempted: %u; accepted: %u; planned: 100", attempted, accepted);
    return (0 == status) ? 0 : 1;
}
