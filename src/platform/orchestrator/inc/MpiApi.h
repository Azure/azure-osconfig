// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MPI_API_H
#define MPI_API_H

#include <chrono>
#include <cstdio>
#include <future>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

#include "Logging.h"
#include "Http.h"

void MpiApiInitialize();
void MpiApiShutdown();

class Router
{
public:
    using Handler = std::function<void(const http::Request&, http::Response&)>;

    Router() = default;
    ~Router() = default;

    void Get(const std::string& path, const Handler& handler);
    void Post(const std::string& path, const Handler& handler);
    // Other request methods are not needed

    http::Response HandleRequest(const http::Request& request);
private:
    std::map<std::string, std::map<http::Method, Handler>> m_routes;
    void AddRoute(const http::Method method, const std::string& path, const Handler& handler);

};

class Server
{
    public:
        Server() = default;
        ~Server() = default;

        void Listen(Router& router);
        void DoWork(Router& router);
        void Stop();

private:
    int sockfd;
    struct sockaddr_un addr;
    socklen_t socklen;

    std::promise<void> m_exitSignal;
    std::thread m_worker;

    static void Worker(Server& server, Router& router);
    void SendResponse(http::Response& response, int clientSocket);

};

#endif // MPI_API_H