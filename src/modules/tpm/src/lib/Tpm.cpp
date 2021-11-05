// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <cstdio>
#include <regex>
#include <Tpm.h>

const char* g_checkTpmDetected = "ls -d /dev/tpm[0-9]";
const char* g_checkTpmrmDetected = "ls -d /dev/tpm[r][m][0-9]";
const char* g_getTpmDetails = "cat /sys/class/tpm/tpm0/caps";
const char* g_tpmVersionPattern = "TCG\\s+version:\\s+";
const char* g_tpmManufacturerPattern = "Manufacturer:\\s+0x";
const char* g_tpmDetectedPattern = "/dev/tpm[rm]*[0-9]";

const char* g_getVirtualTpmDetails = "cat /sys/class/tpm/tpm0/device/description";
const char* g_virtualTpmPattern = "Microsoft Virtual TPM 2.0";
const char* g_virtualTpmVersion = "\"2.0\"";
const char* g_manufacturer = "\"Microsoft\"";

OSCONFIG_LOG_HANDLE TpmLog::m_logTpm = nullptr;

Tpm::Tpm(int maxPayloadSizeBytes) : m_maxPayloadSizeBytes(maxPayloadSizeBytes) {}

Tpm::~Tpm() {}

std::string Tpm::RunCommand(const char* command)
{
    char* textResult = nullptr;
    std::string commandOutput;

    int status = ExecuteCommand(nullptr, command, false, true, 0, 0, &textResult, nullptr, TpmLog::Get());

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

    if (std::strcmp(objectName, TPM_STATUS) == 0)
    {
        GetStatus(data);
    }
    else if (std::strcmp(objectName, TPM_VERSION) == 0)
    {
        GetVersion(data);
    }
    else if (std::strcmp(objectName, TPM_MANUFACTURER) == 0)
    {
        GetManufacturer(data);
    }
    else
    {
        status = EINVAL;
    }

    data.erase(std::find(data.begin(), data.end(), '\0'), data.end());

    if (!((this->m_maxPayloadSizeBytes > 0) && ((int)data.length() > this->m_maxPayloadSizeBytes)) &&
        (status == MMI_OK))
    {
        *payloadSizeBytes = data.length();
        *payload = new (std::nothrow) char[*payloadSizeBytes];
        if (nullptr == *payload)
        {
            status = ENOMEM;
            if ((IsFullLoggingEnabled()) && (nullptr != payloadSizeBytes))
            {
                OsConfigLogError(TpmLog::Get(), "Tpm::Get insufficient buffer space available to allocate %d bytes", *payloadSizeBytes);
            }
        }
        else
        {
            std::fill(*payload, *payload + *payloadSizeBytes, 0);
            std::memcpy(*payload, data.c_str(), *payloadSizeBytes);
        }
    }

    return status;
}

void Tpm::Trim(std::string& s)
{
    if (!s.empty())
    {
        while (s.find(" ") == 0)
        {
            s.erase(0, 1);
        }

        size_t len = s.size();
        while (s.rfind(" ") == --len)
        {
            s.erase(len, len + 1);
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
    std::string commandOutput = RunCommand(g_checkTpmDetected);
    if (commandOutput.empty())
    {
        commandOutput = RunCommand(g_checkTpmrmDetected);
    }

    std::regex re(g_tpmDetectedPattern);
    std::smatch match;
    status = std::regex_search(commandOutput, match, re) ? std::to_string(Tpm::Status::TpmDetected) : std::to_string(Tpm::Status::TpmNotDetected);
}

void Tpm::GetVersion(std::string& version)
{
    version = "\"\"";
    std::string commandOutput = RunCommand(g_getTpmDetails);
    if (!commandOutput.empty())
    {
        std::regex re(g_tpmVersionPattern);
        std::smatch match;
        if (std::regex_search(commandOutput, match, re))
        {
            std::string tpmDetails(match.suffix().str());
            std::string tpmVersion(tpmDetails.substr(0, tpmDetails.find('\n')));
            Trim(tpmVersion);
            version = char('"') + tpmVersion + char('"');
        }
    }
    else
    {
        commandOutput = RunCommand(g_getVirtualTpmDetails);
        if (!commandOutput.empty())
        {
            std::regex re(g_virtualTpmPattern);
            std::smatch match;
            if (std::regex_search(commandOutput, match, re))
            {
                version = g_virtualTpmVersion;
            }
        }
    }
}

void Tpm::GetManufacturer(std::string& manufacturer)
{
    manufacturer = "\"\"";
    std::string commandOutput = RunCommand(g_getTpmDetails);
    if (!commandOutput.empty())
    {
        std::regex re(g_tpmManufacturerPattern);
        std::smatch match;
        if (std::regex_search(commandOutput, match, re))
        {
            std::string tpmDetails(match.suffix().str());
            std::string tpmManufacturer(tpmDetails.substr(0, tpmDetails.find('\n')));
            HexToText(tpmManufacturer);
            Trim(tpmManufacturer);
            manufacturer = char('"') + tpmManufacturer + char('"');
        }
    }
    else
    {
        commandOutput = RunCommand(g_getVirtualTpmDetails);
        if (!commandOutput.empty())
        {
            std::regex re(g_virtualTpmPattern);
            std::smatch match;
            if (std::regex_search(commandOutput, match, re))
            {
                manufacturer = g_manufacturer;
            }
        }
    }
}