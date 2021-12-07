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

std::string g_interfaceTypes = "InterfaceTypes";
std::string g_macAddresses = "MacAddresses";
std::string g_ipAddresses = "IpAddresses";
std::string g_subnetMasks = "SubnetMasks";
std::string g_defaultGateways = "DefaultGateways";
std::string g_dnsServers = "DnsServers";
std::string g_dhcpEnabled = "DhcpEnabled";
std::string g_enabled = "Enabled";
std::string g_connected = "Connected";

const char* g_getInterfaceNames = "ls -A /sys/class/net";
const char* g_getInterfaceTypesNmcli = "nmcli device show";
const char* g_getInterfaceTypesNetworkctl = "networkctl --no-legend";
const char* g_getIpAddressDetails = "ip addr";
const char* g_getDefaultGateways = "ip route";
const char* g_getDnsServers = "systemd-resolve --status";

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

const char* g_templateWithDots = R"""({"InterfaceTypes":"..","MacAddresses":"..","IpAddresses":"..","SubnetMasks":"..","DefaultGateways":"..","DnsServers":"..","DhcpEnabled":"..","Enabled":"..","Connected":".."})""";
const char* g_emptyValue = "";
const char* g_twoDots = "..";

const std::vector<std::string> g_fields = {"InterfaceTypes", "MacAddresses", "IpAddresses", "SubnetMasks","DefaultGateways","DnsServers", "DhcpEnabled", "Enabled", "Connected"};

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
    std::string commandOutputToReturn = "";
    if (MMI_OK == status)
    {
        commandOutputToReturn = (nullptr != textResult) ? std::string(textResult) : "";
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogError(NetworkingLog::Get(), "Failed to execute command '%s': %d, '%s'", command, status, (nullptr != textResult) ? textResult : "-");
    }

    if (nullptr != textResult)
    {
        free(textResult);
    }

    return commandOutputToReturn;
}

void NetworkingObjectBase::ParseInterfaceDataForSettings(bool hasPrefix, const char* flag, std::stringstream& data, std::vector<std::string>& settings)
{
    std::string token = "";
    while (std::getline(data, token, ' '))
    {
        if (token.find(flag) != std::string::npos)
        {
            if (hasPrefix)
            {
                std::getline(data, token, ' ');
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
            size_t subnetMasksDelimiter = interfaceSettings[i].find("/");
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
            size_t subnetMasksDelimiter = interfaceSettings[i].find("/");
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
            interfaceSettingsString += ",";
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
                    settingsString += ";";
                }
                settingsString += std::get<0>(settings[i]) + "=" + std::get<1>(settings[i]);
            }
        }
    }
}

