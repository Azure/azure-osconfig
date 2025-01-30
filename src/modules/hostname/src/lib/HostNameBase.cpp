// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "HostName.h"
#include <CommonUtils.h>
#include <Mmi.h>
#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <algorithm>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/schema.h>
#include <rapidjson/error/en.h>

#define ERROR_MEMORY_ALLOCATION "%s memory allocation failed: %u bytes"
#define ERROR_PAYLOAD_TOO_LARGE "%s payload too large: %d (expected less than %d)"
#define ERROR_INVALID_VALUE "%s called with an invalid value: '%s'"
#define ERROR_INVALID_PAYLOAD "%s called with an invalid payload"
#define ERROR_INVALID_SESSION "%s called with an invalid client session: '%p'"
#define ERROR_INVALID_COMPONENT "%s called with an invalid component name: '%s' (expected '%s')"
#define ERROR_INVALID_OBJECT "%s called with an invalid object name: '%s' (expected '%s' or '%s')"
#define ERROR_SET_RETURNED "%s(%s) returned %d"
#define ERROR_INVALID_JSON "%s parse failed: '%s' (offset %u)"

const char* HostNameBase::m_componentName = "HostName";
const char* HostNameBase::m_propertyDesiredName = "desiredName";
const char* HostNameBase::m_propertyDesiredHosts = "desiredHosts";
const char* HostNameBase::m_propertyName = "name";
const char* HostNameBase::m_propertyHosts = "hosts";

constexpr const char* g_commandGetName = "cat /etc/hostname";
constexpr const char* g_commandGetHosts = "cat /etc/hosts";
constexpr const char* g_commandSetName = "hostnamectl set-hostname --static '$value'";
constexpr const char* g_commandSetHosts = "echo '$value' > /etc/hosts";

constexpr const char* g_regexHostname =
    "(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*("
    "[A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])";
constexpr const char* g_regexHost =
    "(((([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){"
    "3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5]))|"
    "((([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,"
    "4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9"
    "a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1"
    ",4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3"
    "}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-"
    "fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6"
    "})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|[fF][eE]80:(:[0-9a-fA-F]{"
    "0,4}){0,4}%[0-9a-zA-Z]{1,}|::([fF][eE]{4}(:0{1,4}){0,1}:){"
    "0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25["
    "0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1"
    ",4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25["
    "0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))))"
    "( +((([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\"
    ".)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])))+";

constexpr const char g_emptyPayload[] = "\"\"";

constexpr const char* g_trimDefault = " \n\r\"\';";
constexpr const char g_splitDefault = '\n';
constexpr const char g_splitCustom = ';';

OSCONFIG_LOG_HANDLE HostNameLog::m_logHostName = nullptr;

HostNameBase::HostNameBase(size_t maxPayloadSizeBytes) : m_maxPayloadSizeBytes(maxPayloadSizeBytes)
{
}

HostNameBase::~HostNameBase()
{
}

int HostNameBase::Set(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    if (!IsValidClientSession(clientSession))
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_SESSION, "Set", clientSession);
        return EINVAL;
    }

    if (!IsValidComponentName(componentName))
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_COMPONENT, "Set", componentName, m_componentName);
        return EINVAL;
    }

    if (!IsValidObjectName(objectName, true))
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_OBJECT, "Set", objectName ? objectName : "-", m_propertyDesiredName, m_propertyDesiredHosts);
        return EINVAL;
    }

    if (!payload || payloadSizeBytes < 0)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_PAYLOAD, "Set");
        return EINVAL;
    }

    // Validate payload size.
    int status = MMI_OK;
    if ((m_maxPayloadSizeBytes > 0) && (payloadSizeBytes > static_cast<int>(m_maxPayloadSizeBytes)))
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_PAYLOAD_TOO_LARGE, "Set", payloadSizeBytes, static_cast<int>(m_maxPayloadSizeBytes));
        status = E2BIG;
    }
    else
    {
        std::string data(payload, payloadSizeBytes);
        if (std::strcmp(objectName, m_propertyDesiredName) == 0)
        {
            status = SetName(data);
        }
        else if (std::strcmp(objectName, m_propertyDesiredHosts) == 0)
        {
            status = SetHosts(data);
        }
    }
    return status;
}

