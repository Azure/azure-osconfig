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

enum StatusCode
{
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500
};

const char* CreateUuid()
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
            case 'x': { c = hex[r]; } break;
            case 'M': { c = hex[(r & 0x03) | 0x08]; } break;
            case '-': { c = '-'; } break;
            case 'N': { c = '4'; } break;
        }

        uuid[i] = (i < UUID_LENGTH) ? c : 0x00;
    }

    return uuid;
}

void GetReasonPhrase(StatusCode statusCode, char* phrase)
{
    switch (statusCode)
    {
        case OK:
            strcpy(phrase, "OK");
            break;
        case BAD_REQUEST:
            strcpy(phrase, "Bad Request");
            break;
        case NOT_FOUND:
            strcpy(phrase, "Not Found");
            break;
        case INTERNAL_SERVER_ERROR:
            strcpy(phrase, "Internal Server Error");
            break;
        default:
            strcpy(phrase, "Unknown");
            break;
    }
}

static StatusCode MpiOpenRequest(const std::string& requestPayload, std::string& responsePayload)
{
    StatusCode status = StatusCode::OK;
    rapidjson::Document document;

    if (!document.Parse(requestPayload.c_str()).HasParseError())
    {
        if (document.HasMember(g_clientName) && document.HasMember(g_maxPayloadSizeBytes) && document[g_clientName].IsString() && document[g_maxPayloadSizeBytes].IsInt())
        {
            std::string clientName = document[g_clientName].GetString();
            int maxPayloadSizeBytes = document[g_maxPayloadSizeBytes].GetInt();

            OsConfigLogInfo(PlatformLog::Get(), "Received MpiOpen request for client '%s' with max payload size %d", clientName.c_str(), maxPayloadSizeBytes);

            std::string sessionId = CreateUuid();
            MPI_HANDLE handle = MpiOpen(clientName.c_str(), maxPayloadSizeBytes);

            if (handle != NULL)
            {
                g_sessions[sessionId] = handle;
                status = StatusCode::OK;
                responsePayload = ("\"" + sessionId + "\"");
            }
            else
            {
                responsePayload = "\"\"";
                OsConfigLogError(PlatformLog::Get(), "Failed to create MPI session for client '%s'", clientName.c_str());
            }
        }
        else
        {
            status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiOpen request: %s", requestPayload.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiOpen request");
            }
        }
    }
    else
    {
        status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiOpen request: %s", requestPayload.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiOpen request");
        }
    }

    return status;
}

static StatusCode MpiCloseRequest(const std::string& requestPayload, std::string& responsePayload)
{
    StatusCode status = StatusCode::OK;
    rapidjson::Document document;

    responsePayload = "";

    if (!document.Parse(requestPayload.c_str()).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString())
        {
            std::string session = document[g_clientSession].GetString();
            OsConfigLogInfo(PlatformLog::Get(), "Received MpiClose request for session '%s'", session.c_str());

            if (g_sessions.find(session) != g_sessions.end())
            {
                MpiClose(g_sessions[session]);
                g_sessions.erase(session);
                status = StatusCode::OK;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI close request");
                status = StatusCode::BAD_REQUEST;
            }
        }
        else
        {
            status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiClose request: %s", requestPayload.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiClose request");
            }
        }
    }
    else
    {
        status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiClose request: %s", requestPayload.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiClose request");
        }
    }

    return status;
}

static StatusCode MpiSetRequest(const std::string& requestPayload, std::string& responsePayload)
{
    StatusCode status = StatusCode::OK;
    rapidjson::Document document;

    if (!document.Parse(requestPayload.c_str()).HasParseError())
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
                status = ((mpiStatus == MPI_OK) ? StatusCode::OK : StatusCode::BAD_REQUEST);
                responsePayload = "\""+ std::to_string(mpiStatus) + "\"";
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "No session found for MpiSet request: %s", session.c_str());
                status = StatusCode::BAD_REQUEST;
            }
        }
        else
        {
            status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSet request: %s", requestPayload.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSet request");
            }
        }
    }
    else
    {
        status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSet request: %s", requestPayload.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSet request");
        }
    }

    return status;
}

static StatusCode MpiGetRequest(const std::string& requestPayload, std::string& responsePayload)
{
    StatusCode status = StatusCode::OK;
    rapidjson::Document document;

    if (!document.Parse(requestPayload.c_str()).HasParseError())
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
                MPI_JSON_STRING payload;
                int payloadSizeBytes = 0;
                int mpiStatus = MpiGet(g_sessions[session], component.c_str(), object.c_str(), &payload, &payloadSizeBytes);

                if (mpiStatus == MPI_OK)
                {
                    responsePayload = std::string(payload, payloadSizeBytes);
                    status = StatusCode::OK;
                }
                else
                {
                    responsePayload = "\"" + std::to_string(mpiStatus) + "\"";
                    status = StatusCode::BAD_REQUEST;
                }
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGet request: %s", session.c_str());
                status = StatusCode::BAD_REQUEST;
            }
        }
        else
        {
            status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGet request: %s", requestPayload.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGet request");
            }
        }
    }
    else
    {
        status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGet request: %s", requestPayload.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGet request");
        }
    }

    return status;
}

static StatusCode MpiSetDesiredRequest(const std::string& requestPayload, std::string& responsePayload)
{
    StatusCode status = StatusCode::OK;
    rapidjson::Document document;

    if (!document.Parse(requestPayload.c_str()).HasParseError())
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
                status = ((mpiStatus == MPI_OK) ? StatusCode::OK : StatusCode::BAD_REQUEST);
                responsePayload = "\"" + std::to_string(mpiStatus) + "\"";
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSetDesired request");
                status = StatusCode::BAD_REQUEST;
            }
        }
        else
        {
            status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSetDesired request: %s", requestPayload.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSetDesired request");
            }
        }
    }
    else
    {
        status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSetDesired request: %s", requestPayload.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSetDesired request");
        }
    }

    return status;
}

