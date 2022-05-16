// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PlatformCommon.h>
#include <MpiServer.h>

#define MAX_CONTENTLENGTH_LENGTH 16
#define MAX_REASONSTRING_LENGTH 32
#define MAX_STATUS_CODE_LENGTH 3

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

typedef MPI_HANDLE(*MPI_OPEN)(const char*, const unsigned int);
typedef void(*MPI_CLOSE)(MPI_HANDLE);
typedef int(*MPI_SET)(MPI_HANDLE, const char*, const char*, MPI_JSON_STRING, const int);
typedef int(*MPI_GET)(MPI_HANDLE, const char*, const char*, MPI_JSON_STRING*, int*);
typedef int(*MPI_SET_DESIRED)(MPI_HANDLE, const MPI_JSON_STRING, const int);
typedef int(*MPI_GET_REPORTED)(MPI_HANDLE, MPI_JSON_STRING*, int*);

typedef struct
{
    MPI_OPEN mpiOpen;
    MPI_CLOSE mpiClose;
    MPI_SET mpiSet;
    MPI_GET mpiGet;
    MPI_SET_DESIRED mpiSetDesired;
    MPI_GET_REPORTED mpiGetReported;
} MPI_HANDLERS;

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

    if (NULL != (handle = MpiOpen(clientName, maxPayloadSizeBytes)))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(GetPlatformLog(), "Created session for client '%s': %p ('%s')", clientName, handle, (char*)handle);
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "MpiOpen failed to create a session for client '%s'", clientName);
    }

    return handle;
}

static HTTP_STATUS CallMpiClose(char** response, int* responseSize, void* mpiContext)
{
    HTTP_STATUS status = HTTP_OK;
    MPI_CLOSE_CONTEXT* context = (MPI_CLOSE_CONTEXT*)mpiContext;

    UNUSED(response);
    UNUSED(responseSize);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetPlatformLog(), "Received MpiClose request, session '%s'", context->clientSession);
    }

    MpiClose((MPI_HANDLE)context->clientSession);

    return status;
}

static HTTP_STATUS CallMpiSet(char** response, int* responseSize, void* mpiContext)
{
    HTTP_STATUS status = HTTP_OK;
    MPI_SET_CONTEXT* context = (MPI_SET_CONTEXT*)mpiContext;

    UNUSED(response);
    UNUSED(responseSize);

    if (MPI_OK != MpiSet((MPI_HANDLE)context->clientSession, context->componentName, context->objectName, (MPI_JSON_STRING)context->payload, strlen(context->payload)))
    {
        status = HTTP_BAD_REQUEST;
    }

    return status;
}

static HTTP_STATUS CallMpiGet(JSON_Object* request, char** response, int* responseSize, MPI_GET mpiGet)
{
    HTTP_STATUS status = HTTP_OK;
    const char* clientSession = NULL;
    const char* component = NULL;
    const char* object = NULL;

    if (NULL == request)
    {
        OsConfigLogError(GetPlatformLog(), "Invalid null request");
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == (clientSession = JsonGetString(request, g_clientSession)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to parse client session from request body");
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == (component = JsonGetString(request, g_componentName)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to parse component from request body");
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == (object = JsonGetString(request, g_objectName)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to parse object from request body");
        status = HTTP_BAD_REQUEST;
    }
    else
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(GetPlatformLog(), "Received MpiGet(%s, %s) request, session '%s'", component, object, clientSession);
        }

        if (MPI_OK != mpiGet((MPI_HANDLE)clientSession, component, object, (MPI_JSON_STRING*)response, responseSize))
        {
            status = HTTP_BAD_REQUEST;
        }
    }

    return status;
}

static HTTP_STATUS CallMpiSetDesired(JSON_Object* request, char** response, int* responseSize, MPI_SET_DESIRED mpiSetDesired)
{
    HTTP_STATUS status = HTTP_OK;
    const char* clientSession = NULL;
    const char* payload = NULL;

    UNUSED(response);
    UNUSED(responseSize);

    if (NULL == request)
    {
        OsConfigLogError(GetPlatformLog(), "Invalid null request");
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == (clientSession = JsonGetString(request, g_clientSession)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to parse client session from request body");
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == (payload = JsonSerializeToString(request, g_payload)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to parse payload from request body");
        status = HTTP_BAD_REQUEST;
    }
    else
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(GetPlatformLog(), "Received MpiSetDesired request, session '%s'", clientSession);
        }

        if (MPI_OK != mpiSetDesired((MPI_HANDLE)clientSession, (MPI_JSON_STRING)payload, strlen(payload)))
        {
            status = HTTP_BAD_REQUEST;
        }
    }

    return status;
}

static HTTP_STATUS CallMpiGetReported(JSON_Object* request, char** response, int* responseSize, MPI_GET_REPORTED mpiGetReported)
{
    HTTP_STATUS status = HTTP_OK;
    const char* clientSession = NULL;

    if (NULL == request)
    {
        OsConfigLogError(GetPlatformLog(), "Invalid null request");
        status = HTTP_BAD_REQUEST;
    }
    else if (NULL == (clientSession = JsonGetString(request, g_clientSession)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to parse client session from request body");
        status = HTTP_BAD_REQUEST;
    }
    else
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(GetPlatformLog(), "Received MpiGetReported request, session '%s'", clientSession);
        }

        if (MPI_OK != mpiGetReported((MPI_HANDLE)clientSession, (MPI_JSON_STRING*)response, responseSize))
        {
            status = HTTP_BAD_REQUEST;
        }
    }

    return status;
}

