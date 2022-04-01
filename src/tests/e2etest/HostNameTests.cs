// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace E2eTesting
{

    [TestFixture, Category("HostName")]
    public class HostNameTests : E2ETest
    {
        private static readonly string _componentName = "HostName";
        private static readonly string _desiredHostName = "desiredName";
        private static readonly string _desiredHosts = "desiredHosts";

        private static Regex _namePattern = new Regex(@"(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])");
        private static Regex _hostsPattern = new Regex(@"(((([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5]))|((([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|[fF][eE]80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::([fF][eE]{4}(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))))( +((([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])))+");

        public class HostName
        {
            public string name { get; set; }
            public string hosts { get; set; }
        }

        public class DesiredHostName
        {
            public string desiredName { get; set; }
            public string desiredHosts { get; set; }
        }

        [Test]
        public async Task HostNameTest_Get()
        {
            HostName reported = await GetReported<HostName>(_componentName, (HostName hostname) => (hostname.name != null) && (hostname.hosts != null));

            Assert.Multiple(() =>
            {
                RegexAssert.IsMatch(_namePattern, reported.name);
                RegexAssert.IsMatch(_hostsPattern, reported.hosts);
            });
        }

        [Test]
        public async Task HostNameTest_Set()
        {
            var desired = new DesiredHostName
            {
                desiredName = "TestHost",
                desiredHosts = "127.0.0.1 localhost;127.0.1.1 TestHost;::1 ip6-localhost ip6-loopback;fe00::0 ip6-localnet;ff00::0 ip6-mcastprefix;ff02::1 ip6-allnodes;ff02::2 ip6-allrouters"
            };

            await SetDesired<DesiredHostName>(_componentName, desired);

            Func<GenericResponse<string>, bool> condition = (GenericResponse<string> response) => response.ac == ACK_SUCCESS;

            var desiredNameTask = GetReported<GenericResponse<string>>(_componentName, _desiredHostName, condition);
            desiredNameTask.Wait();
            var desiredHostsTask = GetReported<GenericResponse<string>>(_componentName, _desiredHosts, condition);
            desiredHostsTask.Wait();

            HostName reported = await GetReported<HostName>(_componentName, (HostName hostname) => {
                return (hostname.name == desired.desiredName) && (hostname.hosts == desired.desiredHosts);
            });

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ACK_SUCCESS, desiredNameTask.Result.ac);
                Assert.AreEqual(ACK_SUCCESS, desiredHostsTask.Result.ac);
                Assert.AreEqual(desired.desiredName, reported.name);
                Assert.AreEqual(desired.desiredHosts, reported.hosts);
            });
        }
    }
}