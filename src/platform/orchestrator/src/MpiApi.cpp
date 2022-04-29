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
#include "MpiApi.h"
#include "ModulesManager.h"
#include "Logging.h"

static const char* g_socketPrefix = "/run/osconfig";
static const char* g_mpiSocket = "/run/osconfig/mpid.sock";

const std::string CRLF = "\r\n";
const std::string contentLength = "Content-Length";
const std::string contentType = "Content-Type";
const std::string contentTypeJson = "application/json";

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

enum class Method
{
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    INVALID
};

enum class StatusCode
{
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500
};

struct Request
{
    Method m_method;
    std::string m_version;
    std::string m_uri;
    std::map<std::string, std::string> m_headers;
    std::string m_body;

    bool Parse(const std::string& request);
};

struct Response
{
    StatusCode m_status;
    std::map<std::string, std::string> m_headers;
    std::string m_body;

    std::string ToString();
};

class Server
{
public:
    Server() = default;
    ~Server() = default;

    void Listen();
    static void Worker(Server& server);
    void Stop();

private:
    int m_socketfd;
    struct sockaddr_un m_addr;
    socklen_t m_socketlen;

    std::thread m_worker;
    std::promise<void> m_exitSignal;
};

static Server server;

std::string CreateGUID()
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << std::time(nullptr);
    ss << std::setw(4) << std::rand();
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
            OsConfigLogInfo(PlatformLog::Get(), "Received MPI open request for client '%s' with max payload size %d", clientName.c_str(), maxPayloadSizeBytes);

            std::string sessionId = CreateGUID();
            MPI_HANDLE session = MpiOpen(clientName.c_str(), maxPayloadSizeBytes);

            if (session != nullptr)
            {
                g_sessions[sessionId] = session;

                response.m_status = StatusCode::OK;
                // response.m_headers[contentType] = contentTypeJson;
                response.m_headers[contentLength] = std::to_string(sessionId.size() + 2);
                response.m_body = ("\"" + sessionId + "\"");
            }
            else
            {
                response.m_headers[contentLength] = std::to_string( std::string("\"\"").size());
                response.m_body = "\"\"";
                OsConfigLogError(PlatformLog::Get(), "Failed to create MPI session for client '%s'", clientName.c_str());
            }
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Invalid MpiOpen request");
        }
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiOpen request: %s", request.m_body.c_str());
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
            OsConfigLogInfo(PlatformLog::Get(), "Received MPI close request for session '%s'", session.c_str());

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

            // TODO: full logging only
            OsConfigLogInfo(PlatformLog::Get(), "Received MPI set request for session '%s' component '%s' object '%s' payload '%s'", session.c_str(), component.c_str(), object.c_str(), payload.c_str());

            if (g_sessions.find(session) != g_sessions.end())
            {
                int status = MpiSet(g_sessions[session], component.c_str(), object.c_str(), (MPI_JSON_STRING)payload.c_str(), payload.size());
                std::string responsePayload = "\""+ std::to_string(status) + "\"";

                response.m_status = ((status == MPI_OK) ? StatusCode::OK : StatusCode::BAD_REQUEST);
                // response.headers[contentType] = contentTypeJson;
                response.m_headers[contentLength] = std::to_string(responsePayload.size());
                response.m_body = responsePayload;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "No session found for MPI set request: %s", session.c_str());
            }
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Invalid MPI set request");
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

            // TODO: full logging only
            OsConfigLogInfo(PlatformLog::Get(), "Received MPI get request for session '%s' component '%s' object '%s'", session.c_str(), component.c_str(), object.c_str());

            if (g_sessions.find(session) != g_sessions.end())
            {
                MPI_JSON_STRING payload;
                int payloadSizeBytes = 0;
                int status = MpiGet(g_sessions[session], component.c_str(), object.c_str(), &payload, &payloadSizeBytes);

                std::string payloadString = std::string(payload, payloadSizeBytes);

                response.m_status = ((status == MPI_OK) ? StatusCode::OK : StatusCode::BAD_REQUEST);
                // response.headers[contentType] = contentTypeJson;
                response.m_headers[contentLength] = std::to_string(payloadString.size());
                response.m_body = payloadString;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI get request");
            }
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Invalid MPI get request");
        }
    }
}

static void MpiSetDesiredRequest(const Request& request, Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.m_body.c_str(), request.m_body.size()).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString() && document.HasMember(g_payload) && document[g_payload].IsString())
        {
            std::string session = document[g_clientSession].GetString();

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            document[g_payload].Accept(writer);
            std::string payload = buffer.GetString();

            if (g_sessions.find(session) != g_sessions.end())
            {
                int status = MpiSetDesired(g_sessions[session], (MPI_JSON_STRING)payload.c_str(), payload.size());
                std::string responsePayload = "\"" + std::to_string(status) + "\"";

                response.m_status = ((status == MPI_OK) ? StatusCode::OK : StatusCode::BAD_REQUEST);
                // response.m_headers[contentType] = contentTypeJson;
                response.m_headers[contentLength] = std::to_string(responsePayload.size());
                response.m_body = responsePayload;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI set desired request");
            }
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

                std::string payloadString = std::string(payload, payloadSizeBytes);

                response.m_status = ((status == MPI_OK) ? StatusCode::OK : StatusCode::BAD_REQUEST);
                // response.m_headers[contentType] = contentTypeJson;
                response.m_body = payloadString;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI get desired request");
            }
        }
    }
}

