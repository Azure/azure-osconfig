// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Mmi.h>
#include <sstream>
#include <bits/stdc++.h>
#include "Firewall.h"

OSCONFIG_LOG_HANDLE FirewallLog::m_logFirewall = nullptr;
const string g_iptablesUtility = "iptables";
const string g_queryTableCommand = g_iptablesUtility + " -L -n -v --line-number -t ";
const string g_fingerprintPattern = R"""(\"([a-z0-9]{64})\")""";
const char g_firewallState[] = "firewallState";
const char g_firewallFingerprint[] = "firewallFingerprint";
const vector<string> g_tableNames = {"filter", "nat", "raw", "mangle", "security"};

FirewallObject::FirewallObject(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
}

int FirewallObjectBase::Set(
    MMI_HANDLE /* clientSession */,
    const char* /* componentName */,
    const char* /* objectName */,
    const MMI_JSON_STRING /* payload */,
    const int /* payloadSizeBytes */)
{
    OsConfigLogError(FirewallLog::Get(), "Set not implemented.");
    return ENOSYS;
}

int FirewallObjectBase::Get(
    MMI_HANDLE /*clientSession*/,
    const char* /*componentName*/,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;
    *payloadSizeBytes = 0;
    string payloadString = "";
    ClearTableObjects();
    vector<pair<string, string>> allTableStrings;
    GetAllTables(g_tableNames, allTableStrings);
    ParseAllTables(allTableStrings);
    if ((objectName != nullptr) && (strcmp(objectName, g_firewallState) == 0))
    {
        int state = GetFirewallState();
        payloadString = CreateStatePayload(state);
    }
    else if ((objectName != nullptr) && (strcmp(objectName, g_firewallFingerprint) == 0))
    {
        string fingerprint = GetFingerprint();
        payloadString = CreateFingerprintPayload(fingerprint);
    }
    else
    {
        status = EINVAL;
    }

    if ((status == MMI_OK) && (payloadString.empty() == false))
    {
        *payloadSizeBytes = payloadString.length();
        *payload = new (std::nothrow) char[*payloadSizeBytes];
        if (*payload != nullptr)
        {
            std::fill(*payload, *payload + *payloadSizeBytes, 0);
            std::memcpy(*payload, payloadString.c_str(), *payloadSizeBytes);
        }
    }

    return status;
}

string FirewallObjectBase::CreateStatePayload(int state)
{
    string payloadString = "";
    if ((state >= 0) && (state <= 2))
    {
        payloadString = to_string(state);
    }

    return payloadString;
}

string FirewallObjectBase::CreateFingerprintPayload(string fingerprint)
{
    string payloadString = "";
    string fingerprintString = char('"') + fingerprint + char('"');
    std::regex fingerprintPattern(g_fingerprintPattern);
    std::smatch pieces;
    if (std::regex_match(fingerprintString, pieces, fingerprintPattern) == true)
    {
        payloadString = fingerprintString;
    }

    return payloadString;
}

int FirewallObject::DetectUtility(string utility)
{
    const int commandSuccessExitCode = 0;
    const int commandNotFoundExitCode = 127;

    int status = utilityStatusCodeUnknown;
    if (utility == g_iptablesUtility)
    {
        string commandString = "iptables -L";
        char* char_output = nullptr;
        int tempStatus = ExecuteCommand(nullptr, commandString.c_str(), false, true, 0, 0, &char_output, nullptr, FirewallLog::Get());
        if (tempStatus == commandSuccessExitCode)
        {
            status = utilityStatusCodeInstalled;
        }
        else if (tempStatus == commandNotFoundExitCode)
        {
            status = utilityStatusCodeNotInstalled;
        }

        if (char_output != nullptr)
        {
            free(char_output);
        }
    }

    return status;
}

void FirewallObject::GetTable(string tableName, string& tableString)
{
    string commandString = g_queryTableCommand + tableName;
    tableString = "";
    char* output = nullptr;
    ExecuteCommand(nullptr, commandString.c_str(), false, true, 0, 0, &output, nullptr, FirewallLog::Get());
    tableString = (output != nullptr) ? string(output) : "";
    if (output != nullptr)
    {
        free(output);
    }
}

