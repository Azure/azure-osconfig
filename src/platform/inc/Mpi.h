// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MPI_H
#define MPI_H

typedef void* MPI_HANDLE;

// Plus any error codes from errno.h
#define MPI_OK 0

// Not null terminated, UTF-8, JSON formatted string
typedef char* MPI_JSON_STRING;

#ifdef __cplusplus
extern "C"
{
#endif

MPI_HANDLE MpiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes);
int MpiSet(
    MPI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MPI_JSON_STRING payload,
    const int payloadSizeBytes);
int MpiGet(
    MPI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MPI_JSON_STRING* payload,
    int* payloadSizeBytes);
int MpiSetDesired(
    MPI_HANDLE clientSession,
    const MPI_JSON_STRING payload,
    const int payloadSizeBytes);
int MpiGetReported(
    MPI_HANDLE clientSession,
    MPI_JSON_STRING* payload,
    int* payloadSizeBytes);
void MpiClose(MPI_HANDLE clientSession);

void MpiFree(MPI_JSON_STRING payload);

void MpiInitialize(void);
void MpiDoWork(void);
void MpiShutdown(void);

#ifdef __cplusplus
}
#endif

#endif // MPI_H
