// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MPICLIENT_H
#define MPICLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

MPI_HANDLE CallMpiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
void CallMpiClose(MPI_HANDLE clientSession, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CallMpiSet(const char* componentName, const char* propertyName, const MPI_JSON_STRING payload, const int payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CallMpiGet(const char* componentName, const char* propertyName, MPI_JSON_STRING* payload, int* payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CallMpiSetDesired(const MPI_JSON_STRING payload, const int payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CallMpiGetReported(MPI_JSON_STRING* payload, int* payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
void CallMpiFree(MPI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // MPIPCLIENT_H
