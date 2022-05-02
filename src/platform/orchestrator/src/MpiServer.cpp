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

struct Request
{
    std::string m_uri;
    std::string m_body;
};

struct Response
{
    StatusCode m_status;
    std::string m_body;

    std::string ToString();
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

static void MpiOpenRequest(const Request& request, Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.m_body.c_str(), request.m_body.size()).HasParseError())
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

                response.m_status = StatusCode::OK;
                response.m_body = ("\"" + sessionId + "\"");
            }
            else
            {
                response.m_body = "\"\"";
                OsConfigLogError(PlatformLog::Get(), "Failed to create MPI session for client '%s'", clientName.c_str());
            }
        }
        else
        {
            response.m_status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiOpen request: %s", request.m_body.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiOpen request");
            }
        }
    }
    else
    {
        response.m_status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiOpen request: %s", request.m_body.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiOpen request");
        }
    }
}

static void MpiCloseRequest(const Request& request, Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.m_body.c_str(), request.m_body.size()).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString())
        {
            std::string session = document[g_clientSession].GetString();
            OsConfigLogInfo(PlatformLog::Get(), "Received MpiClose request for session '%s'", session.c_str());

            if (g_sessions.find(session) != g_sessions.end())
            {
                MpiClose(g_sessions[session]);
                g_sessions.erase(session);
                response.m_status = StatusCode::OK;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI close request");
                response.m_status = StatusCode::BAD_REQUEST;
            }
        }
        else
        {
            response.m_status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiClose request: %s", request.m_body.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiClose request");
            }
            OsConfigLogError(PlatformLog::Get(), "Invalid MpiClose request body: %s", request.m_body.c_str());
        }
    }
    else
    {
        response.m_status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiClose request: %s", request.m_body.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiClose request");
        }
    }
}

static void MpiSetRequest(const Request& request, Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.m_body.c_str(), request.m_body.size()).HasParseError())
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
                int status = MpiSet(g_sessions[session], component.c_str(), object.c_str(), (MPI_JSON_STRING)payload.c_str(), payload.size());
                response.m_status = ((status == MPI_OK) ? StatusCode::OK : StatusCode::BAD_REQUEST);
                response.m_body = "\""+ std::to_string(status) + "\"";
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "No session found for MpiSet request: %s", session.c_str());
                response.m_status = StatusCode::BAD_REQUEST;
            }
        }
        else
        {
            response.m_status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSet request: %s", request.m_body.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSet request");
            }
        }
    }
    else
    {
        response.m_status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSet request: %s", request.m_body.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSet request");
        }
    }
}

static void MpiGetRequest(const Request& request, Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.m_body.c_str(), request.m_body.size()).HasParseError())
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
                int status = MpiGet(g_sessions[session], component.c_str(), object.c_str(), &payload, &payloadSizeBytes);

                if (status == MPI_OK)
                {
                    std::string responsePayload = std::string(payload, payloadSizeBytes);
                    response.m_status = StatusCode::OK;
                    response.m_body = responsePayload;
                }
                else
                {
                    response.m_status = StatusCode::BAD_REQUEST;
                    response.m_body = "\"" + std::to_string(status) + "\"";
                }
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGet request: %s", session.c_str());
                response.m_status = StatusCode::BAD_REQUEST;
            }
        }
        else
        {
            response.m_status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGet request: %s", request.m_body.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGet request");
            }
        }
    }
    else
    {
        response.m_status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGet request: %s", request.m_body.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGet request");
        }
    }
}

static void MpiSetDesiredRequest(const Request& request, Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.m_body.c_str(), request.m_body.size()).HasParseError())
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
                int status = MpiSetDesired(g_sessions[session], (MPI_JSON_STRING)payload.c_str(), payload.size());
                response.m_status = ((status == MPI_OK) ? StatusCode::OK : StatusCode::BAD_REQUEST);
                response.m_body = "\"" + std::to_string(status) + "\"";
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSetDesired request");
                response.m_status = StatusCode::BAD_REQUEST;
            }
        }
        else
        {
            response.m_status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSetDesired request: %s", request.m_body.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiSetDesired request");
            }
        }
    }
    else
    {
        response.m_status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSetDesired request: %s", request.m_body.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiSetDesired request");
        }
    }
}

