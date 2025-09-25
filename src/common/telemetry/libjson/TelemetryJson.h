// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRYJSON_H
#define TELEMETRYJSON_H

#define TELEMETRY_BINARY_NAME "OSConfigTelemetry"

#include <Telemetry.h>

// Internal structure to hold logger state
struct TelemetryLogger {
    FILE* logFile;
    char* filename;
    char* binaryDirectory;
    int isOpen;
};

#endif // TELEMETRYJSON_H
