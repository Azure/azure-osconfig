// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <telemetry.h>

// C interface implementations - all stub implementations

TelemetryHandle Telemetry_Open(void)
{
    // Stub implementation - return NULL to indicate failure
    return ((void*)0);
}

int Telemetry_Close(TelemetryHandle* handle)
{
    // Stub implementation - return 0 to indicate success
    (void)handle;         // Suppress unused parameter warning
    return 0;
}

int Telemetry_LogEvent(TelemetryHandle handle, const char* eventName,
                          const char** keyValuePairs, int pairCount)
{
    // Stub implementation - return 0 to indicate success
    (void)handle;         // Suppress unused parameter warning
    (void)eventName;      // Suppress unused parameter warning
    (void)keyValuePairs;  // Suppress unused parameter warning
    (void)pairCount;      // Suppress unused parameter warning
    return 0;
}

int Telemetry_SetBinaryDirectory(TelemetryHandle handle, const char* directory)
{
    // Stub implementation - return 0 to indicate success
    (void)handle;         // Suppress unused parameter warning
    (void)directory;      // Suppress unused parameter warning
    return 0;
}

const char* Telemetry_GetFilepath(TelemetryHandle handle)
{
    // Stub implementation - not implemented
    (void)handle;         // Suppress unused parameter warning
    return ((void*)0);
}
