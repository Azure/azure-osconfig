// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "CommonUtils.h"
#include "MpiApi.h"
#include "ModulesManager.h"

static const char* g_socketPrefix = "/run/osconfig";
static const char* g_mpiSocket = "/run/osconfig/mpid.sock";

static const char* g_clientName = "ClientName";
static const char* g_maxPayloadSizeBytes = "MaxPayloadSizeBytes";
static const char* g_clientSession = "ClientSession";
static const char* g_componentName = "ComponentName";
static const char* g_objectName = "ObjectName";
static const char* g_payload = "Payload";

// TODO: use dynamic memory allocation
static const size_t g_maxPayloadSize = 4096;

static std::map<std::string, MPI_HANDLE> g_sessions;

static Server server;

OSCONFIG_LOG_HANDLE PlatformLog::m_log = nullptr;

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

static void MpiOpenRequest(const http::Request& request, http::Response& response)
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

                response.status = http::StatusCode::OK;
                // response.headers[http::contentType] = http::contentTypeJson;
                response.headers[http::contentLength] = std::to_string(sessionId.size() + 2);
                response.body = ("\"" + sessionId + "\"");
            }
            else
            {
                response.headers[http::contentLength] = std::to_string( std::string("\"\"").size());
                response.body = "\"\"";
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

static void MpiCloseRequest(const http::Request& request, http::Response& response)
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
                response.status = http::StatusCode::OK;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI close request");
                response.status = http::StatusCode::BAD_REQUEST;
            }
        }
    }
}

static void MpiSetRequest(const http::Request& request, http::Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.body.c_str(), request.body.size()).HasParseError())
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

                response.status = ((status == MPI_OK) ? http::StatusCode::OK : http::StatusCode::BAD_REQUEST);
                // response.headers[http::contentType] = http::contentTypeJson;
                response.headers[http::contentLength] = std::to_string(responsePayload.size());
                response.body = responsePayload;
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

static void MpiGetRequest(const http::Request& request, http::Response& response)
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

                response.status = ((status == MPI_OK) ? http::StatusCode::OK : http::StatusCode::BAD_REQUEST);
                // response.headers[http::contentType] = http::contentTypeJson;
                response.headers[http::contentLength] = std::to_string(payloadString.size());
                response.body = payloadString;
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

static void MpiSetDesiredRequest(const http::Request& request, http::Response& response)
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

                response.status = ((status == MPI_OK) ? http::StatusCode::OK : http::StatusCode::BAD_REQUEST);
                // response.headers[http::contentType] = http::contentTypeJson;
                response.headers[http::contentLength] = std::to_string(responsePayload.size());
                response.body = responsePayload;
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI set desired request");
            }
        }
    }
}

static void MpiGetReportedRequest(const http::Request& request, http::Response& response)
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

                response.status = ((status == MPI_OK) ? http::StatusCode::OK : http::StatusCode::BAD_REQUEST);
                // response.headers[http::contentType] = http::contentTypeJson;
                response.body = payloadString;
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

std::string StatusText(http::StatusCode statusCode)
{
    std::string result;
    switch (statusCode)
    {
        case http::StatusCode::OK:
            result = "OK";
            break;
        case http::StatusCode::BAD_REQUEST:
            result = "BAD_REQUEST";
            break;
        case http::StatusCode::NOT_FOUND:
            result = "NOT_FOUND";
            break;
        default:
            result = "INTERNAL_SERVER_ERROR";
            break;
    }
    return result;
}

static http::Method MethodFromString(const std::string& string)
{
    if (string == "GET")
    {
        return http::Method::GET;
    }
    else if (string == "POST")
    {
        return http::Method::POST;
    }
    else if (string == "PUT")
    {
        return http::Method::PUT;
    }
    else if (string == "DELETE")
    {
        return http::Method::DELETE;
    }
    else
    {
        return http::Method::INVALID;
    }
}

