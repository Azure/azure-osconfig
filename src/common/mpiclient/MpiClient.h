// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MPICLIENT_H
#define MPICLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

MPI_HANDLE CallMpiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes, OsConfigLogHandle log);
void CallMpiClose(MPI_HANDLE clientSession, OsConfigLogHandle log);
int CallMpiSet(const char* componentName, const char* propertyName, const MPI_JSON_STRING payload, const int payloadSizeBytes, OsConfigLogHandle log);
int CallMpiGet(const char* componentName, const char* propertyName, MPI_JSON_STRING* payload, int* payloadSizeBytes, OsConfigLogHandle log);
int CallMpiSetDesired(const MPI_JSON_STRING payload, const int payloadSizeBytes, OsConfigLogHandle log);
int CallMpiGetReported(MPI_JSON_STRING* payload, int* payloadSizeBytes, OsConfigLogHandle log);
void CallMpiFree(MPI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // MPIPCLIENT_H
