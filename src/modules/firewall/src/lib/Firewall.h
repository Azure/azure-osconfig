// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <cstdarg>
#include <memory>
#include <ostream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <regex>
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

// TODO: create an abstract class for Parse/Serialize methods

class Rule
{
public:
    enum class State
    {
        Present,
        Absent
    };

    enum class Action
    {
        Accept,
        Drop,
        Reject
    };

    enum class Direction
    {
        In,
        Out
    };

    enum class Protocol
    {
        Any,
        Tcp,
        Udp,
        Icmp,
    };

    Rule() = default;
    virtual ~Rule() = default;

    // TODO: methods for serializing enums to JSON (Action, Direction)
    // template<typename T>
    // static void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const T& value);

    // TODO: methods for parsing firewall strings to enums (Action, Direction, Protocol)
    // template<typename T>
    // static int Parse(const std::string& str, T& value);

    virtual Rule& Parse(const rapidjson::Value& rule);

    virtual std::string GetParseError() const
    {
        return m_parseError;
    }

    virtual bool HasParseError() const
    {
        return !m_parseError.empty();
    }

    virtual State GetDesiredState() const
    {
        return m_desiredState;
    }

    virtual std::string Specification() const = 0;

    virtual std::string ActionToString() const = 0;
    virtual std::string DirectionToString() const = 0;
    virtual std::string ProtocolToString() const = 0;

protected:
    State m_desiredState;
    Action m_action;
    Direction m_direction;
    Protocol m_protocol;
    std::string m_sourceAddress;
    std::string m_destinationAddress;
    std::string m_sourcePort;
    std::string m_destinationPort;

    virtual int ActionFromString(const std::string& str) = 0;
    virtual int DirectionFromString(const std::string& str) = 0;
    virtual int StateFromString(const std::string& str) = 0;
    virtual int ProtocolFromString(const std::string& str) = 0;

private:
    // TODO: allow this parse error to contain errors from several different things
    // This data will be surfaced to the Firewall reported state detail
    std::string m_parseError;
};

// TODO: policy should be an inner class on GenericFirewall
class Policy
{
public:
    typedef Rule::Action Action;
    typedef Rule::Direction Direction;

    Policy() = default;
    Policy(Action action, Direction direction) : m_action(action), m_direction(direction) {};
    virtual ~Policy() = default;

    virtual bool HasParseError() const
    {
        return !m_parseError.empty();
    }

    virtual std::string GetParseError() const
    {
        return m_parseError;
    }

    virtual Policy& Parse(const rapidjson::Value& policy);
    virtual void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

    // TODO: can this be hidden from the public interface without "friend"
    virtual std::string ActionToString() const;
    virtual std::string DirectionToString() const;

    virtual int ActionFromString(const std::string& str);
    virtual int DirectionFromString(const std::string& str);

private:
    std::string m_parseError;
    Action m_action;
    Direction m_direction;
};

class IpTablesRule : public Rule
{
public:
    IpTablesRule() = default;
    virtual ~IpTablesRule() = default;
    virtual std::string Specification() const override;

// protected:
//     friend IpTables;

    // TODO: maybe friend methods for these (this could help with sharing code between rules and policies)
    // TODO: can this be hidden from the public interface without "friend"
    virtual std::string ActionToString() const override;
    virtual std::string DirectionToString() const override;
    virtual std::string ProtocolToString() const override;

    virtual int ActionFromString(const std::string& str) override;
    virtual int DirectionFromString(const std::string& str) override;
    virtual int StateFromString(const std::string& str) override;
    virtual int ProtocolFromString(const std::string& str) override;
};

template<class RuleT>
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

    virtual ~GenericFirewall() = default;

    virtual Status GetStatus() const
    {
        return m_status;
    }

    virtual std::string GetStatusMessage() const
    {
        return m_statusMessage;
    }

    virtual State Detect() const = 0;
    virtual std::string Fingerprint() const = 0;
    virtual std::vector<Policy> GetDefaultPolicies() const = 0;

    virtual int SetRules(const std::vector<RuleT>& rules) = 0;
    virtual int SetDefaultPolicies(const std::vector<Policy> policies) = 0;

protected:
    Status m_status;
    std::string m_statusMessage;
};

class IpTables : public GenericFirewall<IpTablesRule>
{
public:
    typedef IpTablesRule Rule;
    typedef GenericFirewall::State State;

    IpTables() = default;
    ~IpTables() = default;

    State Detect() const override;
    std::string Fingerprint() const override;
    std::vector<Policy> GetDefaultPolicies() const override;