static void MpiGetReportedRequest(const Request& request, Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.m_body.c_str(), request.m_body.size()).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString())
        {
            std::string session = document[g_clientSession].GetString();

            if (g_sessions.find(session) != g_sessions.end())
            {
                MPI_JSON_STRING payload;
                int payloadSizeBytes = 0;
                int status = MpiGetReported(g_sessions[session], &payload, &payloadSizeBytes);

                if (status == MPI_OK)
                {
                    std::string responsePayload = std::string(payload, payloadSizeBytes);
                    response.m_status = StatusCode::OK;
                    response.m_body = responsePayload;
                }
                else
                {
                    response.m_status = StatusCode::BAD_REQUEST;
                    response.m_body = "\"" + std::to_string(status) + "\"";
                }
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGetReported request");
                response.m_status = StatusCode::BAD_REQUEST;
            }
        }
        else
        {
            response.m_status = StatusCode::BAD_REQUEST;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGetReported request: %s", request.m_body.c_str());
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MpiGetReported request");
            }
        }
    }
    else
    {
        response.m_status = StatusCode::BAD_REQUEST;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGetReported request: %s", request.m_body.c_str());
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiGetReported request");
        }
    }
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

std::string Response::ToString()
{
    std::string result;
    result += g_httpVersion + " " + std::to_string(static_cast<int>(m_status)) + " " + StatusText(m_status) + g_CRLF;
    result += g_contentTypeJson + g_CRLF;
    result += g_contentLength + std::to_string(m_body.size()) + g_CRLF;
    result += g_CRLF;
    result += m_body;
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
            if (listen(m_socketfd, 5) == 0)
            {
                OsConfigLogInfo(PlatformLog::Get(), "Listening on socket: '%s'", g_mpiSocket);

                m_worker = std::thread(Server::Worker, std::ref(*this));
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to listen on socket: '%s'", g_mpiSocket);
            }
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to bind socket: '%s'", g_mpiSocket);
        }
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to create socket: '%s'", g_mpiSocket);
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

static bool ReadRequest(int connfd, Request& request)
{
    bool success = true;
    char* uri = ReadUriFromSocket(connfd, PlatformLog::Get());
    int contentLength = 0;

    if (uri == nullptr)
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to read request URI");
        success = false;
    }
    else if (0 >= (contentLength = ReadHttpContentLengthFromSocket(connfd, PlatformLog::Get())))
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to read HTTP Content-Length");
        success = false;
    }
    else
    {
        request.m_uri = uri;
        ssize_t bytesRead = 0;
        char* buffer = new (std::nothrow) char[contentLength];

        if (nullptr != buffer)
        {
            if (contentLength != (bytesRead = read(connfd, buffer, contentLength)))
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to read complete HTTP body: Content-Length %d, bytes read %d", contentLength, (int)bytesRead);
                success = false;
            }

            request.m_body = std::string(buffer, contentLength);
            FREE_MEMORY(buffer);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to allocate memory for HTTP body");
            success = false;
        }
    }

    return success;
}

static bool SendResponse(int connfd, Response response)
{
    bool success = true;
    std::string payload = response.ToString();
    ssize_t bytesWritten = 0, payloadSize = payload.size();

    if (payloadSize != (bytesWritten = write(connfd, payload.c_str(), payloadSize)))
    {
        success = false;
        if (bytesWritten < 0)
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to write response to socket: %s", strerror(errno));
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to write response to socket: %d, bytes written %d", static_cast<int>(payload.size()), static_cast<int>(bytesWritten));
        }
    }

    return success;
}

static Response HandleRequest(const Request& request)
{
    Response response;

    if (request.m_uri == "MpiOpen")
    {
        MpiOpenRequest(request, response);
    }
    else if (request.m_uri == "MpiClose")
    {
        MpiCloseRequest(request, response);
    }
    else if (request.m_uri == "MpiSet")
    {
        MpiSetRequest(request, response);
    }
    else if (request.m_uri == "MpiGet")
    {
        MpiGetRequest(request, response);
    }
    else if (request.m_uri == "MpiSetDesired")
    {
        MpiSetDesiredRequest(request, response);
    }
    else if (request.m_uri == "MpiGetReported")
    {
        MpiGetReportedRequest(request, response);
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "Invalid request for uri '%s'", request.m_uri.c_str());
        response.m_status = StatusCode::NOT_FOUND;
    }

    return response;
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
                OsConfigLogInfo(PlatformLog::Get(), "Accepted connection: %s %d", server.m_addr.sun_path, connfd);
            }

            Request request;
            Response response;
            if (ReadRequest(connfd, request))
            {
                response = HandleRequest(request);
            }
            else
            {
                response.m_status = StatusCode::BAD_REQUEST;
            }

            if (!SendResponse(connfd, response))
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to send response to socket:  %s %d", server.m_addr.sun_path, connfd);
            }

            if (0 != close(connfd))
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to close socket: %s %d", g_mpiSocket, connfd);
            }
            else if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PlatformLog::Get(), "Closed connection: %s %d", server.m_addr.sun_path, connfd);
            }
        }
    }
}