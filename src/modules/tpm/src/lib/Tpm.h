// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TPM_H
#define TPM_H

#include <cstring>
#include <string>
#include <Logging.h>

#define TPM_LOGFILE "/var/log/osconfig_tpm.log"
#define TPM_ROLLEDLOGFILE "/var/log/osconfig_tpm.bak"

#define TPM_RESPONSE_MAX_SIZE 4096
#define INT_MAX 0x7FFFFFF

class TpmLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_logTpm;
    }

    static void OpenLog()
    {
        m_logTpm = ::OpenLog(TPM_LOGFILE, TPM_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_logTpm);
    }

private:
    static OSCONFIG_LOG_HANDLE m_logTpm;
};

class Tpm
{
public:
    static const std::string m_tpm;
    static const std::string m_tpmVersion;
    static const std::string m_tpmManufacturer;
    static const std::string m_tpmStatus;

    enum Status
    {
        Unknown = 0,
        TpmDetected,
        TpmNotDetected
    };

    struct Properties
    {
        std::string version;
        std::string manufacturer;
    };

    Tpm(const unsigned int maxPayloadSizeBytes);
    virtual ~Tpm() = default;

    virtual std::string RunCommand(const char* command);
    static unsigned char HexVal(char c);
    static std::string HexToString(const std::string str);
    static void Trim(std::string& str);
    static int UnsignedInt8ToUnsignedInt64(uint8_t* buffer, uint32_t size, uint32_t offset, uint32_t length, uint64_t* output);

    int GetPropertiesFromCapabilitiesFile(Properties& properties);
    int GetPropertiesFromDeviceFile(Properties& properties);
    void LoadProperties();

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);

    Status GetStatus() const;
    std::string GetVersion() const;
    std::string GetManufacturer() const;

private:
    const unsigned int m_maxPayloadSizeBytes;
    Status m_status;
    Properties m_properties;
};

#endif // TPM_H
