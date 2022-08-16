// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Firewall.h"

const std::string FirewallModule::MODULE_INFO = R"""({
    "Name": "Firewall",
    "Description": "Provides functionality to remotely manage firewall rules on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 2,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["Firewall"],
    "Lifetime": 1,
    "UserAccount": 0})""";

const std::string FirewallModule::FIREWALL_COMPONENT = "Firewall";
const std::string FirewallModule::FIREWALL_REPORTED_FINGERPRINT = "firewallFingerprint";
const std::string FirewallModule::FIREWALL_REPORTED_STATE = "firewallState";

OSCONFIG_LOG_HANDLE FirewallLog::LOG_HANDLE = nullptr;

int FirewallModule::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid (null) client name");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid (null) payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid (null) payload size");
        status = EINVAL;
    }
    else
    {
        size_t len = strlen(MODULE_INFO.c_str());
        *payload = new (std::nothrow) char[len];

        if (nullptr == *payload)
        {
            OsConfigLogError(FirewallLog::Get(), "Failed to allocate memory for payload");
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, MODULE_INFO.c_str(), len);
            *payloadSizeBytes = len;
        }
    }

    return status;
}

int FirewallModule::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == componentName)
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid (null) component name");
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid (null) object name");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid (null) payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid (null) payload size");
        status = EINVAL;
    }
    else if (0 != FIREWALL_COMPONENT.compare(componentName))
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid component name: %s", componentName);
        status = EINVAL;
    }
    else
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        *payloadSizeBytes = 0;
        *payload = nullptr;

        if (0 == FIREWALL_REPORTED_STATE.compare(objectName))
        {
            status = GetState(writer);
        }
        else if (0 == FIREWALL_REPORTED_FINGERPRINT.compare(objectName))
        {
            status = GetFingerprint(writer);
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "Invalid object name: %s", objectName);
            status = EINVAL;
        }

        if (MMI_OK == status)
        {
            if ((m_maxPayloadSizeBytes > 0) && (buffer.GetSize() > m_maxPayloadSizeBytes))
            {
                OsConfigLogError(FirewallLog::Get(), "Payload size exceeds maximum payload size");
                status = E2BIG;
            }
            else
            {
                *payloadSizeBytes = buffer.GetSize();
                *payload = new (std::nothrow) char[*payloadSizeBytes];

                if (*payload != nullptr)
                {
                    std::fill(*payload, *payload + *payloadSizeBytes, 0);
                    std::memcpy(*payload, buffer.GetString(), *payloadSizeBytes);
                }
                else
                {
                    *payloadSizeBytes = 0;
                    status = ENOMEM;
                }
            }
        }
    }

    return status;
}

int FirewallModule::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = -1;

    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    OsConfigLogError(FirewallLog::Get(), "Firewall does not support desired properties");

    return status;
}

namespace utility
{
    std::string Hash(const std::string str)
    {
        char* hash = nullptr;
        std::string command = "echo \"" + str + "\"";
        return (hash = HashCommand(command.c_str(), FirewallLog::Get())) ? hash : "";
    }

    int Execute(const std::string command, std::string& result)
    {
        const char trimChars[] = " \n\r\t";

        char* textResult = nullptr;
        int status = ExecuteCommand(nullptr, command.c_str(), false, false, 0, 0, &textResult, nullptr, FirewallLog::Get());

        if (textResult)
        {
            result = textResult;
            result.erase(0, result.find_first_not_of(trimChars));
            result.erase(result.find_last_not_of(trimChars) + 1);
            free(textResult);
        }

        return status;
    }

    int Execute(const std::string command)
    {
        return ExecuteCommand(nullptr, command.c_str(), false, false, 0, 0, nullptr, nullptr, FirewallLog::Get());
    }
}