// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <sstream>

#include <CommonUtils.h>
#include <Http.h>

OSCONFIG_LOG_HANDLE PlatformLog::m_log = nullptr;

using namespace http;

std::string TrimStart(const std::string& str, const std::string& trim)
{
    size_t pos = str.find_first_not_of(trim);
    if (pos == std::string::npos)
    {
        return "";
    }
    return str.substr(pos);
}

std::string TrimEnd(const std::string &str, const std::string &trim)
{
    size_t pos = str.find_last_not_of(trim);
    if (pos == std::string::npos)
    {
        return "";
    }
    return str.substr(0, pos + 1);
}

std::string Trim(const std::string &str, const std::string &trim)
{
    return TrimStart(TrimEnd(str, trim), trim);
}

std::vector<std::string> Split(const std::string &str, const std::string &delimiter)
{
    std::vector<std::string> result;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delimiter, end)) != std::string::npos)
    {
        end = str.find(delimiter, start);
        result.push_back(str.substr(start, end - start));
    }
    return result;
}

Method MethodFromString(std::string& method)
{
    Method result = Method::UNKNOWN;
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);

    if (method == "GET")
    {
        result = Method::GET;
    }
    else if (method == "POST")
    {
        result = Method::POST;
    }
    else if (method == "PUT")
    {
        result = Method::PUT;
    }
    else if (method == "DELETE")
    {
        result = Method::DELETE;
    }
    else if (method == "PATCH")
    {
        result = Method::PATCH;
    }
    else
    {
        result = Method::UNKNOWN;
    }

    return result;
}

Version VersionFromString(std::string& version)
{
    Version result = Version::UNKNOWN;
    std::transform(version.begin(), version.end(), version.begin(), ::toupper);

    if (version == "HTTP/1.0")
    {
        result = Version::HTTP_1_0;
    }
    else if (version == "HTTP/1.1")
    {
        result = Version::HTTP_1_1;
    }
    else if (version == "HTTP/2.0")
    {
        result = Version::HTTP_2_0;
    }
    else
    {
        result = Version::UNKNOWN;
    }

    return result;
}

int Request::Parse(const std::string& data, Request& request)
{
    int status = 0;

    if (!data.empty())
    {
        std::vector<std::string> httpRequest = Split(data, "\r\n\r\n");
        std::string headerData = httpRequest[0];
        std::vector<std::string> headerLines = Split(headerData, "\r\n");
        std::string requestLine = headerLines[0];

        std::vector<std::string> requestLineParts = Split(requestLine, " ");
        std::string requestMethod = Trim(requestLineParts[0], " ");

        if (requestLineParts.size() == 3)
        {
            request.method = MethodFromString(requestLineParts[0]);
            request.uri = requestLineParts[1];
            request.version = VersionFromString(requestLineParts[2]);
        }
        else
        {
            status = EINVAL;
        }

        std::map<std::string, std::string> headers;
        for (size_t i = 1; i < headerLines.size(); i++)
        {
            std::vector<std::string> headerParts = Split(headerLines[i], ": ");
            if (headerParts.size() == 2)
            {
                headers[headerParts[0]] = headerParts[1];
            }
            else
            {
                status = EINVAL;
            }
        }

        if (headers.find("Content-Length") != headers.end())
        {
            size_t contentLength = std::stoi(headers["Content-Length"]);
            request.body = httpRequest[1].substr(0, contentLength);
        }
    }

    return status;
}

Response::Response(StatusCode status, const std::string& body, const std::map<std::string, std::string>& headers) :
    status(status),
    headers(headers),
    body(body)
{
}

void Response::SetStatus(StatusCode status)
{
    this->status = status;
}

void Response::SetBody(const std::string& body)
{
    this->body = body;
}

void Response::SetHeader(const std::string& key, const std::string& value)
{
    headers[key] = value;
}
