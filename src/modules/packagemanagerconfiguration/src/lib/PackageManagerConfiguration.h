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
    struct DesiredState
    {
        std::vector<std::string> packages;
        std::map<std::string, std::string> sources;
    };

    struct State
    {
        std::string executionState;
        std::string packagesFingerprint;
        std::vector<std::string> packages;
        std::string sourcesFingerprint;
        std::vector<std::string> sourcesFilenames;
    };

    PackageManagerConfigurationBase(unsigned int maxPayloadSizeBytes, std::string sourcesDir);
    PackageManagerConfigurationBase(unsigned int maxPayloadSizeBytes);
    virtual ~PackageManagerConfigurationBase() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual unsigned int GetMaxPayloadSizeBytes();
    virtual int RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds) = 0;

private:
    int ExecuteUpdate(const std::string& value);
    int ExecuteUpdates(const std::vector<std::string> packages);
    int ConfigureSources(const std::map<std::string, std::string> sources);
    std::string GetFingerprint();
    std::vector<std::string> GetReportedPackages(std::vector<std::string> packages);
    std::string GetSourcesFingerprint(std::string sourcesDir);
    std::vector<std::string> GetSourcesFilenames();
    int DeserializeDesiredState(rapidjson::Document& document, DesiredState& object);
    static int SerializeState(State reportedState, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes);
    static int CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    static std::vector<std::string> Split(const std::string &str, const std::string &delimiter);
    static std::vector<std::string> GetPackagesNames(std::vector<std::string> packages);
    static std::string TrimStart(const std::string& str, const std::string& trim);
    static std::string TrimEnd(const std::string& str, const std::string& trim);
    static std::string Trim(const std::string& str, const std::string& trim);

    ExecutionState m_executionState;
    std::vector<std::string> m_desiredPackages;
    unsigned int m_maxPayloadSizeBytes;
    std::string m_sourcesConfigurationDir;
};

class PackageManagerConfiguration : public PackageManagerConfigurationBase
{
public:
    PackageManagerConfiguration(unsigned int maxPayloadSizeBytes);
    ~PackageManagerConfiguration() = default;

    int RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds) override;
};