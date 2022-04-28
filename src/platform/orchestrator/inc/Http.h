// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// TODO: IFDEF for header
#ifndef MPI_HTTP_H
#define MPI_HTTP_H

#include <functional>
#include <cstring>
#include <string>
#include <map>
#include <thread>
#include <vector>

#include <Logging.h>

// TODO: reuse modules manager (platform) logger

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
    const char* const CRLF = "\r\n";

    enum class Method
    {
        UNKNOWN,
        GET,
        POST,
        PUT,
        DELETE,
        PATCH
    };

    enum class Version
    {
        UNKNOWN,
        HTTP_1_0,
        HTTP_1_1,
        HTTP_2_0
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
        Method method;
        Version version;
        std::string uri;
        std::map<std::string, std::string> headers;
        std::string body;

        static int Parse(const std::string& data, Request& request);
    };

    class Response
    {
    public:
        Response(StatusCode status = StatusCode::OK, const std::string& body = "", const std::map<std::string, std::string>& headers = {});
        ~Response() = default;

        void SetStatus(StatusCode status);
        void SetHeader(const std::string& name, const std::string& value);
        void SetBody(const std::string& body);

        StatusCode status;
        std::map<std::string, std::string> headers;
        std::string body;
    };
} // namespace http

#endif // MPI_HTTP_H