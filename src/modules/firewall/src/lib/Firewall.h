// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <cstdarg>
#include <memory>
#include <ostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <set>
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
    static OsConfigLogHandle Get()
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
    static OsConfigLogHandle m_logHandle;
};

class StringEnum
{
public:
    StringEnum() = default;
    StringEnum(const std::string& value) : m_value(value) {}
    virtual ~StringEnum() = default;

    virtual bool IsValid() const = 0;

    std::string ToString() const
    {
        return m_value;
    }

    bool operator==(const char* other) const
    {
        return (m_value.compare(other) == 0);
    }

    bool operator!=(const char* other) const
    {
        return (m_value.compare(other) != 0);
    }

    friend std::ostream& operator<<(std::ostream& os, const StringEnum& se)
    {
        os << se.ToString();
        return os;
    }

protected:
    std::string m_value;
};

class DesiredState : public StringEnum
{
public:
    DesiredState() = default;
    DesiredState(const std::string& value) : StringEnum(value) {}

    bool IsValid() const override
    {
        return (m_values.find(m_value) != m_values.end());
    }

private:
    static const std::set<std::string> m_values;
};

class Action : public StringEnum
{
public:
    Action() = default;
    Action(const std::string& value) : StringEnum(value) {}
    virtual ~Action() = default;

    virtual bool IsValid() const override
    {
        return (m_values.find(m_value) != m_values.end());
    }

private:
    static const std::set<std::string> m_values;
};

class Direction : public StringEnum
{
public:
    Direction() = default;
    Direction(const std::string& value) : StringEnum(value) {}

    bool IsValid() const override
    {
        return (m_values.find(m_value) != m_values.end());
    }

private:
    static const std::set<std::string> m_values;
};

class Protocol : public StringEnum
{
public:
    Protocol() = default;
    Protocol(const std::string& value) : StringEnum(value) {}

    bool IsValid() const override
    {
        return (m_values.find(m_value) != m_values.end());
    }

private:
    static const std::set<std::string> m_values;
};

class GenericRule
{
public:
    virtual GenericRule& Parse(const nlohmann::json& rule);

    virtual std::vector<std::string> GetParseError() const
    {
        return m_parseError;
    }

    virtual bool HasParseError() const
    {
        return !m_parseError.empty();
    }

    virtual DesiredState GetDesiredState() const
    {
        return m_desiredState;
    }

    virtual std::string Specification() const = 0;

protected:
    Action m_action;
    Direction m_direction;
    Protocol m_protocol;
    std::string m_sourceAddress;
    std::string m_destinationAddress;
    std::string m_sourcePort;
    std::string m_destinationPort;

    std::vector<std::string> m_parseError;

private:
    DesiredState m_desiredState;
};

class GenericPolicy
{
public:
    virtual GenericPolicy& Parse(const nlohmann::json& rule);
    virtual nlohmann::json Serialize() const;

    virtual std::vector<std::string> GetParseError() const
    {
        return m_parseError;
    }

    virtual bool HasParseError() const
    {
        return !m_parseError.empty();
    }

    virtual std::string Specification() const = 0;

protected:
    Action m_action;
    Direction m_direction;

    std::vector<std::string> m_parseError;
};

class IpTablesRule : public GenericRule
{
public:
    IpTablesRule() = default;

    virtual std::string Specification() const override;
};

class IpTablesPolicy : public GenericPolicy
{
public:
    IpTablesPolicy() = default;

    virtual std::string Specification() const override;

    virtual int SetActionFromTarget(const std::string& str);
    virtual int SetDirectionFromChain(const std::string& str);
};

template<class RuleT, class PolicyT>
class GenericFirewall
{
public:
    enum class Status
    {
        Unknown,
        Success,
        Failure
    };

    enum class State
    {
        Unknown = 0,
        Enabled,
        Disabled
    };

    virtual Status GetStatus() const
    {
        return (m_policyStatusMessage.empty() && m_ruleStatusMessage.empty()) ? Status::Success : Status::Failure;
    }

    virtual std::string GetStatusMessage() const
    {
        return m_policyStatusMessage + m_ruleStatusMessage;
    }

    virtual State Detect() const = 0;
    virtual std::string Fingerprint() const = 0;
    virtual std::vector<PolicyT> GetDefaultPolicies() const = 0;

    virtual int SetRules(const std::vector<RuleT>& rules) = 0;
    virtual int SetDefaultPolicies(const std::vector<PolicyT> policies) = 0;

protected:
    std::string m_policyStatusMessage;
    std::string m_ruleStatusMessage;
};

