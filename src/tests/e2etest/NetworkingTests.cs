// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace E2eTesting
{
    [TestFixture, Category("Networking")]
    public class NetworkingTests : E2ETest
    {
        private static readonly string _componentName = "Networking";
        private static readonly string _reportedPropertyName = "networkConfiguration";

        private static readonly string _interfaceDelimiter =";";
        private static readonly string _propertyDelimiter =",";
        private static readonly Regex _interfacePattern = new Regex(@"([A-Za-z0-9]+)\=([A-Za-z0-9]+)");
        private static readonly Regex _macAddressPattern = new Regex(@"^(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}|(?:[0-9a-fA-F]{2}-){5}[0-9a-fA-F]{2}|(?:[0-9a-fA-F]{2}){5}[0-9a-fA-F]{2}$");
        private static readonly Regex _ipAddressPattern = new Regex(@"((^\s*((([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]))\s*$)|(^\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?\s*$))");
        private static readonly Regex _subnetMaskPattern = new Regex(@"[\d]+");
        private static readonly Regex _statePattern = new Regex(@"^[A-Za-z0-9]+\=true|false|unknown$");
        private static readonly Regex _delimiterPattern = new Regex(@"[A-Za-z0-9]+\=(.*)");

        public class NetworkConfiguration
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

        public void NetworkPropertyPatternMatch(Regex pattern, string value)
        {
            string[] substrings = Regex.Split(value, _interfaceDelimiter);
            foreach (string substring in substrings)
            {
                if (!pattern.IsMatch(substring))
                {
                    Assert.Fail("The substring {0} does not match the pattern {1}", substring, pattern);
                }
            }
        }

        public void NetworkPropertyPatternMatch(Regex pattern, Regex delimiterPattern, string value)
        {
            string[] substrings = Regex.Split(value, _interfaceDelimiter);
            foreach (string substring in substrings)
            {
                Match match = delimiterPattern.Match(substring);
                if (match.Success)
                {
                    string[] tokens = Regex.Split(match.Groups[1].Value, _propertyDelimiter);
                    foreach(string token in tokens)
                    {
                        if (!pattern.IsMatch(token))
                        {
                            Assert.Fail("The substring {0} does not match the pattern {1}", token, pattern);
                        }
                    }
                }
                else
                {
                    Assert.Fail("No match found for pattern in substring\nPattern: {0}\nSubstring: {1}", pattern, substring);
                }
            }
        }

        [Test]
        public async Task NetworkingTest_Get()
        {
            NetworkConfiguration reported = await GetReported<NetworkConfiguration>(_componentName, _reportedPropertyName, (NetworkConfiguration network) => {
                return network.InterfaceTypes != null &&
                       network.MacAddresses != null &&
                       network.IpAddresses != null &&
                       network.SubnetMasks != null &&
                       network.DefaultGateways != null &&
                       network.DhcpEnabled != null &&
                       network.Enabled != null &&
                       network.Connected != null;
            });

            Assert.Multiple(() =>
            {
                // Interface
                NetworkPropertyPatternMatch(_interfacePattern, reported.InterfaceTypes);
                // MacAddress
                NetworkPropertyPatternMatch(_macAddressPattern, _delimiterPattern, reported.MacAddresses);
                // IpAddress
                NetworkPropertyPatternMatch(_ipAddressPattern, _delimiterPattern, reported.IpAddresses);
                // SubnetMask
                NetworkPropertyPatternMatch(_subnetMaskPattern, _delimiterPattern, reported.SubnetMasks);
                // DefaultGateway
                NetworkPropertyPatternMatch(_ipAddressPattern, _delimiterPattern, reported.DefaultGateways);
                // DnsServer
                if (!string.IsNullOrEmpty(reported.DnsServers))
                {
                    NetworkPropertyPatternMatch(_ipAddressPattern, _delimiterPattern, reported.DnsServers);
                }
                // dhcpEnabled
                NetworkPropertyPatternMatch(_statePattern, reported.DhcpEnabled);
                // enabled
                NetworkPropertyPatternMatch(_statePattern, reported.Enabled);
                // connected
                NetworkPropertyPatternMatch(_statePattern, reported.Connected);
                ValidateLocalReported(reported, _componentName, _reportedPropertyName);
            });
        }
    }
}