// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MPICLIENT_H
#define MPICLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

MPI_HANDLE CallMpiOpen(void* log, const char* clientName, const unsigned int maxPayloadSizeBytes);
void CallMpiClose(void* log, MPI_HANDLE clientSession);
int CallMpiSet(void* log, const char* componentName, const char* propertyName, const MPI_JSON_STRING payload, const int payloadSizeBytes);
int CallMpiGet(void* log, const char* componentName, const char* propertyName, MPI_JSON_STRING* payload, int* payloadSizeBytes);
int CallMpiSetDesired(void* log, const MPI_JSON_STRING payload, const int payloadSizeBytes);
int CallMpiGetReported(void* log, MPI_JSON_STRING* payload, int* payloadSizeBytes);
void CallMpiFree(MPI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // MPIPCLIENT_H