void FirewallObject::GetAllTables(vector<string> tableNames, vector<pair<string, string>>& allTableStrings)
{
    for (unsigned int i = 0; i < tableNames.size(); i++)
    {
        string tableString = "";
        GetTable(tableNames[i], tableString);
        if (!tableString.empty())
        {
            allTableStrings.push_back(make_pair(tableNames[i], tableString));
        }
    }
}

Rule* FirewallObjectBase::ParseRule(string ruleString)
{
    Rule* rule = nullptr;
    // Rules without target value are not parsed since we take no action on packets that match these rules
    std::regex rulePattern(R"""(\s*(\d+)\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\s+([\w-]+)\s+([\w+\*]+)\s+([\w+\*]+)\s+([0-9\.\!\/]+)\s+([0-9\.\!\/]+)\s+(.*))""" );
    std::smatch pieces;

    if (std::regex_match(ruleString, pieces, rulePattern))
    {
        if (pieces.size() != (ruleTokenRawOptionsIndex + 1))
        {
            return rule;
        }

        rule = new Rule();
        if (rule != nullptr)
        {
            rule->SetRuleNum(std::stoi(pieces.str(ruleTokenNumIndex)));
            rule->SetTarget(pieces.str(ruleTokenTargetIndex));
            rule->SetProtocol(pieces.str(ruleTokenProtocolIndex));
            rule->SetInInterface(pieces.str(ruleTokenInIndex));
            rule->SetOutInterface(pieces.str(ruleTokenOutIndex));
            rule->SetSource(pieces.str(ruleTokenSourceIndex));
            rule->SetDestination(pieces.str(ruleTokenDestinationIndex));
            rule->SetRawOptions(pieces.str(ruleTokenRawOptionsIndex));
        }
    }

    return rule;
}

Rule::Rule(int ruleNum, string target, string protocol, string source, string destination, string sourcePort, string destinationPort, string inInterface, string outInterface, string rawOptions)
{
    m_ruleNum = ruleNum;
    m_target = target;
    m_protocol = protocol;
    m_source = source;
    m_destination = destination;
    m_sourcePort = sourcePort;
    m_destinationPort = destinationPort;
    m_inInterface = inInterface;
    m_outInterface = outInterface;
    m_rawOptions = rawOptions;
}

Rule::Rule()
{
    Rule(0, "", "", "", "", "", "", "", "", "");
}

int Rule::GetRuleNum()
{
    return m_ruleNum;
}

string Rule::GetTarget()
{
    return m_target;
}

string Rule::GetProtocol()
{
    return m_protocol;
}

string Rule::GetSource()
{
    return m_source;
}

string Rule::GetDestination()
{
    return m_destination;
}

string Rule::GetSourcePort()
{
    return m_sourcePort;
}

string Rule::GetDestinationPort()
{
    return m_destinationPort;
}

string Rule::GetInInterface()
{
    return m_inInterface;
}

string Rule::GetOutInterface()
{
    return m_outInterface;
}

string Rule::GetRawOptions()
{
    return m_rawOptions;
}

void Rule::SetRuleNum(int ruleNum)
{
    m_ruleNum = ruleNum;
}

void Rule::SetTarget(string target)
{
    m_target = target;
}

void Rule::SetProtocol(string protocol)
{
    m_protocol = protocol;
}

void Rule::SetSource(string source)
{
    m_source = source;
}

void Rule::SetDestination(string destination)
{
    m_destination = destination;
}

void Rule::SetSourcePort(string sourcePort)
{
    m_sourcePort = sourcePort;
}

void Rule::SetDestinationPort(string destinationPort)
{
    m_destinationPort = destinationPort;
}

void Rule::SetInInterface(string inInterface)
{
    m_inInterface = inInterface;
}

void Rule::SetOutInterface(string outInterface)
{
    m_outInterface = outInterface;
}

