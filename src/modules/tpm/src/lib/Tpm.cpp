// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <cstdio>
#include <regex>
#include <Tpm2Utils.h>

const char* g_getTpmDetected = "ls -d /dev/tpm[0-9]";
const char* g_getTpmrmDetected = "ls -d /dev/tpm[r][m][0-9]";
const char* g_getTpmCapabilities = "cat /sys/class/tpm/tpm0/caps";
const char* g_tpmDetected = "/dev/tpm[rm]*[0-9]";
const char* g_tpmVersionFromCapabilitiesFile = "TCG\\s+version:\\s+";
const char* g_tpmManufacturerFromCapabilitiesFile = "Manufacturer:\\s+0x";
const char* g_tpmVersionFromDeviceFile = "\\d(.\\d)?";
const char* g_tpmManufacturerFromDeviceFile = "[\\w\\s]+";
const char g_quotationCharacter = char('"');


OSCONFIG_LOG_HANDLE TpmLog::m_logTpm = nullptr;

Tpm::Tpm(const unsigned int maxPayloadSizeBytes) : m_maxPayloadSizeBytes(maxPayloadSizeBytes)
{
    m_hasCapabilitiesFile = true;
}

Tpm::~Tpm() {}

std::string Tpm::RunCommand(const char* command)
{
    char* textResult = nullptr;
    std::string commandOutput;

    int status = ExecuteCommand(nullptr, command, false, false, 0, 0, &textResult, nullptr, TpmLog::Get());

    if (status == MMI_OK)
    {
        commandOutput = (nullptr != textResult) ? std::string(textResult) : "";
    }

    if (nullptr != textResult)
    {
        free(textResult);
    }

    return commandOutput;
}

int Tpm::Get(const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    std::string data;

    if ((std::strcmp(objectName, TPM_STATUS) == 0) || (std::strcmp(objectName, TPM_VERSION) == 0) || (std::strcmp(objectName, TPM_MANUFACTURER) == 0))
    {
        if (std::strcmp(objectName, TPM_STATUS) == 0)
        {
            GetStatus(data);
        }
        else if ((std::strcmp(objectName, TPM_VERSION) == 0) && (this->m_hasCapabilitiesFile))
        {
            GetVersionFromCapabilitiesFile(data);
        }
        else if ((std::strcmp(objectName, TPM_MANUFACTURER) == 0) && (this->m_hasCapabilitiesFile))
        {
            GetManufacturerFromCapabilitiesFile(data);
        }
    }
    else
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, objectName %s not found", objectName);
        }
        status = EINVAL;
    }

    if ((2 >= data.length()) && (status == MMI_OK) && ((0 == std::strcmp(objectName, TPM_VERSION)) || (0 == std::strcmp(objectName, TPM_MANUFACTURER))))
    {
        this->m_hasCapabilitiesFile = false;
        std::string tpmProperty;
        status = Tpm2Utils::GetTpmPropertyFromDeviceFile(objectName, tpmProperty);
        if ((status == MMI_OK) && (!tpmProperty.empty()))
        {
            if (0 == std::strcmp(objectName, TPM_VERSION))
            {
                std::regex re(g_tpmVersionFromDeviceFile);
                std::smatch match;
                if (std::regex_search(tpmProperty, match, re))
                {
                    data = g_quotationCharacter + tpmProperty + g_quotationCharacter;
                }
            }
            else if (0 == std::strcmp(objectName, TPM_MANUFACTURER))
            {
                std::regex re(g_tpmManufacturerFromDeviceFile);
                std::smatch match;
                if (std::regex_search(tpmProperty, match, re))
                {
                    data = g_quotationCharacter + tpmProperty + g_quotationCharacter;
                }
            }
        }
        else
        {
            this->m_hasCapabilitiesFile = true;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Tpm property for object %s not found", objectName);
            }
        }
    }

    data.erase(std::find(data.begin(), data.end(), '\0'), data.end());

    if (((data.length() < this->m_maxPayloadSizeBytes) || (this->m_maxPayloadSizeBytes == 0)) && (status == MMI_OK))
    {
        *payloadSizeBytes = data.length();
        *payload = new (std::nothrow) char[*payloadSizeBytes];
        if (nullptr == *payload)
        {
            if (nullptr != payloadSizeBytes)
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(TpmLog::Get(), "Insufficient buffer space available to allocate %d bytes", *payloadSizeBytes);
                }
            }
            status = ENOMEM;
        }
        else
        {
            std::fill(*payload, *payload + *payloadSizeBytes, 0);
            std::memcpy(*payload, data.c_str(), *payloadSizeBytes);
        }
    }

    return status;
}

void Tpm::Trim(std::string& str)
{
    if (!str.empty())
    {
        while (str.find(" ") == 0)
        {
            str.erase(0, 1);
        }

        size_t end = str.size() - 1;
        while (str.rfind(" ") == end)
        {
            str.erase(end, end + 1);
            end--;
        }
    }
}

unsigned char Tpm::Decode(char c)
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

void Tpm::HexToText(std::string& s)
{
    std::string result;
    if ((s.size() % 2) == 0)
    {
        const size_t len = s.size() / 2;
        result.reserve(len);

        for (size_t i = 0; i < len; ++i)
        {
            unsigned char c1 = Decode(s[2 * i]) * 16;
            unsigned char c2 = Decode(s[2 * i + 1]);
            if ((c1 != (unsigned char)-1) && (c2 != (unsigned char)-1))
            {
                result += (c1 + c2);
            }
            else
            {
                result.clear();
                break;
            }
        }
    }

    s = result;
}

void Tpm::GetStatus(std::string& status)
{
    std::string commandOutput = RunCommand(g_getTpmDetected);
    if (commandOutput.empty())
    {
        commandOutput = RunCommand(g_getTpmrmDetected);
    }

    std::regex re(g_tpmDetected);
    std::smatch match;
    status = std::regex_search(commandOutput, match, re) ? std::to_string(Tpm::Status::TpmDetected) : std::to_string(Tpm::Status::TpmNotDetected);
}

void Tpm::GetVersionFromCapabilitiesFile(std::string& version)
{
    version = "\"\"";
    std::string commandOutput = RunCommand(g_getTpmCapabilities);
    if (!commandOutput.empty())
    {
        std::regex re(g_tpmVersionFromCapabilitiesFile);
        std::smatch match;
        if (std::regex_search(commandOutput, match, re))
        {
            std::string tpmProperties(match.suffix().str());
            std::string tpmVersion(tpmProperties.substr(0, tpmProperties.find('\n')));
            Trim(tpmVersion);
            version = g_quotationCharacter + tpmVersion + g_quotationCharacter;
        }
    }
}

void Tpm::GetManufacturerFromCapabilitiesFile(std::string& manufacturer)
{
    manufacturer = "\"\"";
    std::string commandOutput = RunCommand(g_getTpmCapabilities);
    if (!commandOutput.empty())
    {
        std::regex re(g_tpmManufacturerFromCapabilitiesFile);
        std::smatch match;
        if (std::regex_search(commandOutput, match, re))
        {
            std::string tpmProperties(match.suffix().str());
            std::string tpmManufacturer(tpmProperties.substr(0, tpmProperties.find('\n')));
            HexToText(tpmManufacturer);
            Trim(tpmManufacturer);
            manufacturer = g_quotationCharacter + tpmManufacturer + g_quotationCharacter;
        }
    }
}