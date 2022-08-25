// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Firewall.h"

const std::string FirewallModuleBase::m_moduleInfo = R"""({
    "Name": "Firewall",
    "Description": "Provides functionality to remotely manage firewall rules on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 2,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["Firewall"],
    "Lifetime": 1,
    "UserAccount": 0})""";

const std::string FirewallModuleBase::m_firewallComponent = "Firewall";

const std::string FirewallModuleBase::m_firewallReportedFingerprint = "firewallFingerprint";
const std::string FirewallModuleBase::m_firewallReportedState = "firewallState";
const std::string FirewallModuleBase::m_firewallReportedDefaults = "defaultPolicies";
const std::string FirewallModuleBase::m_firewallReportedConfigurationStatus = "configurationStatus";
const std::string FirewallModuleBase::m_firewallReportedConfigurationStatusDetail = "configurationStatusDetail";

const std::string FirewallModuleBase::m_firewallDesiredDefaults = "firewallDesiredDefaults";
const std::string FirewallModuleBase::m_firewallDesiredRules = "desiredFirewallRules";

const char g_desiredState[] = "desiredState";
const char g_action[] = "action";
const char g_direction[] = "direction";
const char g_protocol[] = "protocol";
const char g_source[] = "sourceAddress";
const char g_destination[] = "destinationAddress";
const char g_sourcePort[] = "sourcePort";
const char g_destinationPort[] = "destinationPort";

OSCONFIG_LOG_HANDLE FirewallLog::m_logHandle = nullptr;

int FirewallModuleBase::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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

int FirewallModuleBase::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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
        status = EINVAL;
    }
    else
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        *payloadSizeBytes = 0;
        *payload = nullptr;

        if (0 == m_firewallReportedState.compare(objectName))
        {
            status = GetState(writer);
        }
        else if (0 == m_firewallReportedFingerprint.compare(objectName))
        {
            status = GetFingerprint(writer);
        }
        else if (0 == m_firewallReportedDefaults.compare(objectName))
        {
            status = GetDefaultPolicies(writer);
        }
        else if (0 == m_firewallReportedConfigurationStatus.compare(objectName))
        {
            status = GetConfigurationStatus(writer);
        }
        else if (0 == m_firewallReportedConfigurationStatusDetail.compare(objectName))
        {
            status = GetConfigurationStatusDetail(writer);
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "Invalid object name: %s", objectName);
            status = EINVAL;
        }

        if (MMI_OK == status)
        {
            if ((m_maxPayloadSizeBytes > 0) && (buffer.GetSize() > m_maxPayloadSizeBytes))
            {
                OsConfigLogError(FirewallLog::Get(), "Payload size exceeds maximum payload size: %d > %d", static_cast<int>(buffer.GetSize()), m_maxPayloadSizeBytes);
                status = E2BIG;
            }
            else
            {
                *payloadSizeBytes = buffer.GetSize();
                *payload = new (std::nothrow) char[*payloadSizeBytes];

                if (*payload != nullptr)
                {
                    std::fill(*payload, *payload + *payloadSizeBytes, 0);
                    std::memcpy(*payload, buffer.GetString(), *payloadSizeBytes);
                }
                else
                {
                    *payloadSizeBytes = 0;
                    status = ENOMEM;
                }
            }
        }
    }

    return status;
}

int FirewallModuleBase::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
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
    else if (payloadSizeBytes < 0)
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid payload size: %d", payloadSizeBytes);
        status = EINVAL;
    }
    else
    {
        std::string payloadJson = std::string(payload, payloadSizeBytes);
        size_t payloadHash = HashString(payloadJson.c_str());

        if (payloadHash != m_lastPayloadHash)
        {
            m_lastPayloadHash = payloadHash;

            if (0 == m_firewallComponent.compare(componentName))
            {
                rapidjson::Document document;
                document.Parse(payloadJson.c_str());

                if (!document.HasParseError())
                {
                    if (0 == m_firewallDesiredRules.compare(objectName))
                    {
                        status = SetRules(document);
                    }
                    else if (0 == m_firewallDesiredDefaults.compare(objectName))
                    {
                        status = SetDefaultPolicies(document);
                    }
                    else
                    {
                        OsConfigLogError(FirewallLog::Get(), "Invalid object name: %s", objectName);
                        status = EINVAL;
                    }
                }
                else
                {
                    if (IsFullLoggingEnabled())
                    {
                        OsConfigLogError(FirewallLog::Get(), "Failed to parse JSON payload: %s", payloadJson.c_str());
                        status = EINVAL;
                    }
                    else
                    {
                        OsConfigLogError(FirewallLog::Get(), "Failed to parse JSON payload");
                        status = EINVAL;
                    }
                }
            }
            else
            {
                OsConfigLogError(FirewallLog::Get(), "Invalid component name: %s", componentName);
                status = EINVAL;
            }
        }
    }

    return status;
}

