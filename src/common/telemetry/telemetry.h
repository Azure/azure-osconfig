#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdio.h>
#include <stdlib.h>

#define API_KEY "999999999999999999999999999999999999999999999999999999999999999999999999"

void InitializeTelemetry();
void TelemetryEventWrite_CompletedBaseline(const char *targetName, const char *baselineName, const char *mode, double seconds);
void ShutdownTelemetry();

#endif // TELEMETRY_H
