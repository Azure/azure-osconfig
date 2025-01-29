// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System.Threading.Tasks;

namespace E2eTesting
{
    [TestFixture, Category("DeliveryOptimization")]
    public class DeliveryOptimizationTests : E2ETest
    {
        private static readonly string _componentName = "DeliveryOptimization";

        public class DeliveryOptimization
        {
            public string CacheHost { get; set; }
            public int CacheHostSource { get; set; }
            public int CacheHostFallback { get; set; }
            public int PercentageDownloadThrottle { get; set; }
        }

        public class DesiredDeliveryOptimizationPolicies
        {
            public string CacheHost { get; set; }
            public int CacheHostSource { get; set; }
            public int CacheHostFallback { get; set; }
            public int PercentageDownloadThrottle { get; set; }
        }

        public class DesiredDeliveryOptimization
        {
            public DesiredDeliveryOptimizationPolicies DesiredDeliveryOptimizationPolicies { get; set; }
        }

        [Test]
        public async Task DeliveryOptimizationTest_Get()
        {
            DeliveryOptimization reported = await GetReported(_componentName, (DeliveryOptimization deliveryOptimization) => (deliveryOptimization.CacheHost != null));

            Assert.Multiple(() =>
            {
                Assert.NotNull(reported.CacheHost);
                Assert.That(reported.CacheHostSource, Is.InRange(0, 3));
                Assert.That(reported.PercentageDownloadThrottle, Is.InRange(0, 100));
            });
        }

        [Test]
        public async Task DeliveryOptimizationTest_Set()
        {
            var desired = new DesiredDeliveryOptimization
            {
                DesiredDeliveryOptimizationPolicies = new DesiredDeliveryOptimizationPolicies
                {
                    CacheHost = "10.0.0.0:80,host.com:8080",
                    CacheHostSource = 1,
                    CacheHostFallback = 2,
                    PercentageDownloadThrottle = 3
                }
            };

            await SetDesired(_componentName, desired);

            DeliveryOptimization reported = await GetReported(_componentName, (DeliveryOptimization deliveryOptimization) => (deliveryOptimization.CacheHost == desired.DesiredDeliveryOptimizationPolicies.CacheHost &&
                deliveryOptimization.CacheHostSource == desired.DesiredDeliveryOptimizationPolicies.CacheHostSource &&
                deliveryOptimization.CacheHostFallback == desired.DesiredDeliveryOptimizationPolicies.CacheHostFallback &&
                deliveryOptimization.PercentageDownloadThrottle == desired.DesiredDeliveryOptimizationPolicies.PercentageDownloadThrottle));

            Assert.Multiple(() =>
            {
                Assert.AreEqual(desired.DesiredDeliveryOptimizationPolicies.CacheHost, reported.CacheHost);
                Assert.AreEqual(desired.DesiredDeliveryOptimizationPolicies.CacheHostSource, reported.CacheHostSource);
                Assert.AreEqual(desired.DesiredDeliveryOptimizationPolicies.CacheHostFallback, reported.CacheHostFallback);
                Assert.AreEqual(desired.DesiredDeliveryOptimizationPolicies.PercentageDownloadThrottle, reported.PercentageDownloadThrottle);
            });
        }
    }
}
