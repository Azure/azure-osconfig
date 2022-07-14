// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Mmi.h>
#include <sstream>
#include <bits/stdc++.h>
#include "Firewall.h"

const char g_firewallComponent[] = "Firewall";
const char g_firewallState[] = "firewallState";
const char g_firewallFingerprint[] = "firewallFingerprint";

const std::string g_iptablesUtility = "iptables";
const std::string g_queryTableCommand = g_iptablesUtility + " -L -n -v --line-number -t ";
const std::string g_fingerprintPattern = R"""(\"([a-z0-9]{64})\")""";

const std::vector<std::string> g_tableNames = {"filter", "nat", "raw", "mangle", "security"};

OSCONFIG_LOG_HANDLE FirewallLog::m_logFirewall = nullptr;

FirewallObject::FirewallObject(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
}

int FirewallObjectBase::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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
        size_t len = strlen(g_firewallInfo);
        *payload = new (std::nothrow) char[len];

        if (nullptr == *payload)
        {
            OsConfigLogError(FirewallLog::Get(), "Failed to allocate memory for payload");
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, g_firewallInfo, len);
            *payloadSizeBytes = len;
        }
    }

    return status;
}

int FirewallObjectBase::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    OsConfigLogError(FirewallLog::Get(), "Set not implemented.");
    return ENOSYS;
}

int FirewallObjectBase::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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
    else if (0 != strcmp(componentName, g_firewallComponent))
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid component name: %s", componentName);
    }
    else
    {
        std::string payloadString = "";
        std::vector<std::pair<std::string, std::string>> allTableStrings;

        *payloadSizeBytes = 0;
        *payload = nullptr;

        ClearTableObjects();
        GetAllTables(g_tableNames, allTableStrings);
        ParseAllTables(allTableStrings);

        if (0 == strcmp(objectName, g_firewallState))
        {
            int state = GetFirewallState();
            payloadString = CreateStatePayload(state);
        }
        else if (0 == strcmp(objectName, g_firewallFingerprint))
        {
            std::string fingerprint = GetFingerprint();
            payloadString = CreateFingerprintPayload(fingerprint);
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "Invalid object name: %s", objectName);
            status = EINVAL;
        }

        if (MMI_OK == status)
        {
            *payloadSizeBytes = payloadString.length();
            *payload = new (std::nothrow) char[*payloadSizeBytes];

            if (*payload != nullptr)
            {
                std::fill(*payload, *payload + *payloadSizeBytes, 0);
                std::memcpy(*payload, payloadString.c_str(), *payloadSizeBytes);
            }
        }
    }

    return status;
}

std::string FirewallObjectBase::CreateStatePayload(int state)
{
    std::string payloadString = "";
    if ((state >= 0) && (state <= 2))
    {
        payloadString = std::to_string(state);
    }

    return payloadString;
}

std::string FirewallObjectBase::CreateFingerprintPayload(std::string fingerprint)
{
    std::string payloadString = "";
    std::string fingerprintString = char('"') + fingerprint + char('"');
    std::regex fingerprintPattern(g_fingerprintPattern);
    std::smatch pieces;
    if (std::regex_match(fingerprintString, pieces, fingerprintPattern) == true)
    {
        payloadString = fingerprintString;
    }

    return payloadString;
}

