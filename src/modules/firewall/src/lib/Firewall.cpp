// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Firewall.h"

const std::string FirewallModuleBase::m_moduleInfo = R"""({
    "Name": "Firewall",
    "Description": "Provides functionality to remotely manage firewall rules on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 2,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["Firewall"],
    "Lifetime": 1,
    "UserAccount": 0})""";

const std::string FirewallModuleBase::m_firewallComponent = "Firewall";
const std::string FirewallModuleBase::m_firewallReportedFingerprint = "firewallFingerprint";
const std::string FirewallModuleBase::m_firewallReportedState = "firewallState";

OSCONFIG_LOG_HANDLE FirewallLog::m_logHandle = nullptr;

int FirewallModuleBase::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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
        size_t len = strlen(m_moduleInfo.c_str());
        *payload = new (std::nothrow) char[len];

        if (nullptr == *payload)
        {
            OsConfigLogError(FirewallLog::Get(), "Failed to allocate memory for payload");
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, m_moduleInfo.c_str(), len);
            *payloadSizeBytes = len;
        }
    }

    return status;
}

int FirewallModuleBase::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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
    else if (0 != m_firewallComponent.compare(componentName))
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

        if (0 == m_firewallReportedState.compare(objectName))
        {
            status = GetState(writer);
        }
        else if (0 == m_firewallReportedFingerprint.compare(objectName))
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
                OsConfigLogError(FirewallLog::Get(), "Payload size exceeds maximum payload size: %d > %d", static_cast<int>(buffer.GetSize()), m_maxPayloadSizeBytes);
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

int FirewallModuleBase::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = -1;

    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    OsConfigLogError(FirewallLog::Get(), "Firewall does not support desired properties");

    return status;
}

IpTables::State IpTables::Detect() const
{
    const char* command = "iptables -S";

    State state = State::Unknown;
    char* textResult = nullptr;

    if ((0 == ExecuteCommand(nullptr, command, false, false, 0, 0, &textResult, nullptr, FirewallLog::Get())))
    {
        if (textResult && (strlen(textResult) > 0))
        {
            state = State::Enabled;
        }
        else
        {
            state = State::Disabled;
        }
    }
    else
    {
        state = State::Disabled;
    }

    FREE_MEMORY(textResult);

    return state;
}

std::string IpTables::Fingerprint() const
{
    const char* command = "iptables -S";

    std::string hash;
    char* textResult = nullptr;

    if (nullptr != (textResult = HashCommand(command, FirewallLog::Get())))
    {
        hash = textResult;
    }

    FREE_MEMORY(textResult);

    return hash;
}