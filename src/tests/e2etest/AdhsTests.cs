// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System.Threading.Tasks;

namespace E2eTesting
{
    [TestFixture, Category("Adhs")]
    public class AdhsTests : E2ETest
    {
        private static readonly string _componentName = "Adhs";

        public class Adhs
        {
            public int OptIn { get; set; }
        }

        public class DesiredAdhs
        {
            public int DesiredOptIn { get; set; }
        }

        [Test]
        public async Task AdhsTest_Get()
        {
            Adhs reported = await GetReported(_componentName, (Adhs adhs) => (adhs.OptIn == 0 || adhs.OptIn == 1 || adhs.OptIn == 2));

            Assert.Multiple(() =>
            {
                Assert.That(reported.OptIn, Is.InRange(0, 2));
            });
        }

        [Test]
        public async Task AdhsTest_Set()
        {
            var desired = new DesiredAdhs
            {
                DesiredOptIn = 1
            };

            await SetDesired(_componentName, desired);

            Adhs reported = await GetReported(_componentName, (Adhs adhs) => (adhs.OptIn == 1));

            Assert.Multiple(() =>
            {
                Assert.AreEqual(desired.DesiredOptIn, reported.OptIn);
            });
        }
    }
}
