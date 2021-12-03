// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using Microsoft.Azure.Devices.Shared;
using Microsoft.Azure.Devices;
using System.Text.Json;
using System;
using System.Threading.Tasks;
using System.Text.RegularExpressions;

namespace E2eTesting
{
    [TestFixture]
    public class ZtsiTests : E2eTest
    {   
        const string ComponentName = "Ztsi";
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

        [OneTimeSetUp]
        public void TestSetUp()
        {
            m_twinTimeoutSeconds = base.twinTimeoutSeconds;
            base.twinTimeoutSeconds = base.twinTimeoutSeconds * 2;
        }

        [OneTimeTearDown]
        public void TestTearDown()
        {
            base.twinTimeoutSeconds = m_twinTimeoutSeconds;
        }

        [Test]      
        public void ZtsiTest_Get()
        {
            CheckModuleConnection();
            Ztsi deserializedReportedObject = JsonSerializer.Deserialize<Ztsi>(GetTwin().Properties.Reported[ComponentName].ToString());
            Assert.True(IsRegexMatch(new Regex(@"[0-2]"), deserializedReportedObject.Enabled));
            Assert.True(deserializedReportedObject.ServiceUrl is string);
        }

        [Test]
        [TestCase("https://ztsiendpoint.com", true)]
        [TestCase("", true)]
        [TestCase("https://www.test.com/", false)]
        [TestCase("https:", false)]
        public void ZtsiTest_Set_Get_ServiceUrl_Enabled(string serviceUrl, bool enabled)
        {
            CheckModuleConnection();
            // Set ServiceUrl first then Enabled
            // Test Set Get ServiceUrl
            var desiredZtsi = new DesiredZtsi
            {
                DesiredServiceUrl = serviceUrl
            };
            var expectedZtsi = new Ztsi
            {
                ServiceUrl = desiredZtsi.DesiredServiceUrl
            };
            Twin twinPatch = CreateTwinPatch(ComponentName, desiredZtsi);

            if (UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Ztsi reportedObject = JsonSerializer.Deserialize<Ztsi>(GetTwin().Properties.Reported[ComponentName].ToString());
                // Wait until the reported properties are updated
                DateTime startTime = DateTime.Now;
                while((reportedObject.ServiceUrl != expectedZtsi.ServiceUrl) && ((DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds))
                {
                    Console.WriteLine("[ZtsiTests] waiting for module twin to be updated...");
                    Task.Delay(twinRefreshIntervalMs).Wait();
                    reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                }
                Assert.True(reportedObject.ServiceUrl == expectedZtsi.ServiceUrl);
                string desiredServiceUrl = "DesiredServiceUrl";
                int responseCodeSuccess = 200;
                var responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][desiredServiceUrl].ToString());
                Assert.True(responseObject.ac == responseCodeSuccess);
            }
            else
            {
                Assert.Fail("Timeout for updating module twin");
            }

            // Test Set Get Enabled
            desiredZtsi = new DesiredZtsi
            {
                DesiredEnabled = enabled
            };

            expectedZtsi = new Ztsi
            {
                Enabled = enabled ? 1 : 2
            };

            twinPatch = CreateTwinPatch(ComponentName, desiredZtsi);

