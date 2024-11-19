// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <stdio.h>
#include <Networking.h>
#include <CommonUtils.h>

std::string g_interfaceTypes = "interfaceTypes";
std::string g_macAddresses = "macAddresses";
std::string g_ipAddresses = "ipAddresses";
std::string g_subnetMasks = "subnetMasks";
std::string g_defaultGateways = "defaultGateways";
std::string g_dnsServers = "dnsServers";
std::string g_dhcpEnabled = "dhcpEnabled";
std::string g_enabled = "enabled";
std::string g_connected = "connected";

const char* g_getInterfaceNames = "ls -A /sys/class/net";
const char* g_getInterfaceTypesNmcli = "nmcli device show";
const char* g_getInterfaceTypesNetworkctl = "networkctl --no-legend";
const char* g_getIpAddressDetails = "ip addr";
const char* g_getDefaultGateways = "ip route";
const char* g_getDnsServers = "systemd-resolve --status";

const char* g_systemdResolvedServiceName = "systemd-resolved.service";

const char* g_macAddressesPrefix = "link/";
const char* g_ipAddressesPrefix = "inet";
const char* g_subnetMasksPrefix = "inet";
const char* g_enabledPrefix = "state";

const char* g_dhcpEnabledFlag = "dynamic";
const char* g_connectedFlag = "LOWER_UP";

const char* g_enabledFlag = "UP";
const char* g_disabledFlag = "DOWN";

const char* g_true = "true";
const char* g_false = "false";
const char* g_unknown = "unknown";

const char* g_emptyString = "";
const char* g_comma = ",";
const char* g_colon = ":";
const char* g_semiColon = ";";
const char* g_dash = "-";
const char* g_doubleDash = "--";
const char* g_slash = "/";
const char* g_equals = "=";
const char* g_closeParenthesis = ")";
const char* g_at = "@";
const char* g_twoDots = "..";

const char g_spaceCharacter = ' ';
const std::string g_spaceString = " ";

const char* g_templateWithDots = R"""({"interfaceTypes":"..","macAddresses":"..","ipAddresses":"..","subnetMasks":"..","defaultGateways":"..","dnsServers":"..","dhcpEnabled":"..","enabled":"..","connected":".."})""";

const char* g_ipv4 = "(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])";
const char* g_ipv6 =
    "(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}"
    ":[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}"
    "|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}"
    ":((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|[fF][eE]80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::([fF][eE]{4}(:0{1,4}){0,1}:){0,1}"
    "((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}"
    ":((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))";

const char* g_interfaceNamePrefixNmcli = "GENERAL.DEVICE:\\s+";
const char* g_interfaceNamePrefixSystemdResolve = "Link\\s+[0-9]+\\s+\\(";
const char* g_dnsServersPrefix = "DNS\\s+Servers:\\s+";
const char* g_interfaceTypePrefix = "GENERAL.TYPE:\\s+";
const char* g_interfaceTypesPrefixNetworkctl = "^\\s*[0-9]+\\s+.*$";
const char* g_interfaceDataPrefix = "[0-9]+:\\s+.*:\\s+";
const char* g_interfaceNamePrefixDefaultGateways = "default\\s+via\\s+.*\\s+dev\\s+";
const char* g_defaultGatewaysPrefix = "default\\s+via\\s+";
const char* g_globalDnsServers = "Global\\s*\n";

std::regex g_ipv4Pattern(g_ipv4);
std::regex g_ipv6Pattern(g_ipv6);
std::regex g_interfaceNamePrefixPatternDnsServers(g_interfaceNamePrefixSystemdResolve);
std::regex g_dnsServersPrefixPattern(g_dnsServersPrefix);

const std::vector<std::string> g_fields = {"interfaceTypes", "macAddresses", "ipAddresses", "subnetMasks", "defaultGateways","dnsServers", "dhcpEnabled", "enabled", "connected"};

const unsigned int g_numFields = g_fields.size();
const unsigned int g_twoDotsSize = strlen(g_twoDots);
const unsigned int g_templateWithDotsSize = strlen(g_templateWithDots);
const unsigned int g_templateSize = (g_templateWithDotsSize > (g_numFields * g_twoDotsSize)) ? (g_templateWithDotsSize - (g_numFields * g_twoDotsSize)) : 0;

OSCONFIG_LOG_HANDLE NetworkingLog::m_logNetworking = nullptr;

NetworkingObject::NetworkingObject(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
    m_networkManagementService = NetworkManagementService::Unknown;
}

NetworkingObject::~NetworkingObject() {}

std::string NetworkingObject::RunCommand(const char* command)
{
    char* textResult = nullptr;
    int status = ExecuteCommand(nullptr, command, false, false, 0, 0, &textResult, nullptr, NetworkingLog::Get());
    std::string commandOutputToReturn = g_emptyString;
    if (MMI_OK == status)
    {
        commandOutputToReturn = (nullptr != textResult) ? std::string(textResult) : g_emptyString;
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogError(NetworkingLog::Get(), "Failed to execute command '%s': %d, '%s'", command, status, (nullptr != textResult) ? textResult : g_dash);
    }

    if (nullptr != textResult)
    {
        free(textResult);
    }

    return commandOutputToReturn;
}

