// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// FIXME: clean up these "#includes"
#include <string>
#include <vector>
#include <memory>
#include <Mmi.h>
#include <Logging.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/schema.h>
#include <regex>

#define FIREWALL_LOGFILE "/var/log/osconfig_firewall.log"
#define FIREWALL_ROLLEDLOGFILE "/var/log/osconfig_firewall.bak"

class FirewallLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_logFirewall;
    }
    static void OpenLog()
    {
        m_logFirewall = ::OpenLog(FIREWALL_LOGFILE, FIREWALL_ROLLEDLOGFILE);
    }
    static void CloseLog()
    {
        ::CloseLog(&m_logFirewall);
    }

private:
    static OSCONFIG_LOG_HANDLE m_logFirewall;
};

class Rule
{
public:
    // enum Target
    // {
    //     ACCEPT = 0, // Default
    //     REJECT, // TODO: maybe call this RETURN instead?
    //     DROP
    //     // TODO: there are others (), should they be included
    // };

    // enum Protocol
    // {
    //     ALL = 0, // Default
    //     TCP,
    //     UDP,
    //     ICMP,
    // };

    Rule() = default;
    ~Rule() = default;

    static Rule Parse(const std::string ruleString);
    bool HasParseError() { return m_parseError; }
    std::string ToString();
    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

private:
    // TODO: what is the difference between Target and Policy?
    int m_number;
    // int packets;
    // int bytes;

    std::string m_target;
    // Target m_target; // Can this be an enum ???

    std::string m_inInterface;
    std::string m_outInterface;
    // Protocol m_protocol;
    std::string m_protocol;

    // TODO: can these be specific types/containers instead of just strings to capture the address/port ?
    std::string m_source;
    std::string m_destination;

    // TODO: can this be an enum ?
    std::string m_rawOptions;
    bool m_parseError;

    Rule(int number, std::string target, std::string inInterface, std::string outInterface, std::string protocol, std::string source, std::string destination, std::string rawOptions, bool parseError) :
        m_number(number),
        m_target(target),
        m_inInterface(inInterface),
        m_outInterface(outInterface),
        m_protocol(protocol),
        m_source(source),
        m_destination(destination),
        m_rawOptions(rawOptions),
        m_parseError(parseError) {}
};

class Chain
{
public:
    Chain() = default;
    ~Chain() = default;

    static Chain Parse(const std::string chainString);
    bool HasParseError() { return m_parseError; }
    std::string ToString();
    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

private:
    Chain(std::string name, std::string policy, std::vector<Rule> rules) : m_name(name), m_policy(policy), m_rules(rules) {}

    std::string m_name;
    std::string m_policy;
    std::vector<Rule> m_rules;
    bool m_parseError;
};

class Table
{
public:
    Table() = default;
    ~Table() = default;

    static Table Parse(const std::string name, const std::string table);
    bool HasParseError() { return m_parseError; }
    std::string ToString();
    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

private:
    std::string m_name;
    std::vector<Chain> m_chains;
    bool m_parseError;

    Table(std::string name, std::vector<Chain> chains) : m_name(name), m_chains(chains) {}
};

class Utility
{
public:
    virtual ~Utility() = default;

    // TODO: add a method to check if a utility exists

    virtual std::vector<Table> List() = 0;
    // TODO: add a default implmentation for Create/Delete that calls an
    // implmentation specific method to apply the change
    virtual int Create(const Rule& rule) = 0;
    virtual int Delete(const Rule& rule) = 0;
    virtual std::string Hash() const = 0;

protected:
    std::vector<std::string> m_tableNames;

    Utility(std::vector<std::string> tableNames) : m_tableNames(tableNames) {}
    // virtual int Apply(const Rule& rule);
};

class Iptables : public Utility
{
public:
    Iptables() : Utility({ "filter" }) {}
    // TODO: use all tables and make the vector a static member of Iptables
    // Iptables() : Utility({"filter", "nat", "raw", "mangle", "security"}) {}
    ~Iptables() = default;

    std::vector<Table> List() override;
    int Create(const Rule& rule) override;
    int Delete(const Rule& rule) override;
    std::string Hash() const override;

private:
    std::map<std::string, Table> m_tables;
    std::map<std::string, std::string> m_tableHashes;

    Table GetTable(const std::string tableName);
};

enum class FirewallState
{
    Unknown = 0,
    Enabled,
    Disabled
};

class FirewallBase
{
public:
    static const std::string m_firewallComponent;
    static const std::string m_firewallFingerprint;
    static const std::string m_firewallState;
    static const std::string m_firewallTables;

    static const std::string m_moduleInfo;

    FirewallBase(unsigned int maxPayloadSize);
    virtual ~FirewallBase() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes) = 0;
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes) = 0;

protected:
    unsigned int m_maxPayloadSizeBytes;
};

// template <class T>
class Firewall : public FirewallBase
{
public:
    Firewall(unsigned int maxPayloadSize);
    ~Firewall() = default;

    int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes) override;
    int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes) override;

private:
    // T m_utility;
    Iptables m_utility;

    FirewallState GetState();
    std::string GetFingerprint();
    std::vector<Table> GetTables();
};