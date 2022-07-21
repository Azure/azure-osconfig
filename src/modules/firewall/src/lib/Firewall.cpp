// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// FIXME: clean up these "#includes"
#include <CommonUtils.h>
#include <Mmi.h>
#include <sstream>
#include <bits/stdc++.h>
#include "Firewall.h"

const std::string FirewallBase::m_firewallComponent = "Firewall";
const std::string FirewallBase::m_firewallFingerprint = "firewallFingerprint";
const std::string FirewallBase::m_firewallState = "firewallState";
const std::string FirewallBase::m_firewallTables = "tables";

const std::string FirewallBase::m_moduleInfo = R""""({
    "Name": "Firewall",
    "Description": "Provides functionality to remotely manage firewall rules on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 2,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["Firewall"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

// FIXME: clean up all these constants
const std::string g_iptablesUtility = "iptables";
const std::string g_queryTableCommand = g_iptablesUtility + " -L -n -v --line-number -t ";
const std::string g_fingerprintPattern = R"""(\"([a-z0-9]{64})\")""";

const std::vector<std::string> g_tableNames = {"filter", "nat", "raw", "mangle", "security"};

OSCONFIG_LOG_HANDLE FirewallLog::m_logFirewall = nullptr;

Rule Rule::Parse(const std::string ruleString)
{
    Rule rule;

    std::regex rulePattern(R"""(\s*(\d+)\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\s+([\w-]+)\s+([\w+\*]+)\s+([\w+\*]+)\s+([0-9\.\!\/]+)\s+([0-9\.\!\/]+)\s+(.*))""" );
    std::smatch pieces;

    if (std::regex_match(ruleString, pieces, rulePattern))
    {
        // FIXME: fix 'magic' numbers here
        if (11 <= pieces.size())
        {
            int number = std::stoi(pieces.str(1));
            std::string target = pieces.str(4);
            std::string in = pieces.str(5);
            std::string out = pieces.str(7);
            std::string protocol = pieces.str(8);
            std::string source = pieces.str(9);
            std::string destination = pieces.str(10);
            std::string options = pieces.str(11);
            rule = Rule(number, target, in, out, protocol, source, destination, options, false);
        }
        else
        {
            std::cout << "Error: invalid rule string (regex error )" << pieces.size() << std::endl;
            for (auto& piece : pieces)
            {
                std::cout << piece << std::endl;
            }
            rule.m_parseError = true;
        }
    }
    else
    {
        std::cout << "Error: invalid rule string" << std::endl;
        rule.m_parseError = true;
    }

    return rule;
}

Chain Chain::Parse(const std::string chainString)
{
    Chain chain;
    std::istringstream iss(chainString);
    std::string line;

    // The first line of the chain contains the chain name and policy
    std::getline(iss, line, '\n');

    std::regex chainPattern(R"""(Chain\s+([^\s]+)\s+((\(policy)\s+([^\s]+)\s+([^\s]+)\s+(packets,)\s+([^\s]+)\s+(bytes\)))?)""");
    // TODO: figure this out
    // std::regex userChainPattern(R"""(\s*Chain\s+([^\s]+)\s+.*references.*)""");
    std::smatch pieces;

    if (std::regex_match(line, pieces, chainPattern))
    {
        if (2 != pieces.size())
        {
            std::string name = pieces.str(1);
            std::string policy = pieces.str(4);
            std::vector<Rule> rules;

            // The second line contains the column names:
            // num   pkts bytes target     prot opt in     out     source               destination
            std::getline(iss, line, '\n');

            // TODO: validate/parse the column names (use column names to help parse each rule)

            // The remaining lines contain the rules
            while (std::getline(iss, line, '\n'))
            {
                Rule rule = Rule::Parse(line);
                // TODO: validate the rule and set parse error if it is invalid
                rules.push_back(rule);
            }

            chain = Chain(name, policy, rules);
        }
        else
        {
            // TODO: better error messages
            OsConfigLogError(FirewallLog::Get(), "Invalid firewall chain");
        }
    }
    else
    {
        // TODO: better error messages
        OsConfigLogError(FirewallLog::Get(), "Invalid firewall chain");
    }

    return chain;
}

Table Table::Parse(const std::string name, const std::string tableString)
{
    // Iterate over the chains in the table
    std::vector<Chain> chains;

    // Use the blank line between each chain to split up the table
    std::istringstream iss(tableString);
    std::string line;
    std::string chainString;

    while (std::getline(iss, line, '\n'))
    {
        if (line.empty())
        {
            Chain chain = Chain::Parse(chainString);
            chains.push_back(chain);
            chainString.clear();
        }
        else
        {
            chainString += line + '\n';
        }
    }

    return Table(name, chains);
}

std::string Hash(const std::string str)
{
    char* hash = nullptr;
    std::string command = "echo \"" + str + "\"";
    // REVIEW: why not openssl ???
    return (hash = HashCommand(command.c_str(), FirewallLog::Get())) ? hash : "";
}

// REVIEW: C++ 17 - return a tuple of (int status, std::string result) and unpack it in the caller
int Execute(const std::string command, std::string& result)
{
    char* textResult = nullptr;
    int status = ExecuteCommand(nullptr, command.c_str(), false, true, 0, 0, &textResult, nullptr, FirewallLog::Get());
    if (textResult)
    {
        result = textResult;
    }
    return status;
}

Table Iptables::GetTable(const std::string tableName)
{
    Table table;
    std::string tableString;
    const std::string command = "iptables -L -n -v --line-number -t " + tableName;

    if (0 == Execute(command.c_str(), tableString))
    {
        std::string hash = ::Hash(tableString);

        if ((m_tableHashes.find(tableName) != m_tableHashes.end()) && (m_tableHashes[tableName] == hash))
        {
            table = m_tables[tableName];
        }
        else
        {
            table = Table::Parse(tableName, tableString);
            m_tables[tableName] = table;
            m_tableHashes[tableName] = hash;
        }
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Error retrieving table from iptables: %s", tableName.c_str())
    }

    return table;
}

std::vector<Table> Iptables::List()
{
    std::vector<Table> tables;

    for (auto& tableName : m_tableNames)
    {
        Table table = GetTable(tableName);
        tables.push_back(table);
    }

    return tables;
}

// Creates the given rule using the iptables command line tool
int Iptables::Create(const Rule& rule)
{
    UNUSED(rule);
    return 0;
}

// Deletes the given rule using the iptables command line tool
// TODO: overload this method for different Delete versions
int Iptables::Delete(const Rule& rule)
{
    UNUSED(rule);
    return 0;
}

std::string Iptables::Hash() const
{
    std::string hash;
    std::string rules;
    const std::string command = "iptables -S";

    if (0 == Execute(command.c_str(), rules))
    {
        hash = ::Hash(rules);
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Error retrieving rules sepcification from iptables");
    }

    return hash;
}


FirewallBase::FirewallBase(unsigned int maxPayloadSizeBytes) :
    m_maxPayloadSizeBytes(maxPayloadSizeBytes) {}

int FirewallBase::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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

// template<class T>
// Firewall<T>::Firewall(unsigned int maxPayloadSizeBytes) :
Firewall::Firewall(unsigned int maxPayloadSizeBytes) :
    FirewallBase(maxPayloadSizeBytes) {}

// template<class T>
// int Firewall<T>::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
int Firewall::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    OsConfigLogError(FirewallLog::Get(), "Set not implemented.");
    return ENOSYS;
}

