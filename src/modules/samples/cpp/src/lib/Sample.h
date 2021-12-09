// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <string>

#include <CommonUtils.h>
#include <Logging.h>

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

    virtual int SetValue(const std::string& value);
    virtual std::string GetValue();
    virtual unsigned int GetMaxPayloadSizeBytes();

private:
    unsigned int m_maxPayloadSizeBytes;
    std::string m_value;
};
