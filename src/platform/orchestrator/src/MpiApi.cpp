// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "MpiApi.h"
#include "Http.h"
#include "ModulesManager.h"

#include <sstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

// TODO: static const char* for all string constants used for parsing JSON
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

// TEMPORARY: global map of all the sessions (will be maintained by orchestrator)
static std::map<std::string, MPI_HANDLE> g_sessions;

static Router router;
static Server server;

std::string GenerateRandomString(int length)
{
    std::string s;
    s.resize(length);
    for (int i = 0; i < length; i++)
    {
        s[i] = 'a' + (rand() % 26);
    }
    return s;
}

void ProcessMpiOpenRequest(const http::Request& request, http::Response& response)
{
    rapidjson::Document document;

    if (!document.Parse(request.body.c_str(), request.body.size()).HasParseError())
    {
        if (document.HasMember(g_clientName) && document.HasMember(g_maxPayloadSizeBytes) && document[g_clientName].IsString() && document[g_maxPayloadSizeBytes].IsInt())
        {
            std::string clientName = document[g_clientName].GetString();
            int maxPayloadSizeBytes = document[g_maxPayloadSizeBytes].GetInt();
            OsConfigLogInfo(PlatformLog::Get(), "Received MPI open request for client '%s' with max payload size %d", clientName.c_str(), maxPayloadSizeBytes);

            // Create unique session ID
            std::string sessionId = GenerateRandomString(16);

            // Create session
            MPI_HANDLE session = MpiOpen(clientName.c_str(), maxPayloadSizeBytes);
            g_sessions[sessionId] = session;

            response.SetStatus(http::StatusCode::OK);
            response.SetHeader(g_contentType, g_contentTypeJson);
            response.SetHeader(g_contentLength, std::to_string(sessionId.size()));
            // response.SetBody("\"" + sessionId + "\"");
            response.SetBody(sessionId);
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Invalid MpiOpen request");
        }
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to parse MpiOpen request");
    }
}

// void ProcessMpiCloseRequest(rapidjson::Value& request, http::Response& response)
// {
//     if (request.HasMember(g_clientSession) && request[g_clientSession].IsString())
//     {
//         std::string session = request[g_clientSession].GetString();
//         OsConfigLogInfo(PlatformLog::Get(), "Received MPI close request for session '%s'", session.c_str());

//         // TODO: validate the session ID
//         if (g_sessions.find(session) != g_sessions.end())
//         {
//             MpiClose(g_sessions[session]);
//             g_sessions.erase(session);
//         }
//         else
//         {
//             OsConfigLogError(PlatformLog::Get(), "Invalid MPI close request");
//         }

//         response.SetStatus(http::StatusCode::OK);
//         response.SetHeader(g_contentType, g_contentTypeJson);
//         response.SetBody("{\"status\": \"OK\"}");
//     }
//     else
//     {
//         OsConfigLogError(PlatformLog::Get(), "Invalid MPI close request");
//     }
// }

// void ProcessMpiSetRequest(rapidjson::Value& request, http::Response& response)
// {
//     if (request.HasMember(g_clientSession) && request[g_clientSession].IsString() && request.HasMember(g_componentName) && request[g_componentName].IsString() && request.HasMember(g_objectName) && request[g_objectName].IsString() && request.HasMember(g_payload) && request[g_payload].IsString())
//     {
//         std::string session = request[g_clientSession].GetString();
//         std::string component = request[g_componentName].GetString();
//         std::string object = request[g_objectName].GetString();

//         // Serialize request[g_payload] to string
//         rapidjson::StringBuffer buffer;
//         rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
//         request[g_payload].Accept(writer);
//         std::string payload = buffer.GetString();

//         // TODO: full logging only
//         OsConfigLogInfo(PlatformLog::Get(), "Received MPI set request for session '%s' component '%s' object '%s' payload '%s'", session.c_str(), component.c_str(), object.c_str(), payload.c_str());

//         if (g_sessions.find(session) != g_sessions.end())
//         {
//             int status = MpiSet(g_sessions[session], component.c_str(), object.c_str(), (MPI_JSON_STRING)payload.c_str(), payload.size());
//             response.SetStatus((status == MPI_OK) ? http::StatusCode::OK : http::StatusCode::BAD_REQUEST);
//             response.SetHeader(g_contentType, g_contentTypeJson);
//             response.SetBody("{\"status\": \"" + std::to_string(status) + "\"}");
//         }
//         else
//         {
//             OsConfigLogError(PlatformLog::Get(), "Invalid MPI set request");
//         }
//     }
//     else
//     {
//         OsConfigLogError(PlatformLog::Get(), "Invalid MPI set request");
//     }
// }

// void ProcessMpiGetRequest(rapidjson::Value& request, http::Response& response)
// {
//     if (request.HasMember(g_clientSession) && request[g_clientSession].IsString() && request.HasMember(g_componentName) && request[g_componentName].IsString() && request.HasMember(g_objectName) && request[g_objectName].IsString())
//     {
//         std::string session = request[g_clientSession].GetString();
//         std::string component = request[g_componentName].GetString();
//         std::string object = request[g_objectName].GetString();

//         OsConfigLogInfo(PlatformLog::Get(), "Received MPI get request for session '%s' component '%s' object '%s'", session.c_str(), component.c_str(), object.c_str());

//         if (g_sessions.find(session) != g_sessions.end())
//         {
//             MPI_JSON_STRING payload;
//             int payloadSizeBytes = 0;
//             int status = MpiGet(g_sessions[session], component.c_str(), object.c_str(), &payload, &payloadSizeBytes);

