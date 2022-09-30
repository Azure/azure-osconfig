// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;

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
        public void DeliveryOptimizationTest_Get()
        {
            DeliveryOptimization reported = GetReported(_componentName, (DeliveryOptimization deliveryOptimization) => (deliveryOptimization.CacheHost != null));

            Assert.Multiple(() =>
            {
                Assert.AreEqual("", reported.CacheHost);
                Assert.AreEqual(0, reported.CacheHostSource);
                Assert.AreEqual(0, reported.CacheHostFallback);
                Assert.AreEqual(0, reported.PercentageDownloadThrottle);
            });
        }
        
        [Test]
        public void DeliveryOptimizationTest_Set()
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

            SetDesired(_componentName, desired);

            DeliveryOptimization reported = GetReported(_componentName, (DeliveryOptimization deliveryOptimization) => (deliveryOptimization.CacheHost == desired.DesiredDeliveryOptimizationPolicies.CacheHost && 
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