void NetworkingObjectBase::ParseInterfaceDataForSettings(bool hasPrefix, const char* flag, std::stringstream& data, std::vector<std::string>& settings)
{
    std::string token = g_emptyString;
    while (std::getline(data, token, g_spaceCharacter))
    {
        if (token.find(flag) != std::string::npos)
        {
            if (hasPrefix)
            {
                std::getline(data, token, g_spaceCharacter);
            }
            if (!token.empty())
            {
                token.erase(std::remove(token.begin(), token.end(), '\n'), token.end());
                settings.push_back(token);
            }
        }
    }
}

void NetworkingObjectBase::GetInterfaceTypes(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (this->m_interfaceTypesMap.find(interfaceName) != this->m_interfaceTypesMap.end())
    {
        interfaceSettings.push_back(this->m_interfaceTypesMap[interfaceName]);
    }
}

void NetworkingObjectBase::GetMacAddresses(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (this->m_ipSettingsMap.find(interfaceName) != this->m_ipSettingsMap.end())
    {
        std::stringstream ipSettingsData(this->m_ipSettingsMap[interfaceName]);
        ParseInterfaceDataForSettings(true, g_macAddressesPrefix, ipSettingsData, interfaceSettings);
    }
}

void NetworkingObjectBase::GetIpAddresses(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (this->m_ipSettingsMap.find(interfaceName) != this->m_ipSettingsMap.end())
    {
        std::stringstream ipSettingsData(this->m_ipSettingsMap[interfaceName]);
        ParseInterfaceDataForSettings(true, g_ipAddressesPrefix, ipSettingsData, interfaceSettings);
        size_t size = interfaceSettings.size();
        for (size_t i = 0; i < size; i++)
        {
            size_t subnetMasksDelimiter = interfaceSettings[i].find(g_slash);
            if (subnetMasksDelimiter != std::string::npos)
            {
                interfaceSettings[i] = interfaceSettings[i].substr(0, subnetMasksDelimiter);
            }
            else
            {
                interfaceSettings.erase(interfaceSettings.begin() + i);
            }
        }
    }
}

void NetworkingObjectBase::GetSubnetMasks(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (this->m_ipSettingsMap.find(interfaceName) != this->m_ipSettingsMap.end())
    {
        std::stringstream ipSettingsData(this->m_ipSettingsMap[interfaceName]);
        ParseInterfaceDataForSettings(true, g_subnetMasksPrefix, ipSettingsData, interfaceSettings);
        size_t size = interfaceSettings.size();
        for (size_t i = 0; i < size; i++)
        {
            size_t subnetMasksDelimiter = interfaceSettings[i].find(g_slash);
            if (subnetMasksDelimiter != std::string::npos)
            {
                interfaceSettings[i] = interfaceSettings[i].substr(subnetMasksDelimiter);
            }
            else
            {
                interfaceSettings.erase(interfaceSettings.begin() + i);
            }
        }
    }
}

void NetworkingObjectBase::GetDefaultGateways(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (this->m_defaultGatewaysMap.find(interfaceName) != this->m_defaultGatewaysMap.end())
    {
        for (size_t i = 0; i < this->m_defaultGatewaysMap[interfaceName].size(); i++)
        {
            interfaceSettings.push_back((this->m_defaultGatewaysMap[interfaceName]).at(i));
        }
    }
}

void NetworkingObjectBase::GetDnsServers(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (this->m_dnsServersMap.find(interfaceName) != this->m_dnsServersMap.end())
    {
        for (size_t i = 0; i < this->m_dnsServersMap[interfaceName].size(); i++)
        {
            interfaceSettings.push_back((this->m_dnsServersMap[interfaceName]).at(i));
        }
    }
}

void NetworkingObjectBase::GetDhcpEnabled(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (this->m_ipSettingsMap.find(interfaceName) != this->m_ipSettingsMap.end())
    {
        std::stringstream ipSettingsData(this->m_ipSettingsMap[interfaceName]);
        ParseInterfaceDataForSettings(false, g_dhcpEnabledFlag, ipSettingsData, interfaceSettings);
        if (!interfaceSettings.empty())
        {
            interfaceSettings.clear();
            interfaceSettings.push_back(g_true);
        }
        else
        {
            interfaceSettings.push_back(g_false);
        }
    }
    else
    {
        interfaceSettings.push_back(g_unknown);
    }
}

