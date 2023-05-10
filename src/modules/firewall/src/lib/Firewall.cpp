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
const char g_sourceAddress[] = "sourceAddress";
const char g_destinationAddress[] = "destinationAddress";
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
    JSON_Value* value = nullptr;
    char* json = nullptr;

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
        *payloadSizeBytes = 0;
        *payload = nullptr;

        if (0 == m_reportedState.compare(objectName))
        {
            status = GetState(&value);
        }
        else if (0 == m_reportedFingerprint.compare(objectName))
        {
            status = GetFingerprint(&value);
        }
        else if (0 == m_reportedDefaultPolicies.compare(objectName))
        {
            status = GetDefaultPolicies(&value);
        }
        else if (0 == m_reportedConfigurationStatus.compare(objectName))
        {
            status = GetConfigurationStatus(&value);
        }
        else if (0 == m_reportedConfigurationStatusDetail.compare(objectName))
        {
            status = GetConfigurationStatusDetail(&value);
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "Invalid object name: %s", objectName);
            status = EINVAL;
        }

        if (MMI_OK == status)
        {
            json = json_serialize_to_string(value);

            if (nullptr == json)
            {
                OsConfigLogError(FirewallLog::Get(), "Failed to serialize JSON object");
                status = ENOMEM;
            }
            else if ((m_maxPayloadSizeBytes > 0) && (m_maxPayloadSizeBytes < strlen(json)))
            {
                OsConfigLogError(FirewallLog::Get(), "Payload size exceeds maximum size");
                status = E2BIG;
            }
            else
            {
                *payloadSizeBytes = strlen(json);
                *payload = new (std::nothrow) char[*payloadSizeBytes];

                if (nullptr == *payload)
                {
                    OsConfigLogError(FirewallLog::Get(), "Failed to allocate memory for payload");
                    status = ENOMEM;
                }
                else
                {
                    std::memcpy(*payload, json, *payloadSizeBytes);
                }
            }
        }
    }

    if (nullptr != json)
    {
        json_free_serialized_string(json);
    }

    if (nullptr != value)
    {
        json_value_free(value);
    }

    return status;
}

int FirewallModuleBase::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    JSON_Value* value = nullptr;

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
            if (nullptr == (value = json_parse_string(payloadJson.c_str())))
            {
                OsConfigLogError(FirewallLog::Get(), "Failed to parse JSON payload");
                status = EINVAL;
            }
            else
            {
                if (0 == m_desiredRules.compare(objectName))
                {
                    status = SetRules(value);
                }
                else if (0 == m_desiredDefaultPolicies.compare(objectName))
                {
                    status = SetDefaultPolicies(value);
                }
                else
                {
                    OsConfigLogError(FirewallLog::Get(), "Invalid object name: %s", objectName);
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

    if (nullptr != value)
    {
        json_value_free(value);
    }

    return status;
}

GenericRule& GenericRule::Parse(const JSON_Value* value)
{
    JSON_Object* object = json_value_get_object(value);

    if (nullptr != object)
    {
        if (json_object_has_value_of_type(object, g_desiredState, JSONString))
        {
            DesiredState state = DesiredState(json_object_get_string(object, g_desiredState));
            if (state.IsValid())
            {
                m_desiredState = state;
            }
            else
            {
                m_parseError.push_back("Invalid desired state: " + std::string(json_object_get_string(object, g_desiredState)));
            }
        }
        else
        {
            m_parseError.push_back("Rule must have a '" + std::string(g_desiredState) + "' field");
        }

        if (json_object_has_value_of_type(object, g_action, JSONString))
        {
            Action action = Action(json_object_get_string(object, g_action));
            if (action.IsValid())
            {
                m_action = action;
            }
            else
            {
                m_parseError.push_back("Invalid action: " + std::string(json_object_get_string(object, g_action)));
            }
        }
        else
        {
            m_parseError.push_back("Rule must have a '" + std::string(g_action) + "' field");
        }

        if (json_object_has_value_of_type(object, g_direction, JSONString))
        {
            Direction direction = Direction(json_object_get_string(object, g_direction));
            if (direction.IsValid())
            {
                m_direction = direction;
            }
            else
            {
                m_parseError.push_back("Invalid direction: " + std::string(json_object_get_string(object, g_direction)));
            }
        }
        else
        {
            m_parseError.push_back("Rule must have a '" + std::string(g_direction) + "' field");
        }

        if (json_object_has_value_of_type(object, g_protocol, JSONString))
        {
            Protocol protocol = Protocol(json_object_get_string(object, g_protocol));
            if (protocol.IsValid())
            {
                m_protocol = protocol;
            }
            else
            {
                m_parseError.push_back("Invalid protocol: " + std::string(json_object_get_string(object, g_protocol)));
            }
        }
        else if (json_object_get_value(object, g_protocol))
        {
            m_parseError.push_back("Protocol must be a string");
        }

        if (json_object_has_value_of_type(object, g_sourceAddress, JSONString))
        {
            m_sourceAddress = std::string(json_object_get_string(object, g_sourceAddress));
        }
        else if (json_object_get_value(object, g_sourceAddress))
        {
            m_parseError.push_back("Source address must be a string");
        }

        if (json_object_has_value_of_type(object, g_sourcePort, JSONNumber))
        {
            m_sourcePort = std::string(json_object_get_string(object, g_sourcePort));
        }
        else if (json_object_get_value(object, g_sourcePort))
        {
            m_parseError.push_back("Source port must be a string");
        }

        if (json_object_has_value_of_type(object, g_destinationAddress, JSONString))
        {
            m_destinationAddress = std::string(json_object_get_string(object, g_destinationAddress));
        }
        else if (json_object_get_value(object, g_destinationAddress))
        {
            m_parseError.push_back("Destination address must be a string");
        }

        if (json_object_has_value_of_type(object, g_destinationPort, JSONNumber))
        {
            m_destinationPort = std::string(json_object_get_string(object, g_destinationPort));
        }
        else if (json_object_get_value(object, g_destinationPort))
        {
            m_parseError.push_back("Destination port must be a string");
        }
    }
    else
    {
        m_parseError.push_back("Rule JSON is not an object");
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

        if (!rule.HasParseError())
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

void GenericPolicy::Serialize(JSON_Object* object) const
{
    json_object_set_string(object, g_direction, m_direction.ToString().c_str());
    json_object_set_string(object, g_action, m_action.ToString().c_str());
}

GenericPolicy& GenericPolicy::Parse(const JSON_Value* value)
{
    const JSON_Object* object = json_value_get_object(value);

    if (object)
    {
        if (json_object_has_value_of_type(object, g_action, JSONString))
        {
            Action action = Action(json_object_get_string(object, g_action));
            if (action.IsValid())
            {
                m_action = action;
            }
            else
            {
                m_parseError.push_back("Invalid policy action: " + std::string(json_object_get_string(object, g_action)));
            }
        }
        else
        {
            m_parseError.push_back("Missing action from policy");
        }

        if (json_object_has_value_of_type(object, g_direction, JSONString))
        {
            Direction direction = Direction(json_object_get_string(object, g_direction));
            if (direction.IsValid())
            {
                m_direction = direction;
            }
            else
            {
                m_parseError.push_back("Invalid policy direction: " + std::string(json_object_get_string(object, g_direction)));
            }
        }
        else
        {
            m_parseError.push_back("Missing direction from policy");
        }
    }
    else
    {
        m_parseError.push_back("Invalid JSON object");
    }

    return *this;
}