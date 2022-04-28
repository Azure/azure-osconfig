// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <sstream>
#include <iostream>
#include <iomanip>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "CommonUtils.h"
#include "MpiApi.h"
#include "Http.h"
#include "ModulesManager.h"

static const char* g_clientName = "ClientName";
static const char* g_maxPayloadSizeBytes = "MaxPayloadSizeBytes";
static const char* g_clientSession = "ClientSession";
static const char* g_componentName = "ComponentName";
static const char* g_objectName = "ObjectName";
static const char* g_payload = "Payload";

static const char* g_contentLength = "Content-Length";
static const char* g_contentType = "Content-Type";
static const char* g_contentTypeJson = "application/json";

static const char* g_socketPrefix = "/run/osconfig";
static const char* g_mpiSocket = "/run/osconfig/mpid.sock";

static const std::string g_moduleDir = "/usr/lib/osconfig";
static const std::string g_configJson = "/etc/osconfig/osconfig.json";

static const size_t g_maxPayloadSize = 4096;

static std::map<std::string, MPI_HANDLE> g_sessions;

static Router router;
static Server server;

std::string CreateGUID()
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << std::time(nullptr);
    ss << std::setw(4) << std::rand();
    return ss.str();
}

void MpiOpenRequest(const http::Request& request, http::Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.body.c_str(), request.body.size()).HasParseError())
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

                response.SetStatus(http::StatusCode::OK);
                response.SetHeader(g_contentType, g_contentTypeJson);
                response.SetHeader(g_contentLength, std::to_string(sessionId.size()));
                response.SetBody("\"" + sessionId + "\"");
            }
            else
            {
                response.SetHeader(g_contentLength,std::to_string( std::string("\"\"").size()));
                response.SetBody("error");
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
        OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiOpen request: %s", request.body.c_str());
    }
}

void MpiCloseRequest(const http::Request& request, http::Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.body.c_str(), request.body.size()).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString())
        {
            std::string session = document[g_clientSession].GetString();
            OsConfigLogInfo(PlatformLog::Get(), "Received MPI close request for session '%s'", session.c_str());

            if (g_sessions.find(session) != g_sessions.end())
            {
                MpiClose(g_sessions[session]);
                g_sessions.erase(session);
                response.SetStatus(http::StatusCode::OK);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI close request");
                response.SetStatus(http::StatusCode::BAD_REQUEST);
            }
        }
    }
}

void MpiSetRequest(const http::Request& request, http::Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.body.c_str(), request.body.size()).HasParseError())
    {
        if (document.HasMember(g_clientSession) && document[g_clientSession].IsString() && document.HasMember(g_componentName) && document[g_componentName].IsString() && document.HasMember(g_objectName) && document[g_objectName].IsString() && document.HasMember(g_payload))
        {
            std::string session = document[g_clientSession].GetString();
            std::string component = document[g_componentName].GetString();
            std::string object = document[g_objectName].GetString();

            // Serialize request[g_payload] to string
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

                response.SetStatus((status == MPI_OK) ? http::StatusCode::OK : http::StatusCode::BAD_REQUEST);
                response.SetHeader(g_contentType, g_contentTypeJson);
                response.SetHeader(g_contentLength, std::to_string(responsePayload.size()));
                response.SetBody(responsePayload);
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

void MpiGetRequest(const http::Request& request, http::Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.body.c_str(), request.body.size()).HasParseError())
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

                response.SetStatus((status == MPI_OK) ? http::StatusCode::OK : http::StatusCode::BAD_REQUEST);
                response.SetHeader(g_contentType, g_contentTypeJson);
                response.SetHeader(g_contentLength, std::to_string(payloadString.size()));
                response.SetBody(payloadString);
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

void MpiSetDesiredRequest(const http::Request& request, http::Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.body.c_str(), request.body.size()).HasParseError())
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

                response.SetStatus((status == MPI_OK) ? http::StatusCode::OK : http::StatusCode::BAD_REQUEST);
                response.SetHeader(g_contentType, g_contentTypeJson);
                response.SetHeader(g_contentLength, std::to_string(responsePayload.size()));
                response.SetBody(responsePayload);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI set desired request");
            }
        }
    }
}

void MpiGetReportedRequest(const http::Request& request, http::Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.body.c_str(), request.body.size()).HasParseError())
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

                response.SetStatus((status == MPI_OK) ? http::StatusCode::OK : http::StatusCode::BAD_REQUEST);
                response.SetHeader(g_contentType, g_contentTypeJson);
                response.SetBody(payloadString);
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI get desired request");
            }
        }
    }
}

