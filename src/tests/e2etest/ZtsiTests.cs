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
    public class ZtsiTests : E2eTest
    {   
        string ComponentName = "Ztsi";
        public partial class Ztsi
        {
            public int Enabled { get; set; }
            public string ServiceUrl { get; set; }
        }
        public partial class DesiredZtsi
        {
            public bool DesiredEnabled { get; set; }
            public string DesiredServiceUrl { get; set; }
        }

        public partial class ResponseCode
        {
            public int ac { get; set; }
        }
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
        public void ZtsiTest_Get()
        {
            if ((GetTwin().ConnectionState == DeviceConnectionState.Disconnected) || (GetTwin().Status == DeviceStatus.Disabled))
            {
                Assert.Fail("Module is disconnected or is disabled");
            }

            Ztsi deserializedReportedObject = JsonSerializer.Deserialize<Ztsi>(GetTwin().Properties.Reported[ComponentName].ToString());
            Assert.True(IsRegexMatch(new Regex(@"[0-2]"), deserializedReportedObject.Enabled));
            Assert.True(deserializedReportedObject.ServiceUrl is string);
        }

        [Test]
        public void ZtsiTest_Set_Get()
        {
            var desiredZtsi = new DesiredZtsi
            {
                DesiredEnabled = true,
                DesiredServiceUrl = "https://ztsiendpoint.com"
            };

            var expectedZtsi = new Ztsi
            {
                Enabled = 1,
                ServiceUrl = desiredZtsi.DesiredServiceUrl
            };

            if ((GetNewTwin().ConnectionState == DeviceConnectionState.Disconnected) || (GetTwin().Status == DeviceStatus.Disabled))
            {
                Assert.Fail("Module is disconnected or is disabled");
            }

            Ztsi deserializedReportedObject = JsonSerializer.Deserialize<Ztsi>(GetTwin().Properties.Reported[ComponentName].ToString());

            Twin twinPatch = CreateTwinPatch(ComponentName, desiredZtsi);

            if (UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Ztsi reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                // Wait until the reported properties are updated
                DateTime startTime = DateTime.Now;
                while(((reportedObject.Enabled != expectedZtsi.Enabled) || (reportedObject.ServiceUrl != expectedZtsi.ServiceUrl)) && ((DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds))
                {
                    Console.WriteLine("[ZtsiTests] waiting for twin to be updated...");
                    Task.Delay(twinRefreshIntervalMs).Wait();
                    reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                }

                AreEqualByJson(expectedZtsi, reportedObject);
                string desiredEnabled = "DesiredEnabled";
                string desiredServiceUrl = "DesiredServiceUrl";
                int responseCodeSuccess = 200;
                var responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][desiredEnabled].ToString());
                Assert.True(responseObject.ac == responseCodeSuccess);
                responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][desiredServiceUrl].ToString());
                Assert.True(responseObject.ac == responseCodeSuccess);
            }
            else
            {
                Assert.Fail("Timeout for updating twin");
            }
        }
    }
}