std::string IpTablesRule::ActionToString(Action action)
{
    switch (action)
    {
    case Action::Accept:
        return "ACCEPT";
    case Action::Drop:
        return "DROP";
    case Action::Reject:
        return "REJECT";
    }
    return "";
}

std::string IpTablesRule::DirectionToString(Direction direction)
{
    return (direction == Direction::In) ? "INPUT" : "OUTPUT";
}

std::string IpTablesRule::ProtocolToString(Protocol protocol)
{
    switch (protocol)
    {
    case Protocol::Any:
        return "any";
    case Protocol::Tcp:
        return "tcp";
    case Protocol::Udp:
        return "udp";
    case Protocol::Icmp:
        return "icmp";
    }
    return "";
}

std::string IpTablesRule::Specification() const
{
    std::stringstream ruleSpec;

    ruleSpec << DirectionToString(m_direction) << " ";

    if (m_protocol != Protocol::Any)
    {
        ruleSpec << "-p " << ProtocolToString(m_protocol) << " ";
    }

    if (!m_sourceAddress.empty())
    {
        ruleSpec << "-s " << m_sourceAddress << " ";
    }

    if (!m_sourcePort.empty())
    {
        ruleSpec << "-sport " << m_sourcePort << " ";
    }

    if (!m_destinationAddress.empty())
    {
        ruleSpec << "-d " << m_destinationAddress << " ";
    }

    if (!m_destinationPort.empty())
    {
        ruleSpec << "-dport " << m_destinationPort << " ";
    }

    ruleSpec << "-j " << ActionToString(m_action);

    return ruleSpec.str();
}

IpTables::State IpTables::Detect() const
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

std::string IpTables::Fingerprint() const
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

int IpTables::SetDefaultPolicies(const std::vector<Policy> policies)
{
    int status = 0;

    for (const Policy& policy : policies)
    {
        std::string action = Rule::ActionToString(policy.GetAction());
        std::string direction = Rule::DirectionToString(policy.GetDirection());
        std::string command = "iptables -P " + action + " " + direction;

        if (0 != ExecuteCommand(nullptr, command.c_str(), false, false, 0, 0, nullptr, nullptr, FirewallLog::Get()))
        {
            OsConfigLogError(FirewallLog::Get(), "Failed to set default policy: %s %s", action.c_str(), direction.c_str());
            status = -1;
        }
    }

    return status;
}

int IpTables::SetRules(const std::vector<IpTablesRule>& rules)
{
    int status = 0;

    for (const Rule& rule : rules)
    {
        switch (rule.GetDesiredState())
        {
        case Rule::State::Present:
            Add(rule);
            break;

        case Rule::State::Absent:
            Remove(rule);
            break;

        default:
            OsConfigLogError(FirewallLog::Get(), "Invalid desired rule state: %d", static_cast<int>(rule.GetDesiredState()));
        }
    }

    return status;
}

int IpTablesRule::ActionFromString(const std::string& str, Rule::Action& action)
{
    int status = 0;

    if (0 == str.compare("ACCEPT"))
    {
        action = Rule::Action::Accept;
    }
    else if (0 == str.compare("DROP"))
    {
        action = Rule::Action::Drop;
    }
    else if (0 == str.compare("REJECT"))
    {
        action = Rule::Action::Reject;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid action: %s", str.c_str());
        status = -1;
    }

    return status;
}

int IpTablesRule::DirectionFromString(const std::string& str, Rule::Direction& direction)
{
    int status = 0;

    if (0 == str.compare("INPUT"))
    {
        direction = Rule::Direction::In;
    }
    else if (0 == str.compare("OUTPUT"))
    {
        direction = Rule::Direction::Out;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid direction: %s", str.c_str());
        status = -1;
    }

    return status;
}

std::vector<Policy> IpTables::GetDefaultPolicies() const
{
    const char* command = "iptables -S | grep -E '^-P (INPUT|OUTPUT)'";
    static const std::regex policyRegex("^-P\\s+(\\S+)\\s+(\\S+)$");

    std::vector<Policy> policies;
    char* textResult = nullptr;

    if (0 == ExecuteCommand(nullptr, command, false, false, 0, 0, &textResult, nullptr, FirewallLog::Get()))
    {
        if (textResult && (strlen(textResult) > 0))
        {
            std::string text(textResult);
            std::istringstream iss(text);
            std::string line;

            while (std::getline(iss, line, '\n'))
            {
                std::smatch match;
                Policy::Action action;
                Policy::Direction direction;

                if (std::regex_match(line, match, policyRegex))
                {
                    if (0 == Rule::ActionFromString(match[2], action))
                    {
                        if (0 == Rule::DirectionFromString(match[1], direction))
                        {
                            policies.push_back(Policy(action, direction));
                        }
                        else
                        {
                            OsConfigLogError(FirewallLog::Get(), "Invalid direction: %s", match[1].str().c_str());
                        }
                    }
                    else
                    {
                        OsConfigLogError(FirewallLog::Get(), "Invalid action: %s", match[2].str().c_str());
                    }
                }
                else if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(FirewallLog::Get(), "Invalid policy line: %s", line.c_str());
                }
            }
        }
    }

    FREE_MEMORY(textResult);

    return policies;
}

