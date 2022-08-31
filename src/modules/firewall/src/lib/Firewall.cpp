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
const std::string FirewallModuleBase::m_firewallReportedDefaultPolicies = "defaultPolicies";
const std::string FirewallModuleBase::m_firewallReportedConfigurationStatus = "configurationStatus";
const std::string FirewallModuleBase::m_firewallReportedConfigurationStatusDetail = "configurationStatusDetail";

const std::string FirewallModuleBase::m_firewallDesiredDefaultPolicies = "firewallDesiredDefaultPolicies";
const std::string FirewallModuleBase::m_firewallDesiredRules = "desiredFirewallRules";

const char g_desiredState[] = "desiredState";
const char g_action[] = "action";
const char g_direction[] = "direction";
const char g_protocol[] = "protocol";
const char g_source[] = "sourceAddress";
const char g_destination[] = "destinationAddress";
const char g_sourcePort[] = "sourcePort";
const char g_destinationPort[] = "destinationPort";

const char g_desiredStatePresent[] = "present";
const char g_desiredStateAbsent[] = "notPresent";

const char g_actionAccept[] = "accept";
const char g_actionReject[] = "reject";
const char g_actionDrop[] = "drop";

const char g_directionIn[] = "in";
const char g_directionOut[] = "out";

const char g_protocolAny[] = "any";
const char g_protocolTCP[] = "tcp";
const char g_protocolUDP[] = "udp";
const char g_protocolICMP[] = "icmp";

const char g_targetAccept[] = "ACCEPT";
const char g_targetReject[] = "REJECT";
const char g_targetDrop[] = "DROP";

const char g_chainInput[] = "INPUT";
const char g_chainOutput[] = "OUTPUT";

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
        else if (0 == m_firewallReportedDefaultPolicies.compare(objectName))
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
                    else if (0 == m_firewallDesiredDefaultPolicies.compare(objectName))
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

int GenericRule::ActionFromString(const std::string& str)
{
    int status = 0;

    if (0 == str.compare(g_actionAccept))
    {
        m_action = Action::Accept;
    }
    else if (0 == str.compare(g_actionDrop))
    {
        m_action = Action::Drop;
    }
    else if (0 == str.compare(g_actionReject))
    {
        m_action = Action::Reject;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid action: %s", str.c_str());
        status = -1;
    }

    return status;
}

int GenericRule::DirectionFromString(const std::string& str)
{
    int status = 0;

    if (0 == str.compare(g_directionIn))
    {
        m_direction = Direction::In;
    }
    else if (0 == str.compare(g_directionOut))
    {
        m_direction = Direction::Out;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid direction: %s", str.c_str());
        status = -1;
    }

    return status;
}

int GenericRule::StateFromString(const std::string& str)
{
    int status = 0;

    if (0 == str.compare(g_desiredStatePresent))
    {
        m_desiredState = State::Present;
    }
    else if (0 == str.compare(g_desiredStateAbsent))
    {
        m_desiredState = State::Absent;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid desired state: %s", str.c_str());
        status = -1;
    }

    return status;
}

int GenericRule::ProtocolFromString(const std::string& str)
{
    int status = 0;

    if (0 == str.compare(g_protocolTCP))
    {
        m_protocol = Protocol::Tcp;
    }
    else if (0 == str.compare(g_protocolUDP))
    {
        m_protocol = Protocol::Udp;
    }
    else if (0 == str.compare(g_protocolICMP))
    {
        m_protocol = Protocol::Icmp;
    }
    else if (0 == str.compare(g_protocolAny))
    {
        m_protocol = Protocol::Any;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid protocol: %s", str.c_str());
        status = -1;
    }

    return status;
}

