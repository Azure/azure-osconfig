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


namespace e2etesting
{
    [TestFixture]
    public class SettingsTests : E2eTest
    {   
        string ComponentName = "Settings";
        string ConfigurationProperty = "DeviceHealthTelemetryConfiguration";
        string PoliciesProperty = "DeliveryOptimizationPolicies";
        public partial class DeliveryOptimizationPolicies
        {
            public int PercentageDownloadThrottle { get; set; }
            public int CacheHostSource { get; set; }
            public string CacheHost { get; set; }
            public int CacheHostFallback { get; set; }
        }
        public partial class Settings
        {
            public int DeviceHealthTelemetryConfiguration { get; set; }
            public DeliveryOptimizationPolicies DeliveryOptimizationPolicies { get; set; }
        }
        public partial class ResponseCode
        {
            public int ac { get; set; }
        }
        public partial class DeliveryOptimizationPoliciesPattern
        {
            public Regex PercentageDownloadThrottlePattern { get; set; }
            public Regex CacheHostSourcePattern { get; set; }
            public Regex CacheHostPattern { get; set; }
            public Regex CacheHostFallbackPattern { get; set; }
        }
        public partial class ReportedConfiguration
        {
            public int value { get; set; } 
        }

        public partial class ReportedPolicies
        {
            public DeliveryOptimizationPolicies value { get; set; }
        }
        private int m_twinTimeoutSeconds;


        public Settings GetSettingsReportObject()
        {
            Settings reportedObject = new Settings();
            ReportedConfiguration reportedConfigurationObject = JsonSerializer.Deserialize<ReportedConfiguration>(GetNewTwin().Properties.Reported[ComponentName][ConfigurationProperty].ToString());
            ReportedPolicies reportedPoliciesObject = JsonSerializer.Deserialize<ReportedPolicies>(GetTwin().Properties.Reported[ComponentName][PoliciesProperty].ToString());
            reportedObject.DeviceHealthTelemetryConfiguration = reportedConfigurationObject.value;
            reportedObject.DeliveryOptimizationPolicies = reportedPoliciesObject.value;
            return reportedObject;
        }

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
        public void SettingsTest_Set_Get()
        {
            var desiredDeliveryOptimizationPolicies = new DeliveryOptimizationPolicies
            {
                PercentageDownloadThrottle = 50,
                CacheHostSource = 1,
                CacheHost = "127.0.1.1",
                CacheHostFallback = 90
            };

            var desiredSettings = new Settings
            {
                DeviceHealthTelemetryConfiguration = 2,
                DeliveryOptimizationPolicies = desiredDeliveryOptimizationPolicies
            };

            if ((GetNewTwin().ConnectionState == DeviceConnectionState.Disconnected) || (GetTwin().Status == DeviceStatus.Disabled))
            {
                Assert.Fail("Module is disconnected or is disabled");
            }

            Twin twinPatch = CreateTwinPatch(ComponentName, desiredSettings);

            var reportedObject = new Settings();
            if (UpdateTwinBlockUntilUpdate(twinPatch))
            {
                reportedObject = GetSettingsReportObject();
                // Wait until the reported properties are updated
                DateTime startTime = DateTime.Now;
                while(!IsSameByJson(desiredSettings, reportedObject) && (DateTime.Now - startTime).TotalSeconds < twinTimeoutSeconds)
                {
                    Console.WriteLine("[SettingsTests] waiting for twin to be updated...");
                    Task.Delay(twinRefreshIntervalMs).Wait();
                    reportedObject = GetSettingsReportObject();
                }
            }
            else
            {
                Assert.Fail("Timeout for updating twin");
            }
        }
    }
}