static StatusCode MpiGetReportedRequest(const std::string& requestPayload, std::string& responsePayload)
{
    StatusCode status = StatusCode::OK;
    rapidjson::Document document;
    responsePayload = "{}";

    if (!document.Parse(requestPayload.c_str()).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString())
        {
            std::string session = document[g_clientSession].GetString();

            if (g_sessions.find(session) != g_sessions.end())
            {
                MPI_JSON_STRING payload;
                int payloadSizeBytes = 0;
                int mpiStatus = MpiGetReported(g_sessions[session], &payload, &payloadSizeBytes);

                if (mpiStatus == MPI_OK)
                {
                    responsePayload = std::string(payload, payloadSizeBytes);
                    status = StatusCode::OK;
                }
                else
                {
                    responsePayload = "\"" + std::to_string(mpiStatus) + "\"";
                    status = StatusCode::BAD_REQUEST;
                }
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGetReported request");
                status = StatusCode::BAD_REQUEST;
            }
        }
        else
        {
            status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGetReported request: %s", requestPayload.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGetReported request");
            }
        }
    }
    else
    {
        status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGetReported request: %s", requestPayload.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGetReported request");
        }
    }

    return status;
}

StatusCode RouteRequest(const char* uri, const std::string& request, std::string& response)
{
    StatusCode status = StatusCode::OK;

    if (0 == strcmp(uri, "MpiOpen"))
    {
        status = MpiOpenRequest(request, response);
    }
    else if (0 == strcmp(uri, "MpiClose"))
    {
        status = MpiCloseRequest(request, response);
    }
    else if (0 == strcmp(uri, "MpiSet"))
    {
        status = MpiSetRequest(request, response);
    }
    else if (0 == strcmp(uri, "MpiGet"))
    {
        status = MpiGetRequest(request, response);
    }
    else if (0 == strcmp(uri, "MpiSetDesired"))
    {
        status = MpiSetDesiredRequest(request, response);
    }
    else if (0 == strcmp(uri, "MpiGetReported"))
    {
        status = MpiGetReportedRequest(request, response);
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "%s: invalid request", uri);
        status = StatusCode::NOT_FOUND;
    }

    return status;
}

static void HandleConnection(int connfd)
{
    const char* responseFormat = "HTTP/1.1 %d %s\r\nServer: OSConfig\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s";

    StatusCode status = StatusCode::OK;
    char* uri = NULL;
    char* requestPayload = NULL;
    int contentLength = 0;
    std::string responseBody;
    char* responseBuffer = NULL;
    char reasonPhrase[MAX_REASON_PHRASE_LENGTH];
    int estimatedSize = 0;
    int actualSize = 0;
    ssize_t bytes = 0;

    if (NULL == (uri = ReadUriFromSocket(connfd, PlatformLog::Get())))
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to read request URI %d", connfd);
        return;
    }

    if (0 >= (contentLength = ReadHttpContentLengthFromSocket(connfd, PlatformLog::Get())))
    {
        OsConfigLogError(PlatformLog::Get(), "%s: failed to read HTTP Content-Length", uri);
        return;
    }

    if (NULL != (requestPayload = new (std::nothrow) char[contentLength]))
    {
        if (contentLength != static_cast<int>(bytes = read(connfd, requestPayload, contentLength)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "%s: failed to read complete HTTP body, Content-Length %d, bytes read %d '%.*s'", uri, contentLength, static_cast<int>(bytes), static_cast<int>(bytes), requestPayload);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "%s: failed to read complete HTTP body, Content-Length %d, bytes read %d", uri, contentLength, static_cast<int>(bytes));
            }
        }

        status = RouteRequest(uri, std::string(requestPayload, contentLength), responseBody);
        GetReasonPhrase(status, reasonPhrase);

        FREE_MEMORY(requestPayload);
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "%s: failed to allocate memory for HTTP body, Content-Length %d", uri, contentLength);
    }

    estimatedSize = strlen(responseFormat) + MAX_STATUS_CODE_LENGTH + strlen(reasonPhrase) + responseBody.length() + 1;

    if (NULL != (responseBuffer = (char*)malloc(estimatedSize)))
    {
        snprintf(responseBuffer, estimatedSize, responseFormat, (int)status, reasonPhrase, responseBody.length(), responseBody.c_str());

        bytes = write(connfd, responseBuffer, strlen(responseBuffer));
        actualSize = (int)strlen(responseBuffer);

        if (bytes != actualSize)
        {
            OsConfigLogError(PlatformLog::Get(), "%s: failed to write complete HTTP response, bytes written %d", uri, static_cast<int>(bytes));
        }

        FREE_MEMORY(responseBuffer);
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "%s: failed to allocate memory for HTTP response", uri);
    }

    FREE_MEMORY(uri);
}

static void* Worker(void*)
{
    int connfd = -1;

    while (g_serverActive)
    {
        if (0 <= (connfd = accept(g_socketfd, (struct sockaddr*)&g_socketaddr, &g_socketlen)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PlatformLog::Get(), "Accepted connection %s '%d'", g_socketaddr.sun_path, connfd);
            }

            HandleConnection(connfd);

            if (0 != close(connfd))
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to close socket %s '%d'", g_mpiSocket, connfd);
            }

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PlatformLog::Get(), "Closed connection %s '%d'", g_socketaddr.sun_path, connfd);
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

        // Unlink socket if it is already in use
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