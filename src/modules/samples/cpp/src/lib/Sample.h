// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <vector>

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

    enum IntegerEnumeration
    {
        None = 0,
        Value1 = 1,
        Value2 = 2
    };

    // A sample object with all possible setting types
    struct Object
    {
        std::string stringSetting;
        int integerSetting;
        bool booleanSetting;
        IntegerEnumeration enumerationSetting;
        std::vector<std::string> stringArraySetting;
        std::vector<int> integerArraySetting;
        std::map<std::string, std::string> stringMapSetting;
        std::map<std::string, int> integerMapSetting;

        // Store removed elements to report as 'null'
        // These vectors must have a maximum size relative to the max payload size recieved by MmiOpen()
        std::vector<std::string> removedStringMapSettingKeys;
        std::vector<std::string> removedIntegerMapSettingKeys;
    };

    Sample(unsigned int maxPayloadSizeBytes);
    virtual ~Sample() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual unsigned int GetMaxPayloadSizeBytes();

private:
    static int SerializeObject(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Object& object);
    static int SerializeObjectArray(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::vector<Object>& objectArray);
    static int DeserializeObject(rapidjson::Document& document, Object& object);
    static int DeserializeObjectArray(rapidjson::Document& document, std::vector<Object>& objects);
    static int SerializeJsonPayload(rapidjson::Document& document, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes);
    static int CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes);

    // Store desired settings for reporting
    std::string m_stringValue;
    int m_integerValue;
    bool m_booleanValue;
    Object m_objectValue;
    std::vector<Object> m_objectArrayValue;

    unsigned int m_maxPayloadSizeBytes;
};
