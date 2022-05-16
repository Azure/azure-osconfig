// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PlatformCommon.h>
#include <MpiServer.h>

#define MAX_CONTENTLENGTH_LENGTH 16
#define MAX_REASONSTRING_LENGTH 32
#define MAX_STATUS_CODE_LENGTH 3

#define MPI_OPEN_URI "MpiOpen"
#define MPI_CLOSE_URI "MpiClose"
#define MPI_SET_URI "MpiSet"
#define MPI_GET_URI "MpiGet"
#define MPI_SET_DESIRED_URI "MpiSetDesired"
#define MPI_GET_REPORTED_URI "MpiGetReported"

static const char* g_socketPrefix = "/run/osconfig";
static const char* g_mpiSocket = "/run/osconfig/mpid.sock";

static const char* g_clientName = "ClientName";
static const char* g_maxPayloadSizeBytes = "MaxPayloadSizeBytes";
static const char* g_clientSession = "ClientSession";
static const char* g_componentName = "ComponentName";
static const char* g_objectName = "ObjectName";
static const char* g_payload = "Payload";

static int g_socketfd = -1;
static struct sockaddr_un g_socketaddr = {0};
static socklen_t g_socketlen = 0;

static pthread_t g_worker = 0;
static bool g_serverActive = false;

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

static const char* JsonGetString(JSON_Object* object, const char* member)
{
    const char* value = NULL;
    JSON_Value* jsonValue = NULL;

    if ((NULL != object) && (NULL != member))
    {
        if (NULL != (jsonValue = json_object_get_value(object, member)))
        {
            value = json_value_get_string(jsonValue);
        }
    }

    return value;
}

static int JsonGetNumber(JSON_Object* object, const char* member)
{
    int value = -1;
    JSON_Value* jsonValue = NULL;

    if ((NULL != object) && (NULL != member))
    {
        if (NULL != (jsonValue = json_object_get_value(object, member)))
        {
            value = (int)json_value_get_number(jsonValue);
        }
    }

    return value;
}

static const char* JsonSerializeToString(JSON_Object* object, const char* member)
{
    const char* value = NULL;
    JSON_Value* jsonValue = NULL;

    if ((NULL != object) && (NULL != member))
    {
        if (NULL != (jsonValue = json_object_get_value(object, member)))
        {
            value = json_serialize_to_string(jsonValue);
        }
    }

    return value;
}

static MPI_HANDLE CallMpiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MPI_HANDLE handle = NULL;

    if (NULL == (handle = MpiOpen(clientName, maxPayloadSizeBytes)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiOpen failed to create a session for client '%s'", clientName);
    }

    return handle;
}

static void CallMpiClose(MPI_HANDLE handle)
{
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetPlatformLog(), "Received MpiClose request, session %p ('%s')", handle, (char*)handle);
    }

    MpiClose((MPI_HANDLE)handle);
}

static int CallMpiSet(MPI_HANDLE handle, const char* componentName, const char* objectName, MPI_JSON_STRING payload, const int payloadSize)
{
    int status = MPI_OK;

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetPlatformLog(), "Received MpiSet(%s, %s) request, session %p ('%s')", componentName, objectName, handle, (char*)handle);
    }

    if (MPI_OK != (status = MpiSet((MPI_HANDLE)handle, componentName, objectName, payload, payloadSize)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet(%s, %s) failed, session %p ('%s'): %d", componentName, objectName, handle, (char*)handle, status);
    }

    return status;
}

static int CallMpiGet(MPI_HANDLE handle, const char* componentName, const char* objectName, MPI_JSON_STRING* payload, int* payloadSize)
{
    int status = MPI_OK;

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetPlatformLog(), "Received MpiGet(%s, %s) request, session %p ('%s')", componentName, objectName, handle, (char*)handle);
    }

    if (MPI_OK != (status = MpiGet((MPI_HANDLE)handle, componentName, objectName, payload, payloadSize)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiGet(%s, %s) failed, session %p ('%s'): %d", componentName, objectName, handle, (char*)handle, status);
    }

    return status;
}

static int CallMpiSetDesired(MPI_HANDLE handle, const MPI_JSON_STRING payload, const int payloadSize)
{
    int status = MPI_OK;

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetPlatformLog(), "Received MpiSetDesired request, session %p ('%s'), payload '%s'", handle, (char*)handle, payload);
    }

    if (MPI_OK != (status = MpiSetDesired((MPI_HANDLE)handle, payload, payloadSize)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiSetDesired failed, session %p ('%s'): %d", handle, (char*)handle, status);
    }

    return status;
}

