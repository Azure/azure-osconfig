// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <pthread.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "CommonUtils.h"
#include "MpiServer.h"
#include "ModulesManager.h"
#include "Logging.h"

static const char* g_socketPrefix = "/run/osconfig";
static const char* g_mpiSocket = "/run/osconfig/mpid.sock";

static int g_socketfd;
static struct sockaddr_un g_socketaddr;
static socklen_t g_socketlen;

static pthread_t g_worker;
static bool g_serverActive = false;

static const char* g_clientName = "ClientName";
static const char* g_maxPayloadSizeBytes = "MaxPayloadSizeBytes";
static const char* g_clientSession = "ClientSession";
static const char* g_componentName = "ComponentName";
static const char* g_objectName = "ObjectName";
static const char* g_payload = "Payload";

#define UUID_LENGTH 36
#define MAX_CONTENTLENGTH_LENGTH 16
#define MAX_REASON_PHRASE_LENGTH 32
#define MAX_STATUS_CODE_LENGTH 3

#define PLATFORM_LOGFILE "/var/log/osconfig_platform.log"
#define PLATFORM_ROLLEDLOGFILE "/var/log/osconfig_platform.bak"

static std::map<std::string, MPI_HANDLE> g_sessions;

typedef enum HTTP_STATUS
{
    HTTP_OK = 200,
    HTTP_BAD_REQUEST = 400,
    HTTP_NOT_FOUND = 404,
    HTTP_INTERNAL_SERVER_ERROR = 500
} HTTP_STATUS;

static char* CreateUuid()
{
    char* uuid = NULL;
    static const char uuidTemplate[] = "xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx";
    const char* hex = "0123456789ABCDEF-";

    uuid = (char*)malloc(UUID_LENGTH + 1);
    if (uuid == NULL)
    {
        return NULL;
    }

    srand(clock());

    for (int i = 0; i < UUID_LENGTH + 1; i++)
    {
        int r = rand() % 16;
        char c = ' ';

        switch (uuidTemplate[i])
        {
            case 'x':
                c = hex[r];
                break;
            case '-':
                c = '-';
                break;
            case 'M':
                c = hex[(r & 0x03) | 0x08];
                break;
            case 'N':
                c = '4';
                break;
        }
        uuid[i] = (i < UUID_LENGTH) ? c : 0x00;
    }

    return uuid;
}

static void GetHttpReasonPhrase(HTTP_STATUS statusCode, char* phrase)
{
    switch (statusCode)
    {
        case HTTP_OK:
            strcpy(phrase, "OK");
            break;
        case HTTP_BAD_REQUEST:
            strcpy(phrase, "Bad Request");
            break;
        case HTTP_NOT_FOUND:
            strcpy(phrase, "Not Found");
            break;
        case HTTP_INTERNAL_SERVER_ERROR:
            strcpy(phrase, "Internal Server Error");
            break;
        default:
            strcpy(phrase, "Unknown");
    }
}

static HTTP_STATUS MpiOpenRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Value* clientNameValue = NULL;
    JSON_Value* maxPayloadSizeBytesValue = NULL;
    char* clientName = NULL;
    int maxPayloadSizeBytes = 0;
    const char* responseFormat = "\"%s\"";

    if ((NULL != (rootValue = json_parse_string(request))) && (NULL != (rootObject = json_value_get_object(rootValue))))
    {
        clientNameValue = json_object_get_value(rootObject, g_clientName);
        maxPayloadSizeBytesValue = json_object_get_value(rootObject, g_maxPayloadSizeBytes);

        if (clientNameValue && maxPayloadSizeBytesValue && (JSONString == json_value_get_type(clientNameValue)) && (JSONNumber == json_value_get_type(maxPayloadSizeBytesValue)))
        {
            clientName = (char*)json_value_get_string(clientNameValue);
            maxPayloadSizeBytes = (int)json_value_get_number(maxPayloadSizeBytesValue);

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(GetPlatformLog(), "Received MpiOpen request for client '%s' with max payload size %d", clientName, maxPayloadSizeBytes);
            }

            char* sessionId = CreateUuid();
            MPI_HANDLE handle = MpiOpen(clientName, maxPayloadSizeBytes);

            if (handle)
            {
                g_sessions[sessionId] = handle;

                *responseSize =  UUID_LENGTH + 2;
                if (NULL != (*response = (char*)malloc(*responseSize + 1)))
                {
                    snprintf(*response, *responseSize + 1, responseFormat, sessionId);
                    FREE_MEMORY(sessionId);
                }
                else
                {
                    OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for response");
                    status = HTTP_INTERNAL_SERVER_ERROR;
                    *responseSize = 0;
                }
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "Failed to open session for client '%s'", clientName);
                status = HTTP_INTERNAL_SERVER_ERROR;
            }

            FREE_MEMORY(sessionId);
        }
        else
        {
            status = HTTP_BAD_REQUEST;
        }
    }
    else
    {
        status = HTTP_BAD_REQUEST;
    }

    return status;
}

