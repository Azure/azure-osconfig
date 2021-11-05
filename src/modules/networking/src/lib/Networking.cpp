// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <stdio.h>
#include <Networking.h>
#include <CommonUtils.h>

const char* g_interfaceNamesCommand = "ls -A /sys/class/net";

const char* g_interfaceTypesQueryNmcli = "nmcli device show";
const char* g_interfaceTypesQueryNetworkctl = "networkctl --no-legend";

const char* g_ipAddressQuery = "ip -j addr";
const char* g_ipRouteQuery = "ip route";
const char* g_systemdResolveQuery = "systemd-resolve --status";

const char* g_ifname = "ifname";
const char* g_addrInfo = "addr_info";

const char* g_interfaceTypesString = "InterfaceTypes";

const char* g_macAddressesString = "MacAddresses";
const char* g_address = "address";

const char* g_ipAddressesString = "IpAddresses";
const char* g_local = "local";

const char* g_subnetMasksString = "SubnetMasks";
const char* g_prefixlen = "prefixlen";

const char* g_defaultGatewaysString = "DefaultGateways";
const char* g_dnsServersString = "DnsServers";

const char* g_dhcpEnabledString = "DhcpEnabled";
const char* g_dynamic = "dynamic";

const char* g_enabledString = "Enabled";
const char* g_operstate = "operstate";

const char* g_up = "UP";
const char* g_down = "DOWN";

const char* g_connectedString = "Connected";
const char* g_lowerUp = "LOWER_UP";
const char* g_flags = "flags";

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

const bool g_labeledData = true;
const bool g_nonLabeledData = false;

OSCONFIG_LOG_HANDLE NetworkingLog::m_logNetworking = nullptr;

NetworkingObject::NetworkingObject(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
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

void NetworkingObjectBase::GenerateInterfaceSettingsString(std::vector<std::string> interfaceSettings, std::string& interfaceSettingsString)
{
    size_t interfaceSettingsSize = interfaceSettings.size();
    for (size_t i = 0; i < interfaceSettingsSize; ++i)
    {
        if (i > 0)
        {
            interfaceSettingsString += ",";
        }

        interfaceSettingsString += interfaceSettings[i];
    }
}

void NetworkingObjectBase::ParseMacAddresses(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (!this->m_document.HasParseError())
    {
        if (this->m_document.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < this->m_document.Size(); ++i)
            {
                if ((this->m_document[i].HasMember(g_ifname)) &&
                    (this->m_document[i][g_ifname].IsString()))
                {
                    if ((std::strcmp(this->m_document[i][g_ifname].GetString(), interfaceName.c_str()) == 0) &&
                        (this->m_document[i].HasMember(g_address)) &&
                        (this->m_document[i][g_address].IsString()))
                    {
                        interfaceSettings.push_back(this->m_document[i][g_address].GetString());
                        break;
                    }
                }
            }
        }
    }
}

