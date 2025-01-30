// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Runtime.Serialization;

using Newtonsoft.Json;
using Newtonsoft.Json.Converters;
using Newtonsoft.Json.Serialization;

namespace E2eTesting
{
    [TestFixture, Category("Firewall")]
    public class FirewallTests : E2ETest
    {
        protected static string _componentName = "Firewall";
        protected static string _desiredRules = "desiredRules";
        protected static string _desiredDefaultPolicies = "desiredDefaultPolicies";
        protected static string _reportedDefaultPolicies = "defaultPolicies";

        private static Regex _fingerprintPattern = new Regex(@"[0-9a-z]{64}");

        [JsonConverter(typeof(StringEnumConverter), typeof(CamelCaseNamingStrategy))]
        public enum State
        {
            Unknown,
            Enabled,
            Disabled
        }

        [JsonConverter(typeof(StringEnumConverter), typeof(CamelCaseNamingStrategy))]
        public enum ConfigurationStatus
        {
            Unknown,
            Success,
            Failure
        }

        [JsonConverter(typeof(StringEnumConverter), typeof(CamelCaseNamingStrategy))]
        public enum DesiredState
        {
            Present,
            Absent
        }

        [JsonConverter(typeof(StringEnumConverter), typeof(CamelCaseNamingStrategy))]
        public enum Action
        {
            Accept,
            Drop,
            Reject
        }

        [JsonConverter(typeof(StringEnumConverter), typeof(CamelCaseNamingStrategy))]
        public enum Direction
        {
            In,
            Out
        }

        [JsonConverter(typeof(StringEnumConverter), typeof(CamelCaseNamingStrategy))]
        public enum Protocol
        {
            Any,
            Tcp,
            Udp,
            Icmp
        }

        public class Rule
        {
            public DesiredState DesiredState { get; set; }
            public Action Action { get; set; }
            public Direction Direction { get; set; }
            public Protocol Protocol { get; set; }
            public string SourceAddress { get; set; }
            public string DestinationAddress { get; set; }
            public int? SourcePort { get; set; }
            public int? DestinationPort { get; set; }
        }

        [JsonConverter(typeof(StringEnumConverter), typeof(CamelCaseNamingStrategy))]
        public enum PolicyAction
        {
            Accept,
            Drop
        }

        public class Policy
        {
            public Action Action { get; set; }
            public Direction Direction { get; set; }
        }

        public class Firewall
        {
            public State State { get; set; }
            public string Fingerprint { get; set; }
            public ConfigurationStatus ConfigurationStatus { get; set; }
            public string ConfigurationStatusDetail { get; set; }
            public Policy[] DefaultPolicies { get; set; }
        }


        [Test]
        public async Task FirewallTest_Regex()
        {
            Firewall reported = await GetReported<Firewall>(_componentName, (Firewall firewall) => (firewall.State != State.Unknown) && (firewall.Fingerprint != null));

            Assert.Multiple(() =>
            {
                Assert.AreNotEqual(State.Unknown, reported.State);
                RegexAssert.IsMatch(_fingerprintPattern, reported.Fingerprint);
            });
        }

        [Test]
        public async Task FirewallTest_SetDesiredRules()
        {
            var desired = new Rule[]
            {
                new Rule
                {
                    DesiredState = DesiredState.Present,
                    Action = Action.Accept,
                    Direction = Direction.In,
                    Protocol = Protocol.Tcp,
                    SourceAddress = "bing.com",
                    SourcePort = 1234
                }
            };

            Firewall reported = await GetReported<Firewall>(_componentName, (Firewall firewall) => (firewall.State != State.Unknown) && (firewall.Fingerprint != null));
            var initialFingerprint = reported.Fingerprint;

            await SetDesired<Rule[]>(_componentName, _desiredRules, desired);

            reported = await GetReported<Firewall>(_componentName, (Firewall firewall) => (firewall.Fingerprint != initialFingerprint));

            // Remove the rule added by the test
            desired = new Rule[]
            {
                new Rule
                {
                    DesiredState = DesiredState.Absent,
                    Action = Action.Accept,
                    Direction = Direction.In,
                    Protocol = Protocol.Tcp,
                    SourceAddress = "bing.com",
                    SourcePort = 1234
                }
            };
            await SetDesired<Rule[]>(_componentName, _desiredRules, desired);
            await GetReported<Firewall>(_componentName, (Firewall firewall) => (firewall.Fingerprint != initialFingerprint));

            // Reset the desired state to empty
            await SetDesired<Rule[]>(_componentName, _desiredRules, new Rule[0]);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(State.Enabled, reported.State);
                Assert.AreNotEqual(initialFingerprint, reported.Fingerprint);
                Assert.AreEqual(string.Empty, reported.ConfigurationStatusDetail);
            });
        }

        [Test]
        public async Task FirewallTest_SetGetDefaultPolicies()
        {
            var desired = new Policy[]
            {
                new Policy
                {
                    Action = Action.Accept,
                    Direction = Direction.In,
                },
                new Policy
                {
                    Action = Action.Accept,
                    Direction = Direction.Out,
                }
            };

            await SetDesired<Policy[]>(_componentName, _desiredDefaultPolicies, desired);

            var reported = await GetReported<Firewall>(_componentName, (Firewall firewall) => ((firewall.DefaultPolicies != null) && (firewall.DefaultPolicies.Length == 2)));
            var reportedPolicies = reported.DefaultPolicies;

            Assert.Multiple(() =>
            {
                Assert.AreEqual(desired[0].Action, reportedPolicies[0].Action);
                Assert.AreEqual(desired[0].Direction, reportedPolicies[0].Direction);
                Assert.AreEqual(desired[1].Action, reportedPolicies[1].Action);
                Assert.AreEqual(desired[1].Direction, reportedPolicies[1].Direction);
                Assert.AreEqual(ConfigurationStatus.Success, reported.ConfigurationStatus);
                Assert.AreEqual(string.Empty, reported.ConfigurationStatusDetail);
            });

            await SetDesired<Policy[]>(_componentName, _desiredDefaultPolicies, new Policy[0]);
        }
    }
}