static HTTP_STATUS MpiCloseRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Value* clientSessionValue = NULL;
    char* clientSession = NULL;

    UNUSED(response);
    UNUSED(responseSize);

    if ((NULL != (rootValue = json_parse_string(request))) && (NULL != (rootObject = json_value_get_object(rootValue))))
    {
        clientSessionValue = json_object_get_value(rootObject, g_clientSession);

        if (clientSessionValue && (JSONString == json_value_get_type(clientSessionValue)))
        {
            clientSession = (char*)json_value_get_string(clientSessionValue);

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(GetPlatformLog(), "Received MpiClose request, session '%s'", clientSession);
            }

            if (g_sessions.find(clientSession) != g_sessions.end())
            {
                MpiClose(g_sessions[clientSession]);
                g_sessions.erase(clientSession);
                status = HTTP_OK;
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "No session found for client '%s'", clientSession);
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            OsConfigLogError(GetPlatformLog(),"Failed to parse '%s' from request", g_clientSession);
            status = HTTP_BAD_REQUEST;
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(),"Failed to parse request");
        status = HTTP_BAD_REQUEST;
    }

    return status;
}

static HTTP_STATUS MpiSetRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Value* clientSessionValue = NULL;
    JSON_Value* componentValue = NULL;
    JSON_Value* objectValue = NULL;
    JSON_Value* payloadValue = NULL;
    char* clientSession = NULL;
    char* component = NULL;
    char* object = NULL;
    char* payload = NULL;
    const char* responseFormat = "\"%d\"";

    if ((NULL != (rootValue = json_parse_string(request))) && (NULL != (rootObject = json_value_get_object(rootValue))))
    {
        clientSessionValue = json_object_get_value(rootObject, g_clientSession);
        componentValue = json_object_get_value(rootObject, g_componentName);
        objectValue = json_object_get_value(rootObject, g_objectName);
        payloadValue = json_object_get_value(rootObject, g_payload);

        if (clientSessionValue && componentValue && objectValue && payloadValue && (JSONString == json_value_get_type(clientSessionValue)) && (JSONString == json_value_get_type(componentValue)) && (JSONString == json_value_get_type(objectValue)))
        {
            clientSession = (char*)json_value_get_string(clientSessionValue);
            component = (char*)json_value_get_string(componentValue);
            object = (char*)json_value_get_string(objectValue);
            payload = (char*)json_serialize_to_string(payloadValue);

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(GetPlatformLog(), "Received MpiSet(%s, %s) request, session '%s'", component, object, clientSession);
            }

            if (g_sessions.find(clientSession) != g_sessions.end())
            {
                int mpiStatus = MpiSet(g_sessions[clientSession], component, object, (MPI_JSON_STRING)payload, strlen(payload));
                status = ((mpiStatus == MPI_OK) ? HTTP_OK : HTTP_BAD_REQUEST);

                if (NULL != (*response = (char*)malloc(strlen(responseFormat) + MAX_STATUS_CODE_LENGTH)))
                {
                    *responseSize = snprintf(*response, strlen(responseFormat) + MAX_STATUS_CODE_LENGTH, responseFormat, mpiStatus);
                }
                else
                {
                    OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for response");
                    *responseSize = 0;
                    status = HTTP_INTERNAL_SERVER_ERROR;
                }
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "No session found for client '%s'", clientSession);
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "Failed to parse request");
            status = HTTP_BAD_REQUEST;
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "Failed to parse request");
        status = HTTP_BAD_REQUEST;
    }

    return status;
}