void NetworkingObjectBase::GetEnabled(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (this->m_ipSettingsMap.find(interfaceName) != this->m_ipSettingsMap.end())
    {
        std::stringstream ipSettingsData(this->m_ipSettingsMap[interfaceName]);
        ParseInterfaceDataForSettings(true, g_enabledPrefix, ipSettingsData, interfaceSettings);
        if (!interfaceSettings.empty())
        {
            std::string data(interfaceSettings.front());
            interfaceSettings.clear();
            if (data == g_enabledFlag)
            {
                interfaceSettings.push_back(g_true);
            }
            else if (data == g_disabledFlag)
            {
                interfaceSettings.push_back(g_false);
            }
        }
    }

    if (interfaceSettings.empty())
    {
        interfaceSettings.push_back(g_unknown);
    }
}

void NetworkingObjectBase::GetConnected(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (this->m_ipSettingsMap.find(interfaceName) != this->m_ipSettingsMap.end())
    {
        std::stringstream ipSettingsData(this->m_ipSettingsMap[interfaceName]);
        ParseInterfaceDataForSettings(false, g_connectedFlag, ipSettingsData, interfaceSettings);
        if (!interfaceSettings.empty())
        {
            interfaceSettings.clear();
            interfaceSettings.push_back(g_true);
        }
        else
        {
            interfaceSettings.push_back(g_false);
        }
    }
    else
    {
        interfaceSettings.push_back(g_unknown);
    }
}

void NetworkingObjectBase::GenerateInterfaceSettingsString(const std::string& interfaceName, NetworkingSettingType settingType, std::string& interfaceSettingsString)
{
    std::vector<std::string> interfaceSettings;
    switch (settingType)
    {
        case NetworkingSettingType::InterfaceTypes:
            GetInterfaceTypes(interfaceName, interfaceSettings);
            break;
        case NetworkingSettingType::MacAddresses:
            GetMacAddresses(interfaceName, interfaceSettings);
            break;
        case NetworkingSettingType::IpAddresses:
            GetIpAddresses(interfaceName, interfaceSettings);
            break;
        case NetworkingSettingType::SubnetMasks:
            GetSubnetMasks(interfaceName, interfaceSettings);
            break;
        case NetworkingSettingType::DefaultGateways:
            GetDefaultGateways(interfaceName, interfaceSettings);
            break;
        case NetworkingSettingType::DnsServers:
            GetDnsServers(interfaceName, interfaceSettings);
            break;
        case NetworkingSettingType::DhcpEnabled:
            GetDhcpEnabled(interfaceName, interfaceSettings);
            break;
        case NetworkingSettingType::Enabled:
            GetEnabled(interfaceName, interfaceSettings);
            break;
        case NetworkingSettingType::Connected:
            GetConnected(interfaceName, interfaceSettings);
            break;
    }

    size_t size = interfaceSettings.size();
    for (size_t i = 0; i < size; i++)
    {
        if (i > 0)
        {
            interfaceSettingsString += g_comma;
        }
        interfaceSettingsString += interfaceSettings[i];
    }
}

void NetworkingObjectBase::UpdateSettingsString(NetworkingSettingType settingType, std::string& settingsString)
{
    settingsString.clear();
    std::vector<std::tuple<std::string, std::string>> settings;
    for (size_t i = 0; i < this->m_interfaceNames.size(); i++)
    {
        std::string interfaceSettingsString;
        GenerateInterfaceSettingsString(this->m_interfaceNames[i], settingType, interfaceSettingsString);
        settings.push_back(make_tuple(this->m_interfaceNames[i], interfaceSettingsString));
    }

    if (!settings.empty())
    {
        sort(settings.begin(), settings.end());
        size_t size = settings.size();
        for (size_t i = 0; i < size; i++)
        {
            if (!std::get<1>(settings[i]).empty())
            {
                if (!settingsString.empty())
                {
                    settingsString += g_semiColon;
                }
                settingsString += std::get<0>(settings[i]) + g_equals + std::get<1>(settings[i]);
            }
        }
    }
}