void Rule::SetRawOptions(string rawOptions)
{
    m_rawOptions = rawOptions;
}

Chain::Chain(string chainName)
{
    m_chainName = chainName;
    m_chainPolicy = "";
    m_rules = {};
}

Chain::Chain()
{
    Chain("");
}

void Chain::SetChainName(string chainName)
{
    m_chainName = chainName;
}

void Chain::SetChainPolicy(string chainPolicy)
{
    m_chainPolicy = chainPolicy;
}

string Chain::GetChainName()
{
    return m_chainName;
}

string Chain::GetChainPolicy()
{
    return m_chainPolicy;
}

int Chain::GetRuleCount()
{
    return (int) m_rules.size();
}

void Chain::Append(Rule* rule)
{
    m_rules.push_back(rule);
}

Table::Table(string tableName)
{
    m_tableName = tableName;
    m_chains = {};
}

Table::Table()
{
    Table("");
}

void Table::SetTableName(string tableName)
{
    m_tableName = tableName;
}

string Table::GetTableName()
{
    return m_tableName;
}

int Table::GetChainCount()
{
    return (int) m_chains.size();
}

void Table::Append(Chain* chain)
{
    m_chains.push_back(chain);
}

Chain* FirewallObjectBase::ParseChain(string chainString)
{
    Chain* chain = nullptr;
    const unsigned int chainNameIndex = 1;
    const unsigned int chainPolicyIndex = 2;
    std::regex userChainPattern(R"""(\s*Chain\s+([^\s]+)\s+.*references.*)""");
    std::regex chainPattern(R"""(\s*Chain\s+([^\s]+)\s+\(policy\s+([^\s]+)\s+.*)""");
    std::smatch pieces;
    std::istringstream iss(chainString);
    std::string row;
    bool foundPolicy = false;
    if (getline(iss, row, '\n'))
    {
        if (std::regex_match(row, pieces, chainPattern))
        {
            foundPolicy = true;
            chain = new Chain();
        }
        else if (std::regex_match(row, pieces, userChainPattern))
        {
            chain = new Chain();
        }
    }

    if (chain == nullptr)
    {
        return chain;
    }

    chain->SetChainName(pieces.str(chainNameIndex));
    if (foundPolicy)
    {
        chain->SetChainPolicy(pieces.str(chainPolicyIndex));
    }

    while (getline(iss, row, '\n'))
    {
        Rule* rule = ParseRule(row);
        if (rule != nullptr)
        {
            chain->Append(rule);
        }
    }

    return chain;
}

Table* FirewallObjectBase::ParseTable(string tableName, string tableString)
{
    Table* table = tableName.empty() ? (new Table()) : (new Table(tableName));
    if (table == nullptr)
    {
        return table;
    }

    std::regex simpleChainPattern(R"""(\s*Chain\s+([^\s]+)\s+.*)""");
    std::smatch pieces;
    std::istringstream iss(tableString);
    std::string row;
    std::string chainString = "";
    // A table string contains several chain strings
    // A chain row (found by simpleChainPattern) is the beginning of a new chain
    // When a chain row is found, convert current chain string to a chain object
    while (getline(iss, row, '\n'))
    {
        if (std::regex_match(row, pieces, simpleChainPattern) && (!chainString.empty()))
        {
            Chain* chain = ParseChain(chainString);
            chainString = "";
            if (chain != nullptr)
            {
                table->Append(chain);
            }
        }

        chainString += row + char('\n');
    }

    // Get the last chain in the table
    if (!chainString.empty())
    {
        Chain* chain = ParseChain(chainString);
        if (chain != nullptr)
        {
            table->Append(chain);
        }
    }

    return table;
}

template <typename T>
void ClearVector(vector<T*> myVector)
{
    for (T* t : myVector)
    {
        if (t != nullptr)
        {
            delete t;
        }
    }
    myVector.clear();
}

Chain::~Chain()
{
    ClearVector(m_rules);
}

Table::~Table()
{
    ClearVector(m_chains);
}

