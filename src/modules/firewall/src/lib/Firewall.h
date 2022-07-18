// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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
    Rule(int ruleNum, std::string target, std::string protocol, std::string source, std::string destination, std::string sourcePort, std::string destinationPort, std::string inInterface, std::string outInterface, std::string rawOptions);
    Rule();
    void SetRuleNum(int ruleNum);
    void SetTarget(std::string target);
    void SetProtocol(std::string protocol);
    void SetSource(std::string source);
    void SetDestination(std::string destination);
    void SetSourcePort(std::string sourcePort);
    void SetDestinationPort(std::string destinationPort);
    void SetInInterface(std::string inInterface);
    void SetOutInterface(std::string outInterface);
    void SetRawOptions(std::string rawOptions);

    int GetRuleNum();
    std::string GetTarget();
    std::string GetProtocol();
    std::string GetSource();
    std::string GetDestination();
    std::string GetSourcePort();
    std::string GetDestinationPort();
    std::string GetInInterface();
    std::string GetOutInterface();
    std::string GetRawOptions();

private:
    int m_ruleNum;
    std::string m_policy;
    std::string m_target;
    std::string m_protocol;
    std::string m_source;
    std::string m_destination;
    std::string m_sourcePort;
    std::string m_destinationPort;
    std::string m_inInterface;
    std::string m_outInterface;
    std::string m_rawOptions;
};

enum FirewallStateCode
{
    firewallStateCodeUnknown = 0,
    firewallStateCodeEnabled,
    firewallStateCodeDisabled
};

enum UtilityStatusCode
{
    utilityStatusCodeUnknown = 0,
    utilityStatusCodeInstalled,
    utilityStatusCodeNotInstalled
};

enum RuleToken
{
    ruleTokenNumIndex = 1,
    ruleTokenPacketsIndex,
    ruleTokenBytesIndex,
    ruleTokenTargetIndex,
    ruleTokenProtocolIndex,
    ruleTokenOptIndex,
    ruleTokenInIndex,
    ruleTokenOutIndex,
    ruleTokenSourceIndex,
    ruleTokenDestinationIndex,
    ruleTokenRawOptionsIndex
};

class Chain
{
public:
    Chain();
    Chain(std::string chainName);
    ~Chain();
    void SetChainName(std::string chainName);
    void SetChainPolicy(std::string chainPolicy);
    std::string GetChainName();
    std::string GetChainPolicy();
    int GetRuleCount();
    std::vector<Rule*> GetRules();
    void Append(Rule* rule);

private:
    std::string m_chainName;
    std::string m_chainPolicy;
    std::vector<Rule*> m_rules;
};

class Table
{
public:
    Table();
    Table(std::string tableName);
    ~Table();
    void SetTableName(std::string tableName);
    std::string GetTableName();
    int GetChainCount();
    void Append(Chain* chain);
    std::vector<Chain*> GetChains();

private:
    std::string m_tableName;
    std::vector<Chain*> m_chains;
};

class FirewallObjectBase
{
public:
    static const char* m_firewallInfo;

    virtual ~FirewallObjectBase() {};
    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);

    virtual int DetectUtility(std::string utility) = 0;
    virtual void GetTable(std::string tableName, std::string& tableString) = 0;
    virtual void GetAllTables(std::vector<std::string> tableNames, std::vector<std::pair<std::string, std::string>>& allTableStrings) = 0;
    Rule* ParseRule(std::string ruleString);
    Chain* ParseChain(std::string chainString);
    Table* ParseTable(std::string tableName, std::string tableString);
    void ParseAllTables(std::vector<std::pair<std::string, std::string>>& allTableStrings);
    void AppendTable(Table* table);
    std::vector<Table*> GetTableObjects();
    int GetTableCount();
    int GetFirewallState();
    std::string RulesToString(std::vector<Rule*> rules);
    std::string ChainsToString(std::vector<Chain*> chains);
    std::string TablesToString(std::vector<Table*> tables);
    std::string FirewallRulesToString();
    std::string GetFingerprint();
    std::string CreateStatePayload(int state);
    std::string CreateFingerprintPayload(std::string fingerprint);
    void ClearTableObjects();
    unsigned int m_maxPayloadSizeBytes;

private:
    std::vector<Table*> m_tables;
};

class FirewallObject : public FirewallObjectBase
{
public:
    FirewallObject(unsigned int maxPayloadSizeBytes);
    ~FirewallObject();
    int DetectUtility(std::string utility);
    void GetTable(std::string tableName, std::string& tableString);
    void GetAllTables(std::vector<std::string> tableNames, std::vector<std::pair<std::string, std::string>>& allTableStrings);
};