static HTTP_STATUS MpiGetRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Value* clientSessionValue = NULL;
    JSON_Value* componentValue = NULL;
    JSON_Value* objectValue = NULL;
    char* clientSession = NULL;
    char* component = NULL;
    char* object = NULL;

    if ((NULL != (rootValue = json_parse_string(request))) && (NULL != (rootObject = json_value_get_object(rootValue))))
    {
        clientSessionValue = json_object_get_value(rootObject, g_clientSession);
        componentValue = json_object_get_value(rootObject, g_componentName);
        objectValue = json_object_get_value(rootObject, g_objectName);

        if (clientSessionValue && componentValue && objectValue && (JSONString == json_value_get_type(clientSessionValue)) && (JSONString == json_value_get_type(componentValue)) && (JSONString == json_value_get_type(objectValue)))
        {
            clientSession = (char*)json_value_get_string(clientSessionValue);
            component = (char*)json_value_get_string(componentValue);
            object = (char*)json_value_get_string(objectValue);

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(GetPlatformLog(), "Received MpiGet(%s, %s) request, session '%s'", component, object, clientSession);
            }

            if (g_sessions.find(clientSession) != g_sessions.end())
            {
                int mpiStatus = MpiGet(g_sessions[clientSession], component, object, (MPI_JSON_STRING*)response, responseSize);
                if (mpiStatus != MPI_OK)
                {
                    OsConfigLogError(GetPlatformLog(), "Failed to get value for component '%s' and object '%s'", component, object);
                    status = HTTP_INTERNAL_SERVER_ERROR;
                }
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "No session found for client '%s'", clientSession);
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "Failed to parse request");
            status = HTTP_BAD_REQUEST;
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "Failed to parse request");
        status = HTTP_BAD_REQUEST;
    }

    return status;
}

static HTTP_STATUS MpiSetDesiredRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Value* clientSessionValue = NULL;
    JSON_Value* payloadValue = NULL;
    char* clientSession = NULL;
    char* payload = NULL;

    UNUSED(response);
    UNUSED(responseSize);

    if ((NULL != (rootValue = json_parse_string(request))) && (NULL != (rootObject = json_value_get_object(rootValue))))
    {
        clientSessionValue = json_object_get_value(rootObject, g_clientSession);
        payloadValue = json_object_get_value(rootObject, g_payload);

        if (clientSessionValue && payloadValue && (JSONString == json_value_get_type(clientSessionValue)))
        {
            clientSession = (char*)json_value_get_string(clientSessionValue);
            payload = (char*)json_serialize_to_string(payloadValue);

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(GetPlatformLog(), "Received MpiSetDesired request for '%s'", clientSession);
            }

            if (g_sessions.find(clientSession) != g_sessions.end())
            {
                int mpiStatus = MpiSetDesired(g_sessions[clientSession], (MPI_JSON_STRING)payload, strlen(payload));
                status = ((mpiStatus == MPI_OK) ? HTTP_OK : HTTP_BAD_REQUEST);
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "No session found for client '%s'", clientSession);
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "Failed to parse request");
            status = HTTP_BAD_REQUEST;
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "Failed to parse request");
        status = HTTP_BAD_REQUEST;
    }

    return status;
}