void NetworkingObjectBase::GetInterfaceTypesFromNetworkManager()
{
    std::string interfaceTypesData = RunCommand(g_getInterfaceTypesNmcli);
    std::regex interfaceNamePrefixPatternNmcli(g_interfaceNamePrefixNmcli);
    std::smatch interfaceNamePrefixMatchNmcli;
    while (std::regex_search(interfaceTypesData, interfaceNamePrefixMatchNmcli, interfaceNamePrefixPatternNmcli))
    {
        if (this->m_networkManagementService == NetworkManagementService::Unknown)
        {
            this->m_networkManagementService = NetworkManagementService::NetworkManager;
        }

        std::string interfaceNamePrefix = interfaceNamePrefixMatchNmcli.str(0);
        std::string interfaceData = interfaceNamePrefixMatchNmcli.suffix().str();
        std::string nextDataToProcess = interfaceData;

        std::string interfaceName;
        size_t interfaceNameSuffixFront = interfaceData.find("\n");
        if (interfaceNameSuffixFront != std::string::npos)
        {
            interfaceName = interfaceData.substr(0, interfaceNameSuffixFront);
        }

        interfaceData = std::regex_search(interfaceData, interfaceNamePrefixMatchNmcli, interfaceNamePrefixPatternNmcli) ? interfaceData.substr(0, interfaceNamePrefixMatchNmcli.position(0)) : interfaceData;

        if (IsKnownInterfaceName(interfaceName))
        {
            std::string interfaceType;
            std::regex interfaceTypePrefixPattern(g_interfaceTypePrefix);
            std::smatch interfaceTypePrefixMatch;
            if (std::regex_search(interfaceData, interfaceTypePrefixMatch, interfaceTypePrefixPattern))
            {
                interfaceType = interfaceTypePrefixMatch.suffix().str();

                size_t interfaceTypeSuffixFront = interfaceType.find("\n");
                if (interfaceTypeSuffixFront != std::string::npos)
                {
                    interfaceType = interfaceType.substr(0, interfaceTypeSuffixFront);
                }
            }

            if ((!interfaceName.empty()) && (!interfaceType.empty()) && (interfaceType != g_doubleDash))
            {
                std::map<std::string, std::string>::iterator interfaceTypesMapIterator = this->m_interfaceTypesMap.find(interfaceName);
                if (interfaceTypesMapIterator != this->m_interfaceTypesMap.end())
                {
                    interfaceTypesMapIterator->second = interfaceType;
                }
                else
                {
                    this->m_interfaceTypesMap.insert(std::pair<std::string, std::string>(interfaceName, interfaceType));
                }
            }
        }

        interfaceTypesData = nextDataToProcess;
    }

    if ((this->m_interfaceTypesMap.empty()) && (this->m_networkManagementService == NetworkManagementService::NetworkManager))
    {
        this->m_networkManagementService = NetworkManagementService::Unknown;
    }
}

void NetworkingObjectBase::GetInterfaceTypesFromSystemdNetworkd()
{
    std::string interfaceTypesData = RunCommand(g_getInterfaceTypesNetworkctl);
    std::stringstream interfaceTypesStream(interfaceTypesData);
    std::string line;
    while(std::getline(interfaceTypesStream, line))
    {
        if (this->m_networkManagementService == NetworkManagementService::Unknown)
        {
            this->m_networkManagementService = NetworkManagementService::SystemdNetworkd;
        }

        std::regex interfaceTypesPatternNetworkctl(g_interfaceTypesPrefixNetworkctl);
        if (std::regex_match(line.begin(), line.end(), interfaceTypesPatternNetworkctl))
        {
            std::string token;
            std::stringstream interfaceDataStream(line);
            while (std::getline(interfaceDataStream, token, g_spaceCharacter))
            {
                if (IsKnownInterfaceName(token))
                {
                    std::string interfaceName = token;
                    do
                    {
                        std::getline(interfaceDataStream, token, g_spaceCharacter);
                    }
                    while(token.empty());

                    std::string interfaceType = token;
                    if ((!interfaceName.empty()) && (!interfaceType.empty()))
                    {
                        std::map<std::string, std::string>::iterator interfaceTypesMapIterator = this->m_interfaceTypesMap.find(interfaceName);
                        if (interfaceTypesMapIterator != this->m_interfaceTypesMap.end())
                        {
                            interfaceTypesMapIterator->second = interfaceType;
                        }
                        else
                        {
                            this->m_interfaceTypesMap.insert(std::pair<std::string, std::string>(interfaceName, interfaceType));
                        }
                    }
                }
            }
        }
    }

    if ((this->m_interfaceTypesMap.empty()) && (this->m_networkManagementService == NetworkManagementService::SystemdNetworkd))
    {
        this->m_networkManagementService = NetworkManagementService::Unknown;
    }
}

void NetworkingObjectBase::GenerateInterfaceTypesMap()
{
    this->m_interfaceTypesMap.clear();

    if ((this->m_networkManagementService == NetworkManagementService::NetworkManager) || (this->m_networkManagementService == NetworkManagementService::Unknown))
    {
        GetInterfaceTypesFromNetworkManager();
    }
    if ((this->m_networkManagementService == NetworkManagementService::SystemdNetworkd) || (this->m_networkManagementService == NetworkManagementService::Unknown))
    {
        GetInterfaceTypesFromSystemdNetworkd();
    }
    if (this->m_networkManagementService == NetworkManagementService::Unknown)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(NetworkingLog::Get(), "Network interface management service not found");
        }
    }
}

