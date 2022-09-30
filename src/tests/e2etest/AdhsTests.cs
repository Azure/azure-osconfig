// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System;
using System.Text.RegularExpressions;
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
        public void AdhsTest_Get()
        {
            Adhs reported = GetReported<Adhs>(_componentName, (Adhs adhs) => (adhs.OptIn == 0));

            Assert.Multiple(() =>
            {
                Assert.AreEqual(0, reported.OptIn);
            });
        }
        
        [Test]
        public void AdhsTest_Set()
        {
            var desired = new DesiredAdhs
            {
                DesiredOptIn = 1 
            };

            SetDesired<DesiredAdhs>(_componentName, desired);

            Adhs reported = GetReported<Adhs>(_componentName, (Adhs adhs) => (adhs.OptIn == 1));

            Assert.Multiple(() =>
            {
                Assert.AreEqual(desired.DesiredOptIn, reported.OptIn);
            });
        }
    }
}