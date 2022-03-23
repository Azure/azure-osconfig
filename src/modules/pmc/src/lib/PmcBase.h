// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include <rapidjson/stringbuffer.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#define PMC_LOGFILE "/var/log/osconfig_packagemanagerconfiguration.log"
#define PMC_ROLLEDLOGFILE "/var/log/osconfig_packagemanagerconfiguration.bak"

class PmcLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_log;
    }

    static void OpenLog()
    {
        m_log = ::OpenLog(PMC_LOGFILE, PMC_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_log);
    }

private:
    static OSCONFIG_LOG_HANDLE m_log;
};

class PmcBase
{
public:
    PmcBase(unsigned int maxPayloadSizeBytes);
    virtual ~PmcBase() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual unsigned int GetMaxPayloadSizeBytes();

private:
    unsigned int m_maxPayloadSizeBytes;
};