// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <regex>
#include <unistd.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <Tpm.h>

const std::string Tpm::m_tpm = "Tpm";
const std::string Tpm::m_tpmStatus = "tpmStatus";
const std::string Tpm::m_tpmVersion = "tpmVersion";
const std::string Tpm::m_tpmManufacturer = "tpmManufacturer";

constexpr const char g_moduleInfo[] = R""""({
    "Name": "Tpm",
    "Description": "Provides functionality to remotely query the TPM on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["Tpm"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

const char* g_tpmPath = "/dev/tpm0";
const char* g_getTpmDetected = "ls -d /dev/tpm[0-9]";
const char* g_getTpmrmDetected = "ls -d /dev/tpm[r][m][0-9]";
const char* g_getTpmCapabilities = "cat /sys/class/tpm/tpm0/caps";
const char* g_tpmDetected = "/dev/tpm[rm]*[0-9]";
const char* g_tpmVersionFromCapabilitiesFile = "TCG\\s+version:\\s+";
const char* g_tpmManufacturerFromCapabilitiesFile = "Manufacturer:\\s+0x";
const char* g_tpmVersionFromDeviceFile = "\\d(.\\d)?";
const char* g_tpmManufacturerFromDeviceFile = "[\\w\\s]+";

static const uint8_t g_getTpmProperties[] =
{
    0x80, 0x01,             // TPM_ST_NO_SESSIONS
    0x00, 0x00, 0x00, 0x16, // commandSize
    0x00, 0x00, 0x01, 0x7A, // TPM_CC_GetCapability
    0x00, 0x00, 0x00, 0x06, // TPM_CAP_TPM_PROPERTIES
    0x00, 0x00, 0x01, 0x00, // Property: TPM_PT_FAMILY_INDICATOR
    0x00, 0x00, 0x00, 0x66  // propertyCount (102)
};

OSCONFIG_LOG_HANDLE TpmLog::m_logTpm = nullptr;

Tpm::Tpm(const unsigned int maxPayloadSizeBytes) :
    m_maxPayloadSizeBytes(maxPayloadSizeBytes),
    m_status(Status::Unknown),
    m_properties() {}

int Tpm::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        std::size_t len = ARRAY_SIZE(g_moduleInfo) - 1;
        *payload = new (std::nothrow) char[len];
        if (nullptr == *payload)
        {
            OsConfigLogError(TpmLog::Get(), "Failed to allocate memory for payload");
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, g_moduleInfo, len);
            *payloadSizeBytes = len;
        }
    }

    return status;
}

std::string Tpm::RunCommand(const char* command)
{
    char* textResult = nullptr;
    std::string commandOutput;

    int status = ExecuteCommand(nullptr, command, false, false, 0, 0, &textResult, nullptr, TpmLog::Get());

    if (status == MMI_OK)
    {
        commandOutput = (nullptr != textResult) ? std::string(textResult) : "";
    }

    FREE_MEMORY(textResult);

    return commandOutput;
}

unsigned char Tpm::HexVal(char c)
{
    if (('0' <= c) && (c <= '9'))
    {
        return c - '0';
    }
    if (('a' <= c) && (c <= 'f'))
    {
        return c + 10 - 'a';
    }
    if (('A' <= c) && (c <= 'F'))
    {
        return c + 10 - 'A';
    }

    return (unsigned char)-1;
}

std::string Tpm::HexToString(const std::string str)
{
    std::string result;

    if (0 == (str.length() % 2))
    {
        result.reserve(str.length() / 2);

        for (auto it = str.begin(); it != str.end(); it++)
        {
            unsigned char c = HexVal(*it);
            it++;
            c = (c << 4) + HexVal(*it);
            result.push_back(c);
        }
    }
    else
    {
        OsConfigLogError(TpmLog::Get(), "Invalid hex string %s (length %d)", str.c_str(), (int)str.length());
    }

    return result;
}

void Tpm::Trim(std::string& str)
{
    str.erase(str.find_last_not_of(' ') + 1); // trailing spaces
    str.erase(0, str.find_first_not_of(' ')); // leading spaces
}

int Tpm::GetPropertiesFromCapabilitiesFile(Properties& properties)
{
    int status = 0;
    std::string commandOutput = RunCommand(g_getTpmCapabilities);

    if (!commandOutput.empty())
    {
        std::regex versionRegex(g_tpmVersionFromCapabilitiesFile);
        std::regex manufacturerRegex(g_tpmManufacturerFromCapabilitiesFile);
        std::smatch versionMatch;
        std::smatch manufacturerMatch;
        std::string property;

        if (std::regex_search(commandOutput, versionMatch, versionRegex) &&
            std::regex_search(commandOutput, manufacturerMatch, manufacturerRegex))
        {
            property = versionMatch.suffix().str();
            properties.version = property.substr(0, property.find('\n'));
            Trim(properties.version);

            property = manufacturerMatch.suffix().str();
            properties.manufacturer = HexToString(property.substr(0, property.find('\n')));
            Trim(properties.manufacturer);
        }
        else
        {
            status = -1;
        }
    }
    else
    {
        status = ENOENT;
    }

    return status;
}

std::string ParseTpmProperty(const std::string& property, const std::string& regex)
{
    std::string result;
    std::regex propertyRegex(regex);
    std::smatch propertyMatch;

    if (std::regex_search(property, propertyMatch, propertyRegex))
    {
        result = propertyMatch.str(0);
    }

    return result;
}

int Tpm::UnsignedInt8ToUnsignedInt64(uint8_t* buffer, uint32_t size, uint32_t offset, uint32_t length, uint64_t* output)
{
    int status = 0;
    uint32_t i = 0;
    uint64_t temp = 0;

    if (nullptr == buffer)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid argument, buffer is null");
        status = EINVAL;
    }
    else if (nullptr == output)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid argument, output is null");
        status = EINVAL;
    }
    else if (offset >= size)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid argument, buffer size %u must be greater than offset %u", size, offset);
        status = EINVAL;
    }
    else if (INT_MAX < size)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid argument, size %u must be less than or equal to %u", size, INT_MAX);
        status = EINVAL;
    }
    else if (0 >= length)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid argument, length %u must greater than 0", length);
        status = EINVAL;
    }
    else if (length > (size - offset))
    {
        OsConfigLogError(TpmLog::Get(), "Invalid argument, length %u must be less than or equal to %i", length, size - offset);
        status = EINVAL;
    }
    else if (sizeof(uint64_t) < length)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid argument, input buffer length remaining from offset must be less than %zu", sizeof(uint64_t));
        status = EINVAL;
    }
    else
    {
        *output = 0;
        for (i = 0; i < length; i++)
        {
            temp = temp << 8; // Make space to add the next byte of data from the input buffer
            temp += buffer[offset + i];
        }
        *output = temp;
    }

    return status;
}

int Tpm::GetPropertiesFromDeviceFile(Properties& properties)
{
    int status = 0;
    int tpm = -1;
    const uint8_t* request = g_getTpmProperties;
    ssize_t requestSize = sizeof(g_getTpmProperties);
    uint8_t* buffer = nullptr;
    ssize_t bytes = 0;
    uint64_t propertyKey = 0;
    std::string property;
    std::regex regex;
    std::smatch match;

    if (nullptr == (buffer = (uint8_t*)malloc(TPM_RESPONSE_MAX_SIZE)))
    {
        OsConfigLogError(TpmLog::Get(), "Insufficient buffer space available to allocate %d bytes", TPM_RESPONSE_MAX_SIZE);
        status = ENOMEM;
    }
    else
    {
        memset(buffer, 0xFF, TPM_RESPONSE_MAX_SIZE);

        if (-1 == (tpm = open(g_tpmPath, O_RDWR)))
        {
            OsConfigLogError(TpmLog::Get(), "Failed to open tpm: %s", g_tpmPath);
            status = ENOENT;
        }
        else if ((-1 == (bytes = write(tpm, request, requestSize))) || (bytes != requestSize))
        {
            OsConfigLogError(TpmLog::Get(), "Error reading response from the device");
            status = errno;
        }
        else if (-1 == (bytes = read(tpm, buffer, TPM_RESPONSE_MAX_SIZE)))
        {
            OsConfigLogError(TpmLog::Get(), "Error reading response from the device");
            status = errno;
        }
        else
        {
            for (int n = 0x13; n < (TPM_RESPONSE_MAX_SIZE - 8); n += 8)
            {
                if (0 != UnsignedInt8ToUnsignedInt64(buffer, TPM_RESPONSE_MAX_SIZE, n, 4, &propertyKey))
                {
                    OsConfigLogError(TpmLog::Get(), "Error converting TPM property key");
                    break;
                }

                unsigned char tpmPropertyBuffer[5] = { buffer[n + 4], buffer[n + 5], buffer[n + 6], buffer[n + 7], '\0' };
                property = std::string((char*)tpmPropertyBuffer);

                switch (propertyKey)
                {
                    case 0x100:
                        properties.version = ParseTpmProperty(property, g_tpmVersionFromDeviceFile);
                        Trim(properties.version);
                        break;

                    case 0x100+5:
                        properties.manufacturer = ParseTpmProperty(property, g_tpmManufacturerFromDeviceFile);
                        Trim(properties.manufacturer);
                        break;

                    default:
                        break;
                }
            }
        }

        if (tpm != -1)
        {
            close(tpm);
        }

        FREE_MEMORY(buffer);
    }

    return status;
}

Tpm::Status Tpm::GetStatus() const
{
    return m_status;
}

std::string Tpm::GetVersion() const
{
    return "\"" + m_properties.version + "\"";
}

std::string Tpm::GetManufacturer() const
{
    return "\"" + m_properties.manufacturer + "\"";
}

void Tpm::LoadProperties()
{
    std::regex re(g_tpmDetected);
    std::smatch match;
    std::string commandOutput = RunCommand(g_getTpmDetected);

    if (commandOutput.empty())
    {
        commandOutput = RunCommand(g_getTpmrmDetected);
    }

    m_status = std::regex_search(commandOutput, match, re) ? Tpm::Status::TpmDetected : Tpm::Status::TpmNotDetected;

    if (GetPropertiesFromCapabilitiesFile(m_properties) != 0)
    {
        if (GetPropertiesFromDeviceFile(m_properties) != 0)
        {
            m_status = Status::TpmNotDetected;
        }
    }
}

int Tpm::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    std::string data;

    if (nullptr == payload)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        if (0 == Tpm::m_tpm.compare(componentName))
        {
            if (m_status == Status::Unknown)
            {
                LoadProperties();
            }

            if (0 == Tpm::m_tpmStatus.compare(objectName))
            {
                data = std::to_string(static_cast<int>(GetStatus()));
            }
            else if (0 == Tpm::m_tpmVersion.compare(objectName))
            {
                data = GetVersion();
            }
            else if (0 == Tpm::m_tpmManufacturer.compare(objectName))
            {
                data = GetManufacturer();
            }
            else
            {
                OsConfigLogError(TpmLog::Get(), "Invalid objectName: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(TpmLog::Get(), "Invalid component name: %s", componentName);
            status = EINVAL;
        }
    }

    if (status == MMI_OK)
    {
        if ((m_maxPayloadSizeBytes > 0) && (data.length() > m_maxPayloadSizeBytes))
        {
            OsConfigLogError(TpmLog::Get(), "Payload size %d exceeds max payload size %d", static_cast<int>(data.size()), m_maxPayloadSizeBytes);
            status = E2BIG;
        }
        else
        {
            *payload = new (std::nothrow) char[data.length()];
            if (nullptr != *payload)
            {
                std::fill(*payload, *payload + data.length(), 0);
                std::memcpy(*payload, data.c_str(), data.length());
                *payloadSizeBytes = data.length();
            }
            else
            {
                OsConfigLogError(TpmLog::Get(), "Failed to allocate memory for payload");
                status = ENOMEM;
            }
        }
    }

    return status;
}
