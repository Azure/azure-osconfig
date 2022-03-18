// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include <rapidjson/stringbuffer.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#define PACKAGEMANAGERCONFIGURATION_LOGFILE "/var/log/osconfig_packagemanagerconfiguration.log"
#define PACKAGEMANAGERCONFIGURATION_ROLLEDLOGFILE "/var/log/osconfig_packagemanagerconfiguration.bak"

class PackageManagerConfigurationLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_log;
    }

    static void OpenLog()
    {
        m_log = ::OpenLog(PACKAGEMANAGERCONFIGURATION_LOGFILE, PACKAGEMANAGERCONFIGURATION_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_log);
    }

private:
    static OSCONFIG_LOG_HANDLE m_log;
};

class PackageManagerConfigurationBase
{
public:
    PackageManagerConfigurationBase(unsigned int maxPayloadSizeBytes);
    virtual ~PackageManagerConfigurationBase() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual unsigned int GetMaxPayloadSizeBytes();

private:
    unsigned int m_maxPayloadSizeBytes;
};