GenericRule& GenericRule::Parse(const rapidjson::Value& value)
{
    if (value.IsObject())
    {
        if (value.HasMember(g_desiredState))
        {
            if (value[g_desiredState].IsString())
            {
                std::string state = value[g_desiredState].GetString();
                if (0 != StateFromString(state))
                {
                    m_parseError.push_back("Invalid enum value for '" + std::string(g_desiredState) + "': " + state);
                }
            }
            else
            {
                m_parseError.push_back("Desired state must be of type string");
            }
        }
        else
        {
            m_parseError.push_back("Rule must have a '" + std::string(g_desiredState) + "' field");
        }

        if (value.HasMember(g_action))
        {
            if (value[g_action].IsString())
            {
                std::string action = value[g_action].GetString();
                if (0 != ActionFromString(action))
                {
                    m_parseError.push_back("Invalid enum value for '" + std::string(g_action) + "': " + action);
                }
            }
            else
            {
                m_parseError.push_back("Action must be of type string");
            }
        }
        else
        {
            m_parseError.push_back("Rule must have a '" + std::string(g_action) + "' field");
        }

        if (value.HasMember(g_direction))
        {
            if (value[g_direction].IsString())
            {
                std::string direction = value[g_direction].GetString();
                if (0 != DirectionFromString(direction))
                {
                    m_parseError.push_back("Invalid enum value for '" + std::string(g_direction) + "': " + direction);
                }
            }
            else
            {
                m_parseError.push_back("Direction must be of type string");
            }
        }
        else
        {
            m_parseError.push_back("Rule must have a '" + std::string(g_direction) + "' field");
        }

        if (value.HasMember(g_protocol))
        {
            if (value[g_protocol].IsString())
            {
                std::string protocol = value[g_protocol].GetString();
                if (0 != ProtocolFromString(protocol))
                {
                    m_parseError.push_back("Invalid enum value for '" + std::string(g_protocol) + "': " + protocol);
                }
            }
            else
            {
                m_parseError.push_back("Protocol must be of type string");
            }
        }

        if (value.HasMember(g_source))
        {
            if (value[g_source].IsString())
            {
                m_sourceAddress = value[g_source].GetString();
            }
            else
            {
                m_parseError.push_back("Source must be of type string");
            }
        }

        if (value.HasMember(g_destination))
        {
            if (value[g_destination].IsString())
            {
                m_destinationAddress = value[g_destination].GetString();
            }
            else
            {
                m_parseError.push_back("Destination must be of type string");
            }
        }

        if (value.HasMember(g_sourcePort))
        {
            if (value[g_sourcePort].IsInt())
            {
                m_sourcePort = std::to_string(value[g_sourcePort].GetInt());
            }
            else
            {
                m_parseError.push_back("Source port must be of type integer");
            }
        }

        if (value.HasMember(g_destinationPort))
        {
            if (value[g_destinationPort].IsInt())
            {
                m_destinationPort = std::to_string(value[g_destinationPort].GetInt());
            }
            else
            {
                m_parseError.push_back("Destination port must be of type integer");
            }
        }
    }
    else
    {
        m_parseError.push_back("Rule JSON is not an object");
    }

    return *this;
}

std::string IpTablesRule::ActionToString() const
{
    switch (m_action)
    {
        case Action::Accept:
            return g_actionAccept;
        case Action::Drop:
            return g_actionDrop;
        case Action::Reject:
            return g_actionReject;
    }
    return "";
}

std::string IpTablesRule::DirectionToString() const
{
    return (m_direction == Direction::In) ? g_directionIn : g_directionOut;
}

std::string IpTablesRule::ProtocolToString() const
{
    switch (m_protocol)
    {
        case Protocol::Any:
            return g_protocolAny;
        case Protocol::Tcp:
            return g_protocolTCP;
        case Protocol::Udp:
            return g_protocolUDP;
        case Protocol::Icmp:
            return g_protocolICMP;
    }
    return "";
}

std::string IpTablesRule::ActionToTarget() const
{
    switch (m_action)
    {
        case Action::Accept:
            return g_targetAccept;
        case Action::Drop:
            return g_targetDrop;
        case Action::Reject:
            return g_targetReject;
    }
    return "";
}

