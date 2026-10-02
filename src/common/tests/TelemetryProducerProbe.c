// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Telemetry.h>
#include <stdlib.h>
#include <string.h>

// Only invoked by the loopback fixture, beside its built worker. Refuse inherited
// real credentials or routes; this is not an alternate live-test executable.
int main(void)
{
    const char* token = getenv("OsConfigTelemetryApiKey");
    const char* proxy = getenv("https_proxy");
    if (!token || strcmp(token, "fixture-token") || !proxy ||
        strncmp(proxy, "http://127.0.0.1:", strlen("http://127.0.0.1:"))) return 2;
    if (setenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, "worker-fixture", 1)) return 3;
    SetConsoleLoggingEnabled(false);
    OsConfigLogHandle log = OpenLog("/var/log/osconfig_telemetry.log", "/var/log/osconfig_telemetry.bak");
    if (!log || !GetLogFile(log)) { if (log) CloseLog(&log); return 4; }
    int status = TelemetryInitialize(log);
    if (!status)
    {
        OSConfigTelemetryCrashDetected("worker-fixture crash\"\\\n");
        OSConfigTelemetryBaselineRun("worker-fixture", "Audit", 1.25);
    }
    TelemetryCleanup(log);
    CloseLog(&log);
    return status ? 1 : 0;
}
