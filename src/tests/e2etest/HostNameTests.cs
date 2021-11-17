// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using Microsoft.Azure.Devices.Shared;
using Microsoft.Azure.Devices;
using System.Text.Json;
using System;
using System.Threading.Tasks;
using System.Text.RegularExpressions;

namespace e2etesting
{
    [TestFixture]
    public class HostNameTests : E2eTest
    {   
        string ComponentName = "HostName";
        public partial class HostName
        {
            public string Name { get; set; }
            public string Hosts { get; set; }
        }
        public partial class DesiredHostName
        {
            public string DesiredName { get; set; }
            public string DesiredHosts { get; set; }
        }
        public partial class ExpectedHostNamePattern
        {
            public Regex NamePattern { get; set; }
            public Regex HostsPattern { get; set; }
        }
        HostName deserializedReportedObject;
        private int m_twinTimeoutSeconds;

        [SetUp]
        public void TestSetUp()
        {
            m_twinTimeoutSeconds = base.twinTimeoutSeconds;
            base.twinTimeoutSeconds = base.twinTimeoutSeconds * 2;
        }

        [TearDown]
        public void TestTearDown()
        {
            base.twinTimeoutSeconds = m_twinTimeoutSeconds;
        }

        [Test]      
        public void HostNameTest_Get()
        {
            var expectedHostNamePattern = new ExpectedHostNamePattern
            {
                NamePattern = new Regex(@"(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])"),
                HostsPattern = new Regex(@"(((([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5]))|((([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|[fF][eE]80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::([fF][eE]{4}(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))))( +((([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])))+")
            };

            if ((GetTwin().ConnectionState == DeviceConnectionState.Disconnected) || (GetTwin().Status == DeviceStatus.Disabled))
            {
                Assert.Fail("Module is disconnected or is disabled - HostNameTest_Get");
            }

            deserializedReportedObject = JsonSerializer.Deserialize<HostName>(GetTwin().Properties.Reported[ComponentName].ToString());

            Assert.True(IsRegexMatch(expectedHostNamePattern.NamePattern, deserializedReportedObject.Name));
            Assert.True(IsRegexMatch(expectedHostNamePattern.HostsPattern, deserializedReportedObject.Hosts));
        }

        [Test]
        public void HostNameTest_Set_Get()
        {
            var desiredHostName = new DesiredHostName
            {
                DesiredName = "TestHost",
                DesiredHosts = "127.0.0.1 localhost;127.0.1.1 TestHost;::1 ip6-localhost ip6-loopback;fe00::0 ip6-localnet;ff00::0 ip6-mcastprefix;ff02::1 ip6-allnodes;ff02::2 ip6-allrouters"
            };

            var expectedHostName = new HostName
            {
                Name = desiredHostName.DesiredName,
                Hosts = desiredHostName.DesiredHosts
            };

            if ((GetTwin().ConnectionState == DeviceConnectionState.Disconnected) || (GetTwin().Status == DeviceStatus.Disabled))
            {
                Assert.Fail("Module is disconnected or is disabled- HostNameTest_Set_Get");
            }

            Twin twinPatch = CreateTwinPatch(ComponentName, desiredHostName);
            int count = 2;
            while (count-- > 0)
            {
                if (count == 0)
                {
                    desiredHostName.DesiredName = deserializedReportedObject.Name;
                    desiredHostName.DesiredHosts = deserializedReportedObject.Hosts;
                    expectedHostName.Name = desiredHostName.DesiredName;
                    expectedHostName.Hosts = desiredHostName.DesiredHosts;
                    twinPatch = CreateTwinPatch(ComponentName, desiredHostName);
                }

                if (UpdateTwinBlockUntilUpdate(twinPatch))
                {
                    HostName reportedObject = JsonSerializer.Deserialize<HostName>(GetTwin().Properties.Reported[ComponentName].ToString());
                    // Wait until the reported properties are updated
                    DateTime startTime = DateTime.Now;
                    while(reportedObject.Name != desiredHostName.DesiredName && (DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds)
                    {
                        Console.WriteLine("[HostTests] waiting for twin to be updated...");
                        Task.Delay(twinRefreshIntervalMs).Wait();
                        reportedObject = JsonSerializer.Deserialize<HostName>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                    }
    
                    AreEqualByJson(expectedHostName, reportedObject);
                }
                else
                {
                    Assert.Fail("Timeout for updating twin - HostNameTest_Set_Get");
                }
            }
        }
    }
}