int IpTablesRule::ActionFromTarget(const std::string& str)
{
    int status = 0;

    if (0 == str.compare(g_targetAccept))
    {
        m_action = Action::Accept;
    }
    else if (0 == str.compare(g_targetDrop))
    {
        m_action = Action::Drop;
    }
    else if (0 == str.compare(g_targetReject))
    {
        m_action = Action::Reject;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid action: %s", str.c_str());
        status = -1;
    }

    return status;
}

int IpTablesRule::DirectionFromChain(const std::string& str)
{
    int status = 0;

    if (0 == str.compare(g_chainInput))
    {
        m_direction = Direction::In;
    }
    else if (0 == str.compare(g_chainOutput))
    {
        m_direction = Direction::Out;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid direction: %s", str.c_str());
        status = -1;
    }

    return status;
}

std::string IpTablesRule::DirectionToChain() const
{
    return (m_direction == Direction::In) ? g_chainInput : g_chainOutput;
}

std::string IpTablesRule::Specification() const
{
    std::stringstream ruleSpec;

    ruleSpec << DirectionToChain() << " ";

    if (m_protocol != Protocol::Any)
    {
        ruleSpec << "-p " << ProtocolToString() << " ";
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

    ruleSpec << "-j " << ActionToTarget();

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

int IpTablesPolicy::ActionFromString(const std::string& str)
{
    int status = 0;

    if (0 == str.compare(g_actionAccept))
    {
        m_action = Action::Accept;
    }
    else if (0 == str.compare(g_actionDrop))
    {
        m_action = Action::Drop;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid action: %s", str.c_str());
        status = -1;
    }

    return status;
}

std::string IpTablesPolicy::Specification() const
{
    std::stringstream spec;
    spec << DirectionToChain() << " " << ActionToTarget();
    return spec.str();
}

int IpTables::SetDefaultPolicies(const std::vector<IpTablesPolicy> policies)
{
    int status = 0;
    std::vector<std::string> errors;

    for (auto& policy : policies)
    {
        std::string specification = policy.Specification();
        std::string command = "iptables -P " + specification;
        char* textResult = nullptr;

        if (0 != ExecuteCommand(nullptr, command.c_str(), false, false, 0, 0, &textResult, nullptr, FirewallLog::Get()))
        {
            errors.push_back("Failed to set default policy (" + specification + "): " + std::string(textResult));
            status = -1;
        }

        FREE_MEMORY(textResult);
    }

    std::string errorMessage = "";

    for (const std::string& error : errors)
    {
        errorMessage += error + "\n";
    }

    m_policyStatusMessage = errorMessage;

    return status;
}

bool IpTables::Exists(const IpTables::Rule& rule) const
{
    bool exists = false;
    char* textResult = nullptr;
    std::string command = "iptables -C " + rule.Specification();

    if (0 == ExecuteCommand(nullptr, command.c_str(), false, false, 0, 0, &textResult, nullptr, FirewallLog::Get()))
    {
        exists = true;
    }

    FREE_MEMORY(textResult);

    return exists;
}

int IpTables::Add(const IpTables::Rule& rule, std::string& error)
{
    int status = 0;
    std::string command = "iptables -A " + rule.Specification();
    char* textResult = nullptr;

    if (0 != (status = ExecuteCommand(nullptr, command.c_str(), false, false, 0, 0, &textResult, nullptr, FirewallLog::Get())))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(FirewallLog::Get(), "Failed to add rule (%s): %s", command.c_str(), textResult);
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "Failed to add rule: %s", textResult);
        }

        error = textResult;
    }

    FREE_MEMORY(textResult);

    return status;
}

int IpTables::Remove(const IpTables::Rule& rule, std::string& error)
{
    int status = 0;
    std::string command = "iptables -D " + rule.Specification();
    char* textResult = nullptr;

    if (0 != (status = ExecuteCommand(nullptr, command.c_str(), false, false, 0, 0, &textResult, nullptr, FirewallLog::Get())))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(FirewallLog::Get(), "Failed to remove rule (%s): %s", command.c_str(), textResult);
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "Failed to remove rule: %s", textResult);
        }

        error = textResult;
    }

    FREE_MEMORY(textResult);

    return status;
}