class IpTables : public GenericFirewall<IpTablesRule, IpTablesPolicy>
{
public:
    typedef GenericFirewall::State State;
    typedef IpTablesPolicy Policy;
    typedef IpTablesRule Rule;

    State Detect() const override;
    std::string Fingerprint() const override;
    std::vector<Policy> GetDefaultPolicies() const override;

    int SetRules(const std::vector<Rule>& rules) override;
    int SetDefaultPolicies(const std::vector<Policy> policies) override;

private:
    int Add(const Rule& rule, std::string& error);
    int Remove(const Rule& rule, std::string& error);
    bool Exists(const Rule& rule) const;
};

class FirewallModuleBase
{
public:
    static const std::string m_moduleInfo;
    static const std::string m_firewallComponent;

    // Reported properties
    static const std::string m_reportedFingerprint;
    static const std::string m_reportedState;
    static const std::string m_reportedDefaultPolicies;
    static const std::string m_reportedConfigurationStatus;
    static const std::string m_reportedConfigurationStatusDetail;

    // Desired properties
    static const std::string m_desiredDefaultPolicies;
    static const std::string m_desiredRules;

    FirewallModuleBase(unsigned int maxPayloadSizeBytes) : m_maxPayloadSizeBytes(maxPayloadSizeBytes) {}
    virtual ~FirewallModuleBase() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);

    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);

protected:
    virtual int GetState(nlohmann::json& result) const = 0;
    virtual int GetFingerprint(nlohmann::json& result) const = 0;
    virtual int GetDefaultPolicies(nlohmann::json& result) const = 0;
    virtual int GetConfigurationStatus(nlohmann::json& result) const = 0;
    virtual int GetConfigurationStatusDetail(nlohmann::json& result) const = 0;

    virtual int SetDefaultPolicies(const nlohmann::json& document) = 0;
    virtual int SetRules(const nlohmann::json& document) = 0;

private:
    unsigned int m_maxPayloadSizeBytes;
};

template<class T>
std::vector<T> ParseArray(const nlohmann::json& value)
{
    std::vector<T> rules;

    if (value.is_array())
    {
        for (const auto& item : value)
        {
            T rule;
            rule.Parse(item);
            rules.push_back(rule);

            if (rule.HasParseError())
            {
                for (auto& error : rule.GetParseError())
                {
                    OsConfigLogError(FirewallLog::Get(), "%s", error.c_str());
                }
            }
        }
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "JSON value is not an array");
    }

    return rules;
}

template <class FirewallT>
class FirewallModule : public FirewallModuleBase
{
public:
    FirewallModule(unsigned int maxPayloadSize) : FirewallModuleBase(maxPayloadSize) {}

protected:
    typedef typename FirewallT::State State;
    typedef typename FirewallT::Status Status;
    typedef typename FirewallT::Policy Policy;
    typedef typename FirewallT::Rule Rule;

    virtual int GetState(nlohmann::json& result) const override
    {
        State state = m_firewall.Detect();
        result = (state == State::Enabled) ? "enabled" : ((state == State::Disabled) ? "disabled" : "unknown");
        return EXIT_SUCCESS;
    }

    virtual int GetFingerprint(nlohmann::json& result) const override
    {
        std::string fingerprint = m_firewall.Fingerprint();
        result = fingerprint;
        return EXIT_SUCCESS;
    }

    virtual int GetDefaultPolicies(nlohmann::json& result) const override
    {
        std::vector<Policy> policies = m_firewall.GetDefaultPolicies();
        result = nlohmann::json::array();
        for (const Policy& policy : policies)
        {
            result.push_back(policy.Serialize());
        }
        return EXIT_SUCCESS;
    }

    virtual int GetConfigurationStatus(nlohmann::json& result) const override
    {
        Status status = m_firewall.GetStatus();
        result = (status == Status::Success) ? "success" : ((status == Status::Failure) ? "failure" : "unknown");
        return EXIT_SUCCESS;
    }

    virtual int GetConfigurationStatusDetail(nlohmann::json& result) const override
    {
        std::string statusDetail = m_firewall.GetStatusMessage();
        result = statusDetail;
        return EXIT_SUCCESS;
    }

    virtual int SetDefaultPolicies(const nlohmann::json& document) override
    {
        std::vector<Policy> policies = ParseArray<Policy>(document);
        return m_firewall.SetDefaultPolicies(policies);
    }

    virtual int SetRules(const nlohmann::json& document) override
    {
        std::vector<Rule> rules = ParseArray<Rule>(document);
        return m_firewall.SetRules(rules);
    }

private:
    FirewallT m_firewall;
};

typedef FirewallModule<IpTables> Firewall;
