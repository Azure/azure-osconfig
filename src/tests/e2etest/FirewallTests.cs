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

        private static Regex _fingerprintPattern = new Regex(@"[0-9a-z]{64}");

        [JsonConverter(typeof(StringEnumConverter), typeof(CamelCaseNamingStrategy))]
        public enum State
        {
            Unknown,
            Enabled,
            Disabled
        }

        public partial class Firewall
        {
            public State State { get; set; }
            public string Fingerprint { get; set; }
        }


        [Test]
        public void FirewallTest_Regex()
        {
            Firewall reported = GetReported<Firewall>(_componentName, (Firewall firewall) => (firewall.State != State.Unknown) && (firewall.Fingerprint != null));

            Assert.Multiple(() =>
            {
                Assert.AreNotEqual(State.Unknown, reported.State);
                RegexAssert.IsMatch(_fingerprintPattern, reported.Fingerprint);
            });
        }
    }
}