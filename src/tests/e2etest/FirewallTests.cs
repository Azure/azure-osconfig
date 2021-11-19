// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using Microsoft.Azure.Devices.Shared;
using System.Text.Json;
using System;
using System.Threading.Tasks;
using System.Text.RegularExpressions;
namespace e2etesting
{
    public class FirewallTests : E2eTest
    {
        string ComponentName = "Firewall";
        public enum FirewallStateCode
        {
            Unknown = 0,
            Enabled,
            Disabled,
        }
        public partial class Firewall
        {
            public FirewallStateCode FirewallState { get; set; }
            public string FirewallFingerprint { get; set; }
        }
        public partial class ExpectedFirewallPattern
        {
            public Regex FirewallState { get; set; }
            public Regex FirewallFingerprint { get; set; }
        }

        [Test]
        public void FirewallTest_Regex()
        {
            var expectedFirewallPattern = new ExpectedFirewallPattern
            {
                FirewallState = new Regex(@"[0-2]"),
                FirewallFingerprint = new Regex(@"[0-9a-z]{64}")
            };
            var deserializedReportedObject = JsonSerializer.Deserialize<Firewall>(GetTwin().Properties.Reported[ComponentName].ToString());
            Assert.True(IsRegexMatch(expectedFirewallPattern.FirewallState, deserializedReportedObject.FirewallState));
            Assert.True(IsRegexMatch(expectedFirewallPattern.FirewallFingerprint, deserializedReportedObject.FirewallFingerprint));
        }
    }
}