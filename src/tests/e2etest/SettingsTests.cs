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
        public async Task SettingsTest_DeviceHealthTelemtryConfiguration()
        {
            int desiredValue = 2;
            var response = await SetDesired<int>(_componentName, "DeviceHealthTelemetryConfiguration", desiredValue);

            Assert.AreEqual(desiredValue, response.value);
        }

        [Test]
        public async Task SettingsTest_DeliveryOptimizationPolicies()
        {
            var desiredDeliveryOptimizationPolicies = new DeliveryOptimizationPolicies
            {
                PercentageDownloadThrottle = 50,
                CacheHostSource = 1,
                CacheHost = "127.0.1.1",
                CacheHostFallback = 90
            };

            var response = await SetDesired<DeliveryOptimizationPolicies>(_componentName, "DeliveryOptimizationPolicies", desiredDeliveryOptimizationPolicies);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(desiredDeliveryOptimizationPolicies.PercentageDownloadThrottle, response.value.PercentageDownloadThrottle);
                Assert.AreEqual(desiredDeliveryOptimizationPolicies.CacheHostSource, response.value.CacheHostSource);
                Assert.AreEqual(desiredDeliveryOptimizationPolicies.CacheHost, response.value.CacheHost);
                Assert.AreEqual(desiredDeliveryOptimizationPolicies.CacheHostFallback, response.value.CacheHostFallback);
            });
        }
    }
}