void Policy::ToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer) const
{
    writer.StartObject();
    writer.String("direction");
    writer.String(m_direction == Direction::In ? "in" : "out");
    writer.String("action");
    writer.String(m_action == Action::Accept ? "accept" : (m_action == Action::Drop ? "drop" : "reject"));
    writer.EndObject();
}

Policy& Policy::Parse(const rapidjson::Value& value)
{
    if (value.HasMember("action"))
    {
        if (value["action"].IsString())
        {
            if (0 != ActionFromString(value["action"].GetString(), m_action))
            {
                m_parseError = "Invalid action: " + std::string(value["action"].GetString());
            }
        }
        else
        {
            m_parseError = "Policy action must be of type string";
        }
    }
    else
    {
        m_parseError = "Policy must contain action";
    }

    if (value.HasMember("direction"))
    {
        if (value["direction"].IsString())
        {
            if (0 != DirectionFromString(value["direction"].GetString(), m_direction))
            {
                m_parseError = "Invalid direction: " + std::string(value["direction"].GetString());
            }
        }
        else
        {
            m_parseError = "Policy direction must be of type string";
        }
    }
    else
    {
        m_parseError = "Policy must contain direction";
    }

    return *this;
}

Rule& Rule::Parse(const rapidjson::Value& value)
{
    // TODO: Store an error message in the rule (HasParseError(), GetParseError())

    if (value.IsObject())
    {
        if (value.HasMember(g_desiredState))
        {
            if (value[g_desiredState].IsString())
            {
                // TODO: convert string to enum
                //m_desiredState = value[g_desiredState].GetString();
            }
            else
            {
                // TODO: store error message
            }
        }
        else
        {
            // TODO: store error message
        }

        if (value.HasMember(g_action))
        {
            if (value[g_action].IsString())
            {
                // TODO: convert string to enum
                // m_action = value[g_action].GetString();
            }
            else
            {
                // TODO: store error message
            }
        }
        else
        {
            // TODO: store error message
        }

        if (value.HasMember(g_direction))
        {
            if (value[g_direction].IsString())
            {
                // TODO: convert string to enum
                // m_direction = value[g_direction].GetString();
            }
            else
            {
                // TODO: store error message
            }
        }
        else
        {
            // TODO: store error message
        }

        if (value.HasMember(g_protocol) && value[g_protocol].IsString())
        {
            // TODO: convert string to enum
            // m_protocol = value[g_protocol].GetString();
        }

        if (value.HasMember(g_source) && value[g_source].IsString())
        {
            // TODO: log error if value is an invalid type
            m_sourceAddress = value[g_source].GetString();
        }

        if (value.HasMember(g_destination) && value[g_destination].IsString())
        {
            // TODO: log error if value is an invalid type
            m_destinationAddress = value[g_destination].GetString();
        }

        if (value.HasMember(g_sourcePort) && value[g_sourcePort].IsInt())
        {
            // TODO: log error if value is an invalid type
            m_sourcePort = std::to_string(value[g_sourcePort].GetInt());
        }

        if (value.HasMember(g_destinationPort) && value[g_destinationPort].IsInt())
        {
            // TODO: log error if value is an invalid type
            m_destinationPort = std::to_string(value[g_destinationPort].GetInt());
        }
    }
    else
    {
        // TODO: store error message
        OsConfigLogError(FirewallLog::Get(), "Rule JSON is not an object");
    }

    return *this;
}

template<class RuleT>
std::vector<RuleT> ParseRules(rapidjson::Value& value)
{
    std::vector<RuleT> rules;

    if (value.IsArray())
    {
        for (auto& value : value.GetArray())
        {
            RuleT rule;

            if (!rule.Parse(value).HasParseError())
            {
                rules.push_back(rule);
            }
            else
            {
                OsConfigLogError(FirewallLog::Get(), "Failed to parse rule: %s", rule.GetParseError().c_str());
            }
        }
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Rules JSON is not an array");
    }

    return rules;
}

std::vector<Policy> ParsePolicies(rapidjson::Value& value)
{
    std::vector<Policy> policies;

    if (value.IsArray())
    {
        for (auto& value : value.GetArray())
        {
            Policy policy;

            if (!policy.Parse(value).HasParseError())
            {
                policies.push_back(policy);
            }
            else
            {
                OsConfigLogError(FirewallLog::Get(), "Failed to parse policy: %s", policy.GetParseError().c_str());
            }
        }
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Policies JSON is not an array");
    }

    return policies;
}