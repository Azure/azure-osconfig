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

class PlatformLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_log;
    }

    static void OpenLog()
    {
        m_log = ::OpenLog(PLATFORM_LOGFILE, PLATFORM_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_log);
    }

    static OSCONFIG_LOG_HANDLE m_log;
};

OSCONFIG_LOG_HANDLE PlatformLog::m_log = NULL;

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

static HTTP_STATUS MpiOpenRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    rapidjson::Document document;
    const char* responseFormat = "\"%s\"";

    if (!document.Parse(request).HasParseError())
    {
        if (document.HasMember(g_clientName) && document.HasMember(g_maxPayloadSizeBytes) && document[g_clientName].IsString() && document[g_maxPayloadSizeBytes].IsInt())
        {
            std::string clientName = document[g_clientName].GetString();
            int maxPayloadSizeBytes = document[g_maxPayloadSizeBytes].GetInt();

            OsConfigLogInfo(PlatformLog::Get(), "Received MpiOpen request for client '%s' with max payload size %d", clientName.c_str(), maxPayloadSizeBytes);

            char* sessionId = CreateUuid();
            MPI_HANDLE handle = MpiOpen(clientName.c_str(), maxPayloadSizeBytes);

            if (handle)
            {
                g_sessions[sessionId] = handle;
                status = HTTP_OK;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to create MPI session for client '%s'", clientName.c_str());
                status = HTTP_INTERNAL_SERVER_ERROR;
            }

            FREE_MEMORY(sessionId);
        }
        else
        {
            status = HTTP_BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiOpen request: %s", request);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiOpen request");
            }
        }
    }
    else
    {
        status = HTTP_BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiOpen request: %s", request);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiOpen request");
        }
    }

    return status;
}

static HTTP_STATUS MpiCloseRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    rapidjson::Document document;

    if (!document.Parse(request).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString())
        {
            std::string session = document[g_clientSession].GetString();
            OsConfigLogInfo(PlatformLog::Get(), "Received MpiClose request for session '%s'", session.c_str());

            if (g_sessions.find(session) != g_sessions.end())
            {
                MpiClose(g_sessions[session]);
                g_sessions.erase(session);
                status = HTTP_OK;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI close request");
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            status = HTTP_BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiClose request: %s", request);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiClose request");
            }
        }
    }
    else
    {
        status = HTTP_BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiClose request: %s", request);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiClose request");
        }
    }

    return status;
}

static HTTP_STATUS MpiSetRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    rapidjson::Document document;

    if (!document.Parse(request).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString() && document.HasMember(g_componentName) && document[g_componentName].IsString() && document.HasMember(g_objectName) && document[g_objectName].IsString() && document.HasMember(g_payload))
        {
            std::string session = document[g_clientSession].GetString();
            std::string component = document[g_componentName].GetString();
            std::string object = document[g_objectName].GetString();

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            document[g_payload].Accept(writer);
            std::string payload = buffer.GetString();

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PlatformLog::Get(), "Received MpiSet request for session '%s' component '%s' object '%s' payload '%s'", session.c_str(), component.c_str(), object.c_str(), payload.c_str());
            }

            if (g_sessions.find(session) != g_sessions.end())
            {
                int mpiStatus = MpiSet(g_sessions[session], component.c_str(), object.c_str(), (MPI_JSON_STRING)payload.c_str(), payload.size());
                status = ((mpiStatus == MPI_OK) ? HTTP_OK : HTTP_BAD_REQUEST);
                responsePayload = "\""+ std::to_string(mpiStatus) + "\"";
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "No session found for MpiSet request: %s", session.c_str());
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            status = HTTP_BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSet request: %s", request);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSet request");
            }
        }
    }
    else
    {
        status = HTTP_BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSet request: %s", request);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSet request");
        }
    }

    return status;
}

static HTTP_STATUS MpiGetRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    rapidjson::Document document;

    if (!document.Parse(request).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString() && document.HasMember(g_componentName) && document[g_componentName].IsString() && document.HasMember(g_objectName) && document[g_objectName].IsString())
        {
            std::string session = document[g_clientSession].GetString();
            std::string component = document[g_componentName].GetString();
            std::string object = document[g_objectName].GetString();

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PlatformLog::Get(), "Received MpiGet request for session '%s' component '%s' object '%s'", session.c_str(), component.c_str(), object.c_str());
            }

            if (g_sessions.find(session) != g_sessions.end())
            {
                int mpiStatus = MpiGet(g_sessions[session], component.c_str(), object.c_str(), (MPI_JSON_STRING*)response, responseSize);
                status = ((mpiStatus == MPI_OK) ? HTTP_OK : HTTP_INTERNAL_SERVER_ERROR);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGet request: %s", session.c_str());
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            status = HTTP_BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGet request: %s", request);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGet request");
            }
        }
    }
    else
    {
        status = HTTP_BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGet request: %s", request);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGet request");
        }
    }

    return status;
}

static HTTP_STATUS MpiSetDesiredRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    rapidjson::Document document;

    if (!document.Parse(request).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString() && document.HasMember(g_payload))
        {
            std::string session = document[g_clientSession].GetString();

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            document[g_payload].Accept(writer);
            std::string payload = buffer.GetString();

            if (g_sessions.find(session) != g_sessions.end())
            {
                int mpiStatus = MpiSetDesired(g_sessions[session], (MPI_JSON_STRING)payload.c_str(), payload.size());
                status = ((mpiStatus == MPI_OK) ? HTTP_OK : HTTP_BAD_REQUEST);
                responsePayload = "\"" + std::to_string(mpiStatus) + "\"";
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSetDesired request");
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            status = HTTP_BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSetDesired request: %s", request);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSetDesired request");
            }
        }
    }
    else
    {
        status = HTTP_BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSetDesired request: %s", request);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSetDesired request");
        }
    }

    return status;
}

