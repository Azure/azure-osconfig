// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using Microsoft.Azure.Devices;
using System.Text.Json;
using System;
using System.Threading.Tasks;
using System.Text.RegularExpressions;
using System.Collections.Generic;

namespace E2eTesting
{
    public class NetworkingTests : E2eTest
    {
        const string ComponentName = "Networking";
        const string PropertyName = "NetworkConfiguration";
        public partial class Networking
        {
            public string InterfaceTypes { get; set; }
            public string MacAddresses { get; set; }
            public string IpAddresses { get; set; }
            public string SubnetMasks { get; set; }
            public string DefaultGateways { get; set; }
            public string DnsServers { get; set; }
            public string DhcpEnabled { get; set; }
            public string Enabled { get; set; }
            public string Connected { get; set; }
        }

        public void RegexTest_SingletonInterfaceProperties(string testString, string patternString)
        {
            string interfaceDelimiter = ";";
            Regex regexPattern = new Regex(@patternString);
            string[] result = Regex.Split(testString, interfaceDelimiter);
            foreach (string piece in result)
            {
                Assert.True(regexPattern.IsMatch(piece));
            }
        }

        public void RegexTest_Properties(string testString, string delimiterPatternString, string smallPatternString)
        {
            string interfaceDelimiter = ";";
            string propertyDelimiter = ",";
            Regex regexPattern = new Regex(@delimiterPatternString);
            Regex smallRegexPattern = new Regex(@smallPatternString);
            string[] result = Regex.Split(testString, interfaceDelimiter);
            foreach (string piece in result)
            {
                Match match = regexPattern.Match(piece);
                Assert.True(match.Success);    
                string[] tokens = Regex.Split(match.Groups[1].Value, propertyDelimiter);
                foreach(string token in tokens)
                {
                    Assert.True(smallRegexPattern.IsMatch(token));
                }
            }
        }

        [Test]
        public void NetworkingTest_Get()
        {
            if ((GetTwin().ConnectionState == DeviceConnectionState.Disconnected) || (GetTwin().Status == DeviceStatus.Disabled))
            {
                Assert.Fail("Module is disconnected or is disabled");
            }

            var deserializedReportedObject = JsonSerializer.Deserialize<Networking>(GetTwin().Properties.Reported[ComponentName][PropertyName].ToString());

            // Test InterfaceTypes
            string patternString = @"([A-Za-z0-9]+)\=([A-Za-z0-9]+)";
            RegexTest_SingletonInterfaceProperties(deserializedReportedObject.InterfaceTypes, patternString);

            string delimiterPatternString = @"[A-Za-z0-9]+\=(.*)";
            // Test MacAddresses
            string macAddressesPatternString = @"^(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}|(?:[0-9a-fA-F]{2}-){5}[0-9a-fA-F]{2}|(?:[0-9a-fA-F]{2}){5}[0-9a-fA-F]{2}$";
            RegexTest_Properties(deserializedReportedObject.MacAddresses, delimiterPatternString, macAddressesPatternString);
            // Test IpAddresses
            string ipv4Ipv6PatternString = @"((^\s*((([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]))\s*$)|(^\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?\s*$))";
            RegexTest_Properties(deserializedReportedObject.IpAddresses, delimiterPatternString, ipv4Ipv6PatternString);

            // Test SubnetMasks
            string subnetMaskPatternString = @"[\d]+";
            RegexTest_Properties(deserializedReportedObject.SubnetMasks, delimiterPatternString, subnetMaskPatternString);

            // Test DefaultGateways
            RegexTest_Properties(deserializedReportedObject.DefaultGateways, delimiterPatternString, ipv4Ipv6PatternString); 

            // Test DnsServers
            if (!String.IsNullOrEmpty(deserializedReportedObject.DnsServers))
            {
                RegexTest_Properties(deserializedReportedObject.DnsServers, delimiterPatternString, ipv4Ipv6PatternString);
            }
            string statePatternString = @"^[A-Za-z0-9]+\=true|false|unknown$";
            // Test DhcpEnabled
            RegexTest_SingletonInterfaceProperties(deserializedReportedObject.DhcpEnabled, statePatternString);
            // Test Enabled
            RegexTest_SingletonInterfaceProperties(deserializedReportedObject.Enabled, statePatternString);
            // Test Connected
            RegexTest_SingletonInterfaceProperties(deserializedReportedObject.Connected, statePatternString);
        }
    }
}