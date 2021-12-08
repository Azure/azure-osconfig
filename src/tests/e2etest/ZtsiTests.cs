// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using Microsoft.Azure.Devices.Shared;
using Microsoft.Azure.Devices;
using System.Text.Json;
using System;
using System.Threading.Tasks;
using System.Text.RegularExpressions;
using System.IO;
namespace E2eTesting
{
    [TestFixture, Category("Ztsi")]
    public class ZtsiTests : E2eTest
    {
        const string ComponentName = "Ztsi";
        const string desiredEnabled = "DesiredEnabled";
        const string desiredServiceUrl = "DesiredServiceUrl";
        const string removeFileCommand = "rm /etc/ztsi/config.json";
        const string urlRegex = "((http|https)://)(www.)?[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|]";

        const int responseStatusSuccess = 200;
        const int responseStatusFailed = 400;

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
        public void RemoveConfigurationFile()
        {
            PerformCommandViaCommandRunner(removeFileCommand);
        }

        public void Set_ServiceUrl(string serviceUrl)
        {
            CheckModuleConnection();
            // Test Set Get ServiceUrl
            var desiredZtsi = new DesiredZtsi
            {
                DesiredServiceUrl = serviceUrl
            };
            Twin twinPatch = CreateTwinPatch(ComponentName, desiredZtsi);
            if (!UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Assert.Fail("Timeout for updating module twin");
            }
        }

        public void Get_ServiceUrl(string expectedServiceUrl, int serviceUrlResponseCode)
        {
            Ztsi reportedObject = JsonSerializer.Deserialize<Ztsi>(GetTwin().Properties.Reported[ComponentName].ToString());
            DateTime startTime = DateTime.Now;
            while ((reportedObject.ServiceUrl != expectedServiceUrl) && ((DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds))
            {
                Console.WriteLine("[ZtsiTest_Get_ServiceUrl] waiting for module twin to be updated...");
                Task.Delay(twinRefreshIntervalMs).Wait();
                reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
            }
            if (reportedObject.ServiceUrl != expectedServiceUrl)
            {
                Assert.Fail("Timeout for updating reported ServiceUrl in module twin");
            }

            var responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][desiredServiceUrl].ToString());
            Assert.True(responseObject.ac == serviceUrlResponseCode);
        }
        public void Set_Enabled(bool enabled)
        {
            CheckModuleConnection();
            var desiredZtsi = new DesiredZtsi
            {
                DesiredEnabled = enabled
            };
            Twin twinPatch = CreateTwinPatch(ComponentName, desiredZtsi);
            if (!UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Assert.Fail("Timeout for updating module twin");
            }
        }