static int CallMpiGetReported(MPI_HANDLE handle, MPI_JSON_STRING* payload, int* payloadSize)
{
    int status = MPI_OK;

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetPlatformLog(), "Received MpiGetReported request, session %p ('%s')", handle, (char*)handle);
    }

    if (MPI_OK != (status = MpiGetReported((MPI_HANDLE)handle, payload, payloadSize)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiGetReported failed, session %p ('%s'): %d", handle, (char*)handle, status);
    }

    return status;
}

HTTP_STATUS HandleMpiCall(const char* uri, const char* requestBody, char** response, int* responseSize, MPI_CALLS handlers)
{
    HTTP_STATUS status = HTTP_OK;
    JSON_Value* rootValue = NULL;
    JSON_Object* requestObject = NULL;
    int mpiStatus = MPI_OK;
    char* uuid = NULL;
    const char* client = NULL;
    int maxPayloadSizeBytes = 0;
    const char* component = NULL;
    const char* object = NULL;
    const char* payload = NULL;
    int estimatedSize = 0;
    const char* responseFormat = "\"%s\"";

    if (NULL == uri)
    {
        OsConfigLogError(GetPlatformLog(), "MpiRequest: called with invalid null URI");
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == requestBody)
    {
        OsConfigLogError(GetPlatformLog(), "MpiRequest(%s): called with invalid null request body", uri);
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == response)
    {
        OsConfigLogError(GetPlatformLog(), "MpiRequest(%s): called with invalid null response", uri);
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == responseSize)
    {
        OsConfigLogError(GetPlatformLog(), "MpiRequest(%s): called with invalid null response size", uri);
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == (rootValue = json_parse_string(requestBody)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiRequest(%s): failed to parse request body", uri);
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == (requestObject = json_value_get_object(rootValue)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiRequest(%s): failed to get object from request body", uri);
        status = HTTP_BAD_REQUEST;
    }
    else
    {
        if (0 == strcmp(uri, MPI_OPEN_URI))
        {
            if (NULL == (client = JsonGetString(requestObject, g_clientName)))
            {
                OsConfigLogError(GetPlatformLog(), "%s: failed to parse '%s' from request body", uri, g_clientName);
                status = HTTP_BAD_REQUEST;
            }
            else if (0 > (maxPayloadSizeBytes = JsonGetNumber(requestObject, g_maxPayloadSizeBytes)))
            {
                OsConfigLogError(GetPlatformLog(), "%s: failed to parse '%s' from request body", uri, g_maxPayloadSizeBytes);
                status = HTTP_BAD_REQUEST;
            }
            else
            {
                uuid = (char*)handlers.mpiOpen(client, maxPayloadSizeBytes);
                if (uuid)
                {
                    estimatedSize = strlen(responseFormat) + strlen(uuid) + 1;

                    if (NULL != (*response = (char*)malloc(estimatedSize)))
                    {
                        snprintf(*response, estimatedSize, responseFormat, uuid);
                        *responseSize = strlen(*response);
                    }
                    else
                    {
                        OsConfigLogError(GetPlatformLog(), "%s: failed to allocate memory for response", uri);
                        status = HTTP_INTERNAL_SERVER_ERROR;
                        *responseSize = 0;
                    }

                    FREE_MEMORY(uuid);
                }
                else
                {
                    OsConfigLogError(GetPlatformLog(), "%s: failed to open client '%s'", client, uri);
                    status = HTTP_INTERNAL_SERVER_ERROR;
                    *responseSize = 0;
                }
            }
        }
        else if ((0 == strcmp(uri, MPI_CLOSE_URI)) || (0 == strcmp(uri, MPI_SET_URI)) || (0 == strcmp(uri, MPI_GET_URI)) || (0 == strcmp(uri, MPI_SET_DESIRED_URI)) || (0 == strcmp(uri, MPI_GET_REPORTED_URI)))
        {
            if (NULL != (client = JsonGetString(requestObject, g_clientSession)))
            {
                if (0 == strcmp(uri, MPI_CLOSE_URI))
                {
                    handlers.mpiClose((MPI_HANDLE)client);
                    status = HTTP_OK;
                }
                else if ((0 == strcmp(uri, MPI_SET_URI)) || (0 == strcmp(uri, MPI_GET_URI)))
                {
                    if (NULL == (client = JsonGetString(requestObject, g_clientSession)))
                    {
                        OsConfigLogError(GetPlatformLog(), "%s: failed to parse '%s' from request body", uri, g_clientSession);
                        status = HTTP_BAD_REQUEST;
                    }
                    else if (NULL == (component = JsonGetString(requestObject, g_componentName)))
                    {
                        OsConfigLogError(GetPlatformLog(), "%s: failed to parse '%s' from request body", uri, g_componentName);
                        status = HTTP_BAD_REQUEST;
                    }
                    else if (NULL == (object = JsonGetString(requestObject, g_objectName)))
                    {
                        OsConfigLogError(GetPlatformLog(), "%s: failed to parse '%s' from request body", uri, g_objectName);
                        status = HTTP_BAD_REQUEST;
                    }
                    else
                    {
                        if (0 == strcmp(uri, MPI_SET_URI))
                        {
                            if (NULL != (payload = JsonSerializeToString(requestObject, g_payload)))
                            {
                                if (MPI_OK != (mpiStatus = handlers.mpiSet((MPI_HANDLE)client, component, object, (MPI_JSON_STRING)payload, strlen(payload))))
                                {
                                    status = HTTP_INTERNAL_SERVER_ERROR;
                                    if (IsFullLoggingEnabled())
                                    {
                                        OsConfigLogError(GetPlatformLog(), "%s(%s, %s): failed to for client '%s'", uri, component, object, client);
                                    }
                                }
                            }
                            else
                            {
                                OsConfigLogError(GetPlatformLog(), "%s: failed to serialize payload from request body", uri);
                                status = HTTP_BAD_REQUEST;
                            }
                        }
                        else if (MPI_OK != (mpiStatus = handlers.mpiGet((MPI_HANDLE)client, component, object, response, responseSize)))
                        {
                            status = HTTP_INTERNAL_SERVER_ERROR;
                            if (IsFullLoggingEnabled())
                            {
                                OsConfigLogError(GetPlatformLog(), "%s(%s, %s): failed to for client '%s'", uri, component, object, client);
                            }
                        }
                    }
                }
                else if (0 == strcmp(uri, MPI_SET_DESIRED_URI))
                {
                    if (NULL == (payload = JsonSerializeToString(requestObject, g_payload)))
                    {
                        OsConfigLogError(GetPlatformLog(), "%s: failed to parse '%s' from request body", uri, g_payload);
                        status = HTTP_BAD_REQUEST;
                    }
                    else if (MPI_OK != (mpiStatus = handlers.mpiSetDesired((MPI_HANDLE)client, (MPI_JSON_STRING)payload, strlen(payload))))
                    {
                        OsConfigLogError(GetPlatformLog(), "%s: failed for client %s, status %d", uri, client, mpiStatus);
                        status = HTTP_INTERNAL_SERVER_ERROR;
                    }
                }
                else if (0 == strcmp(uri, MPI_GET_REPORTED_URI))
                {
                    if (MPI_OK != (mpiStatus = handlers.mpiGetReported((MPI_HANDLE)client, response, responseSize)))
                    {
                        OsConfigLogError(GetPlatformLog(), "%s: failed for client %s, status %d", uri, client, mpiStatus);
                        status = HTTP_INTERNAL_SERVER_ERROR;
                    }
                }
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "%s: failed to parse '%s' from request body", uri, g_clientSession);
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "%s: invalid request URI", uri);
            status = HTTP_NOT_FOUND;
        }
    }

    json_value_free(rootValue);

    return status;
}

static char* HttpReasonAsString(HTTP_STATUS statusCode)
{
    char* reason = NULL;

    if (NULL != (reason = (char*)malloc(MAX_REASONSTRING_LENGTH)))
    {
        switch (statusCode)
        {
        case HTTP_OK:
            strcpy(reason, "OK");
            break;
        case HTTP_BAD_REQUEST:
            strcpy(reason, "Bad Request");
            break;
        case HTTP_NOT_FOUND:
            strcpy(reason, "Not Found");
            break;
        case HTTP_INTERNAL_SERVER_ERROR:
            strcpy(reason, "Internal Server Error");
            break;
        default:
            strcpy(reason, "Unknown");
        }
    }

    return reason;
}

static void HandleConnection(int socketHandle, MPI_CALLS mpiCalls)
{
    const char* responseFormat = "HTTP/1.1 %d %s\r\nServer: OSConfig\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%.*s";

    char* uri = NULL;
    int contentLength = 0;
    char* requestBody = NULL;
    HTTP_STATUS status = HTTP_OK;
    char* httpReason = NULL;
    char* responseBody = NULL;
    int responseSize = 0;
    char* buffer = NULL;
    int estimatedSize = 0;
    int actualSize = 0;
    ssize_t bytes = 0;

    if (NULL == (uri = ReadUriFromSocket(socketHandle, GetPlatformLog())))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to read request URI %d", socketHandle);
        status = HTTP_BAD_REQUEST;
    }

    if ((contentLength = ReadHttpContentLengthFromSocket(socketHandle, GetPlatformLog())))
    {
        if (NULL == (requestBody = (char*)malloc(contentLength + 1)))
        {
            OsConfigLogError(GetPlatformLog(), "%s: failed to allocate memory for HTTP body, Content-Length %d", uri, contentLength);
            status = HTTP_BAD_REQUEST;
        }

        memset(requestBody, 0, contentLength + 1);

        if (contentLength != (int)(bytes = read(socketHandle, requestBody, contentLength)))
        {
            OsConfigLogError(GetPlatformLog(), "%s: failed to read complete HTTP body, Content-Length %d, bytes read %d", uri, contentLength, (int)bytes);
            status = HTTP_BAD_REQUEST;
        }
    }

    if (status == HTTP_OK)
    {
        status = HandleMpiCall(uri, requestBody, &responseBody, &responseSize, mpiCalls);
    }

    FREE_MEMORY(requestBody);

    httpReason = HttpReasonAsString(status);
    estimatedSize = strlen(responseFormat) + MAX_STATUS_CODE_LENGTH + strlen(httpReason) + MAX_CONTENTLENGTH_LENGTH + responseSize + 1;

    if (NULL == (buffer = (char*)malloc(estimatedSize)))
    {
        OsConfigLogError(GetPlatformLog(), "%s: failed to allocate memory for HTTP response, %d bytes of %d", uri, 0, estimatedSize);
        return;
    }

    memset(buffer, 0, estimatedSize);

    snprintf(buffer, estimatedSize, responseFormat, (int)status, httpReason, responseSize, responseSize, (responseBody ? responseBody : ""));
    actualSize = (int)strlen(buffer);

    FREE_MEMORY(responseBody);

    bytes = write(socketHandle, buffer, actualSize);

    if (bytes != actualSize)
    {
        OsConfigLogError(GetPlatformLog(), "%s: failed to write complete HTTP response, %d bytes of %d", uri, (int)bytes, actualSize);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetPlatformLog(), "%s: HTTP response sent (%d bytes)\n%s", uri, (int)bytes, buffer);
    }

    FREE_MEMORY(httpReason);
    FREE_MEMORY(buffer);
    FREE_MEMORY(uri);
}

