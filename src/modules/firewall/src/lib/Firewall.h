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
using namespace std;

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
    Rule(int ruleNum, string target, string protocol, string source, string destination, string sourcePort, string destinationPort, string inInterface, string outInterface, string rawOptions);
    Rule();
    void SetRuleNum(int ruleNum);
    void SetTarget(string target);
    void SetProtocol(string protocol);
    void SetSource(string source);
    void SetDestination(string destination);
    void SetSourcePort(string sourcePort);
    void SetDestinationPort(string destinationPort);
    void SetInInterface(string inInterface);
    void SetOutInterface(string outInterface);
    void SetRawOptions(string rawOptions);

    int GetRuleNum();
    string GetTarget();
    string GetProtocol();
    string GetSource();
    string GetDestination();
    string GetSourcePort();
    string GetDestinationPort();
    string GetInInterface();
    string GetOutInterface();
    string GetRawOptions();

private:
    int m_ruleNum;
    string m_policy;
    string m_target;
    string m_protocol;
    string m_source;
    string m_destination;
    string m_sourcePort;
    string m_destinationPort;
    string m_inInterface;
    string m_outInterface;
    string m_rawOptions;
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
    Chain(string chainName);
    ~Chain();
    void SetChainName(string chainName);
    void SetChainPolicy(string chainPolicy);
    string GetChainName();
    string GetChainPolicy();
    int GetRuleCount();
    vector<Rule*> GetRules();
    void Append(Rule* rule);

private:
    string m_chainName;
    string m_chainPolicy;
    vector<Rule*> m_rules;
};

class Table
{
public:
    Table();
    Table(string tableName);
    ~Table();
    void SetTableName(string tableName);
    string GetTableName();
    int GetChainCount();
    void Append(Chain* chain);
    vector<Chain*> GetChains();

private:
    string m_tableName;
    vector<Chain*> m_chains;
};

class FirewallObjectBase
{
public:
    virtual ~FirewallObjectBase() {};
    int Get(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING*  payload, int* payloadSizeBytes);
    int Set(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int DetectUtility(string utility) = 0;
    virtual void GetTable(string tableName, string& tableString) = 0;
    virtual void GetAllTables(vector<string> tableNames, vector<pair<string, string>>& allTableStrings) = 0;
    Rule* ParseRule(string ruleString);
    Chain* ParseChain(string chainString);
    Table* ParseTable(string tableName, string tableString);
    void ParseAllTables(vector<pair<string, string>>& allTableStrings);
    void AppendTable(Table* table);
    vector<Table*> GetTableObjects();
    int GetTableCount();
    int GetFirewallState();
    string RulesToString(vector<Rule*> rules);
    string ChainsToString(vector<Chain*> chains);
    string TablesToString(vector<Table*> tables);
    string FirewallRulesToString();
    string GetFingerprint();
    string CreateStatePayload(int state);
    string CreateFingerprintPayload(string fingerprint);
    void ClearTableObjects();
    unsigned int m_maxPayloadSizeBytes;

private:
    vector<Table*> m_tables;
};

class FirewallObject : public FirewallObjectBase
{
public:
    FirewallObject(unsigned int maxPayloadSizeBytes);
    ~FirewallObject();
    int DetectUtility(string utility);
    void GetTable(string tableName, string& tableString);
    void GetAllTables(vector<string> tableNames, vector<pair<string, string>>& allTableStrings);
};