void NetworkingObjectBase::GenerateIpSettingsMap()
{
    this->m_ipSettingsMap.clear();
    std::string ipData = RunCommand(g_getIpAddressDetails);
    std::regex interfaceDataPrefixPattern(g_interfaceDataPrefix);
    std::smatch interfaceDataPrefixMatch;
    while (std::regex_search(ipData, interfaceDataPrefixMatch, interfaceDataPrefixPattern))
    {
        std::string interfaceDataPrefix = interfaceDataPrefixMatch.str(0);
        size_t interfaceNamePrefixBack = interfaceDataPrefix.find(g_spaceString);
        size_t interfaceDataPrefixBack = interfaceDataPrefix.find_last_of(g_colon);

        ipData = interfaceDataPrefixMatch.suffix().str();

        std::string interfaceName;
        if ((interfaceNamePrefixBack != std::string::npos) && (interfaceDataPrefixBack != std::string::npos))
        {
            interfaceName = interfaceDataPrefix.substr(interfaceNamePrefixBack + 1, interfaceDataPrefixBack - interfaceNamePrefixBack - 1);
        }

        if (!IsKnownInterfaceName(interfaceName))
        {
            size_t interfaceNameSuffixFront = interfaceName.find(g_at);
            if (interfaceNameSuffixFront != std::string::npos)
            {
                interfaceName = interfaceName.substr(0, interfaceNameSuffixFront);
            }
        }

        if (IsKnownInterfaceName(interfaceName))
        {
            std::string interfaceData = std::regex_search(ipData, interfaceDataPrefixMatch, interfaceDataPrefixPattern) ? ipData.substr(0, interfaceDataPrefixMatch.position(0)) : ipData;
            std::map<std::string, std::string>::iterator ipDataMapIterator = this->m_ipSettingsMap.find(interfaceName);
            if (ipDataMapIterator != this->m_ipSettingsMap.end())
            {
                ipDataMapIterator->second = interfaceData;
            }
            else
            {
                this->m_ipSettingsMap.insert(std::pair<std::string, std::string>(interfaceName, interfaceData));
            }
        }
    }
}

void NetworkingObjectBase::GenerateDefaultGatewaysMap()
{
    this->m_defaultGatewaysMap.clear();
    std::string defaultGatewaysData = RunCommand(g_getDefaultGateways);
    std::regex interfaceNamePrefixPatternDefaultGateways(g_interfaceNamePrefixDefaultGateways);
    std::smatch interfaceNamePrefixMatch;
    while (std::regex_search(defaultGatewaysData, interfaceNamePrefixMatch, interfaceNamePrefixPatternDefaultGateways))
    {
        std::string interfaceNamePrefix = interfaceNamePrefixMatch.str(0);
        std::string interfaceData = interfaceNamePrefixMatch.suffix().str();
        size_t interfaceNameSuffixFront = interfaceData.find(g_spaceString);

        std::string interfaceName;
        if (interfaceNameSuffixFront != std::string::npos)
        {
            interfaceName = interfaceData.substr(0, interfaceNameSuffixFront);
        }

        if (IsKnownInterfaceName(interfaceName))
        {
            std::string defaultGateway;
            std::regex defaultGatewayPrefixPattern(g_defaultGatewaysPrefix);
            std::smatch defaultGatewayPrefixMatch;

            if (std::regex_search(defaultGatewaysData, defaultGatewayPrefixMatch, defaultGatewayPrefixPattern))
            {
                defaultGateway = defaultGatewayPrefixMatch.suffix().str();

                size_t defaultGatewaySuffixFront = defaultGateway.find(g_spaceString);
                if (defaultGatewaySuffixFront != std::string::npos)
                {
                    defaultGateway = defaultGateway.substr(0, defaultGatewaySuffixFront);
                }
            }

            if (!defaultGateway.empty())
            {
                std::map<std::string, std::vector<std::string>>::iterator defaultGatewaysMapIterator = this->m_defaultGatewaysMap.find(interfaceName);
                if (defaultGatewaysMapIterator != this->m_defaultGatewaysMap.end())
                {
                    (defaultGatewaysMapIterator->second).push_back(defaultGateway);
                }
                else
                {
                    std::vector<std::string> defaultGateways{ defaultGateway };
                    this->m_defaultGatewaysMap.insert(std::pair<std::string, std::vector<std::string>>(interfaceName, defaultGateways));
                }
            }
        }

        defaultGatewaysData = interfaceNamePrefixMatch.suffix().str();
    }
}