void MpiApiInitialize()
{
    PlatformLog::OpenLog();

    router.Post("/MpiOpen/", MpiOpenRequest);
    router.Post("/MpiClose/", MpiCloseRequest);
    router.Post("/MpiSet/", MpiSetRequest);
    router.Post("/MpiGet/", MpiGetRequest);
    router.Post("/MpiSetDesired/", MpiSetDesiredRequest);
    router.Post("/MpiGetReported/", MpiGetReportedRequest);

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

void MpiDoWork()
{
    OsConfigLogInfo(PlatformLog::Get(), "MPI API thread started");
    server.DoWork(router);
}

int Router::Post(const std::string& uri, const Handler& handler)
{
    return AddRoute(http::Method::POST, uri, handler);
}

int Router::AddRoute(const http::Method method, const std::string& uri, const Handler& handler)
{
    int status = 0;

    if (m_routes.find(uri) != m_routes.end())
    {
        // Check if the method already exists for the uri
        if (m_routes[uri].find(method) != m_routes[uri].end())
        {
            OsConfigLogError(PlatformLog::Get(), "Route already exists for method and uri");
            status = EINVAL;
        }
    }
    else
    {
        m_routes[uri] = std::map<http::Method, Handler>();
    }

    if (status == 0)
    {
        m_routes[uri][method] = handler;
    }

    return status;
}

http::Response Router::HandleRequest(const http::Request& request)
{
    http::Response response;

    if (m_routes.find(request.uri) != m_routes.end())
    {
        if (m_routes[request.uri].find(request.method) != m_routes[request.uri].end())
        {
            OsConfigLogInfo(PlatformLog::Get(), "Received request for uri '%s' method '%d'", request.uri.c_str(), (int)request.method);
            // TODO: validate request (Host/Accept/Content-Type)
            m_routes[request.uri][request.method](request, response);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Invalid request method '%d' for uri '%s'", (int)request.method, request.uri.c_str());
            response.SetStatus(http::StatusCode::NOT_FOUND);
        }
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "Invalid request for uri '%s'", request.uri.c_str());
        response.SetStatus(http::StatusCode::NOT_FOUND);
    }

    return response;
}

void Server::Listen()
{
    struct stat st;
    if (stat(g_socketPrefix, &st) == -1)
    {
        mkdir(g_socketPrefix, 0700);
    }

    if (0 <= (sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)))
    {
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, g_mpiSocket, sizeof(addr.sun_path) - 1);
        socklen = sizeof(addr);

        // Unlink socket if already in-use
        unlink(g_mpiSocket);

        if (bind(sockfd, (struct sockaddr*)&addr, socklen) == 0)
        {
            if (listen(sockfd, 5) == 0)
            {
                OsConfigLogInfo(PlatformLog::Get(), "Listening on socket: '%s'", g_mpiSocket);
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

    close(sockfd);
    unlink(g_mpiSocket);
}

void Server::DoWork(Router& router)
{
    int status = 0;
    int connfd = -1;
    if (0 <= (connfd = accept(server.sockfd, (struct sockaddr*)&server.addr, &server.socklen)))
    {
        OsConfigLogInfo(PlatformLog::Get(), "Accepted connection: '%s' %d", server.addr.sun_path, connfd);

        // TODO: read the request dynamically, do not use a fixed size buffer
        char buffer[g_maxPayloadSize];
        ssize_t bytesRead = read(connfd, buffer, g_maxPayloadSize);

        if (bytesRead > 0)
        {
            OsConfigLogInfo(PlatformLog::Get(), "Read %d bytes from socket", (int)bytesRead);
            OsConfigLogInfo(PlatformLog::Get(), "Recieved HTTP request:\n%s\n", std::string(buffer, bytesRead).c_str());

            http::Request request;
            if (0 == (status = http::Request::Parse(std::string(buffer, bytesRead), request)))
            {
                // Route the request to the correct handler
                http::Response response = router.HandleRequest(request);

                std::time_t t = std::time(NULL);
                struct tm *tm = std::localtime(&t);
                char now[64];
                std::strftime(now, sizeof(now), "%c", tm);

                // TEMPORARY: static response header
                std::stringstream ss;
                ss << "HTTP/1.1" << " 200 OK" << http::CRLF;
                ss << "Date: " << now << http::CRLF;
                ss << "Server: OSConfig" << http::CRLF;
                ss << "Content-Type: " << g_contentTypeJson << http::CRLF;
                ss << "Content-Length: " << response.body.size() << http::CRLF;
                ss << "Connection: Closed" << http::CRLF << http::CRLF;
                ss << response.body.c_str();
                std::string s = ss.str();

                OsConfigLogInfo(PlatformLog::Get(), "Sending HTTP response:\n%s\n", s.c_str());

                int n = write(connfd, s.c_str(), s.size());
                if (n != (int)s.size())
                {
                    OsConfigLogError(PlatformLog::Get(), "Failed to write response to socket: '%s' '%d'", g_mpiSocket, connfd);
                }
                else
                {
                    OsConfigLogInfo(PlatformLog::Get(), "Sent response to socket: '%s' '%d'", g_mpiSocket, connfd);
                }
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to parse HTTP request: '%s' '%d'", g_mpiSocket, connfd);
            }
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to read request");
        }

        if (0 != close(connfd))
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to close socket: '%s' '%d'", g_mpiSocket, connfd);
        }
        else if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(PlatformLog::Get(), "Closed connection: '%s' %d", server.addr.sun_path, connfd);
        }
    }

    OsConfigLogInfo(PlatformLog::Get(), "MPI server stopped");
}