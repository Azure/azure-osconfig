// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Firewall.h"

const std::string FirewallModuleBase::m_moduleInfo = R"""({
    "Name": "Firewall",
    "Description": "Provides functionality to remotely manage firewall rules on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 4,
    "VersionMinor": 0,
    "VersionInfo": "Zinc",
    "Components": ["Firewall"],
    "Lifetime": 1,
    "UserAccount": 0})""";

const std::string FirewallModuleBase::m_firewallComponent = "Firewall";

const std::string FirewallModuleBase::m_reportedFingerprint = "fingerprint";
const std::string FirewallModuleBase::m_reportedState = "state";
const std::string FirewallModuleBase::m_reportedDefaultPolicies = "defaultPolicies";
const std::string FirewallModuleBase::m_reportedConfigurationStatus = "configurationStatus";
const std::string FirewallModuleBase::m_reportedConfigurationStatusDetail = "configurationStatusDetail";

const std::string FirewallModuleBase::m_desiredDefaultPolicies = "desiredDefaultPolicies";
const std::string FirewallModuleBase::m_desiredRules = "desiredRules";

const std::set<std::string> DesiredState::m_values = { "present", "absent" };
const std::set<std::string> Action::m_values = { "accept", "reject", "drop" };
const std::set<std::string> Direction::m_values = { "in", "out" };
const std::set<std::string> Protocol::m_values = { "any", "tcp", "udp", "icmp" };