static HTTP_STATUS MpiGetReportedRequest(const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;
    rapidjson::Document document;
    const char* defaultResponse = "{}";

    if (!document.Parse(request).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString())
        {
            std::string session = document[g_clientSession].GetString();

            if (g_sessions.find(session) != g_sessions.end())
            {
                int mpiStatus = MpiGetReported(g_sessions[session], (MPI_JSON_STRING*)response, responseSize);
                status = ((mpiStatus == MPI_OK) ? HTTP_OK : HTTP_INTERNAL_SERVER_ERROR);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGetReported request");
                status = HTTP_BAD_REQUEST;
            }
        }
        else
        {
            status = HTTP_BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGetReported request: %s", request);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGetReported request");
            }
        }
    }
    else
    {
        status = HTTP_BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGetReported request: %s", request);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGetReported request");
        }
    }

    return status;
}

HTTP_STATUS RouteRequest(const char* uri, const char* request, char** response, int* responseSize)
{
    HTTP_STATUS status = HTTP_OK;

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
        OsConfigLogError(PlatformLog::Get(), "%s: invalid request", uri);
        status = HTTP_NOT_FOUND;
    }

    return status;
}

static void GetReasonPhrase(HTTP_STATUS statusCode, char* phrase)
{
    switch (statusCode)
    {
        case HTTP_OK:
            strcpy(phrase, "HTTP_OK");
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

static void HandleConnection(int socketHandle)
{
    const char* responseFormat = "HTTP/1.1 %d %s\r\nServer: OSConfig\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s";

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

    if (NULL == (uri = ReadUriFromSocket(socketHandle, PlatformLog::Get())))
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to read request URI %d", socketHandle);
    }

    if (0 >= (contentLength = ReadHttpContentLengthFromSocket(socketHandle, PlatformLog::Get())))
    {
        OsConfigLogError(PlatformLog::Get(), "%s: failed to read HTTP Content-Length", uri);
    }

    if (NULL == (requestBody = (char*)malloc(contentLength + 1)))
    {
        OsConfigLogError(PlatformLog::Get(), "%s: failed to allocate memory for HTTP body, Content-Length %d", uri, contentLength);
    }

    memset(requestBody, 0, contentLength + 1);

    if (contentLength != (int)(bytes = read(socketHandle, requestBody, contentLength)))
    {
        OsConfigLogError(PlatformLog::Get(), "%s: failed to read complete HTTP body, Content-Length %d, bytes read %d", uri, contentLength, (int)bytes);
    }

    if ((NULL == uri) || (NULL == requestBody))
    {
        // TODO: need to malloc ???
        responseBody = "{\"error\":\"Failed to read request\"}";
        responseSize = strlen(responseBody);
        status = HTTP_BAD_REQUEST;
    }
    else
    {
        status = RouteRequest(uri, requestBody, &responseBody, &responseSize);
        GetReasonPhrase(status, reasonPhrase);
    }

    FREE_MEMORY(requestBody);

    estimatedSize = strlen(responseFormat) + MAX_STATUS_CODE_LENGTH + strlen(reasonPhrase) + MAX_CONTENTLENGTH_LENGTH + responseSize + 1;

    if (NULL == (buffer = (char*)malloc(estimatedSize)))
    {
        OsConfigLogError(PlatformLog::Get(), "%s: failed to allocate memory for HTTP response, %d bytes of %d", uri, 0, estimatedSize);
        return;
    }

    memset(buffer, 0, estimatedSize);

    snprintf(buffer, estimatedSize, responseFormat, (int)status, reasonPhrase, responseSize, (responseBody ? responseBody : ""));
    actualSize = (int)strlen(buffer);

    bytes = write(socketHandle, buffer, strlen(buffer));

    if (bytes != actualSize)
    {
        OsConfigLogError(PlatformLog::Get(), "%s: failed to write complete HTTP response, %d bytes of %d", uri, (int)bytes, actualSize);
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
                OsConfigLogInfo(PlatformLog::Get(), "Accepted connection: path %s, handle '%d'", g_socketaddr.sun_path, socketHandle);
            }

            HandleConnection(socketHandle);

            if (0 != close(socketHandle))
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to close socket: path %s, handle '%d'", g_socketaddr.sun_path, socketHandle);
            }

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PlatformLog::Get(), "Closed connection: path %s, handle '%d'", g_socketaddr.sun_path, socketHandle);
            }
        }
    }

    return NULL;
}

void MpiApiInitialize(void)
{
    PlatformLog::OpenLog();

    struct stat st;
    if (stat(g_socketPrefix, &st) == -1)
    {
        mkdir(g_socketPrefix, 0700);
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
            if (listen(g_socketfd, 5) == 0)
            {
                OsConfigLogInfo(PlatformLog::Get(), "Listening on socket '%s'", g_mpiSocket);

                g_serverActive = true;
                g_worker = pthread_create(&g_worker, NULL, Worker, NULL);;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to listen on socket '%s'", g_mpiSocket);
            }
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to bind socket '%s'", g_mpiSocket);
        }
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to create socket '%s'", g_mpiSocket);
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

    PlatformLog::CloseLog();
}