void RemoveDuplicates(std::vector<std::string>& vec)
{
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

void NetworkingObjectBase::GetGlobalDnsServers(std::string dnsServersData, std::vector<std::string>& globalDnsServers)
{
    std::regex globalDnsServersPattern(g_globalDnsServers);
    std::smatch globalDnsServersPrefixMatch;
    while (std::regex_search(dnsServersData, globalDnsServersPrefixMatch, globalDnsServersPattern))
    {
        dnsServersData = globalDnsServersPrefixMatch.suffix().str();
        std::smatch globalDnsServersSuffixMatch;
        std::string globalDnsServersData = std::regex_search(dnsServersData, globalDnsServersSuffixMatch, g_interfaceNamePrefixPatternDnsServers) ? dnsServersData.substr(0, globalDnsServersSuffixMatch.position(0)) : dnsServersData;

        std::smatch dnsServersPrefixMatch;
        if (std::regex_search(globalDnsServersData, dnsServersPrefixMatch, g_dnsServersPrefixPattern))
        {
            bool isValidData = true;
            std::string dnsServers;
            std::stringstream globalDnsServersStream(dnsServersPrefixMatch.suffix().str());
            while(isValidData && std::getline(globalDnsServersStream, dnsServers))
            {
                std::string dnsServer;
                std::stringstream line(dnsServers);
                while (std::getline(line, dnsServer, g_spaceCharacter))
                {
                    dnsServer.erase(remove(dnsServer.begin(), dnsServer.end(), g_spaceCharacter), dnsServer.end());
                    if ((std::regex_match(dnsServer, g_ipv4Pattern)) || (std::regex_match(dnsServer, g_ipv6Pattern)))
                    {
                        globalDnsServers.push_back(dnsServer);
                    }
                    else
                    {
                        isValidData = false;
                        break;
                    }
                }
            }
        }
    }
}

void NetworkingObjectBase::GenerateDnsServersMap()
{
    this->m_dnsServersMap.clear();
    if (true != EnableAndStartDaemon(g_systemdResolvedServiceName, NetworkingLog::Get()))
    {
        OsConfigLogError(NetworkingLog::Get(), "Unable to start service %s. DnsServers data will be empty.", g_systemdResolvedServiceName);
        return;
    }

    std::string dnsServersData = RunCommand(g_getDnsServers);

    std::vector<std::string> globalDnsServers;
    GetGlobalDnsServers(dnsServersData, globalDnsServers);

    std::smatch interfaceNamePrefixMatch;
    while (std::regex_search(dnsServersData, interfaceNamePrefixMatch, g_interfaceNamePrefixPatternDnsServers))
    {
        std::string interfaceNamePrefix = interfaceNamePrefixMatch.str(0);
        std::string interfaceNameData = interfaceNamePrefixMatch.suffix().str();
        size_t interfaceNameSuffixFront = interfaceNameData.find(g_closeParenthesis);

        std::string interfaceName;
        if (interfaceNameSuffixFront != std::string::npos)
        {
            interfaceName = interfaceNameData.substr(0, interfaceNameSuffixFront);
        }

        dnsServersData = interfaceNameData;
        std::string interfaceData = std::regex_search(dnsServersData, interfaceNamePrefixMatch, g_interfaceNamePrefixPatternDnsServers) ? dnsServersData.substr(0, interfaceNamePrefixMatch.position(0)) : dnsServersData;

        if (IsKnownInterfaceName(interfaceName))
        {
            std::map<std::string, std::vector<std::string>>::iterator dnsServersMapIterator;
            std::string dnsServers;
            std::smatch dnsServersPrefixMatch;
            if (std::regex_search(interfaceData, dnsServersPrefixMatch, g_dnsServersPrefixPattern))
            {
                bool isValidData = true;
                std::stringstream dnsServerStream(dnsServersPrefixMatch.suffix().str());
                while(isValidData && std::getline(dnsServerStream, dnsServers))
                {
                    std::string dnsServer;
                    std::stringstream line(dnsServers);
                    while (std::getline(line, dnsServer, g_spaceCharacter))
                    {
                        dnsServer.erase(remove(dnsServer.begin(), dnsServer.end(), g_spaceCharacter), dnsServer.end());
                        if ((std::regex_match(dnsServer, g_ipv4Pattern)) || (std::regex_match(dnsServer, g_ipv6Pattern)))
                        {
                            dnsServersMapIterator = this->m_dnsServersMap.find(interfaceName);
                            if (dnsServersMapIterator != this->m_dnsServersMap.end())
                            {
                                (dnsServersMapIterator->second).push_back(dnsServer);
                            }
                            else
                            {
                                std::vector<std::string> dnsServers{ dnsServer };
                                this->m_dnsServersMap.insert(std::pair<std::string, std::vector<std::string>>(interfaceName, dnsServers));
                            }
                        }
                        else
                        {
                            isValidData = false;
                            break;
                        }
                    }
                }
            }

            if (!globalDnsServers.empty())
            {
                dnsServersMapIterator = this->m_dnsServersMap.find(interfaceName);
                if (dnsServersMapIterator != this->m_dnsServersMap.end())
                {
                    (dnsServersMapIterator->second).insert((dnsServersMapIterator->second).end(), globalDnsServers.begin(), globalDnsServers.end());
                    RemoveDuplicates(dnsServersMapIterator->second);
                }
                else
                {
                    this->m_dnsServersMap.insert(std::pair<std::string, std::vector<std::string>>(interfaceName, globalDnsServers));
                }
            }
        }
    }
}

void NetworkingObjectBase::RefreshInterfaceNames(std::vector<std::string>& interfaceNames)
{
    interfaceNames.clear();
    std::string interfaceNamesData = RunCommand(g_getInterfaceNames);
    if (!interfaceNamesData.empty())
    {
        std::stringstream interfaceNamesStream(interfaceNamesData);
        std::string token = g_emptyString;
        while (std::getline(interfaceNamesStream, token))
        {
            interfaceNames.push_back(token);
        }
    }
}

void NetworkingObjectBase::RefreshInterfaceData()
{
    GenerateInterfaceTypesMap();
    GenerateIpSettingsMap();
    GenerateDefaultGatewaysMap();
    GenerateDnsServersMap();
}

void NetworkingObjectBase::RefreshSettingsStrings()
{
    RefreshInterfaceNames(this->m_interfaceNames);
    if (this->m_interfaceNames.size() > 0)
    {
        RefreshInterfaceData();
        UpdateSettingsString(NetworkingSettingType::InterfaceTypes, this->m_settings.interfaceTypes);
        UpdateSettingsString(NetworkingSettingType::MacAddresses, this->m_settings.macAddresses);
        UpdateSettingsString(NetworkingSettingType::IpAddresses, this->m_settings.ipAddresses);
        UpdateSettingsString(NetworkingSettingType::SubnetMasks, this->m_settings.subnetMasks);
        UpdateSettingsString(NetworkingSettingType::DefaultGateways, this->m_settings.defaultGateways);
        UpdateSettingsString(NetworkingSettingType::DnsServers, this->m_settings.dnsServers);
        UpdateSettingsString(NetworkingSettingType::DhcpEnabled, this->m_settings.dhcpEnabled);
        UpdateSettingsString(NetworkingSettingType::Enabled, this->m_settings.enabled);
        UpdateSettingsString(NetworkingSettingType::Connected, this->m_settings.connected);
    }
}

bool NetworkingObjectBase::IsKnownInterfaceName(std::string str)
{
    for (size_t i = 0; i < this->m_interfaceNames.size(); i++)
    {
        if (str == this->m_interfaceNames[i])
        {
            return true;
        }
    }

    return false;
}

int NetworkingObject::WriteJsonElement(rapidjson::Writer<rapidjson::StringBuffer>* writer, const char* key, const char* value)
{
    int result = 0;
    if (false == writer->Key(key))
    {
        result = 1;
    }

    if (false == writer->String(value))
    {
        result = 1;
    }

    return result;
}

int NetworkingObjectBase::TruncateValueStrings(std::vector<std::pair<std::string, std::string>>& fieldValueVector)
{
    // If m_maxPayloadSizeBytes is zero or a negative number, or too small we do not truncate
    if ((m_maxPayloadSizeBytes <= 0) || (m_maxPayloadSizeBytes <= g_templateWithDotsSize))
    {
        return MMI_OK;
    }

    unsigned int maxValueSize = 0;
    unsigned int totalValueSize = 0;
    maxValueSize =  (m_maxPayloadSizeBytes > g_templateSize) ? m_maxPayloadSizeBytes - g_templateSize : 0;
    std::vector<std::string> fields;

    for (size_t i = 0; i < fieldValueVector.size(); i++)
    {
        fields.push_back(fieldValueVector[i].first);
        totalValueSize += fieldValueVector[i].second.length();
    }

    if (totalValueSize > maxValueSize)
    {
        sort(fieldValueVector.begin(), fieldValueVector.end(), [](std::pair<std::string, std::string>& a, std::pair<std::string, std::string>& b)
        {
            return (a.second.length() < b.second.length()) || ((a.second.length() == b.second.length()) && a.first < b.first);
        });

        for (size_t i = 0; i < fieldValueVector.size(); i++)
        {
            std::string keyString = fieldValueVector[i].first;
            std::string valueString = fieldValueVector[i].second;
            if (totalValueSize > maxValueSize)
            {
                unsigned int cutPerField = 0;
                // Number of fields to truncate
                unsigned int numFieldsToCut = fieldValueVector.size() - i;
                // Size to truncate for each field (value string)
                cutPerField = ((totalValueSize - maxValueSize) / numFieldsToCut) + (((totalValueSize - maxValueSize) % numFieldsToCut) ? 1 : 0);
                unsigned int lengthBeforeCut = valueString.length();
                if (valueString.length() > g_twoDotsSize)
                {
                    if (valueString.length() < cutPerField + g_twoDotsSize)
                    {
                        // If we truncate a string, the mininum length is two
                        valueString = g_twoDots;
                    }
                    else
                    {
                        valueString = valueString.substr(0, valueString.length() - g_twoDotsSize - cutPerField) + g_twoDots;
                    }
                }
                // If value string size is less than two bytes, keep it as is
                unsigned int lengthAfterCut = valueString.length();
                totalValueSize -= ((lengthBeforeCut > lengthAfterCut) ? (lengthBeforeCut - lengthAfterCut) : 0);

                fieldValueVector[i].second = valueString;
            }
        }

        // Sort vector back to orginial order
        std::unordered_map<std::string, int> position;
        for (int i = 0; i < (int)fields.size(); i++)
        {
            position[fields[i]] = i;
        }

        sort(fieldValueVector.begin(), fieldValueVector.end(), [&position](std::pair<std::string, std::string>& a, std::pair<std::string, std::string>& b)
        {
            return (position[a.first] < position[b.first]) || ((position[a.first] == position[b.first]) && (a.second < b.second));
        });
    }

    return (totalValueSize + g_templateSize <= m_maxPayloadSizeBytes) ? MMI_OK : ENODATA;
}

int NetworkingObjectBase::Get(
        const char* componentName,
        const char* objectName,
        MMI_JSON_STRING* payload,
        int* payloadSizeBytes)
{
    int status = MMI_OK;

    if ((nullptr == componentName) || (0 != std::strcmp(componentName, NETWORKING)))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(NetworkingLog::Get(), "NetworkingObjectBase::Get(%s, %s, %.*s, %d) componentName %s is invalid, %s is expected",
                componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), componentName, NETWORKING);
        }
        status = EINVAL;
    }
    else if ((nullptr == objectName) || (0 != std::strcmp(objectName, NETWORK_CONFIGURATION)))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(NetworkingLog::Get(), "NetworkingObjectBase::Get(%s, %s, %.*s, %d) objectName %s is invalid, %s is expected",
                componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), objectName, NETWORK_CONFIGURATION);
        }
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(NetworkingLog::Get(), "NetworkingObjectBase::Get(%s, %s, %.*s, %d) payload %.*s is null",
                componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), (payloadSizeBytes ? *payloadSizeBytes : 0), *payload);
        }
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(NetworkingLog::Get(), "NetworkingObjectBase::Get(%s, %s, %.*s, %d) payloadSizeBytes %d is null",
                componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), (payloadSizeBytes ? *payloadSizeBytes : 0));
        }
        status = EINVAL;
    }
    else
    {
        RefreshSettingsStrings();

        std::vector<std::pair<std::string, std::string>> fieldValueVector;
        fieldValueVector.push_back(make_pair(g_interfaceTypes, m_settings.interfaceTypes));
        fieldValueVector.push_back(make_pair(g_macAddresses, m_settings.macAddresses));
        fieldValueVector.push_back(make_pair(g_ipAddresses, m_settings.ipAddresses));
        fieldValueVector.push_back(make_pair(g_subnetMasks, m_settings.subnetMasks));
        fieldValueVector.push_back(make_pair(g_defaultGateways, m_settings.defaultGateways));
        fieldValueVector.push_back(make_pair(g_dnsServers, m_settings.dnsServers));
        fieldValueVector.push_back(make_pair(g_dhcpEnabled, m_settings.dhcpEnabled));
        fieldValueVector.push_back(make_pair(g_enabled, m_settings.enabled));
        fieldValueVector.push_back(make_pair(g_connected, m_settings.connected));

        status = TruncateValueStrings(fieldValueVector);
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        writer.StartObject();
        int writeResult = 0;
        for (size_t i = 0; i < fieldValueVector.size(); i++)
        {
            writeResult += WriteJsonElement(&writer, fieldValueVector[i].first.c_str(), fieldValueVector[i].second.c_str());
        }
        writer.EndObject();

        std::string networkingJsonString(sb.GetString());
        networkingJsonString.erase(std::find(networkingJsonString.begin(), networkingJsonString.end(), '\0'), networkingJsonString.end());

        *payloadSizeBytes = networkingJsonString.length();

        if (((m_maxPayloadSizeBytes > 0) && (m_maxPayloadSizeBytes <= g_templateWithDotsSize) && ((unsigned int)*payloadSizeBytes != g_templateWithDotsSize)) ||
            ((m_maxPayloadSizeBytes > g_templateWithDotsSize) && ((unsigned int)*payloadSizeBytes > m_maxPayloadSizeBytes)))
        {
            OsConfigLogInfo(NetworkingLog::Get(), "Networking payload to report %u bytes, need to report %u bytes, reporting empty strings", (unsigned int)*payloadSizeBytes, m_maxPayloadSizeBytes);
            *payloadSizeBytes = g_templateWithDotsSize;
        }

        if (writeResult > 0)
        {
            *payloadSizeBytes = g_templateWithDotsSize;
        }

        *payload = new (std::nothrow) char[*payloadSizeBytes];
        if (nullptr == *payload)
        {
            if (nullptr != payloadSizeBytes)
            {
                OsConfigLogError(NetworkingLog::Get(), "Networking::Get insufficient buffer space available to allocate %d bytes", *payloadSizeBytes);
            }
            status = ENOMEM;
        }
        else
        {
            std::fill(*payload, *payload + *payloadSizeBytes, 0);
            std::memcpy(*payload, ((*payloadSizeBytes) == (int)g_templateWithDotsSize) ? g_templateWithDots : networkingJsonString.c_str(), *payloadSizeBytes);
        }
    }

    return status;
}
