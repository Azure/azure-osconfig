// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

void LogPerfClockTelemetry(PerfClock* clock, const char* targetName, const char* componentName, const char* objectName, int objectResult, const char* reason, OsConfigLogHandle log)
{
    TelemetryLevel telemetryLevel = GetTelemetryLevel();
    long microseconds = -1;

    if (NULL == clock)
    {
        OsConfigLogError(log, "LogPerfClockTelemetry called with an invalid clock argument");
        return;
    }

    microseconds = GetPerfClockTime(clock, log);

    if (OptionalTelemetry > telemetryLevel)
    {
        if (SESSIONS_TELEMETRY_MARKER == objectResult)
        {
            OsConfigLogTelemetry(log, RequiredTelemetry, ",\"TargetName\":\"%s\",\"BaselineName\":\"%s\",\"Mode\":\"%s\",\"Seconds\":\"%.02f\"",
                targetName, componentName, objectName, microseconds / 1000000.0);
        }
        else
        {
            OsConfigLogTelemetry(log, RequiredTelemetry, ",\"TargetName\":\"%s\",\"ComponentName\":\"%s\",\"ObjectName\":\"%s\",\"ObjectResult\":\"%s (%d)\",\"Microseconds\":\"%ld\"",
                targetName, componentName, objectName, strerror(objectResult), objectResult, microseconds);
        }
    }
    else if ((OptionalTelemetry <= telemetryLevel) && (NULL != reason))
    {
        OsConfigLogTelemetry(log, OptionalTelemetry, ",\"TargetName\":\"%s\",\"ComponentName\":\"%s\",\"ObjectName\":\"%s\",\"ObjectResult\":\"%s (%d)\",\"Reason\":%s,\"Microseconds\":\"%ld\"",
            targetName, componentName, objectName, strerror(objectResult), objectResult, reason, microseconds);
    }
}