//             std::string payloadString = std::string(payload, payloadSizeBytes);

//             response.SetStatus((status == MPI_OK) ? http::StatusCode::OK : http::StatusCode::BAD_REQUEST);
//             response.SetHeader(g_contentType, g_contentTypeJson);
//             response.SetBody("{\"payload\":" + payloadString + "}");
//         }
//         else
//         {
//             OsConfigLogError(PlatformLog::Get(), "Invalid MPI get request");
//         }
//     }
//     else
//     {
//         OsConfigLogError(PlatformLog::Get(), "Invalid MPI get request");
//     }
// }

void MpiApiInitialize()
{
    PlatformLog::OpenLog();

    router.Post("/mpiopen", ProcessMpiOpenRequest);
    router.Post("/mpiclose", [](const http::Request& request, http::Response& response) {
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
    });

    router.Post("/mpiset", [](const http::Request& request, http::Response& response) {
        rapidjson::Document document;

        if (!document.Parse(request.body.c_str(), request.body.size()).HasParseError())
        {
            if (document.HasMember(g_clientSession) && document[g_clientSession].IsString() && document.HasMember(g_componentName) && document[g_componentName].IsString() && document.HasMember(g_objectName) && document[g_objectName].IsString() && document.HasMember(g_payload) && document[g_payload].IsString())
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
                    OsConfigLogError(PlatformLog::Get(), "Invalid MPI set request");
                }
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Invalid MPI set request");
            }
        }
    });

    router.Get("/mpiget", [](const http::Request& request, http::Response& response) {
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
    });

    router.Post("/mpisetdesired", [](const http::Request& request, http::Response& response) {
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
    });

    router.Get("/mpigetreported", [](const http::Request& request, http::Response& response) {
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
                    response.SetBody("{\"payload\":" + payloadString + "}");
                }
                else
                {
                    OsConfigLogError(PlatformLog::Get(), "Invalid MPI get desired request");
                }
            }
        }
    });

    server.Listen(router);
}

void MpiApiShutdown()
{
    // TODO: close all open sessions via MpiClose()
    server.Stop();
    PlatformLog::CloseLog();
}

void Router::Get(const std::string& uri, const Handler& handler)
{
    AddRoute(http::Method::GET, uri, handler);
}

void Router::Post(const std::string& uri, const Handler& handler)
{
    AddRoute(http::Method::POST, uri, handler);
}

void Router::AddRoute(const http::Method method, const std::string& uri, const Handler& handler)
{
    if (m_routes.find(uri) != m_routes.end())
    {
        // Check if the method already exists for the uri
        if (m_routes[uri].find(method) != m_routes[uri].end())
        {
            // TODO: log error and change to not throw
            throw std::runtime_error("Route already exists for method and uri");
        }
    }
    else
    {
        m_routes[uri] = std::map<http::Method, Handler>();
    }

    m_routes[uri][method] = handler;
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

void Server::Listen(Router& router)
{
    struct stat st;
    if (stat(g_socketPrefix, &st) == -1)
    {
        // TODO: use modes for root user
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

                m_worker = std::thread(&Server::Worker, std::ref(*this), std::ref(router));
            }
            else
            {
                OsConfigLogError(PlatformLog::Get(), "Failed to listen on IPC socket: '%s'", g_mpiSocket);
            }
        }
        else
        {
            OsConfigLogError(PlatformLog::Get(), "Failed to bind IPC socket: '%s'", g_mpiSocket);
        }
    }
    else
    {
        OsConfigLogError(PlatformLog::Get(), "Failed to create IPC socket: '%s'", g_mpiSocket);
    }
}

void Server::Stop()
{
    m_exitSignal.set_value();
    m_worker.join();

    OsConfigLogInfo(PlatformLog::Get(), "Server stopped");

    close(sockfd);
    unlink(g_mpiSocket);
}

void Server::Worker(Server& server, Router& router)
{
    int connfd = -1;
    std::future<void> exitSignal = server.m_exitSignal.get_future();

    while (exitSignal.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
    {
        // OsConfigLogInfo(PlatformLog::Get(), "Waiting for IPC connection");

        if (0 <= (connfd = accept(server.sockfd, (struct sockaddr*)&server.addr, &server.socklen)))
        {
            OsConfigLogInfo(PlatformLog::Get(), "Accepted connection: '%s' %d", server.addr.sun_path, connfd);

            // TODO: read the request dynamically, do not use a fixed size buffer
            char buffer[1024];
            ssize_t bytesRead = read(connfd, buffer, 1024);
            if (bytesRead > 0)
            {
                http::Request request = http::Request::Parse(std::string(buffer, bytesRead));

                // TODO: full logging only
                OsConfigLogInfo(PlatformLog::Get(), "Received HTTP request %d %s %s", (int)(request.method), request.uri.c_str(), request.body.c_str());

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
                OsConfigLogError(PlatformLog::Get(), "Failed to read request");
            }

            // TODO: check return value
            close(connfd);

            // TODO: full logging only
            OsConfigLogInfo(PlatformLog::Get(), "Closed connection: '%s' %d", server.addr.sun_path, connfd);
        }

        // TODO: may want to sleep here between each accept() call
    }

    OsConfigLogInfo(PlatformLog::Get(), "IPC server stopped");
}

// TEMPORARY MAIN FOR TESTING
int main()
{
    MpiInitialize();
    MpiApiInitialize();

    // sleep
    std::this_thread::sleep_for(std::chrono::seconds(100000));

    MpiApiShutdown();
    MpiShutdown();
}