void NetworkingObjectBase::GenerateInterfaceTypesMap()
{
    this->m_interfaceTypesMap.clear();
    std::string interfaceTypesData, interfaceData, interfaceName, interfaceType;
    std::map<std::string, std::string>::iterator interfaceTypesMapIterator;
    if ((this->m_networkManagementService == NetworkManagementService::NetworkManager) || (this->m_networkManagementService == NetworkManagementService::Unknown))
    {
        interfaceTypesData = RunCommand(g_getInterfaceTypesNmcli);

        std::regex interfaceNamePrefixPatternNmcli("GENERAL.DEVICE:\\s+");
        std::smatch interfaceNamePrefixMatchNmcli;
        while (std::regex_search(interfaceTypesData, interfaceNamePrefixMatchNmcli, interfaceNamePrefixPatternNmcli))
        {
            if (this->m_networkManagementService == NetworkManagementService::Unknown)
            {
                this->m_networkManagementService = NetworkManagementService::NetworkManager;
            }

            std::string interfaceNamePrefix = interfaceNamePrefixMatchNmcli.str(0);
            interfaceData = interfaceNamePrefixMatchNmcli.suffix().str();
            std::string nextDataToProcess = interfaceData;

            size_t interfaceNameSuffixFront = interfaceData.find("\n");
            if (interfaceNameSuffixFront != std::string::npos)
            {
                interfaceName = interfaceData.substr(0, interfaceNameSuffixFront);
            }

            interfaceData = std::regex_search(interfaceData, interfaceNamePrefixMatchNmcli, interfaceNamePrefixPatternNmcli) ? interfaceData.substr(0, interfaceNamePrefixMatchNmcli.position(0)) : interfaceData;

            if (IsKnownInterfaceName(interfaceName))
            {
                std::regex interfaceTypePrefixPattern("GENERAL.TYPE:\\s+");
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

                if ((!interfaceName.empty()) && (!interfaceType.empty()) && (interfaceType != "--"))
                {
                    interfaceTypesMapIterator = this->m_interfaceTypesMap.find(interfaceName);
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

            interfaceData.clear();
            interfaceName.clear();
            interfaceType.clear();
            interfaceTypesData = nextDataToProcess;
        }
        
        if ((this->m_interfaceTypesMap.empty()) && (this->m_networkManagementService == NetworkManagementService::NetworkManager))
        {
            this->m_networkManagementService = NetworkManagementService::Unknown;
        }
    }

    if ((this->m_networkManagementService == NetworkManagementService::SystemdNetworkd) || (this->m_networkManagementService == NetworkManagementService::Unknown))
    {
        interfaceTypesData = RunCommand(g_getInterfaceTypesNetworkctl);
        std::stringstream interfaceTypesDataStream(interfaceTypesData);
        while(std::getline(interfaceTypesDataStream, interfaceData))
        {
            if (this->m_networkManagementService == NetworkManagementService::Unknown)
            {
                this->m_networkManagementService = NetworkManagementService::SystemdNetworkd;
            }

            std::regex interfaceTypesDataPatternNetworkctl("^\\s*[0-9]+\\s+.*$");
            if (std::regex_match(interfaceData.begin(), interfaceData.end(), interfaceTypesDataPatternNetworkctl))
            {
                std::stringstream interfaceDataStream(interfaceData);
                std::string data;
                while (std::getline(interfaceDataStream, data, ' '))
                {
                    if (IsKnownInterfaceName(data))
                    {
                        interfaceName = data;
                        do
                        {
                            std::getline(interfaceDataStream, data, ' ');
                        }
                        while(data.empty());

                        interfaceType = data;          
                        if ((!interfaceName.empty()) && (!interfaceType.empty()))
                        {
                            interfaceTypesMapIterator = this->m_interfaceTypesMap.find(interfaceName);
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

            interfaceName.clear();
            interfaceType.clear();
        }

        if ((this->m_interfaceTypesMap.empty()) && (this->m_networkManagementService == NetworkManagementService::SystemdNetworkd))
        {
            this->m_networkManagementService = NetworkManagementService::Unknown;
        }
    }
}

void NetworkingObjectBase::GenerateIpSettingsMap()
{
    this->m_ipSettingsMap.clear();
    std::string ipData = RunCommand(g_getIpAddressDetails);
    std::regex interfaceDataPrefixPattern("[0-9]+:\\s+.*:\\s+");
    std::smatch interfaceDataPrefixMatch;
    while (std::regex_search(ipData, interfaceDataPrefixMatch, interfaceDataPrefixPattern)) 
    {
        std::string interfaceDataPrefix = interfaceDataPrefixMatch.str(0);
        size_t interfaceNamePrefixBack = interfaceDataPrefix.find(" ");
        size_t interfaceDataPrefixBack = interfaceDataPrefix.find_last_of(":");

        ipData = interfaceDataPrefixMatch.suffix().str();

        std::string interfaceName;
        if ((interfaceNamePrefixBack != std::string::npos) && (interfaceDataPrefixBack != std::string::npos))
        {
            interfaceName = interfaceDataPrefix.substr(interfaceNamePrefixBack + 1, interfaceDataPrefixBack - interfaceNamePrefixBack - 1);
        }

        if (!IsKnownInterfaceName(interfaceName))
        {
            size_t interfaceNameSuffixFront = interfaceName.find("@");
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
    std::regex interfaceNamePrefixPattern("default\\s+via\\s+.*\\s+dev\\s+");
    std::smatch interfaceNamePrefixMatch;
    while (std::regex_search(defaultGatewaysData, interfaceNamePrefixMatch, interfaceNamePrefixPattern))
    {
        std::string interfaceNamePrefix = interfaceNamePrefixMatch.str(0);
        std::string interfaceData = interfaceNamePrefixMatch.suffix().str();
        size_t interfaceNameSuffixFront = interfaceData.find(" ");

        std::string interfaceName;
        if (interfaceNameSuffixFront != std::string::npos)
        {
            interfaceName = interfaceData.substr(0, interfaceNameSuffixFront);
        }

        if (IsKnownInterfaceName(interfaceName))
        {
            std::string defaultGateway;
            std::regex defaultGatewayPrefixPattern("default\\s+via\\s+");
            std::smatch defaultGatewayPrefixMatch;

            if (std::regex_search(defaultGatewaysData, defaultGatewayPrefixMatch, defaultGatewayPrefixPattern))
            {
                defaultGateway = defaultGatewayPrefixMatch.suffix().str();

                size_t defaultGatewaySuffixFront = defaultGateway.find(" ");
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

void NetworkingObjectBase::GenerateDnsServersMap()
{
    this->m_dnsServersMap.clear();
    std::regex ipv4Pattern("(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])");
    std::regex ipv6Pattern("(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}"
    ":[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}"
    "|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}"
    ":((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|[fF][eE]80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::([fF][eE]{4}(:0{1,4}){0,1}:){0,1}"
    "((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}"
    ":((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))");
    std::regex interfaceNamePrefixPattern("Link\\s+[0-9]+\\s+\\(");
    std::regex dnsServersPrefixPattern("DNS\\s+Servers:\\s+");

    std::string dnsServersData = RunCommand(g_getDnsServers);

    std::vector<std::string> globalDnsServers;
    std::regex globalDnsServersSectionPattern("Global\\s*\n");
    std::smatch globalDnsServersSectionMatch;
    while (std::regex_search(dnsServersData, globalDnsServersSectionMatch, globalDnsServersSectionPattern))
    {
        dnsServersData = globalDnsServersSectionMatch.suffix().str();
        std::smatch networkInterfacesSectionMatch;
        std::string globalDnsServersData = std::regex_search(dnsServersData, networkInterfacesSectionMatch, interfaceNamePrefixPattern) ? dnsServersData.substr(0, networkInterfacesSectionMatch.position(0)) : dnsServersData; 
        std::string globalDnsServer;
        std::smatch globalDnsServersPrefixMatch;
        if (std::regex_search(globalDnsServersData, globalDnsServersPrefixMatch, dnsServersPrefixPattern))
        {
            std::stringstream globalDnsServerStream(globalDnsServersPrefixMatch.suffix().str());
            while(std::getline(globalDnsServerStream, globalDnsServer))
            {
                globalDnsServer.erase(remove(globalDnsServer.begin(), globalDnsServer.end(), ' '), globalDnsServer.end());
                if ((std::regex_match(globalDnsServer, ipv4Pattern)) || (std::regex_match(globalDnsServer, ipv6Pattern)))
                {
                    globalDnsServers.push_back(globalDnsServer);
                }
                else
                {
                    break;
                }
            }
        }
    }

    std::smatch interfaceNamePrefixMatch;
    while (std::regex_search(dnsServersData, interfaceNamePrefixMatch, interfaceNamePrefixPattern))
    {
        std::string interfaceNamePrefix = interfaceNamePrefixMatch.str(0);
        std::string interfaceNameData = interfaceNamePrefixMatch.suffix().str();
        size_t interfaceNameSuffixFront = interfaceNameData.find(")");

        std::string interfaceName;
        if (interfaceNameSuffixFront != std::string::npos)
        {
            interfaceName = interfaceNameData.substr(0, interfaceNameSuffixFront);
        }

        dnsServersData = interfaceNameData;
        std::string interfaceData = std::regex_search(dnsServersData, interfaceNamePrefixMatch, interfaceNamePrefixPattern) ? dnsServersData.substr(0, interfaceNamePrefixMatch.position(0)) : dnsServersData; 

        if (IsKnownInterfaceName(interfaceName))
        {
            std::map<std::string, std::vector<std::string>>::iterator dnsServersMapIterator;
            std::string dnsServer;
            std::smatch dnsServersPrefixMatch;
            if (std::regex_search(interfaceData, dnsServersPrefixMatch, dnsServersPrefixPattern))
            {
                std::stringstream dnsServerStream(dnsServersPrefixMatch.suffix().str());
                while(std::getline(dnsServerStream, dnsServer))
                {
                    dnsServer.erase(remove(dnsServer.begin(), dnsServer.end(), ' '), dnsServer.end());
                    if ((std::regex_match(dnsServer, ipv4Pattern)) || (std::regex_match(dnsServer, ipv6Pattern)))
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
                        break;
                    }
                }
            }

            if (!globalDnsServers.empty())
            {
                dnsServersMapIterator = this->m_dnsServersMap.find(interfaceName);
                if (dnsServersMapIterator != this->m_dnsServersMap.end())
                {
                    (dnsServersMapIterator->second).insert((dnsServersMapIterator->second).end(), globalDnsServers.begin(), globalDnsServers.end());
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
        std::string token = "";
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
        MMI_HANDLE clientSession,
        const char* componentName,
        const char* objectName,
        MMI_JSON_STRING* payload,
        int* payloadSizeBytes)
{
    UNUSED(clientSession);
    UNUSED(componentName);
    UNUSED(objectName);

    int status = MMI_OK;

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

    return status;
}