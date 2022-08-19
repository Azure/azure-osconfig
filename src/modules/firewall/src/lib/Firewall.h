// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <cstdarg>
#include <memory>
#include <ostream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <sstream>
#include <string>
#include <vector>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#define FIREWALL_LOGFILE "/var/log/osconfig_firewall.log"
#define FIREWALL_ROLLEDLOGFILE "/var/log/osconfig_firewall.bak"

class FirewallLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_logHandle;
    }
    static void OpenLog()
    {
        m_logHandle = ::OpenLog(FIREWALL_LOGFILE, FIREWALL_ROLLEDLOGFILE);
    }
    static void CloseLog()
    {
        ::CloseLog(&m_logHandle);
    }

private:
    static OSCONFIG_LOG_HANDLE m_logHandle;
};

class GenericUtility
{
public:
    enum class State
    {
        Unknown = 0,
        Enabled,
        Disabled
    };

    virtual ~GenericUtility() = default;

    virtual State Detect() const = 0;
    virtual std::string Hash() const = 0;
};

class IpTables : public GenericUtility
{
public:
    typedef GenericUtility::State State;

    IpTables() = default;
    ~IpTables() = default;

    State Detect() const override
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

    std::string Hash() const override
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
};

class FirewallModule
{
public:
    static const std::string m_moduleInfo;
    static const std::string m_firewallComponent;

    // Reported properties
    static const std::string m_firewallReportedFingerprint;
    static const std::string m_firewallReportedState;

    FirewallModule(unsigned int maxPayloadSizeBytes) : m_maxPayloadSizeBytes(maxPayloadSizeBytes) {}
    virtual ~FirewallModule() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);

    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);

protected:
    virtual int GetState(rapidjson::Writer<rapidjson::StringBuffer>& writer) = 0;
    virtual int GetFingerprint(rapidjson::Writer<rapidjson::StringBuffer>& writer) = 0;

private:
    unsigned int m_maxPayloadSizeBytes;
    size_t m_lastPayloadHash;
};

template <class UtilityT>
class GenericFirewall : public FirewallModule
{
public:
    GenericFirewall(unsigned int maxPayloadSize) : FirewallModule(maxPayloadSize) {}
    ~GenericFirewall() = default;

protected:
    typedef typename UtilityT::State State;

    virtual int GetState(rapidjson::Writer<rapidjson::StringBuffer>& writer) override
    {
        State state = m_utility.Detect();
        int value = static_cast<int>(state);
        writer.Int(value);
        return 0;
    }

    virtual int GetFingerprint(rapidjson::Writer<rapidjson::StringBuffer>& writer) override
    {
        std::string fingerprint = m_utility.Hash();
        writer.String(fingerprint.c_str());
        return 0;
    }

private:
    UtilityT m_utility;
};

typedef GenericFirewall<IpTables> Firewall;