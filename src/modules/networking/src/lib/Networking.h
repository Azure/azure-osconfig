// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <Logging.h>
#include <Mmi.h>

#define NETWORKING_LOGFILE "/var/log/osconfig_networking.log"
#define NETWORKING_ROLLEDLOGFILE "/var/log/osconfig_networking.bak"
#define NETWORKING "Networking"
#define NETWORK_CONFIGURATION "NetworkConfiguration"

class NetworkingLog
{
    public:
        static OSCONFIG_LOG_HANDLE Get()
        {
            return m_logNetworking;
        }

        static void OpenLog()
        {
            m_logNetworking = ::OpenLog(NETWORKING_LOGFILE, NETWORKING_ROLLEDLOGFILE);
        }

        static void CloseLog()
        {
            ::CloseLog(&m_logNetworking);
        }

    private:
        static OSCONFIG_LOG_HANDLE m_logNetworking;
};

struct NetworkingData
{
    std::string interfaceTypes;
    std::string macAddresses;
    std::string ipAddresses;
    std::string subnetMasks;
    std::string defaultGateways;
    std::string dnsServers;
    std::string dhcpEnabled;
    std::string enabled;
    std::string connected;
};

enum class NetworkingSettingType
{
    InterfaceTypes,
    MacAddresses,
    IpAddresses,
    SubnetMasks,
    DefaultGateways,
    DnsServers,
    DhcpEnabled,
    Enabled,
    Connected
};

class NetworkingObjectBase
{
    public:
        virtual ~NetworkingObjectBase() {};
        virtual std::string RunCommand(const char* command) = 0;

        int Get(
            MMI_HANDLE clientSession,
            const char* componentName,
            const char* objectName,
            MMI_JSON_STRING* payload,
            int* payloadSizeBytes);

        int TruncateValueStrings(std::vector<std::pair<std::string, std::string>> &fieldValueVector);

        unsigned int m_maxPayloadSizeBytes;

    private:
        void GenerateInterfaceSettingsString(std::vector<std::string> interfaceSettings, std::string& interfaceSettingsString);
        void ParseMacAddresses(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
        void ParseIpAddresses(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
        void ParseSubnetMasks(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
        void ParseDhcpEnabled(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
        void ParseEnabled(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
        void ParseConnected(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
        void ParseCommandOutput(const std::string& interfaceName, NetworkingSettingType networkingSettingType, std::string& interfaceSettingsString);
        void GetInterfaceNames(std::vector<std::string>& interfaceNames);
        const char* NetworkingSettingTypeToString(NetworkingSettingType networkingSettingType);
        void GenerateNetworkingSettingsString(std::vector<std::tuple<std::string, std::string>> networkingSettings, std::string& networkingSettingsString);
        void GetData(NetworkingSettingType networkingSettingType, std::string& networkingSettingsString);
        bool IsInterfaceName(std::string name);
        void GenerateInterfaceTypesMap();
        void GenerateIpData();
        void GenerateDefaultGatewaysMap();
        void GenerateDnsServersMap();
        void GenerateNetworkingData();
        virtual int WriteJsonElement(rapidjson::Writer<rapidjson::StringBuffer>* writer, const char* key, const char* value) = 0;

        std::vector<std::string> m_interfaceNames;
        std::map<std::string, std::string> m_interfaceTypesMap;
        std::map<std::string, std::vector<std::string>> m_defaultGatewaysMap;
        std::map<std::string, std::vector<std::string>> m_dnsServersMap;
        
        NetworkingData m_networkingData;    
        rapidjson::Document m_document;
};

class NetworkingObject : public NetworkingObjectBase
{
public:
    std::string RunCommand(const char* command) override;
    int WriteJsonElement(rapidjson::Writer<rapidjson::StringBuffer>* writer, const char* key, const char* value) override;
    NetworkingObject(unsigned int maxPayloadSizeBytes);
    ~NetworkingObject();
};