// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MPI_API_H
#define MPI_API_H

#include <cstdio>
#include <chrono>
#include <future>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

#include "Logging.h"

#define PLATFORM_LOGFILE "/var/log/osconfig_platform.log"
#define PLATFORM_ROLLEDLOGFILE "/var/log/osconfig_platform.bak"

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

namespace http
{
    const std::string CRLF = "\r\n";
    const std::string contentLength = "Content-Length";
    const std::string contentType = "Content-Type";
    const std::string contentTypeJson = "application/json";

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
} // namespace http

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

#endif // MPI_API_H