    int SetRules(const std::vector<Rule>& rules) override;
    int SetDefaultPolicies(const std::vector<Policy> policies) override;

private:
    int Add(const Rule& rule);
    int Remove(const Rule& rule);
    bool Exists(const Rule& rule) const;
};

class FirewallModuleBase
{
public:
    static const std::string m_moduleInfo;
    static const std::string m_firewallComponent;

    // Reported properties
    static const std::string m_firewallReportedFingerprint;
    static const std::string m_firewallReportedState;
    static const std::string m_firewallReportedDefaults;
    static const std::string m_firewallReportedConfigurationStatus;
    static const std::string m_firewallReportedConfigurationStatusDetail;

    // Desired properties
    static const std::string m_firewallDesiredDefaults;
    static const std::string m_firewallDesiredRules;

    FirewallModuleBase(unsigned int maxPayloadSizeBytes) : m_maxPayloadSizeBytes(maxPayloadSizeBytes) {}
    virtual ~FirewallModuleBase() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);

    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);

protected:
    // TODO: make all these const
    virtual int GetState(rapidjson::Writer<rapidjson::StringBuffer>& writer) = 0;
    virtual int GetFingerprint(rapidjson::Writer<rapidjson::StringBuffer>& writer) = 0;
    virtual int GetDefaultPolicies(rapidjson::Writer<rapidjson::StringBuffer>& writer) = 0;
    virtual int GetConfigurationStatus(rapidjson::Writer<rapidjson::StringBuffer>& writer) = 0;
    virtual int GetConfigurationStatusDetail(rapidjson::Writer<rapidjson::StringBuffer>& writer) = 0;

    virtual int SetDefaultPolicies(rapidjson::Document& document) = 0;
    virtual int SetRules(rapidjson::Document& document) = 0;

private:
    unsigned int m_maxPayloadSizeBytes;
    size_t m_lastPayloadHash;
};

template<class RuleT>
std::vector<RuleT> ParseRules(const rapidjson::Value& rules);

std::vector<Policy> ParsePolicies(const rapidjson::Value& rules);

template <class FirewallT>
class FirewallModule : public FirewallModuleBase
{
public:
    FirewallModule(unsigned int maxPayloadSize) : FirewallModuleBase(maxPayloadSize) {}
    ~FirewallModule() = default;

protected:
    typedef typename FirewallT::State State;
    typedef typename FirewallT::Status Status;
    // typedef typename FirewallT::Policy Policy;

    virtual int GetState(rapidjson::Writer<rapidjson::StringBuffer>& writer) override
    {
        State state = m_firewall.Detect();
        // TODO: Convert enum to string value
        int value = static_cast<int>(state);
        writer.Int(value);
        return 0;
    }

    virtual int GetFingerprint(rapidjson::Writer<rapidjson::StringBuffer>& writer) override
    {
        std::string fingerprint = m_firewall.Fingerprint();
        writer.String(fingerprint.c_str());
        return 0;
    }

    virtual int GetDefaultPolicies(rapidjson::Writer<rapidjson::StringBuffer>& writer) override
    {
        std::vector<Policy> policies = m_firewall.GetDefaultPolicies();
        writer.StartArray();
        for (const Policy& policy : policies)
        {
            policy.Serialize(writer);
        }
        writer.EndArray();
        return 0;
    }

    virtual int GetConfigurationStatus(rapidjson::Writer<rapidjson::StringBuffer>& writer) override
    {
        Status status = m_firewall.GetStatus();
        // TODO: Convert enum to string value
        int value = static_cast<int>(status);
        writer.Int(value);
        return 0;
    }

    virtual int GetConfigurationStatusDetail(rapidjson::Writer<rapidjson::StringBuffer>& writer) override
    {
        std::string statusDetail = m_firewall.GetStatusMessage();
        writer.String(statusDetail.c_str());
        return 0;
    }

    virtual int SetDefaultPolicies(rapidjson::Document& document) override
    {
        int status = 0;
        std::vector<Policy> policies = ParsePolicies(document);

        if (!policies.empty())
        {
            status = m_firewall.SetDefaultPolicies(policies);
        }
        else
        {
            status = -1;
        }

        return status;
    }

    virtual int SetRules(rapidjson::Document& document) override
    {
        int status = 0;

        std::vector<Rule> rules = ParseRules<Rule>(document);

        return status;
    }

private:
    FirewallT m_firewall;
};

typedef FirewallModule<IpTables> Firewall;