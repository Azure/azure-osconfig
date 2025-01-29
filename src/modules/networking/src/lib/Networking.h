// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include <vector>
#include <rapidjson/writer.h>
#include <Logging.h>
#include <Mmi.h>

#define NETWORKING_LOGFILE "/var/log/osconfig_networking.log"
#define NETWORKING_ROLLEDLOGFILE "/var/log/osconfig_networking.bak"
#define NETWORKING "Networking"
#define NETWORK_CONFIGURATION "networkConfiguration"

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

struct NetworkingSettings
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

class NetworkingObjectBase
{
public:
    enum class NetworkManagementService
    {
        Unknown,
        NetworkManager,
        SystemdNetworkd
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

    virtual ~NetworkingObjectBase() {};
    virtual std::string RunCommand(const char* command) = 0;

    int Get(
        const char* componentName,
        const char* objectName,
        MMI_JSON_STRING* payload,
        int* payloadSizeBytes);

    int TruncateValueStrings(std::vector<std::pair<std::string, std::string>>& fieldValueVector);

    unsigned int m_maxPayloadSizeBytes;
    NetworkManagementService m_networkManagementService;

private:
    void ParseInterfaceDataForSettings(bool labeled, const char* flag, std::stringstream& data, std::vector<std::string>& settings);
    void GetInterfaceTypes(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
    void GetMacAddresses(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
    void GetIpAddresses(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
    void GetSubnetMasks(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
    void GetDefaultGateways(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
    void GetDnsServers(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
    void GetDhcpEnabled(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
    void GetEnabled(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
    void GetConnected(const std::string& interfaceName, std::vector<std::string>& interfaceSettings);
    void GenerateInterfaceSettingsString(const std::string& interfaceName, NetworkingSettingType settingType, std::string& interfaceSettingsString);
    void UpdateSettingsString(NetworkingSettingType settingType, std::string& settingsString);
    void GenerateInterfaceTypesMap();
    void GenerateIpSettingsMap();
    void GenerateDefaultGatewaysMap();
    void GenerateDnsServersMap();
    void GetInterfaceTypesFromSystemdNetworkd();
    void GetInterfaceTypesFromNetworkManager();
    void GetGlobalDnsServers(std::string dnsServersData, std::vector<std::string>& globalDnsServers);
    void RefreshInterfaceNames(std::vector<std::string>& interfaceNames);
    void RefreshInterfaceData();
    void RefreshSettingsStrings();
    bool IsKnownInterfaceName(std::string str);
    virtual int WriteJsonElement(rapidjson::Writer<rapidjson::StringBuffer>* writer, const char* key, const char* value) = 0;

    NetworkingSettings m_settings;

    std::vector<std::string> m_interfaceNames;
    std::map<std::string, std::string> m_interfaceTypesMap;
    std::map<std::string, std::string> m_ipSettingsMap;
    std::map<std::string, std::vector<std::string>> m_defaultGatewaysMap;
    std::map<std::string, std::vector<std::string>> m_dnsServersMap;
};

class NetworkingObject : public NetworkingObjectBase
{
public:
    std::string RunCommand(const char* command) override;
    int WriteJsonElement(rapidjson::Writer<rapidjson::StringBuffer>* writer, const char* key, const char* value) override;
    NetworkingObject(unsigned int maxPayloadSizeBytes);
    ~NetworkingObject();
};