const char g_desiredState[] = "desiredState";
const char g_action[] = "action";
const char g_direction[] = "direction";
const char g_protocol[] = "protocol";
const char g_source[] = "sourceAddress";
const char g_destination[] = "destinationAddress";
const char g_sourcePort[] = "sourcePort";
const char g_destinationPort[] = "destinationPort";

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

        if (0 == m_reportedState.compare(objectName))
        {
            status = GetState(writer);
        }
        else if (0 == m_reportedFingerprint.compare(objectName))
        {
            status = GetFingerprint(writer);
        }
        else if (0 == m_reportedDefaultPolicies.compare(objectName))
        {
            status = GetDefaultPolicies(writer);
        }
        else if (0 == m_reportedConfigurationStatus.compare(objectName))
        {
            status = GetConfigurationStatus(writer);
        }
        else if (0 == m_reportedConfigurationStatusDetail.compare(objectName))
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

        if (0 == m_firewallComponent.compare(componentName))
        {
            rapidjson::Document document;
            document.Parse(payloadJson.c_str());

            if (!document.HasParseError())
            {
                if (0 == m_desiredRules.compare(objectName))
                {
                    status = SetRules(document);
                }
                else if (0 == m_desiredDefaultPolicies.compare(objectName))
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
                DesiredState state = DesiredState(value[g_desiredState].GetString());
                if (state.IsValid())
                {
                    m_desiredState = state;
                }
                else
                {
                    m_parseError.push_back("Invalid desired state: " + std::string(value[g_desiredState].GetString()));
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
                Action action = Action(value[g_action].GetString());
                if (action.IsValid())
                {
                    m_action = action;
                }
                else
                {
                    m_parseError.push_back("Invalid action: " + std::string(value[g_action].GetString()));
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
                Direction direction = Direction(value[g_direction].GetString());
                if (direction.IsValid())
                {
                    m_direction = direction;
                }
                else
                {
                    m_parseError.push_back("Invalid direction: " + std::string(value[g_direction].GetString()));
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
                Protocol protocol = Protocol(value[g_protocol].GetString());
                if (protocol.IsValid())
                {
                    m_protocol = protocol;
                }
                else
                {
                    m_parseError.push_back("Invalid protocol: " + std::string(value[g_protocol].GetString()));
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

    if (IsFullLoggingEnabled())
    {
        for (auto& error : m_parseError)
        {
            OsConfigLogError(FirewallLog::Get(), "%s", error.c_str());
        }
    }

    return *this;
}

std::string IpTablesRule::Specification() const
{
    std::stringstream ruleSpec;

    if (m_direction == "in")
    {
        ruleSpec << g_chainInput;
    }
    else if (m_direction == "out")
    {
        ruleSpec << g_chainOutput;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid direction: %s", m_direction.ToString().c_str());
    }

    ruleSpec << " ";

    if (m_protocol != "any")
    {
        ruleSpec << "-p " << m_protocol << " ";
    }

    if (!m_sourceAddress.empty())
    {
        ruleSpec << "-s " << m_sourceAddress << " ";
    }

    if (!m_sourcePort.empty())
    {
        ruleSpec << "--source-port " << m_sourcePort << " ";
    }

    if (!m_destinationAddress.empty())
    {
        ruleSpec << "-d " << m_destinationAddress << " ";
    }

    if (!m_destinationPort.empty())
    {
        ruleSpec << "--destination-port " << m_destinationPort << " ";
    }

    ruleSpec << "-j ";

    if (m_action == "accept")
    {
        ruleSpec << g_targetAccept;
    }
    else if (m_action == "drop")
    {
        ruleSpec << g_targetDrop;
    }
    else if (m_action == "reject")
    {
        ruleSpec << g_targetReject;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid action: %s", m_action.ToString().c_str());
    }

    return ruleSpec.str();
}

IpTables::State IpTables::Detect() const
{
    const char* command = "iptables -S | grep -E \"^-A (INPUT|OUTPUT)\" | wc -l";

    State state = State::Unknown;
    char* textResult = nullptr;

    if ((0 == ExecuteCommand(nullptr, command, false, false, 0, 0, &textResult, nullptr, FirewallLog::Get())))
    {
        if (textResult && (strlen(textResult) > 0))
        {
            int ruleCount = atoi(textResult);
            state = (ruleCount > 0) ? State::Enabled : State::Disabled;
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

std::string IpTablesPolicy::Specification() const
{
    std::string chain;
    std::string target;

    if (m_direction == "in")
    {
        chain = g_chainInput;
    }
    else if (m_direction == "out")
    {
        chain = g_chainOutput;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid direction: '%s'", m_direction.ToString().c_str());
    }

    if (m_action == "accept")
    {
        target = g_targetAccept;
    }
    else if (m_action == "drop")
    {
        target = g_targetDrop;
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid action: '%s'", m_action.ToString().c_str());
    }

    return chain + " " + target;
}

int IpTables::SetDefaultPolicies(const std::vector<IpTablesPolicy> policies)
{
    int status = 0;
    int index = 0;
    std::vector<std::string> errors;

    for (auto& policy : policies)
    {
        if (!policy.HasParseError())
        {
            std::string specification = policy.Specification();
            std::string command = "iptables -P " + specification;
            int commandStatus = 0;
            char* textResult = nullptr;

            if (0 != (commandStatus = ExecuteCommand(nullptr, command.c_str(), true, false, 0, 0, &textResult, nullptr, FirewallLog::Get())))
            {
                errors.push_back("Failed to set default policy (" + specification + "): " + std::string(textResult));
                status = commandStatus;
            }

            FREE_MEMORY(textResult);
        }
        else
        {
            errors.push_back("Failed to set default policy (" + std::to_string(index) + ")");
            status = EINVAL;
        }

        index++;
    }

    std::string errorMessage = "";

    for (const std::string& error : errors)
    {
        errorMessage += error + "\n";
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(FirewallLog::Get(), "%s", error.c_str());
        }
    }

    m_policyStatusMessage = errorMessage;

    return status;
}

bool IpTables::Exists(const IpTables::Rule& rule) const
{
    bool exists = false;
    char* textResult = nullptr;
    std::string command = "iptables -C " + rule.Specification();

    if (0 == ExecuteCommand(nullptr, command.c_str(), true, false, 0, 0, &textResult, nullptr, FirewallLog::Get()))
    {
        exists = true;
    }

    FREE_MEMORY(textResult);

    return exists;
}

int IpTables::Add(const IpTables::Rule& rule, std::string& error)
{
    int status = 0;
    std::string command = "iptables -I " + rule.Specification();
    char* textResult = nullptr;

    if (0 != (status = ExecuteCommand(nullptr, command.c_str(), true, false, 0, 0, &textResult, nullptr, FirewallLog::Get())))
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

    if (0 != (status = ExecuteCommand(nullptr, command.c_str(), true, false, 0, 0, &textResult, nullptr, FirewallLog::Get())))
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
            DesiredState state = rule.GetDesiredState();
            if (state == "present")
            {
                if (!Exists(rule))
                {
                    if (0 != Add(rule, error))
                    {
                        errors.push_back("Failed to add rule (" + std::to_string(index) + "): " + error);
                    }
                }
            }
            else if (state == "absent")
            {
                while (Exists(rule))
                {
                    if (0 != Remove(rule, error))
                    {
                        errors.push_back("Failed to remove rule (" + std::to_string(index) + "): " + error);
                    }
                }
            }
            else
            {
                OsConfigLogError(FirewallLog::Get(), "Invalid desired rule state (%d): %s", index, rule.GetDesiredState().ToString().c_str());
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
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(FirewallLog::Get(), "%s", error.c_str());
            }
        }

        m_ruleStatusMessage = errorMessage;
        status = EINVAL;
    }
    else
    {
        m_ruleStatusMessage = "";
    }

    return status;
}

int IpTablesPolicy::SetActionFromTarget(const std::string& str)
{
    int status = 0;

    if (str == g_targetAccept)
    {
        m_action = Action("accept");
    }
    else if (str == g_targetDrop)
    {
        m_action = Action("drop");
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid target: '%s'", str.c_str());
        status = EINVAL;
    }

    return status;
}

int IpTablesPolicy::SetDirectionFromChain(const std::string& str)
{
    int status = 0;

    if (str == g_chainInput)
    {
        m_direction = Direction("in");
    }
    else if (str == g_chainOutput)
    {
        m_direction = Direction("out");
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "Invalid chain: '%s')", str.c_str());
        status = EINVAL;
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
                    if (0 == policy.SetActionFromTarget(match[2]))
                    {
                        if (0 == policy.SetDirectionFromChain(match[1]))
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

void GenericPolicy::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const
{
    writer.StartObject();
    writer.String(g_direction);
    writer.String(m_direction.ToString().c_str());
    writer.String(g_action);
    writer.String(m_action.ToString().c_str());
    writer.EndObject();
}

GenericPolicy& GenericPolicy::Parse(const rapidjson::Value& value)
{
    if (value.HasMember(g_action))
    {
        if (value[g_action].IsString())
        {
            Action action = Action(value[g_action].GetString());
            if (action.IsValid() && (action != "reject"))
            {
                m_action = action;
            }
            else
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
            Direction direction = Direction(value[g_direction].GetString());
            if (direction.IsValid())
            {
                m_direction = direction;
            }
            else
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

    for (auto& error : m_parseError)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(FirewallLog::Get(), "%s", error.c_str());
        }
    }

    return *this;
}
