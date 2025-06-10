// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MPI_SERVER_H
#define MPI_SERVER_H

#define MPI_CALL_MESSAGE_LENGTH 256

#define MPI_OPEN_URI "MpiOpen"
#define MPI_CLOSE_URI "MpiClose"
#define MPI_SET_URI "MpiSet"
#define MPI_GET_URI "MpiGet"
#define MPI_SET_DESIRED_URI "MpiSetDesired"
#define MPI_GET_REPORTED_URI "MpiGetReported"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum HTTP_STATUS
{
    HTTP_OK = 200,
    HTTP_BAD_REQUEST = 400,
    HTTP_NOT_FOUND = 404,
    HTTP_INTERNAL_SERVER_ERROR = 500
} HTTP_STATUS;

typedef MPI_HANDLE(*MpiOpenCall)(const char*, const unsigned int);
typedef void(*MpiCloseCall)(MPI_HANDLE);
typedef int(*MpiSetCall)(MPI_HANDLE, const char*, const char*, MPI_JSON_STRING, const int);
typedef int(*MpiGetCall)(MPI_HANDLE, const char*, const char*, MPI_JSON_STRING*, int*);
typedef int(*MpiSetDesiredCall)(MPI_HANDLE, const MPI_JSON_STRING, const int);
typedef int(*MpiGetReportedCall)(MPI_HANDLE, MPI_JSON_STRING*, int*);

typedef struct MPI_CALLS
{
    MpiOpenCall mpiOpen;
    MpiCloseCall mpiClose;
    MpiSetCall mpiSet;
    MpiGetCall mpiGet;
    MpiSetDesiredCall mpiSetDesired;
    MpiGetReportedCall mpiGetReported;
} MPI_CALLS;

HTTP_STATUS HandleMpiCall(const char* uri, const char* requestBody, char** response, int* responseSize, MPI_CALLS handlers);

#ifdef __cplusplus
}
#endif

#endif // MPI_SERVER_H
