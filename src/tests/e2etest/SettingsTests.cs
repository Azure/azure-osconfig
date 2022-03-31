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
            public int percentageDownloadThrottle { get; set; }
            public int cacheHostSource { get; set; }
            public string cacheHost { get; set; }
            public int cacheHostFallback { get; set; }
        }

        public class Settings
        {
            public int deviceHealthTelemetryConfiguration { get; set; }
            public DeliveryOptimizationPolicies deliveryOptimizationPolicies { get; set; }
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
                percentageDownloadThrottle = 50,
                cacheHostSource = 1,
                cacheHost = "127.0.1.1",
                cacheHostFallback = 90
            };

            var response = await SetDesired<DeliveryOptimizationPolicies>(_componentName, "DeliveryOptimizationPolicies", desiredDeliveryOptimizationPolicies);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(desiredDeliveryOptimizationPolicies.percentageDownloadThrottle, response.value.percentageDownloadThrottle);
                Assert.AreEqual(desiredDeliveryOptimizationPolicies.cacheHostSource, response.value.cacheHostSource);
                Assert.AreEqual(desiredDeliveryOptimizationPolicies.cacheHost, response.value.cacheHost);
                Assert.AreEqual(desiredDeliveryOptimizationPolicies.cacheHostFallback, response.value.cacheHostFallback);
            });
        }
    }
}