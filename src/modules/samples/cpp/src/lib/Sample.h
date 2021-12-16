// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <string>
#include <rapidjson/document.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#define SAMPLE_LOGFILE "/var/log/osconfig_sample.log"
#define SAMPLE_ROLLEDLOGFILE "/var/log/osconfig_sample.bak"

class SampleLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_log;
    }

    static void OpenLog()
    {
        m_log = ::OpenLog(SAMPLE_LOGFILE, SAMPLE_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_log);
    }

private:
    static OSCONFIG_LOG_HANDLE m_log;
};

class Sample
{
public:
    Sample(unsigned int maxPayloadSizeBytes);
    virtual ~Sample() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual unsigned int GetMaxPayloadSizeBytes();

private:
    static int SerializeJsonPayload(rapidjson::Document& document, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes);

    std::string m_value;
    unsigned int m_maxPayloadSizeBytes;
};
