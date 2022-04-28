// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <sstream>

#include <CommonUtils.h>

#include "Http.h"

OSCONFIG_LOG_HANDLE PlatformLog::m_log = nullptr;


std::string TrimStart(const std::string &str, const std::string &trim)
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

std::string RemoveRepeatedCharacters(const std::string &str, const char c)
{
    std::string result = str;
    for (size_t i = 1; i < result.length(); i++)
    {
        if ((result[i] == c) && (result[i - 1] == result[i]))
        {
            result.erase(i, 1);
            i--;
        }
    }
    return result;
}

std::string ToUpper(const std::string &str)
{
    std::string result = str;
    std::for_each(result.begin(), result.end(), [](char &c) { c = std::toupper(c); });
    return result;
}

std::string ToLower(const std::string &str)
{
    std::string result = str;
    std::for_each(result.begin(), result.end(), [](char &c) { c = std::tolower(c); });
    return result;
}

using namespace http;

Request::Request(const std::string& uri, Method method, Version version) :
    Request(uri, method, std::map<std::string, std::string>(), std::string(), version)
{
}

Request::Request(const std::string& uri, Method method, const std::string& body, Version version) :
    Request::Request(uri, method, std::map<std::string, std::string>(), body, version)
{
}

Request::Request(const std::string& uri, Method method, const std::map<std::string, std::string>& headers, const std::string& body, Version version) :
    method(method),
    version(version),
    uri(uri),
    headers(headers),
    body(body)
{
}

// TODO: put this function somewhere better
std::vector<std::string> Split(const std::string& string)
{
    std::vector<std::string> result;
    std::stringstream ss(string);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        result.push_back(item);
    }
    return result;
}

// TODO: refactor to parse stream dynmaically instead of static buffer
Request Request::Parse(const std::string& data)
{
    // int status = 0;
    // TODO: read these values from the header data
    Method method = Method::POST;
    Version version = Version::HTTP_1_1;

    if (!data.empty())
    {
        std::vector<std::string> httpRequest = Split(data, "\r\n\r\n");
        std::string headerData = httpRequest[0];
        std::vector<std::string> headerLines = Split(headerData, "\r\n");
        std::string requestLine = headerLines[0];

        std::string requestMethod = Trim(Split(requestLine, " ")[0], " ");
        if (requestMethod == "GET")
        {
            method = Method::GET;
        }
        else if (requestMethod == "POST")
        {
            method = Method::POST;
        }
        else
        {
            // TODO: need to catch this somewhere
            throw std::runtime_error("Unknown HTTP method: " + requestMethod);
        }

        std::vector<std::string> requestLineParts = Split(requestLine, " ");
        if (requestLineParts.size() == 3)
        {
            // TODO:
            // method = MethodFromString(requestLineParts[0]);
            // version = VersionFromString(requestLineParts[2]);
        }

        std::map<std::string, std::string> headers;
        for (size_t i = 1; i < headerLines.size(); i++)
        {
            std::vector<std::string> headerParts = Split(headerLines[i], ": ");
            if (headerParts.size() == 2)
            {
                headers[headerParts[0]] = headerParts[1];
            }
        }

        // std::string body = httpRequest[1];
        std::string body = "";
        if (headers.find("Content-Length") != headers.end())
        {
            size_t contentLength = std::stoi(headers["Content-Length"]);
            body = httpRequest[1].substr(0, contentLength);
        }

        std::string uri = requestLineParts[1];
        // transform(uri.begin(), uri.end(), uri.begin(), ::tolower);

        return Request(uri, method, headers, body, version);
    }

    // TODO: return error
    return Request("", method);
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