int IpTables::SetRules(const std::vector<IpTables::Rule>& rules)
{
    int status = 0;
    int index = rules.size() - 1;
    std::string error;
    std::vector<std::string> errors;

    // Iterate through the rules in reverse order to ensure that the resulting
    // rule order in the iptables chain is the same as the desired order
    for (auto it = rules.rbegin(); it != rules.rend(); ++it, --index)
    {
        Rule rule = *it;

        if (rule.HasParseError())
        {
            for (const std::string& parseError : rule.GetParseError())
            {
                errors.push_back("[" + std::to_string(index) + "] " + parseError);
            }
        }
        else
        {
            switch (rule.GetDesiredState())
            {
                case Rule::State::Present:
                    if (!Exists(rule))
                    {
                        if (0 != Add(rule, error))
                        {
                            errors.push_back("Failed to add rule (" + std::to_string(index) + "): " + error);
                        }
                    }
                    break;

                case Rule::State::Absent:
                    while (Exists(rule))
                    {
                        if (0 != Remove(rule, error))
                        {
                            errors.push_back("Failed to remove rule (" + std::to_string(index) + "): " + error);
                        }
                    }
                    break;

                default:
                    OsConfigLogError(FirewallLog::Get(), "Invalid desired rule state (%d): %d", index, static_cast<int>(rule.GetDesiredState()));
                    status = EINVAL;
            }
        }
    }

    if (errors.size() > 0)
    {
        // Errors are in reverse order, so reverse them back to normal order
        // and reset the status message/status code if there were any errors
        std::reverse(errors.begin(), errors.end());
        std::string errorMessage = "";

        for (const std::string& error : errors)
        {
            errorMessage += error + "\n";
        }

        m_ruleStatusMessage = errorMessage;
        status = -1;
    }
    else
    {
        m_policyStatusMessage = "";
    }

    return status;
}

std::vector<IpTablesPolicy> IpTables::GetDefaultPolicies() const
{
    const char* command = "iptables -S | grep -E '^-P (INPUT|OUTPUT)'";
    static const std::regex policyRegex("^-P\\s+(\\S+)\\s+(\\S+)$");

    std::vector<IpTablesPolicy> policies;
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
                IpTablesPolicy policy;

                if (std::regex_match(line, match, policyRegex))
                {
                    if (0 == policy.ActionFromTarget(match[2]))
                    {
                        if (0 == policy.DirectionFromChain(match[1]))
                        {
                            policies.push_back(policy);
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

void IpTablesPolicy::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const
{
    writer.StartObject();
    writer.String(g_direction);
    writer.String(DirectionToString().c_str());
    writer.String(g_action);
    writer.String(ActionToString().c_str());
    writer.EndObject();
}

IpTablesPolicy& IpTablesPolicy::Parse(const rapidjson::Value& value)
{
    if (value.HasMember(g_action))
    {
        if (value[g_action].IsString())
        {
            if (0 != ActionFromString(value[g_action].GetString()))
            {
                m_parseError.push_back("Invalid action: " + std::string(value[g_action].GetString()));
            }
        }
        else
        {
            m_parseError.push_back("Policy action must be of type string");
        }
    }
    else
    {
        m_parseError.push_back("Policy must contain action");
    }

    if (value.HasMember(g_direction))
    {
        if (value[g_direction].IsString())
        {
            if (0 != DirectionFromString(value[g_direction].GetString()))
            {
                m_parseError.push_back("Invalid direction: " + std::string(value[g_direction].GetString()));
            }
        }
        else
        {
            m_parseError.push_back("Policy direction must be of type string");
        }
    }
    else
    {
        m_parseError.push_back("Policy must contain direction");
    }

    return *this;
}