static void* Worker(void* arg)
{
    int socketHandle = -1;

    MPI_CALLS mpiCalls = {
        CallMpiOpen,
        CallMpiClose,
        CallMpiSet,
        CallMpiGet,
        CallMpiSetDesired,
        CallMpiGetReported
    };

    UNUSED(arg);

    while (g_serverActive)
    {
        if (0 <= (socketHandle = accept(g_socketfd, (struct sockaddr*)&g_socketaddr, &g_socketlen)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(GetPlatformLog(), "Accepted connection: path %s, handle '%d'", g_socketaddr.sun_path, socketHandle);
            }

            HandleConnection(socketHandle, mpiCalls);

            if (0 != close(socketHandle))
            {
                OsConfigLogError(GetPlatformLog(), "Failed to close socket: path %s, handle '%d'", g_socketaddr.sun_path, socketHandle);
            }

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(GetPlatformLog(), "Closed connection: path %s, handle '%d'", g_socketaddr.sun_path, socketHandle);
            }
        }
    }

    return NULL;
}

void MpiApiInitialize(void)
{
    struct stat st;
    if (stat(g_socketPrefix, &st) == -1)
    {
        // S_IRUSR (0x00400): Read permission, owner
        // S_IWUSR (0x00200): Write permission, owner
        // S_IXUSR (0x00100): Execute/search permission, owner
        mkdir(g_socketPrefix, S_IRUSR | S_IWUSR | S_IXUSR);
    }

    if (0 <= (g_socketfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)))
    {
        memset(&g_socketaddr, 0, sizeof(g_socketaddr));
        g_socketaddr.sun_family = AF_UNIX;

        strncpy(g_socketaddr.sun_path, g_mpiSocket, sizeof(g_socketaddr.sun_path) - 1);
        g_socketlen = sizeof(g_socketaddr);

        unlink(g_mpiSocket);

        if (bind(g_socketfd, (struct sockaddr*)&g_socketaddr, g_socketlen) == 0)
        {
            RestrictFileAccessToCurrentAccountOnly(g_mpiSocket);

            if (listen(g_socketfd, 5) == 0)
            {
                OsConfigLogInfo(GetPlatformLog(), "Listening on socket '%s'", g_mpiSocket);

                g_serverActive = true;
                g_worker = pthread_create(&g_worker, NULL, Worker, NULL);;
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "Failed to listen on socket '%s'", g_mpiSocket);
            }
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "Failed to bind socket '%s'", g_mpiSocket);
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "Failed to create socket '%s'", g_mpiSocket);
    }
}

void MpiApiShutdown(void)
{
    g_serverActive = false;
    pthread_join(g_worker, NULL);

    close(g_socketfd);
    unlink(g_mpiSocket);
}