// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <Mmi.h>
#include <Logging.h>

#define HOST_NAME_CONFIGURATOR_LOGFILE "/var/log/osconfig_hostname.log"
#define HOST_NAME_CONFIGURATOR_ROLLEDLOGFILE "/var/log/osconfig_hostname.bak"

inline void HostNameFree(MMI_JSON_STRING payload)
{
    if (payload)
    {
        delete[] payload;
    }
}

class HostNameLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_logHostName;
    }

    static void OpenLog()
    {
        m_logHostName = ::OpenLog(HOST_NAME_CONFIGURATOR_LOGFILE, HOST_NAME_CONFIGURATOR_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_logHostName);
    }

private:
    static OSCONFIG_LOG_HANDLE m_logHostName;
};

class HostNameBase
{
public:
    static const char* m_componentName;
    static const char* m_propertyDesiredName;
    static const char* m_propertyDesiredHosts;
    static const char* m_propertyName;
    static const char* m_propertyHosts;

    HostNameBase(size_t maxPayloadSizeBytes);
    virtual ~HostNameBase();
    virtual int RunCommand(const char* command, bool replaceEol, std::string* textResult) = 0;

    int Get(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    int Set(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);

    std::string GetName();
    std::string GetHosts();
    int SetName(const std::string &value);
    int SetHosts(const std::string &value);

    static bool IsValidClientSession(MMI_HANDLE clientSession);
    static bool IsValidComponentName(const char* componentName);
    static bool IsValidObjectName(const char* objectName, const bool desired);
    static bool IsValidJsonString(const char* data, const int size);

private:
    const size_t m_maxPayloadSizeBytes;

    static std::string TrimStart(const std::string &str, const std::string &trim);
    static std::string TrimEnd(const std::string &str, const std::string &trim);
    static std::string Trim(const std::string &str, const std::string &trim);
    static std::vector<std::string> Split(const std::string &str, const std::string &delimeter);
    static std::string RemoveRepeatedCharacters(const std::string &str, const char c);
};