int HostNameBase::Get(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    if (!IsValidClientSession(clientSession))
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_SESSION, "Get", clientSession);
        return EINVAL;
    }

    if (!IsValidComponentName(componentName))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_COMPONENT, "Get", componentName, m_componentName);
        }
        return EINVAL;
    }

    if (!IsValidObjectName(objectName, false))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_OBJECT, "Get", objectName ? objectName : "-", m_propertyName, m_propertyHosts);
        }
        return EINVAL;
    }

    if (!payload || !payloadSizeBytes)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_PAYLOAD, "Get");
        }
        return EINVAL;
    }

    std::string data;
    if (std::strcmp(objectName, m_propertyName) == 0)
    {
        data = GetName();
    }
    else if (std::strcmp(objectName, m_propertyHosts) == 0)
    {
        data = GetHosts();
    }

    // Serialize data.
    rapidjson::Document document;
    document.SetString(data.c_str(), document.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    // Validate payload size.
    int status = MMI_OK;
    *payloadSizeBytes = buffer.GetSize();
    if ((0 != m_maxPayloadSizeBytes) && (*payloadSizeBytes > static_cast<int>(m_maxPayloadSizeBytes)))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(HostNameLog::Get(), ERROR_PAYLOAD_TOO_LARGE, "Get", *payloadSizeBytes, static_cast<int>(m_maxPayloadSizeBytes));
        }
        status = E2BIG;
    }
    else
    {
        // Allocate payload.
        *payload = new char[buffer.GetSize()];
        if (!payload)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(HostNameLog::Get(), ERROR_MEMORY_ALLOCATION, "Get", static_cast<uint>(buffer.GetSize()));
            }
            status = ENOMEM;
        }
        else
        {
            // Copy data to payload.
            std::fill(*payload, *payload + *payloadSizeBytes, 0);
            std::memcpy(*payload, buffer.GetString(), *payloadSizeBytes);

            // Validate payload.
            if (!HostNameBase::IsValidJsonString(*payload, *payloadSizeBytes))
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_PAYLOAD, "Get");
                }
                status = EINVAL;
                delete *payload;
            }
        }
    }

    // If an error occurred, reset the status and return an empty payload.
    if (status != MMI_OK)
    {
        status = MMI_OK;
        std::size_t len = sizeof(g_emptyPayload) - 1;
        *payloadSizeBytes = len;
        *payload = new char[len];
        if (!payload)
        {
            // Unable to allocate an empty payload, cannot return MMI_OK at this point.
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(HostNameLog::Get(), ERROR_MEMORY_ALLOCATION, "Get", static_cast<uint>(buffer.GetSize()));
            }
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, g_emptyPayload, len);
        }
    }
    return status;
}

std::string HostNameBase::GetName()
{
    std::string value;
    RunCommand(g_commandGetName, true, &value);
    return value.empty() ? value : TrimEnd(value, g_trimDefault);
}

std::string HostNameBase::GetHosts()
{
    // Do not replace EOL's so that the output can be split into individual lines.
    std::string value;
    RunCommand(g_commandGetHosts, false, &value);
    if (!value.empty())
    {
        value = TrimEnd(value, g_trimDefault);
        std::vector<std::string> lines = Split(value, std::string(&g_splitDefault, 1));
        value.clear();
        for (auto it = lines.begin(); it != lines.end(); ++it)
        {
            // Compress line and skip if empty or a comment to preserve space.
            std::string line = RemoveRepeatedCharacters(Trim(*it, g_trimDefault), ' ');
            if (!line.empty() && (line[0] != '#'))
            {
                if (!value.empty())
                {
                    value.append(std::string(&g_splitCustom, 1));
                }
                value.append(line);
            }
        }
    }
    return value;
}

