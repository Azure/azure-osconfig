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
#define PMC_0K 0

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
        std::vector<std::string> Packages;
        std::map<std::string, std::string> Sources;
        std::map<std::string, std::string> GpgKeys;
    };

    struct State
    {
        ::ExecutionState ExecutionState;
        std::string PackagesFingerprint;
        std::vector<std::string> Packages;
        std::string SourcesFingerprint;
        std::vector<std::string> SourcesFilenames;
    };

    PmcBase(unsigned int maxPayloadSizeBytes, const char* sourcesDirectory);
    PmcBase(unsigned int maxPayloadSizeBytes);
    virtual ~PmcBase() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual unsigned int GetMaxPayloadSizeBytes();

protected:
    virtual int RunCommand(const char* command, std::string* textResult, bool isLongRunning = false) = 0;
    virtual std::string GetPackagesFingerprint() = 0;
    virtual std::string GetSourcesFingerprint(const char* sourcesDirectory) = 0;
    static bool ValidateAndUpdatePackageSource(std::string& packageSource, const std::map<std::string, std::string>& gpgKeys);

private:
    virtual bool CanRunOnThisPlatform() = 0;
    int ExecuteUpdate(const std::string& value);
    int ExecuteUpdates(const std::vector<std::string>& packages);
    std::vector<std::string> GetReportedPackages(const std::vector<std::string>& packages);
    int ConfigureSources(const std::map<std::string, std::string>& sources, const std::map<std::string, std::string>& gpgKeys);
    int ValidateAndGetPackagesNames(const std::vector<std::string>& packagesLines);
    int ValidateDocument(const rapidjson::Document& document);
    int DownloadGpgKeys(const std::map<std::string, std::string>& gpgKeys);
    int DeserializeDesiredState(const rapidjson::Document& document, DesiredState& object);
    int DeserializeGpgKeys(const rapidjson::Document& document, DesiredState& object);
    int DeserializeSources(const rapidjson::Document& document, DesiredState& object);
    int DeserializePackages(const rapidjson::Document& document, DesiredState& object);
    static std::vector<std::string> ListFiles(const char* directory, const char* fileNameExtension);
    static int SerializeState(const State& reportedState, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes);
    static int CopyJsonPayload(const rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    static std::vector<std::string> Split(const std::string& text, const std::string& delimiter);
    static std::string TrimStart(const std::string& text, const std::string& trim);
    static std::string TrimEnd(const std::string& text, const std::string& trim);
    static std::string Trim(const std::string& text, const std::string& trim);
    static std::string GenerateGpgKeyPath(const std::string& gpgKeyId);

    ExecutionState m_executionState;
    std::vector<std::string> m_desiredPackages;
    unsigned int m_maxPayloadSizeBytes;
    size_t m_lastReachedStateHash;
    const char* m_sourcesConfigurationDirectory;
};