static HTTP_STATUS MpiGetReportedRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Value* clientSessionValue = NULL;
    char* clientSession = NULL;

    if ((NULL != (rootValue = json_parse_string(request))) && (NULL != (rootObject = json_value_get_object(rootValue))))
    {
        clientSessionValue = json_object_get_value(rootObject, g_clientSession);

        if (clientSessionValue && (JSONString == json_value_get_type(clientSessionValue)))
        {
            clientSession = (char*)json_value_get_string(clientSessionValue);

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(GetPlatformLog(), "Received MpiGetReported request for '%s'", clientSession);
            }

            if (g_sessions.find(clientSession) != g_sessions.end())
            {
                int mpiStatus = MpiGetReported(g_sessions[clientSession], (MPI_JSON_STRING*)response, responseSize);
                status = ((mpiStatus == MPI_OK) ? HTTP_OK : HTTP_INTERNAL_SERVER_ERROR);
            }
            else
            {
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            status = HTTP_BAD_REQUEST;
        }
    }
    else
    {
        status = HTTP_BAD_REQUEST;
    }

    return status;
}

HTTP_STATUS RouteRequest(const char* uri, const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;

    OsConfigLogInfo(GetPlatformLog(), "REQUEST %s\n %s", uri, request);

    if (0 == strcmp(uri, "MpiOpen"))
    {
        status = MpiOpenRequest(request, response, responseSize);
    }
    else if (0 == strcmp(uri, "MpiClose"))
    {
        status = MpiCloseRequest(request, response, responseSize);
    }
    else if (0 == strcmp(uri, "MpiSet"))
    {
        status = MpiSetRequest(request, response, responseSize);
    }
    else if (0 == strcmp(uri, "MpiGet"))
    {
        status = MpiGetRequest(request, response, responseSize);
    }
    else if (0 == strcmp(uri, "MpiSetDesired"))
    {
        status = MpiSetDesiredRequest(request, response, responseSize);
    }
    else if (0 == strcmp(uri, "MpiGetReported"))
    {
        status = MpiGetReportedRequest(request, response, responseSize);
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "%s: invalid request", uri);
        status = HTTP_NOT_FOUND;
    }

    if ((status != HTTP_OK) && (status != HTTP_NOT_FOUND) && IsFullLoggingEnabled())
    {
        OsConfigLogError(GetPlatformLog(), "Invalid %s request body: %s", uri, request);
    }

    return status;
}

static void HandleConnection(int socketHandle)
{
    const char* responseFormat = "HTTP/1.1 %d %s\r\nServer: OSConfig\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%.*s";

    char* uri = NULL;
    int contentLength = 0;
    char* requestBody = NULL;
    HTTP_STATUS status = HTTP_OK;
    char reasonPhrase[MAX_REASON_PHRASE_LENGTH] = {0};
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

    if (0 >= (contentLength = ReadHttpContentLengthFromSocket(socketHandle, GetPlatformLog())))
    {
        OsConfigLogError(GetPlatformLog(), "%s: failed to read HTTP Content-Length", uri);
        status = HTTP_BAD_REQUEST;
    }

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

    if (status == HTTP_OK)
    {
        status = RouteRequest(uri, requestBody, &responseBody, &responseSize);
    }

    FREE_MEMORY(requestBody);

    GetHttpReasonPhrase(status, reasonPhrase);
    estimatedSize = strlen(responseFormat) + MAX_STATUS_CODE_LENGTH + strlen(reasonPhrase) + MAX_CONTENTLENGTH_LENGTH + responseSize + 1;

    if (NULL == (buffer = (char*)malloc(estimatedSize)))
    {
        OsConfigLogError(GetPlatformLog(), "%s: failed to allocate memory for HTTP response, %d bytes of %d", uri, 0, estimatedSize);
        return;
    }

    memset(buffer, 0, estimatedSize);

    snprintf(buffer, estimatedSize, responseFormat, (int)status, reasonPhrase, responseSize, responseSize, (responseBody ? responseBody : ""));
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

    FREE_MEMORY(buffer);
    FREE_MEMORY(uri);
}

static void* Worker(void*)
{
    int socketHandle = -1;

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
    for (auto session : g_sessions)
    {
        MpiClose(session.second);
    }

    g_serverActive = false;
    pthread_join(g_worker, NULL);

    close(g_socketfd);
    unlink(g_mpiSocket);
}