int FirewallObject::DetectUtility(std::string utility)
{
    const int commandSuccessExitCode = 0;
    const int commandNotFoundExitCode = 127;

    int status = utilityStatusCodeUnknown;
    if (utility == g_iptablesUtility)
    {
        std::string commandString = "iptables -L";
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

void FirewallObject::GetTable(std::string tableName, std::string& tableString)
{
    std::string commandString = g_queryTableCommand + tableName;
    tableString = "";
    char* output = nullptr;
    ExecuteCommand(nullptr, commandString.c_str(), false, true, 0, 0, &output, nullptr, FirewallLog::Get());
    tableString = (output != nullptr) ? std::string(output) : "";
    if (output != nullptr)
    {
        free(output);
    }
}

void FirewallObject::GetAllTables(std::vector<std::string> tableNames, std::vector<std::pair<std::string, std::string>>& allTableStrings)
{
    for (unsigned int i = 0; i < tableNames.size(); i++)
    {
        std::string tableString = "";
        GetTable(tableNames[i], tableString);
        if (!tableString.empty())
        {
            allTableStrings.push_back(std::make_pair(tableNames[i], tableString));
        }
    }
}

Rule* FirewallObjectBase::ParseRule(std::string ruleString)
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

Rule::Rule(int ruleNum, std::string target, std::string protocol, std::string source, std::string destination, std::string sourcePort, std::string destinationPort, std::string inInterface, std::string outInterface, std::string rawOptions)
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

std::string Rule::GetTarget()
{
    return m_target;
}

std::string Rule::GetProtocol()
{
    return m_protocol;
}

std::string Rule::GetSource()
{
    return m_source;
}

std::string Rule::GetDestination()
{
    return m_destination;
}

std::string Rule::GetSourcePort()
{
    return m_sourcePort;
}

std::string Rule::GetDestinationPort()
{
    return m_destinationPort;
}

std::string Rule::GetInInterface()
{
    return m_inInterface;
}

std::string Rule::GetOutInterface()
{
    return m_outInterface;
}

std::string Rule::GetRawOptions()
{
    return m_rawOptions;
}

void Rule::SetRuleNum(int ruleNum)
{
    m_ruleNum = ruleNum;
}

void Rule::SetTarget(std::string target)
{
    m_target = target;
}

void Rule::SetProtocol(std::string protocol)
{
    m_protocol = protocol;
}

void Rule::SetSource(std::string source)
{
    m_source = source;
}

void Rule::SetDestination(std::string destination)
{
    m_destination = destination;
}

void Rule::SetSourcePort(std::string sourcePort)
{
    m_sourcePort = sourcePort;
}

void Rule::SetDestinationPort(std::string destinationPort)
{
    m_destinationPort = destinationPort;
}

void Rule::SetInInterface(std::string inInterface)
{
    m_inInterface = inInterface;
}

void Rule::SetOutInterface(std::string outInterface)
{
    m_outInterface = outInterface;
}

void Rule::SetRawOptions(std::string rawOptions)
{
    m_rawOptions = rawOptions;
}

Chain::Chain(std::string chainName)
{
    m_chainName = chainName;
    m_chainPolicy = "";
    m_rules = {};
}

Chain::Chain()
{
    Chain("");
}

void Chain::SetChainName(std::string chainName)
{
    m_chainName = chainName;
}

void Chain::SetChainPolicy(std::string chainPolicy)
{
    m_chainPolicy = chainPolicy;
}

std::string Chain::GetChainName()
{
    return m_chainName;
}

std::string Chain::GetChainPolicy()
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

Table::Table(std::string tableName)
{
    m_tableName = tableName;
    m_chains = {};
}

Table::Table()
{
    Table("");
}

void Table::SetTableName(std::string tableName)
{
    m_tableName = tableName;
}

std::string Table::GetTableName()
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

Chain* FirewallObjectBase::ParseChain(std::string chainString)
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

Table* FirewallObjectBase::ParseTable(std::string tableName, std::string tableString)
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
    // A table std::string contains several chain std::strings
    // A chain row (found by simpleChainPattern) is the beginning of a new chain
    // When a chain row is found, convert current chain std::string to a chain object
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
void ClearVector(std::vector<T*> myVector)
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

std::vector<Chain*> Table::GetChains()
{
    return m_chains;
}

void FirewallObjectBase::AppendTable(Table* table)
{
    m_tables.push_back(table);
}

std::vector<Table*> FirewallObjectBase::GetTableObjects()
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

    std::vector<Table*> tableVector = GetTableObjects();
    for (Table* table : tableVector)
    {
        std::vector<Chain*> chainVector = table->GetChains();
        for (Chain* chain : chainVector)
        {
            std::string policy = "";
            std::string acceptPolicy = "ACCEPT";
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

std::vector<Rule*> Chain::GetRules()
{
    return m_rules;
}

std::string FirewallObjectBase::RulesToString(std::vector<Rule*> rules)
{
    std::string result = "";
    std::string whitespace = " ";
    for (Rule* rule : rules)
    {
        if (rule != nullptr)
        {
            result += std::to_string(rule->GetRuleNum()) + whitespace;
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

std::string FirewallObjectBase::ChainsToString(std::vector<Chain*> chains)
{
    std::string result = "";
    std::string whitespace = " ";
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

std::string FirewallObjectBase::TablesToString(std::vector<Table*> tables)
{
    std::string result = "";
    std::string whitespace = " ";
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

std::string FirewallObjectBase::FirewallRulesToString()
{
    return TablesToString(GetTableObjects());
}

std::string FirewallObjectBase::GetFingerprint()
{
    std::string firewallRulesString = FirewallRulesToString();
    std::string hashCommand = "echo \"" + firewallRulesString + "\"";
    std::string hashString = HashCommand(hashCommand.c_str(), FirewallLog::Get());
    return hashString;
}

void FirewallObjectBase::ParseAllTables(std::vector<std::pair<std::string, std::string>>& allTableStrings)
{
    for (std::pair<std::string, std::string>& pair : allTableStrings)
    {
        Table* table = ParseTable(pair.first,pair.second);
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