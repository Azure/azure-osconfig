// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <map>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>

#include <CommonUtils.h>
#include <ExecutionState.h>
#include <Logging.h>
#include <Mmi.h>

#define PMC_LOGFILE "/var/log/osconfig_pmc.log"
#define PMC_ROLLEDLOGFILE "/var/log/osconfig_pmc.bak"
#define TIMEOUT_LONG_RUNNING 600

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
    struct DesiredState
    {
        std::vector<std::string> packages;
        std::map<std::string, std::string> sources;
    };

    struct State
    {
        ExecutionState executionState;
        std::string packagesFingerprint;
        std::vector<std::string> packages;
        std::string sourcesFingerprint;
        std::vector<std::string> sourcesFilenames;
    };

    PmcBase(unsigned int maxPayloadSizeBytes, const char* sourcesDirectory);
    PmcBase(unsigned int maxPayloadSizeBytes);
    virtual ~PmcBase() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual unsigned int GetMaxPayloadSizeBytes();
    virtual int RunCommand(const char* command, std::string* textResult, bool isLongRunning = false) = 0;

private:
    bool CanRunOnThisPlatform();
    int ExecuteUpdate(const std::string& value);
    int ExecuteUpdates(const std::vector<std::string> packages);
    std::string GetFingerprint();
    std::vector<std::string> GetReportedPackages(std::vector<std::string> packages);
    int ConfigureSources(const std::map<std::string, std::string> sources);
    std::string GetSourcesFingerprint(const char* sourcesDirectory);
    int DeserializeDesiredState(rapidjson::Document& document, DesiredState& object);
    int ValidateAndGetPackagesNames(std::vector<std::string> packagesLines);
    static std::vector<std::string> ListFiles(const char* directory, const char* fileNameExtension);
    static int SerializeState(State reportedState, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes);
    static int CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    static std::vector<std::string> Split(const std::string& text, const std::string& delimiter);
    static std::string TrimStart(const std::string& text, const std::string& trim);
    static std::string TrimEnd(const std::string& text, const std::string& trim);
    static std::string Trim(const std::string& text, const std::string& trim);

    ExecutionState m_executionState;
    std::vector<std::string> m_desiredPackages;
    unsigned int m_maxPayloadSizeBytes;
    size_t m_lastReachedStateHash;
    const char* m_sourcesConfigurationDirectory;
};