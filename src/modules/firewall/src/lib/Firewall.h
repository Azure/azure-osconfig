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
        return LOG_HANDLE;
    }
    static void OpenLog()
    {
        LOG_HANDLE = ::OpenLog(FIREWALL_LOGFILE, FIREWALL_ROLLEDLOGFILE);
    }
    static void CloseLog()
    {
        ::CloseLog(&LOG_HANDLE);
    }

private:
    static OSCONFIG_LOG_HANDLE LOG_HANDLE;
};

namespace utility
{
    std::string Hash(const std::string str);
    int Execute(const std::string command, std::string& result);
    int Execute(const std::string command);
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

class Iptables : public GenericUtility
{
public:
    typedef GenericUtility::State State;

    Iptables() = default;
    ~Iptables() = default;

    State Detect() const override
    {
        // If the utility is not installed/available, the the state is Disabled
        // If the utility is installed check if there are rules/chain policies in the tables
        // If there are rules/chain policies, the state is Enabled

        std::string result;
        return ((0 == utility::Execute("iptables -S", result)) && !result.empty()) ? State::Enabled : State::Disabled;
    }

    std::string Hash() const override
    {
        std::string hash;
        std::string rules;
        const std::string command = "iptables -S";

        if (0 == utility::Execute(command.c_str(), rules))
        {
            hash = utility::Hash(rules);
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "Error retrieving rules specification from iptables");
        }

        return hash;
    }
};

class FirewallModule
{
public:
    static const std::string MODULE_INFO;
    static const std::string FIREWALL_COMPONENT;

    // Reported properties
    static const std::string FIREWALL_REPORTED_FINGERPRINT;
    static const std::string FIREWALL_REPORTED_STATE;

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
        return fingerprint.empty() ? -1 : 0;
    }

private:
    UtilityT m_utility;
};

typedef GenericFirewall<Iptables> Firewall;