        public void Get_Enabled(int expectedEnabled, int enabledResponseCode)
        {
            Ztsi reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
            // Wait until the reported properties are updated
            DateTime startTime = DateTime.Now;
            while ((reportedObject.Enabled != expectedEnabled) && ((DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds))
            {
                Console.WriteLine("[ZtsiTest_Get_Enabled] waiting for module twin to be updated...");
                Task.Delay(twinRefreshIntervalMs).Wait();
                reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
            }
            if (reportedObject.Enabled != expectedEnabled)
            {
                Assert.Fail("Timeout for updating reported Enabled in module twin");
            }
            var responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][desiredEnabled].ToString());
            Assert.True(responseObject.ac == enabledResponseCode);
        }

        public void Set_Both(string serviceUrl, bool enabled)
        {
            CheckModuleConnection();
            // Set Enabled and ServiceUrl at the same time
            var desiredZtsi = new DesiredZtsi
            {
                DesiredEnabled = enabled,
                DesiredServiceUrl = serviceUrl
            };
            Twin twinPatch = CreateTwinPatch(ComponentName, desiredZtsi);
            if (!UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Assert.Fail("Timeout for updating module twin");
            }
        }

        public void Get_Both(string expectedServiceUrl,  int serviceUrlResponseCode, int expectedEnabled, int enabledResponseCode)
        {
            // Get Enabled and ServiceUrl at the same time
            Ztsi reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
            DateTime startTime = DateTime.Now;
            // Wait until the reported properties are updated
            while (((reportedObject.Enabled != expectedEnabled) || (reportedObject.ServiceUrl != expectedServiceUrl)) && ((DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds))
            {
                Console.WriteLine("[ZtsiTests_Get_Both] waiting for module twin to be updated...");
                Task.Delay(twinRefreshIntervalMs).Wait();
                reportedObject = JsonSerializer.Deserialize<Ztsi>(GetNewTwin().Properties.Reported[ComponentName].ToString());
            }
            if (reportedObject.Enabled != expectedEnabled)
            {
                Assert.Fail("Timeout for updating reported Enabled in module twin");
            }

            if (reportedObject.ServiceUrl != expectedServiceUrl)
            {
                Assert.Fail("Timeout for updating reported ServiceUrl in module twin");
            }

            var responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][desiredEnabled].ToString());
            Assert.True(responseObject.ac == enabledResponseCode);
            responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][desiredServiceUrl].ToString());
            Assert.True(responseObject.ac == serviceUrlResponseCode);
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
        [TestCase(false, "", true, "", responseStatusSuccess, 0, responseStatusSuccess)]
        [TestCase(false, "", false, "", responseStatusSuccess, 2, responseStatusSuccess)]
        [TestCase(false, "Invalid url", true, "", responseStatusFailed, 0, responseStatusSuccess)]
        [TestCase(false, "Invalid url", false, "", responseStatusFailed, 2, responseStatusSuccess)]
        [TestCase(false, "https://www.test.com/", true, "https://www.test.com/", responseStatusSuccess, 0, responseStatusSuccess)]
        [TestCase(false, "https://www.test.com/", false, "https://www.test.com/", responseStatusSuccess, 2, responseStatusSuccess)]
        [TestCase(true, "https://www.example.com/", true, "https://www.example.com/", responseStatusSuccess, 1, responseStatusSuccess)]
        public void ZtsiTest_Set_Get_Enabled_ServiceUrl(bool ConfigurationFileExits, string serviceUrl, bool enabled, string expectedServiceUrl, int serviceUrlResponseCode, int expectedEnabled, int enabledResponseCode)
        {
            if (!ConfigurationFileExits)
            {
                RemoveConfigurationFile();
            }

            Set_Enabled(enabled);
            Get_Enabled(expectedEnabled, enabledResponseCode);
            Set_ServiceUrl(serviceUrl);
            Get_ServiceUrl(expectedServiceUrl, serviceUrlResponseCode);
        }

        [Test]
        [TestCase(false, "", true, "", responseStatusSuccess, 0, responseStatusSuccess)]
        [TestCase(false, "", false, "", responseStatusSuccess, 0, responseStatusSuccess)]
        [TestCase(false, "Invalid url", true, "", responseStatusFailed, 0, responseStatusSuccess)]
        [TestCase(false, "Invalid url", false, "", responseStatusFailed, 0, responseStatusSuccess)]
        [TestCase(false, "https://www.test.com/", true, "https://www.test.com/", responseStatusSuccess, 1, responseStatusSuccess)]
        [TestCase(false, "https://www.test.com/", false, "https://www.test.com/", responseStatusSuccess, 2, responseStatusSuccess)]
        [TestCase(true, "https://www.example.com/", true, "https://www.example.com/", responseStatusSuccess, 1, responseStatusSuccess)]
        public void ZtsiTest_Set_Get_ServiceUrl_Enabled(bool ConfigurationFileExits, string serviceUrl, bool enabled, string expectedServiceUrl, int serviceUrlResponseCode,int expectedEnabled, int enabledResponseCode)
        {
            if (!ConfigurationFileExits)
            {
                RemoveConfigurationFile();
            }

            Set_ServiceUrl(serviceUrl);
            Get_ServiceUrl(expectedServiceUrl, serviceUrlResponseCode);
            Set_Enabled(enabled);
            Get_Enabled(expectedEnabled, enabledResponseCode);
        }

        [Test]
        [TestCase(false, "", true, "", responseStatusFailed, 0, responseStatusSuccess)]
        [TestCase(false, "", false, "", responseStatusSuccess, 0, responseStatusSuccess)]
        [TestCase(false, "Invalid url", true, "", responseStatusFailed, 0, responseStatusSuccess)]
        [TestCase(false, "Invalid url", false, "", responseStatusFailed, 2, responseStatusSuccess)]
        [TestCase(false, "https://www.test.com/", true, "https://www.test.com/", responseStatusSuccess, 1, responseStatusSuccess)]
        [TestCase(false, "https://www.test.com/", false, "https://www.test.com/", responseStatusSuccess, 2, responseStatusSuccess)]
        [TestCase(true, "https://www.example.com/", true, "https://www.example.com/", responseStatusSuccess, 1, responseStatusSuccess)]
        public void ZtsiTest_Set_Get_Both(bool ConfigurationFileExits, string serviceUrl, bool enabled, string expectedServiceUrl, int serviceUrlResponseCode, int expectedEnabled, int enabledResponseCode)
        {
            if (!ConfigurationFileExits)
            {
                RemoveConfigurationFile();
            }

            Set_Both(serviceUrl, enabled);
            Get_Both(expectedServiceUrl, serviceUrlResponseCode, expectedEnabled, enabledResponseCode);
        }
    }
}