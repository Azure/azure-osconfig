// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MPI_API_H
#define MPI_API_H

#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

#include "Logging.h"
#include "Http.h"

class Router
{
public:
    using Handler = std::function<void(const http::Request&, http::Response&)>;

    Router() = default;
    ~Router() = default;

    int Post(const std::string& path, const Handler& handler);
    http::Response HandleRequest(const http::Request& request);
private:
    std::map<std::string, std::map<http::Method, Handler>> m_routes;

    int AddRoute(const http::Method method, const std::string& path, const Handler& handler);
};

class Server
{
public:
    Server() = default;
    ~Server() = default;

    void Listen();
    void DoWork(Router& router);
    void Stop();

private:
    int sockfd;
    struct sockaddr_un addr;
    socklen_t socklen;
};

#endif // MPI_API_H