// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>
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

static const std::string g_CRLF = "\r\n";
static const std::string g_httpVersion = "HTTP/1.1";
static const std::string g_contentTypeJson = "Content-Type: application/json";
static const std::string g_contentLength = "Content-Length: ";

static const char* g_clientName = "ClientName";
static const char* g_maxPayloadSizeBytes = "MaxPayloadSizeBytes";
static const char* g_clientSession = "ClientSession";
static const char* g_componentName = "ComponentName";
static const char* g_objectName = "ObjectName";
static const char* g_payload = "Payload";

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

OSCONFIG_LOG_HANDLE PlatformLog::m_log = nullptr;

enum class StatusCode
{
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500
};

class Server
{
public:
    Server() = default;
    ~Server() = default;

    void Listen();
    void Stop();

private:
    int m_socketfd;
    struct sockaddr_un m_addr;
    socklen_t m_socketlen;

    std::thread m_worker;
    std::promise<void> m_exitSignal;

    static void Worker(Server& server);
};

static Server server;

std::string CreateGUID()
{
    std::stringstream ss;
    std::srand(std::time(nullptr));
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << std::rand();
    return ss.str();
}

std::string StatusText(StatusCode statusCode)
{
    std::string result;
    switch (statusCode)
    {
        case StatusCode::OK:
            result = "OK";
            break;
        case StatusCode::BAD_REQUEST:
            result = "BAD_REQUEST";
            break;
        case StatusCode::NOT_FOUND:
            result = "NOT_FOUND";
            break;
        default:
            result = "INTERNAL_SERVER_ERROR";
            break;
    }
    return result;
}

std::string SerializeResponse(StatusCode status, const std::string& payload)
{
    std::string result;
    result += g_httpVersion + " " + std::to_string(static_cast<int>(status)) + " " + StatusText(status) + g_CRLF;
    result += g_contentTypeJson + g_CRLF;
    result += g_contentLength + std::to_string(payload.size()) + g_CRLF;
    result += g_CRLF;
    result += payload;
    return result;
}

void Server::Listen()
{
    struct stat st;
    if (stat(g_socketPrefix, &st) == -1)
    {
        mkdir(g_socketPrefix, 0700);
    }

    if (0 <= (m_socketfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)))
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        strncpy(m_addr.sun_path, g_mpiSocket, sizeof(m_addr.sun_path) - 1);
        m_socketlen = sizeof(m_addr);

        // Unlink socket if it is already in use
        unlink(g_mpiSocket);

        if (bind(m_socketfd, (struct sockaddr*)&m_addr, m_socketlen) == 0)
        {
            RestrictFileAccessToCurrentAccountOnly(g_mpiSocket);
            
            if (listen(m_socketfd, 5) == 0)
            {
                OsConfigLogInfo(PlatformLog::Get(), "Listening on socket '%s'", g_mpiSocket);

                m_worker = std::thread(Server::Worker, std::ref(*this));
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

void Server::Stop()
{
    OsConfigLogInfo(PlatformLog::Get(), "Server stopped");

    m_exitSignal.set_value();
    m_worker.join();

    close(m_socketfd);
    unlink(g_mpiSocket);
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

            std::string sessionId = CreateGUID();
            MPI_HANDLE session = MpiOpen(clientName.c_str(), maxPayloadSizeBytes);

            if (session != nullptr)
            {
                g_sessions[sessionId] = session;

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
            OsConfigLogError(PlatformLog::Get(), "Invalid MpiClose request body: %s", requestPayload.c_str());
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

static void HandleClient(int connfd)
{
    char* uri = nullptr;
    char* requestPayload = nullptr;
    int contentLength = 0;
    std::string responsePayload;
    StatusCode status = StatusCode::OK;
    ssize_t bytes = 0;

    if (nullptr == (uri = ReadUriFromSocket(connfd, PlatformLog::Get())))
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to read request URI %d", connfd);
        return;
    }

    if (0 >= (contentLength = ReadHttpContentLengthFromSocket(connfd, PlatformLog::Get())))
    {
        OsConfigLogError(PlatformLog::Get(), "%s: failed to read HTTP Content-Length", uri);
        return;
    }

    if (nullptr != (requestPayload = new (std::nothrow) char[contentLength]))
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

        status = RouteRequest(uri, std::string(requestPayload, contentLength), responsePayload);
        FREE_MEMORY(requestPayload);
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "%s: failed to allocate memory for HTTP body, Content-Length %d", uri, contentLength);
    }

    std::string response = SerializeResponse(status, responsePayload);

    if (response.size() != static_cast<size_t>(bytes = write(connfd, response.c_str(), response.size())))
    {
        if (bytes < 0)
        {
            OsConfigLogError(PlatformLog::Get(), "%s: failed to write response to socket '%s'", uri, strerror(errno));
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "%s: failed to write response to socket '%d', bytes written %d", uri, static_cast<int>(response.size()), static_cast<int>(bytes));
        }
    }

    FREE_MEMORY(uri);
}

void Server::Worker(Server& server)
{
    int connfd = -1;
    std::future<void> exitSignal = server.m_exitSignal.get_future();

    while (exitSignal.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout)
    {
        if (0 <= (connfd = accept(server.m_socketfd, (struct sockaddr*)&server.m_addr, &server.m_socketlen)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PlatformLog::Get(), "Accepted connection %s '%d'", server.m_addr.sun_path, connfd);
            }

            HandleClient(connfd);

            if (0 != close(connfd))
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to close socket %s '%d'", g_mpiSocket, connfd);
            }
            else if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PlatformLog::Get(), "Closed connection %s '%d'", server.m_addr.sun_path, connfd);
            }
        }
    }
}

void MpiApiInitialize()
{
    PlatformLog::OpenLog();
    server.Listen();
}

void MpiApiShutdown()
{
    for (auto session : g_sessions)
    {
        MpiClose(session.second);
    }

    server.Stop();
    PlatformLog::CloseLog();
}