bool http::Request::Parse(const std::string& request)
{
    bool result = false;

    // Split the request into header and body
    size_t headerEnd = request.find(http::CRLF + http::CRLF);
    if (headerEnd == std::string::npos)
    {
        result = false;
    }
    else
    {
        std::string header = request.substr(0, headerEnd);
        body = request.substr(headerEnd + (2 * http::CRLF.length()));

        // Split the header into lines
        std::vector<std::string> lines = Split(header, http::CRLF);

        // Parse the first line
        std::vector<std::string> requestLine = Split(lines[0], " ");
        if (requestLine.size() != 3)
        {
            result = false;
        }
        else
        {
            method = MethodFromString(requestLine[0]);
            uri = requestLine[1];
            version = requestLine[2];

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
                    headers[headerLine[0]] = headerLine[1];
                }
            }

            // Parse the body
            // if (body.length() > 0)
            // {
            //     if (headers.find(http::contentType) != headers.end())
            //     {
            //         if (headers[http::contentType] == http::contentTypeJson)
            //         {
            //             rapidjson::Document document;
            //             if (!document.Parse(body.c_str(), body.size()).HasParseError())
            //             {
            //                 body = document.GetString();
            //             }
            //             else
            //             {
            //                 result = false;
            //             }
            //         }
            //     }
            // }

            // this->body = body;
            result = true;
        }
    }

    return result;
}

std::string http::Response::ToString()
{
    std::string result;

    result += "HTTP/1.1 " + std::to_string(static_cast<int>(status)) + " " + StatusText(status) + http::CRLF;

    for (auto header : headers)
    {
        result += header.first + ": " + header.second + http::CRLF;
    }

    result += http::CRLF;
    result += body;

    return result;
}

void Server::Listen()
{
    struct stat st;
    if (stat(g_socketPrefix, &st) == -1)
    {
        mkdir(g_socketPrefix, 0700);
    }

    if (0 <= (socketfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)))
    {
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, g_mpiSocket, sizeof(addr.sun_path) - 1);
        socketlen = sizeof(addr);

        // Unlink socket if already in-use
        unlink(g_mpiSocket);

        if (bind(socketfd, (struct sockaddr*)&addr, socketlen) == 0)
        {
            if (listen(socketfd, 5) == 0)
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

    close(socketfd);
    unlink(g_mpiSocket);
}

static bool ReadRequestFromSocket(int connfd, http::Request& request)
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

static http::Response HandleRequest(const http::Request& request)
{
    http::Response response;

    if (request.method == http::Method::POST)
    {
        if (request.uri == "/MpiOpen/")
        {
            MpiOpenRequest(request, response);
        }
        else if (request.uri == "/MpiClose/")
        {
            MpiCloseRequest(request, response);
        }
        else if (request.uri == "/MpiSet/")
        {
            MpiSetRequest(request, response);
        }
        else if (request.uri == "/MpiGet/")
        {
            MpiGetRequest(request, response);
        }
        else if (request.uri == "/MpiSetDesired/")
        {
            MpiSetDesiredRequest(request, response);
        }
        else if (request.uri == "/MpiGetReported/")
        {
            MpiGetReportedRequest(request, response);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Invalid request for uri '%s'", request.uri.c_str());
            response.status = http::StatusCode::NOT_FOUND;
        }
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "Invalid request method '%d' for uri '%s'", (int)request.method, request.uri.c_str());
        response.status = http::StatusCode::NOT_FOUND;
    }

    return response;
}

void Server::Worker(Server& server)
{
    int connfd = -1;
    std::future<void> exitSignal = server.m_exitSignal.get_future();

    while (exitSignal.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout)
    {
        if (0 <= (connfd = accept(server.socketfd, (struct sockaddr*)&server.addr, &server.socketlen)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PlatformLog::Get(), "Accepted connection: %s %d", server.addr.sun_path, connfd);
            }

            http::Request request;
            if (ReadRequestFromSocket(connfd, request))
            {
                http::Response response = HandleRequest(request);
                std::string responseString = response.ToString();

                if (write(connfd, responseString.c_str(), responseString.size()) < 0)
                {
                    OsConfigLogError(PlatformLog::Get(), "Failed to write response to socket");
                }
                else if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(PlatformLog::Get(), "Sending response: %s %d (%d bytes)", server.addr.sun_path, connfd, (int)responseString.size());
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
                OsConfigLogInfo(PlatformLog::Get(), "Closed connection: '%s' %d", server.addr.sun_path, connfd);
            }
        }
    }
}