vector<Chain*> Table::GetChains()
{
    return m_chains;
}

void FirewallObjectBase::AppendTable(Table* table)
{
    m_tables.push_back(table);
}

vector<Table*> FirewallObjectBase::GetTableObjects()
{
    return m_tables;
}

int FirewallObjectBase::GetTableCount()
{
    return (int)m_tables.size();
}

int FirewallObjectBase::GetFirewallState()
{
    int state = firewallStateCodeDisabled;
    int utilityStatus = utilityStatusCodeUnknown;
    bool isPolicyChanged = false;
    bool hasRules = false;
    utilityStatus = DetectUtility(g_iptablesUtility);
    if (utilityStatus == utilityStatusCodeNotInstalled)
    {
        return state;
    }

    if (utilityStatus == utilityStatusCodeUnknown)
    {
        state = firewallStateCodeUnknown;
        return state;
    }

    vector<Table*> tableVector = GetTableObjects();
    for (Table* table : tableVector)
    {
        vector<Chain*> chainVector = table->GetChains();
        for (Chain* chain : chainVector)
        {
            string policy = "";
            string acceptPolicy = "ACCEPT";
            policy = chain->GetChainPolicy();
            if ((!policy.empty()) && (policy != acceptPolicy))
            {
                isPolicyChanged = true;
            }
            int ruleCount = chain->GetRuleCount();
            if (ruleCount > 0)
            {
                hasRules = true;
            }

            if ((isPolicyChanged == true) || (hasRules == true))
            {
                state = firewallStateCodeEnabled;
                return state;
            }
        }
    }

    return state;
}

vector<Rule*> Chain::GetRules()
{
    return m_rules;
}

string FirewallObjectBase::RulesToString(vector<Rule*> rules)
{
    string result = "";
    string whitespace = " ";
    for (Rule* rule : rules)
    {
        if (rule != nullptr)
        {
            result += to_string(rule->GetRuleNum()) + whitespace;
            result += rule->GetTarget() + whitespace;
            result += rule->GetProtocol()+ whitespace;
            result += rule->GetSource() + whitespace;
            result += rule->GetDestination() + whitespace;
            result += rule->GetInInterface() + whitespace;
            result += rule->GetOutInterface() + whitespace;
            result += rule->GetRawOptions() + whitespace;
        }
    }

    return result;
}

string FirewallObjectBase::ChainsToString(vector<Chain*> chains)
{
    string result = "";
    string whitespace = " ";
    for (Chain* chain : chains)
    {
        if (chain != nullptr)
        {
            result += chain->GetChainName() + whitespace;
            result += chain->GetChainPolicy() + whitespace;
            result += RulesToString(chain->GetRules());
            result += whitespace;
        }
    }

    return result;
}

string FirewallObjectBase::TablesToString(vector<Table*> tables)
{
    string result = "";
    string whitespace = " ";
    for (Table* table : tables)
    {
        if (table != nullptr)
        {
            result += table->GetTableName() + whitespace;
            result += ChainsToString(table->GetChains());
            result += whitespace;
        }
    }

    return result;
}

string FirewallObjectBase::FirewallRulesToString()
{
    return TablesToString(GetTableObjects());
}

string FirewallObjectBase::GetFingerprint()
{
    string firewallRulesString = FirewallRulesToString();
    string hashCommand = "echo \"" + firewallRulesString + "\"";
    string hashString = HashCommand(hashCommand.c_str(), FirewallLog::Get());
    return hashString;
}

void FirewallObjectBase::ParseAllTables(vector<pair<string, string>>& allTableStrings)
{
    for (pair<string, string>& pair : allTableStrings)
    {
        Table* table = ParseTable(pair.first, pair.second);
        if (table != nullptr)
        {
            AppendTable(table);
        }
    }
}

void FirewallObjectBase::ClearTableObjects()
{
    ClearVector(m_tables);
    m_tables = {};
}

FirewallObject::~FirewallObject()
{
    ClearTableObjects();
}