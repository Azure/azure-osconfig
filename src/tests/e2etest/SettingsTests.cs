// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System.Threading.Tasks;

namespace E2eTesting
{
    [TestFixture, Category("Settings")]
    public class SettingsTests : E2ETest
    {
        private static readonly string _componentName = "Settings";

        public class DeliveryOptimizationPolicies
        {
            public int PercentageDownloadThrottle { get; set; }
            public int CacheHostSource { get; set; }
            public string CacheHost { get; set; }
            public int CacheHostFallback { get; set; }
        }

        public class Settings
        {
            public int DeviceHealthTelemetryConfiguration { get; set; }
            public DeliveryOptimizationPolicies DeliveryOptimizationPolicies { get; set; }
        }

        [Test]
        [Ignore("Turned off until we have a fix for Settings module tests")]
        public void SettingsTest_DeviceHealthTelemtryConfiguration()
        {
            Assert.IsTrue(SetDesired<int>(_componentName, "deviceHealthTelemetryConfiguration", 2));
        }

        [Test]
        [Ignore("Turned off until we have a fix for Settings module tests")]
        public void SettingsTest_DeliveryOptimizationPolicies()
        {
            var desiredDeliveryOptimizationPolicies = new DeliveryOptimizationPolicies
            {
                PercentageDownloadThrottle = 50,
                CacheHostSource = 1,
                CacheHost = "127.0.1.1",
                CacheHostFallback = 90
            };

            Assert.IsTrue(SetDesired<DeliveryOptimizationPolicies>(_componentName, "deliveryOptimizationPolicies", desiredDeliveryOptimizationPolicies));
        }
    }
}