void NetworkingObjectBase::ParseIpAddresses(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (!this->m_document.HasParseError())
    {
        if (this->m_document.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < this->m_document.Size(); ++i)
            {
                if ((this->m_document[i].HasMember(g_ifname)) &&
                    (this->m_document[i][g_ifname].IsString()))
                {
                    if ((std::strcmp(this->m_document[i][g_ifname].GetString(), interfaceName.c_str()) == 0) &&
                        (this->m_document[i].HasMember(g_addrInfo)) &&
                        (this->m_document[i][g_addrInfo].IsArray()))
                    {
                        for (rapidjson::SizeType j = 0; j < this->m_document[i][g_addrInfo].Size(); ++j)
                        {
                            if ((this->m_document[i][g_addrInfo][j].HasMember(g_local)) &&
                                (this->m_document[i][g_addrInfo][j][g_local].IsString()))
                            {
                                interfaceSettings.push_back(this->m_document[i][g_addrInfo][j][g_local].GetString());
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

void NetworkingObjectBase::ParseSubnetMasks(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    if (!this->m_document.HasParseError())
    {
        if (this->m_document.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < this->m_document.Size(); ++i)
            {
                if ((this->m_document[i].HasMember(g_ifname)) &&
                    (this->m_document[i][g_ifname].IsString()))
                {
                    if ((std::strcmp(this->m_document[i][g_ifname].GetString(), interfaceName.c_str()) == 0) &&
                        (this->m_document[i].HasMember(g_addrInfo)) &&
                        (this->m_document[i][g_addrInfo].IsArray()))
                    {
                        for (rapidjson::SizeType j = 0; j < this->m_document[i][g_addrInfo].Size(); ++j)
                        {
                            if ((this->m_document[i][g_addrInfo][j].HasMember(g_prefixlen)) &&
                                (this->m_document[i][g_addrInfo][j][g_prefixlen].IsInt()))
                            {
                                interfaceSettings.push_back(std::to_string(this->m_document[i][g_addrInfo][j][g_prefixlen].GetInt()));
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

void NetworkingObjectBase::ParseDhcpEnabled(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    std::string result (g_unknown);
    if (!this->m_document.HasParseError())
    {
        if (this->m_document.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < this->m_document.Size(); ++i)
            {
                if ((this->m_document[i].HasMember(g_ifname)) &&
                    (this->m_document[i][g_ifname].IsString()))
                {
                    if ((std::strcmp(this->m_document[i][g_ifname].GetString(), interfaceName.c_str()) == 0) &&
                        (this->m_document[i].HasMember(g_addrInfo)) &&
                        (this->m_document[i][g_addrInfo].IsArray()))
                    {
                        for (rapidjson::SizeType j = 0; j < this->m_document[i][g_addrInfo].Size(); ++j)
                        {
                            if ((this->m_document[i][g_addrInfo][j].HasMember(g_dynamic)) &&
                                (this->m_document[i][g_addrInfo][j][g_dynamic].IsBool()))
                            {
                                if (this->m_document[i][g_addrInfo][j][g_dynamic].GetBool())
                                {
                                    result = g_true;
                                }
                                else
                                {
                                    result = g_false;
                                }
                            }
                            break;
                        }
                        break;
                    }
                }
            }
        }
    }

    interfaceSettings.push_back(result);
}

void NetworkingObjectBase::ParseEnabled(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    std::string result (g_unknown);
    if (!this->m_document.HasParseError())
    {
        if (this->m_document.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < this->m_document.Size(); ++i)
            {
                if ((this->m_document[i].HasMember(g_ifname)) &&
                    (this->m_document[i][g_ifname].IsString()))
                {
                    if ((std::strcmp(this->m_document[i][g_ifname].GetString(), interfaceName.c_str()) == 0) &&
                        (this->m_document[i].HasMember(g_operstate)) &&
                        (this->m_document[i][g_operstate].IsString()))
                    {
                        if (std::strcmp(this->m_document[i][g_operstate].GetString(), g_up) == 0)
                        {
                            result = g_true;
                        }
                        else if (std::strcmp(this->m_document[i][g_operstate].GetString(), g_down) == 0)
                        {
                            result = g_false;
                        }
                        break;
                    }
                }
            }
        }
    }

    interfaceSettings.push_back(result);
}

void NetworkingObjectBase::ParseConnected(const std::string& interfaceName, std::vector<std::string>& interfaceSettings)
{
    std::string result (g_unknown);
    if (!this->m_document.HasParseError())
    {
        if (this->m_document.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < this->m_document.Size(); ++i)
            {
                if ((this->m_document[i].HasMember(g_ifname)) && (this->m_document[i][g_ifname].IsString()))
                {
                    if ((std::strcmp(this->m_document[i][g_ifname].GetString(), interfaceName.c_str()) == 0) &&
                        (this->m_document[i].HasMember(g_flags)) &&
                        (this->m_document[i][g_flags].IsArray()))
                    {
                        for (rapidjson::SizeType j = 0; j < this->m_document[i][g_flags].Size(); ++j)
                        {
                            std::string data(this->m_document[i][g_flags][j].GetString());
                            if (data == g_lowerUp)
                            {
                                result = g_true;
                                break;
                            }
                        }
                        if (result != g_true)
                        {
                            result = g_false;
                        }
                        break;
                    }
                }
            }
        }
    }

    interfaceSettings.push_back(result);
}

void NetworkingObjectBase::ParseCommandOutput(const std::string& interfaceName, NetworkingSettingType networkingSettingType, std::string& interfaceSettingsString)
{
    std::vector<std::string> interfaceSettings;
    switch (networkingSettingType)
    {
        case NetworkingSettingType::InterfaceTypes:
            if (this->m_interfaceTypesMap.find(interfaceName) != this->m_interfaceTypesMap.end())
            {
                interfaceSettings.push_back(this->m_interfaceTypesMap[interfaceName]);
            }
            break;

        case NetworkingSettingType::MacAddresses:
            ParseMacAddresses(interfaceName, interfaceSettings);
            break;

        case NetworkingSettingType::IpAddresses:
            ParseIpAddresses(interfaceName, interfaceSettings);
            break;

        case NetworkingSettingType::SubnetMasks:
            ParseSubnetMasks(interfaceName, interfaceSettings);
            break;

        case NetworkingSettingType::DefaultGateways:
            if (this->m_defaultGatewaysMap.find(interfaceName) != this->m_defaultGatewaysMap.end())
            {
                for (size_t i = 0; i < this->m_defaultGatewaysMap[interfaceName].size(); ++i)
                {
                    interfaceSettings.push_back((this->m_defaultGatewaysMap[interfaceName]).at(i));
                }
            }
            break;

        case NetworkingSettingType::DnsServers:
            if (this->m_dnsServersMap.find(interfaceName) != this->m_dnsServersMap.end())
            {
                for (size_t i = 0; i < this->m_dnsServersMap[interfaceName].size(); ++i)
                {
                    interfaceSettings.push_back((this->m_dnsServersMap[interfaceName]).at(i));
                }
            }
            break;

        case NetworkingSettingType::DhcpEnabled:
            ParseDhcpEnabled(interfaceName, interfaceSettings);
            break;

        case NetworkingSettingType::Enabled:
            ParseEnabled(interfaceName, interfaceSettings);
            break;

        case NetworkingSettingType::Connected:
            ParseConnected(interfaceName, interfaceSettings);
            break;
    }

    GenerateInterfaceSettingsString(interfaceSettings, interfaceSettingsString);
}

void NetworkingObjectBase::GetInterfaceNames(std::vector<std::string>& interfaceNames)
{
    std::string commandOutput = RunCommand(g_interfaceNamesCommand);
    std::vector<std::string> interfaceNamesAccumulator;
    if (!commandOutput.empty())
    {
        std::stringstream commandOutputStream(commandOutput);
        std::string token = "";
        while (std::getline(commandOutputStream, token))
        {
            interfaceNamesAccumulator.push_back(token);
        }
    }

    interfaceNames = interfaceNamesAccumulator;
}

const char* NetworkingObjectBase::NetworkingSettingTypeToString(NetworkingSettingType networkingSettingType)
{
    switch (networkingSettingType)
    {
        case NetworkingSettingType::InterfaceTypes:
            return g_interfaceTypesString;
        case NetworkingSettingType::MacAddresses:
            return g_macAddressesString;
        case NetworkingSettingType::IpAddresses:
            return g_ipAddressesString;
        case NetworkingSettingType::SubnetMasks:
            return g_subnetMasksString;
        case NetworkingSettingType::DefaultGateways:
            return g_defaultGatewaysString;
        case NetworkingSettingType::DnsServers:
            return g_dnsServersString;
        case NetworkingSettingType::DhcpEnabled:
            return g_dhcpEnabledString;
        case NetworkingSettingType::Enabled:
            return g_enabledString;
        case NetworkingSettingType::Connected:
            return g_connectedString;
        default:
            return nullptr;
    }

    return nullptr;
}

void NetworkingObjectBase::GenerateNetworkingSettingsString(std::vector<std::tuple<std::string, std::string>> networkingSettings, std::string& networkingSettingsString)
{
    std::string networkingSettingsAccumulator;
    if (!networkingSettings.empty())
    {
        sort(networkingSettings.begin(), networkingSettings.end());
        int networkingSettingsSize = networkingSettings.size();
        for (int i = 0; i < networkingSettingsSize; ++i)
        {
            if (!std::get<1>(networkingSettings[i]).empty())
            {
                if (!networkingSettingsAccumulator.empty())
                {
                    networkingSettingsAccumulator += ";";
                }
                networkingSettingsAccumulator += std::get<0>(networkingSettings[i]) + "=" + std::get<1>(networkingSettings[i]);
            }
        }
    }

    networkingSettingsString = networkingSettingsAccumulator;
}

void NetworkingObjectBase::GetData(NetworkingSettingType networkingSettingType, std::string& networkingSettingsString)
{
    std::vector<std::tuple<std::string, std::string>> networkingSettings;
    for (size_t i = 0; i < this->m_interfaceNames.size(); ++i)
    {
        std::string interfaceSettingsString;
        ParseCommandOutput(this->m_interfaceNames[i], networkingSettingType, interfaceSettingsString);

        networkingSettings.push_back(make_tuple(this->m_interfaceNames[i], interfaceSettingsString));
    }

    GenerateNetworkingSettingsString(networkingSettings, networkingSettingsString);
}

bool NetworkingObjectBase::IsInterfaceName(std::string name)
{
    for (size_t i = 0; i < this->m_interfaceNames.size(); ++i)
    {
        if (name == this->m_interfaceNames[i])
        {
            return true;
        }
    }

    return false;
}

void NetworkingObjectBase::GenerateInterfaceTypesMap()
{
    bool usingNetworkManager = false;
    std::string interfaceTypesData;
    std::string interfaceData;
    std::string interfaceName;
    std::string interfaceType;
    std::map<std::string, std::string>::iterator interfaceTypesMapIterator;

    interfaceTypesData = RunCommand(g_interfaceTypesQueryNmcli);
    std::regex interfaceNamePrefixPatternNmcli("GENERAL.DEVICE:\\s+");
    std::smatch interfaceNamePrefixMatchNmcli;
    while (std::regex_search(interfaceTypesData, interfaceNamePrefixMatchNmcli, interfaceNamePrefixPatternNmcli))
    {
        usingNetworkManager = true;
        std::string interfaceNamePrefix = interfaceNamePrefixMatchNmcli.str(0);
        interfaceData = interfaceNamePrefixMatchNmcli.suffix().str();
        std::string nextData = interfaceData;

        size_t interfaceNameSuffixFront = interfaceData.find("\n", 0);

        if (interfaceNameSuffixFront != std::string::npos)
        {
            interfaceName = interfaceData.substr(0, interfaceNameSuffixFront);
        }

        interfaceData = std::regex_search(interfaceData, interfaceNamePrefixMatchNmcli, interfaceNamePrefixPatternNmcli) ? interfaceData.substr(0, interfaceNamePrefixMatchNmcli.position(0)) : interfaceData;

        if (IsInterfaceName(interfaceName))
        {
            std::regex interfaceTypePrefixPattern("GENERAL.TYPE:\\s+");
            std::smatch interfaceTypePrefixMatch;

            if (std::regex_search(interfaceData, interfaceTypePrefixMatch, interfaceTypePrefixPattern))
            {
                interfaceType = interfaceTypePrefixMatch.suffix().str();

                size_t interfaceTypeSuffixFront = interfaceType.find("\n", 0);
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
        interfaceTypesData = nextData;
    }

    if (!usingNetworkManager)
    {
        interfaceTypesData = RunCommand(g_interfaceTypesQueryNetworkctl);
        std::stringstream interfaceTypesDataStream(interfaceTypesData);
        while(std::getline(interfaceTypesDataStream, interfaceData))
        {
            std::regex interfaceTypesDataPatternNetworkctl("^\\s*[0-9]+\\s+.*$");
            if (std::regex_match(interfaceData.begin(), interfaceData.end(), interfaceTypesDataPatternNetworkctl))
            {
                std::stringstream interfaceDataStream(interfaceData);
                std::string data;
                while (std::getline(interfaceDataStream, data, ' '))
                {
                    if (IsInterfaceName(data))
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
    }
}

void NetworkingObjectBase::GenerateIpData()
{
    std::string ipJson = RunCommand(g_ipAddressQuery);
    if (!ipJson.empty())
    {
        this->m_document.Parse<0>(ipJson.c_str());
        if (this->m_document.HasParseError())
        {
            OsConfigLogError(NetworkingLog::Get(), "Parse operation failed with error: %s (offset: %u)\n",
                GetParseError_En(this->m_document.GetParseError()),
                (unsigned)this->m_document.GetErrorOffset());
        }
    }
}

void NetworkingObjectBase::GenerateDefaultGatewaysMap()
{
    this->m_defaultGatewaysMap.clear();

    std::string defaultGatewaysData = RunCommand(g_ipRouteQuery);
    std::regex interfaceNamePrefixPattern("default\\s+via\\s+.*\\s+dev\\s+");
    std::smatch interfaceNamePrefixMatch;

    while (std::regex_search(defaultGatewaysData, interfaceNamePrefixMatch, interfaceNamePrefixPattern))
    {
        std::string interfaceNamePrefix = interfaceNamePrefixMatch.str(0);
        std::string interfaceNameData = interfaceNamePrefixMatch.suffix().str();
        size_t interfaceNameSuffixFront = interfaceNameData.find(" ", 0);

        std::string interfaceName;
        if (interfaceNameSuffixFront != std::string::npos)
        {
            interfaceName = interfaceNameData.substr(0, interfaceNameSuffixFront);
        }

        if (IsInterfaceName(interfaceName))
        {
            std::string defaultGateway;
            std::regex defaultGatewayPrefixPattern("default\\s+via\\s+");
            std::smatch defaultGatewayPrefixMatch;

            if (std::regex_search(defaultGatewaysData, defaultGatewayPrefixMatch, defaultGatewayPrefixPattern))
            {
                defaultGateway = defaultGatewayPrefixMatch.suffix().str();

                size_t defaultGatewaySuffixFront = defaultGateway.find(" ", 0);
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

    std::string dnsServersData = RunCommand(g_systemdResolveQuery);
    std::regex interfaceNamePrefixPattern("Link\\s+[0-9]+\\s+\\(");
    std::smatch interfaceNamePrefixMatch;

    while (std::regex_search(dnsServersData, interfaceNamePrefixMatch, interfaceNamePrefixPattern))
    {
        std::string interfaceNamePrefix = interfaceNamePrefixMatch.str(0);
        std::string interfaceNameData = interfaceNamePrefixMatch.suffix().str();
        size_t interfaceNameSuffixFront = interfaceNameData.find(")", 0);

        std::string interfaceName;
        if (interfaceNameSuffixFront != std::string::npos)
        {
            interfaceName = interfaceNameData.substr(0, interfaceNameSuffixFront);
        }

        dnsServersData = interfaceNameData;
        std::string interfaceData = std::regex_search(dnsServersData, interfaceNamePrefixMatch, interfaceNamePrefixPattern) ? dnsServersData.substr(0, interfaceNamePrefixMatch.position(0)) : dnsServersData;

        if (IsInterfaceName(interfaceName))
        {
            std::string dnsServer;
            std::regex dnsServerPrefixPattern("DNS\\s+Servers:\\s+");
            std::smatch dnsServerPrefixMatch;

            if (std::regex_search(interfaceData, dnsServerPrefixMatch, dnsServerPrefixPattern))
            {
                std::stringstream dnsServerStream(dnsServerPrefixMatch.suffix().str());
                std::regex ipv4Pattern("(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])");
                std::regex ipv6Pattern("(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}"
                    ":[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}"
                    "|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}"
                    ":((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|[fF][eE]80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::([fF][eE]{4}(:0{1,4}){0,1}:){0,1}"
                    "((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}"
                    ":((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))");

                while(std::getline(dnsServerStream, dnsServer) && ((std::regex_match(dnsServer, ipv4Pattern)) || (std::regex_match(dnsServer, ipv6Pattern))))
                {
                    dnsServer.erase(remove(dnsServer.begin(), dnsServer.end(), ' '), dnsServer.end());

                    std::map<std::string, std::vector<std::string>>::iterator dnsServersMapIterator = this->m_dnsServersMap.find(interfaceName);
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
            }
        }
    }
}

void NetworkingObjectBase::GenerateNetworkingData()
{
    GetInterfaceNames(this->m_interfaceNames);
    if (this->m_interfaceNames.size() > 0)
    {
        GenerateInterfaceTypesMap();
        GenerateIpData();
        GenerateDefaultGatewaysMap();
        GenerateDnsServersMap();
        GetData(NetworkingSettingType::InterfaceTypes, this->m_networkingData.interfaceTypes);
        GetData(NetworkingSettingType::MacAddresses, this->m_networkingData.macAddresses);
        GetData(NetworkingSettingType::IpAddresses, this->m_networkingData.ipAddresses);
        GetData(NetworkingSettingType::SubnetMasks, this->m_networkingData.subnetMasks);
        GetData(NetworkingSettingType::DefaultGateways, this->m_networkingData.defaultGateways);
        GetData(NetworkingSettingType::DnsServers, this->m_networkingData.dnsServers);
        GetData(NetworkingSettingType::DhcpEnabled, this->m_networkingData.dhcpEnabled);
        GetData(NetworkingSettingType::Enabled, this->m_networkingData.enabled);
        GetData(NetworkingSettingType::Connected, this->m_networkingData.connected);
    }
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

int NetworkingObjectBase::TruncateValueStrings(std::vector<std::pair<std::string, std::string>> &fieldValueVector)
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

    GenerateNetworkingData();

    std::vector<std::pair<std::string, std::string>> fieldValueVector;

    fieldValueVector.push_back(make_pair(std::string(g_interfaceTypesString), m_networkingData.interfaceTypes));
    fieldValueVector.push_back(make_pair(std::string(g_macAddressesString), m_networkingData.macAddresses));
    fieldValueVector.push_back(make_pair(std::string(g_ipAddressesString), m_networkingData.ipAddresses));
    fieldValueVector.push_back(make_pair(std::string(g_subnetMasksString), m_networkingData.subnetMasks));
    fieldValueVector.push_back(make_pair(std::string(g_defaultGatewaysString), m_networkingData.defaultGateways));
    fieldValueVector.push_back(make_pair(std::string(g_dnsServersString), m_networkingData.dnsServers));
    fieldValueVector.push_back(make_pair(std::string(g_dhcpEnabledString), m_networkingData.dhcpEnabled));
    fieldValueVector.push_back(make_pair(std::string(g_enabledString), m_networkingData.enabled));
    fieldValueVector.push_back(make_pair(std::string(g_connectedString), m_networkingData.connected));

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
        status = ENOMEM;
        if ((IsFullLoggingEnabled()) && (nullptr != payloadSizeBytes))
        {
            OsConfigLogError(NetworkingLog::Get(), "Networking::Get insufficient buffer space available to allocate %d bytes", *payloadSizeBytes);
        }
    }
    else
    {
        std::fill(*payload, *payload + *payloadSizeBytes, 0);
        std::memcpy(*payload, ((*payloadSizeBytes) == (int)g_templateWithDotsSize) ? g_templateWithDots : networkingJsonString.c_str(), *payloadSizeBytes);
    }

    return status;
}