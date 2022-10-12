// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System;
using System.Text.RegularExpressions;

namespace E2eTesting
{
    [TestFixture, Category("Ztsi"), Ignore("Stability Issues")]
    public class ZtsiTests : E2ETest
    {
        private static readonly string _componentName = "Ztsi";
        private static readonly string _desiredEnabled = "desiredEnabled";
        private static readonly string _desiredServiceUrl = "desiredServiceUrl";

        private static readonly Regex _urlPattern = new Regex("((http|https)://)(www.)?[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|]");

        public enum Enabled
        {
            Unknown = 0,
            Enabled,
            Disabled
        }
        public class Ztsi
        {
            public Enabled Enabled { get; set; }
            public string ServiceUrl { get; set; }
        }

        public class DesiredZtsi
        {
            public bool DesiredEnabled { get; set; }
            public string DesiredServiceUrl { get; set; }
        }

        public bool SetServiceUrl(string serviceUrl)
        {
            return SetDesired<string>(_componentName, _desiredServiceUrl, serviceUrl);
        }

        public bool SetEnabled(bool enabled)
        {
            return SetDesired<bool>(_componentName, _desiredEnabled, enabled);
        }

        public void SetZtsiConfiguration(string serviceUrl, bool enabled)
        {
            var desiredConfiguration = new DesiredZtsi
            {
                DesiredEnabled = enabled,
                DesiredServiceUrl = serviceUrl
            };
            SetDesired<DesiredZtsi>(_componentName, desiredConfiguration);
        }

        public Ztsi GetZtsiConfiguration(string expectedServiceUrl, Enabled expectedEnabled)
        {
            return GetReported<Ztsi>(_componentName, (Ztsi ztsi) => ((ztsi.ServiceUrl == expectedServiceUrl) && (ztsi.Enabled == expectedEnabled)));
        }

        public string GetServiceUrl(string expectedServiceUrl)
        {
            return GetReported<Ztsi>(_componentName, (Ztsi ztsi) => (ztsi.ServiceUrl == expectedServiceUrl)).ServiceUrl;
        }

        public Enabled GetEnabled(Enabled expectedEnabled)
        {
            return GetReported<Ztsi>(_componentName, (Ztsi ztsi) => (ztsi.Enabled == expectedEnabled)).Enabled;
        }

        [Test]
        [TestCase("http://")]
        [TestCase("https://")]
        [TestCase("http:\\\\example.com")]
        [TestCase("htp://example.com")]
        [TestCase("//example.com")]
        [TestCase("www.example.com")]
        [TestCase("example.com")]
        [TestCase("example.com/params?a=1")]
        [TestCase("/example")]
        [TestCase("localhost")]
        [TestCase("localhost:5000")]
        public void ZtsiTest_InvalidServiceUrl(string serviceUrl)
        {
            Assert.Multiple(() =>
            {
                Assert.IsTrue(SetEnabled(false));
                Assert.IsFalse(SetServiceUrl(serviceUrl), "Unexpected for service url");
            });
        }

        [Test]
        [TestCase("")]
        [TestCase("http://example.com")]
        [TestCase("https://example.com")]
        [TestCase("http://example.com/")]
        [TestCase("https://example.com/")]
        [TestCase("http://www.example.com")]
        [TestCase("https://www.example.com")]
        [TestCase("https://www.example.com/path/to/something/")]
        [TestCase("https://www.example.com/params?a=1")]
        [TestCase("https://www.example.com/params?a=1&b=2")]
        public void ZtsiTest_ValidServiceUrl(string serviceUrl)
        {
            Assert.Multiple(() =>
            {
                Assert.IsTrue(SetEnabled(false), "Unexpected for enabled");
                Assert.IsTrue(SetServiceUrl(serviceUrl), "Unexpected for service url");
                Assert.AreEqual(serviceUrl, GetServiceUrl(serviceUrl));
            });
        }

        [Test]
        public void ZtsiTest_Enabled()
        {
            Assert.Multiple(() =>
            {
                Assert.IsTrue(SetServiceUrl("http://example.com"), "Unexpected for service url");
                Assert.IsTrue(SetEnabled(true), "Unexpected for enabled");
                Assert.AreEqual(Enabled.Enabled, GetEnabled(Enabled.Enabled));
            });
        }


        [Test]
        [TestCase("http://example1.com", false, "http://example1.com", Enabled.Disabled)]
        [TestCase("http://example2.com", true, "http://example2.com", Enabled.Enabled)]
        [TestCase("", false, "", Enabled.Disabled)]
        public void ZtsiTest_Configuration(string serviceUrl, bool enabled, string expectedServiceUrl, Enabled expectedEnabled)
        {
            Assert.Multiple(() =>
            {
                SetZtsiConfiguration(serviceUrl, enabled);
                Assert.AreEqual(expectedServiceUrl, GetServiceUrl(expectedServiceUrl));
                Assert.AreEqual(expectedEnabled, GetEnabled(expectedEnabled));
            });
        }

        [Test]
        public void ZtsiTest_InvalidConfiguration()
        {
            Assert.Multiple(() =>
            {
                SetZtsiConfiguration("http://test.com", true);
                SetZtsiConfiguration("", true);
                var config = GetZtsiConfiguration("http://test.com", Enabled.Enabled);
                Assert.AreEqual("http://test.com", config.ServiceUrl);
                Assert.AreEqual(Enabled.Enabled, config.Enabled);
            });
        }
    }
}