int HostNameBase::SetName(const std::string &value)
{
    std::string name = Trim(value, g_trimDefault);

    // Validate input.
    std::regex pattern(g_regexHostname);
    if (!std::regex_match(name, pattern))
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_VALUE, "SetName", IsFullLoggingEnabled() ? name.c_str() : "-");
        return EINVAL;
    }

    std::string command = std::regex_replace(g_commandSetName, std::regex("\\$value"), name);
    int status = RunCommand(command.c_str(), true, nullptr);
    if (status != MMI_OK)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_SET_RETURNED, "SetName", IsFullLoggingEnabled() ? name.c_str() : "-", status);
    }
    return status;
}

int HostNameBase::SetHosts(const std::string &value)
{
    std::string hosts;

    // Validate input.
    std::regex pattern(g_regexHost);
    std::vector<std::string> lines = Split(value, std::string(&g_splitCustom, 1));
    for (auto it = lines.begin(); it != lines.end(); ++it)
    {
        std::string line = RemoveRepeatedCharacters(Trim(*it, g_trimDefault), ' ');
        if (!std::regex_match(line, pattern))
        {
            OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_VALUE, "SetHosts", IsFullLoggingEnabled() ? line.c_str() : "-");
            return EINVAL;
        }

        if (!hosts.empty())
        {
            hosts.append(std::string(&g_splitDefault, 1));
        }
        hosts.append(line);
    }

    std::string command = std::regex_replace(g_commandSetHosts, std::regex("\\$value"), hosts);
    int status = RunCommand(command.c_str(), true, nullptr);
    if (status != MMI_OK)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_SET_RETURNED, "SetHosts", IsFullLoggingEnabled() ? hosts.c_str() : "-", status);
    }
    return status;
}

bool HostNameBase::IsValidClientSession(MMI_HANDLE clientSession)
{
    return (clientSession != nullptr);
}

bool HostNameBase::IsValidComponentName(const char* componentName)
{
    return (componentName && (std::strcmp(componentName, m_componentName) == 0));
}

bool HostNameBase::IsValidObjectName(const char* objectName, const bool desired)
{
    return (objectName && (desired ? (std::strcmp(objectName, m_propertyDesiredName) == 0 || std::strcmp(objectName, m_propertyDesiredHosts) == 0) : (std::strcmp(objectName, m_propertyName) == 0 || std::strcmp(objectName, m_propertyHosts) == 0)));
}

bool HostNameBase::IsValidJsonString(const char* data, const int size)
{
    if (!data || (size < 0))
    {
        return false;
    }

    std::string str(data, size);
    rapidjson::Document document;
    document.Parse(str.c_str());
    if (document.HasParseError())
    {
        if (IsFullLoggingEnabled())
        {
            const size_t errorOffset = document.GetErrorOffset();
            const char* errorCode = rapidjson::GetParseError_En(document.GetParseError());
            OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_JSON, "IsValidJsonString", errorCode, static_cast<uint>(errorOffset));
        }
        return false;
    }
    return document.IsString();
}

std::string HostNameBase::TrimStart(const std::string &str, const std::string &trim)
{
    size_t pos = str.find_first_not_of(trim);
    if (pos == std::string::npos)
    {
        return "";
    }
    return str.substr(pos);
}

std::string HostNameBase::TrimEnd(const std::string &str, const std::string &trim)
{
    size_t pos = str.find_last_not_of(trim);
    if (pos == std::string::npos)
    {
        return "";
    }
    return str.substr(0, pos + 1);
}

std::string HostNameBase::Trim(const std::string &str, const std::string &trim)
{
    return TrimStart(TrimEnd(str, trim), trim);
}

std::vector<std::string> HostNameBase::Split(const std::string &str, const std::string &delimiter)
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

std::string HostNameBase::RemoveRepeatedCharacters(const std::string &str, const char c)
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