// recieve structure to all the mpi handlers
// make all the handlers as small as possible to only make calls in the the MPI C interface
// create a typedef func ptr for the handler type that recieves the request params and response/ptrs

HTTP_STATUS HandleMpiCall(const char* uri, const char* requestBody, char** response, int* responseSize, MPI_HANDLERS handlers)
{
    HTTP_STATUS status = HTTP_OK;
    JSON_Value* rootValue = NULL;
    JSON_Object* requestObject = NULL;

    char* uuid = NULL;
    char* clientName = NULL;
    int maxPayloadSizeBytes = 0;
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
        OsConfigLogError(GetPlatformLog(), "MpiRequest(%s): failed to parse request object", uri);
        status = HTTP_BAD_REQUEST;
    }
    else
    {
        if (0 == strcmp(uri, "MpiOpen"))
        {
            if (NULL == (clientName = JsonGetString(requestObject, g_clientName)))
            {
                OsConfigLogError(GetPlatformLog(), "Failed to parse client name from request body");
                status = HTTP_BAD_REQUEST;
            }
            else if (0 > (maxPayloadSizeBytes = JsonGetNumber(requestObject, g_maxPayloadSizeBytes)))
            {
                OsConfigLogError(GetPlatformLog(), "Failed to parse max payload size from request body");
                status = HTTP_BAD_REQUEST;
            }
            else
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(GetPlatformLog(), "Received MpiOpen request for client '%s' with max payload size %d", clientName, maxPayloadSizeBytes);
                }

                uuid = (char*)handlers.mpiOpen(clientName, maxPayloadSizeBytes);

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
                        OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for response");
                        status = HTTP_INTERNAL_SERVER_ERROR;
                        *responseSize = 0;
                    }

                    FREE_MEMORY(uuid);
                }
                else
                {
                    OsConfigLogError(GetPlatformLog(), "Failed to open client '%s'", clientName);
                    status = HTTP_INTERNAL_SERVER_ERROR;
                    *responseSize = 0;
                }

            }
        }
        else if (0 == strcmp(uri, "MpiClose"))
        {
            // if (NULL == (clientSession = JsonGetString(requestObject, g_clientSession)))
            // {
            //     OsConfigLogError(GetPlatformLog(), "Failed to parse client session from request body");
            //     status = HTTP_BAD_REQUEST;
            // }
            // else
            // {
            //     if (IsFullLoggingEnabled())
            //     {
            //         OsConfigLogInfo(GetPlatformLog(), "Received MpiClose request, session '%s'", clientSession);
            //     }

            //     status = handlers.close(response, responseSize, &mpiCloseContext);
            // }
        }
        else if (0 == strcmp(uri, "MpiSet"))
        {
            // if (NULL == (clientSession = JsonGetString(requestObject, g_clientSession)))
            // {
            //     OsConfigLogError(GetPlatformLog(), "Failed to parse client session from request body");
            //     status = HTTP_BAD_REQUEST;
            // }
            // else if (NULL == (componentName = JsonGetString(requestObject, g_componentName)))
            // {
            //     OsConfigLogError(GetPlatformLog(), "Failed to parse component from request body");
            //     status = HTTP_BAD_REQUEST;
            // }
            // else if (NULL == (objectName = JsonGetString(requestObject, g_objectName)))
            // {
            //     OsConfigLogError(GetPlatformLog(), "Failed to parse object from request body");
            //     status = HTTP_BAD_REQUEST;
            // }
            // else if (NULL == (mpiSetContext.payload = (MPI_JSON_STRING)JsonSerializeToString(requestObject, g_payload)))
            // {
            //     OsConfigLogError(GetPlatformLog(), "Failed to parse payload from request body");
            //     status = HTTP_BAD_REQUEST;
            // }
            // else
            // {
            //     if (IsFullLoggingEnabled())
            //     {
            //         OsConfigLogInfo(GetPlatformLog(), "Received MpiSet(%s, %s) request, session '%s'", component, object, clientSession);
            //     }

            //     status = handlers.set(response, responseSize, &mpiSetContext);
            // }
        }
        else if (0 == strcmp(uri, "MpiGet"))
        {
            // ...
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

static void HandleConnection(int socketHandle)
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

    MPI_HANDLERS handlers = {
        CallMpiOpen,
        CallMpiClose,
        CallMpiSet,
        CallMpiGet,
        CallMpiSetDesired,
        CallMpiGetReported
    };

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
        status = HandleMpiCall(uri, requestBody, &responseBody, &responseSize, handlers);
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

    UNUSED(arg);

    while (g_serverActive)
    {
        if (0 <= (socketHandle = accept(g_socketfd, (struct sockaddr*)&g_socketaddr, &g_socketlen)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(GetPlatformLog(), "Accepted connection: path %s, handle '%d'", g_socketaddr.sun_path, socketHandle);
            }

            HandleConnection(socketHandle);

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