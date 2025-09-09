// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <telemetry.h>

// C interface implementations - all stub implementations

OSConfigTelemetryHandle OSConfigTelemetryOpen(void)
{
    // Stub implementation - return NULL to indicate failure
    return ((void*)0);
}

int OSConfigTelemetryClose(OSConfigTelemetryHandle* handle)
{
    // Stub implementation - return 0 to indicate success
    (void)handle;         // Suppress unused parameter warning
    return 0;
}

int OSConfigTelemetryLogEvent(OSConfigTelemetryHandle handle, const char* eventName,
                          const char** keyValuePairs, int pairCount)
{
    // Stub implementation - return 0 to indicate success
    (void)handle;         // Suppress unused parameter warning
    (void)eventName;      // Suppress unused parameter warning
    (void)keyValuePairs;  // Suppress unused parameter warning
    (void)pairCount;      // Suppress unused parameter warning
    return 0;
}

int OSConfigTelemetrySetBinaryDirectory(OSConfigTelemetryHandle handle, const char* directory)
{
    // Stub implementation - return 0 to indicate success
    (void)handle;         // Suppress unused parameter warning
    (void)directory;      // Suppress unused parameter warning
    return 0;
}

const char* OSConfigTelemetryGetFilepath(OSConfigTelemetryHandle handle)
{
    // Stub implementation - not implemented
    (void)handle;         // Suppress unused parameter warning
    return ((void*)0);
}

const char* OSConfigTelemetryGetModuleDirectory()
{
    // Stub implementation - not implemented
    return ((void*)0);
}