            if (UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Ztsi reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                // Wait until the reported properties are updated
                DateTime startTime = DateTime.Now;
                while((reportedObject.Enabled != expectedZtsi.Enabled) && ((DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds))
                {
                    Console.WriteLine("[ZtsiTests] waiting for module twin to be updated...");
                    Task.Delay(twinRefreshIntervalMs).Wait();
                    reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                }
                Assert.True(reportedObject.Enabled == expectedZtsi.Enabled);
                string desiredEnabled = "DesiredEnabled";
                int responseCodeSuccess = 200;
                var responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][desiredEnabled].ToString());
                Assert.True(responseObject.ac == responseCodeSuccess);
            }
            else
            {
                Assert.Fail("Timeout for updating module twin");
            }
        }

        [Test]
        [TestCase("https://ztsiendpoint.com", true)]
        [TestCase("", true)]
        [TestCase("https://www.test.com/", false)]
        [TestCase("https:", false)]
        public void ZtsiTest_Set_Get_Enabled_ServiceUrl(string serviceUrl, bool enabled)
        {
            CheckModuleConnection();
            // Set Enabled first then ServiceUrl
            // Test Set Get Enabled
            var desiredZtsi = new DesiredZtsi
            {
                DesiredEnabled = enabled
            };
            var expectedZtsi = new Ztsi
            {
                Enabled = enabled ? 1 : 2
            };
            Twin twinPatch = CreateTwinPatch(ComponentName, desiredZtsi);
            if (UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Ztsi reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                // Wait until the reported properties are updated
                DateTime startTime = DateTime.Now;
                while((reportedObject.Enabled != expectedZtsi.Enabled) && ((DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds))
                {
                    Console.WriteLine("[ZtsiTests] waiting for module twin to be updated...");
                    Task.Delay(twinRefreshIntervalMs).Wait();
                    reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                }
                Assert.True(reportedObject.Enabled == expectedZtsi.Enabled);
                string desiredEnabled = "DesiredEnabled";
                int responseCodeSuccess = 200;
                var responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][desiredEnabled].ToString());
                Assert.True(responseObject.ac == responseCodeSuccess);
            }
            else
            {
                Assert.Fail("Timeout for updating module twin");
            }

            // Test Set Get ServiceUrl
            desiredZtsi = new DesiredZtsi
            {
                DesiredServiceUrl = serviceUrl
            };
            expectedZtsi = new Ztsi
            {
                ServiceUrl = desiredZtsi.DesiredServiceUrl
            };
            twinPatch = CreateTwinPatch(ComponentName, desiredZtsi);
            if (UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Ztsi reportedObject = JsonSerializer.Deserialize<Ztsi>(GetTwin().Properties.Reported[ComponentName].ToString());
                // Wait until the reported properties are updated
                DateTime startTime = DateTime.Now;
                while((reportedObject.ServiceUrl != expectedZtsi.ServiceUrl) && ((DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds))
                {
                    Console.WriteLine("[ZtsiTests] waiting for module twin to be updated...");
                    Task.Delay(twinRefreshIntervalMs).Wait();
                    reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                }
                Assert.True(reportedObject.ServiceUrl == expectedZtsi.ServiceUrl);
                string desiredServiceUrl = "DesiredServiceUrl";
                int responseCodeSuccess = 200;
                var responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][desiredServiceUrl].ToString());
                Assert.True(responseObject.ac == responseCodeSuccess);
            }
            else
            {
                Assert.Fail("Timeout for updating module twin");
            }
        }

        [Test]
        [TestCase("https://ztsiendpoint.com", true)]
        [TestCase("", true)]
        [TestCase("https://www.test.com/", false)]
        [TestCase("https:", false)]
        public void ZtsiTest_Set_Get(string serviceUrl, bool enabled)
        {
            CheckModuleConnection();
            // Set Enabled and ServiceUrl at the same time
            var desiredZtsi = new DesiredZtsi
            {
                DesiredEnabled = enabled,
                DesiredServiceUrl = serviceUrl
            };
            var expectedZtsi = new Ztsi
            {
                Enabled = enabled ? 1 : 2,
                ServiceUrl = serviceUrl
            };
            Twin twinPatch = CreateTwinPatch(ComponentName, desiredZtsi);
            if (UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Ztsi reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                // Wait until the reported properties are updated
                DateTime startTime = DateTime.Now;
                while(((reportedObject.Enabled != expectedZtsi.Enabled) || (reportedObject.ServiceUrl != expectedZtsi.ServiceUrl)) && ((DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds))
                {
                    Console.WriteLine("[ZtsiTests] waiting for module twin to be updated...");
                    Task.Delay(twinRefreshIntervalMs).Wait();
                    reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
                }
                Assert.True(reportedObject.Enabled == expectedZtsi.Enabled);
                Assert.True(reportedObject.ServiceUrl == expectedZtsi.ServiceUrl);
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
                Assert.Fail("Timeout for updating module twin");
            }
        }
    }
}