// template<class T>
// FirewallState Firewall<T>::GetState()
FirewallState Firewall::GetState()
{
    FirewallState state = FirewallState::Unknown;

    // If the utility is not installed/available, the the state is Disabled
    // If the utility is installed check if there are rules/chain policies in the tables
    // If there are rules/chain policies, the state is Enabled

    return state;
}

// template<class T>
// std::string Firewall<T>::GetFingerprint()
std::string Firewall::GetFingerprint()
{
    return this->m_utility.Hash();
}

// template<class T>
// std::vector<Table> Firewall<T>::GetTables()
std::vector<Table> Firewall::GetTables()
{
    return this->m_utility.List();
}

void Rule::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const
{
    writer.StartObject();
    writer.Key("number");
    writer.Int(m_number);
    writer.Key("target");
    writer.String(m_target.c_str());
    writer.Key("protocol");
    writer.String(m_protocol.c_str());
    writer.Key("source");
    writer.String(m_source.c_str());
    writer.Key("destination");
    writer.String(m_destination.c_str());
    writer.Key("in");
    writer.String(m_inInterface.c_str());
    writer.Key("out");
    writer.String(m_outInterface.c_str());
    writer.Key("options");
    writer.String(m_rawOptions.c_str());
    writer.EndObject();
}

void Chain::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const
{
    writer.StartObject();
    writer.Key("name");
    writer.String(m_name.c_str());
    writer.Key("policy");
    writer.String(m_policy.c_str());
    writer.Key("rules");
    writer.StartArray();
    for (auto& rule : m_rules)
    {
        rule.Serialize(writer);
    }
    writer.EndArray();
    writer.EndObject();
}

void Table::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const
{
    writer.StartObject();
    writer.String("name");
    writer.String(m_name.c_str());
    writer.String("chains");
    writer.StartArray();

    for (auto& chain : m_chains)
    {
        chain.Serialize(writer);
    }

    writer.EndArray();
    writer.EndObject();
}

// template<class T>
// int Firewall<T>::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
int Firewall::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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
    }
    else
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        *payloadSizeBytes = 0;
        *payload = nullptr;

        if (0 == m_firewallState.compare(objectName))
        {
            int state = static_cast<int>(GetState());
            writer.Int(state);
        }
        else if (0 == m_firewallFingerprint.compare(objectName))
        {
            std::string fingerprint = GetFingerprint();
            writer.String(fingerprint.c_str());
        }
        else if (0 == m_firewallTables.compare(objectName))
        {
            std::vector<Table> tables = GetTables();
            writer.StartArray();

            for (auto& table : tables)
            {
                table.Serialize(writer);
            }

            writer.EndArray();
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "Invalid object name: %s", objectName);
            status = EINVAL;
        }

        // TODO: use max payload size
        if (MMI_OK == status)
        {
            *payloadSizeBytes = buffer.GetSize();
            *payload = new (std::nothrow) char[*payloadSizeBytes];

            if (*payload != nullptr)
            {
                std::fill(*payload, *payload + *payloadSizeBytes, 0);
                std::memcpy(*payload, buffer.GetString(), *payloadSizeBytes);
            }
        }
    }

    return status;
}