static std::vector<std::string> Split(const std::string& string, const std::string& delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = 0;

    while ((end = string.find(delimiter, start)) != std::string::npos)
    {
        tokens.push_back(string.substr(start, end - start));
        start = end + delimiter.length();
    }

    tokens.push_back(string.substr(start));

    return tokens;
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

static Method MethodFromString(const std::string& string)
{
    if (string == "GET")
    {
        return Method::GET;
    }
    else if (string == "POST")
    {
        return Method::POST;
    }
    else if (string == "PUT")
    {
        return Method::PUT;
    }
    else if (string == "DELETE")
    {
        return Method::DELETE;
    }
    else
    {
        return Method::INVALID;
    }
}

bool Request::Parse(const std::string& request)
{
    bool result = false;

    // Split the request into header and body
    size_t headerEnd = request.find(CRLF + CRLF);
    if (headerEnd == std::string::npos)
    {
        result = false;
    }
    else
    {
        std::string header = request.substr(0, headerEnd);
        m_body = request.substr(headerEnd + (2 * CRLF.length()));

        // Split the header into lines
        std::vector<std::string> lines = Split(header, CRLF);

        // Parse the first line
        std::vector<std::string> requestLine = Split(lines[0], " ");
        if (requestLine.size() != 3)
        {
            result = false;
        }
        else
        {
            m_method = MethodFromString(requestLine[0]);
            m_uri = requestLine[1];
            m_version = requestLine[2];

            // Parse the header lines
            for (size_t i = 1; i < lines.size(); i++)
            {
                std::vector<std::string> headerLine = Split(lines[i], ": ");
                if (headerLine.size() != 2)
                {
                    continue;
                }
                else
                {
                    m_headers[headerLine[0]] = headerLine[1];
                }
            }

            result = true;
        }
    }

    return result;
}

std::string Response::ToString()
{
    std::string result;

    result += "HTTP/1.1 " + std::to_string(static_cast<int>(m_status)) + " " + StatusText(m_status) + CRLF;

    for (auto header : m_headers)
    {
        result += header.first + ": " + header.second + CRLF;
    }

    result += CRLF;
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

static bool ReadRequestFromSocket(int connfd, Request& request)
{
    bool result = false;
    // int bytesRead = 0;
    char buffer[4096];
    std::stringstream requestStream;

    read(connfd, buffer, sizeof(buffer));
    requestStream << buffer;
    // while (0 < (bytesRead = read(connfd, buffer, sizeof(buffer))))
    // {
    //     requestStream << std::string(buffer, bytesRead);
    // }

    // if (0 <= bytesRead)
    // {
    //     OsConfigLogError(PlatformLog::Get(), "Failed to read request from socket");
    // }
    // else
    if (requestStream.str().empty())
    {
        OsConfigLogError(PlatformLog::Get(), "Empty request from socket");
    }
    else if (!request.Parse(requestStream.str()))
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to parse request from socket");
    }
    else
    {
        result = true;
    }

    return result;
}

static Response HandleRequest(const Request& request)
{
    Response response;

    if (request.m_method == Method::POST)
    {
        if (request.m_uri == "/MpiOpen/")
        {
            MpiOpenRequest(request, response);
        }
        else if (request.m_uri == "/MpiClose/")
        {
            MpiCloseRequest(request, response);
        }
        else if (request.m_uri == "/MpiSet/")
        {
            MpiSetRequest(request, response);
        }
        else if (request.m_uri == "/MpiGet/")
        {
            MpiGetRequest(request, response);
        }
        else if (request.m_uri == "/MpiSetDesired/")
        {
            MpiSetDesiredRequest(request, response);
        }
        else if (request.m_uri == "/MpiGetReported/")
        {
            MpiGetReportedRequest(request, response);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Invalid request for uri '%s'", request.m_uri.c_str());
            response.m_status = StatusCode::NOT_FOUND;
        }
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "Invalid request method '%d' for uri '%s'", static_cast<int>(request.m_method), request.m_uri.c_str());
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
            if (ReadRequestFromSocket(connfd, request))
            {
                Response response = HandleRequest(request);
                std::string responseString = response.ToString();

                if (write(connfd, responseString.c_str(), responseString.size()) < 0)
                {
                    OsConfigLogError(PlatformLog::Get(), "Failed to write response to socket");
                }
                else if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(PlatformLog::Get(), "Sending response: %s %d (%d bytes)", server.m_addr.sun_path, connfd, (int)responseString.size());
                }
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to read request from socket");
            }

            if (0 != close(connfd))
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to close socket: '%s' '%d'", g_mpiSocket, connfd);
            }
            else if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PlatformLog::Get(), "Closed connection: '%s' %d", server.m_addr.sun_path, connfd);
            }
        }
    }
}