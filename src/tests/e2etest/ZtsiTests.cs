// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System;
using System.Text.RegularExpressions;

namespace E2eTesting
{
    [TestFixture, Category("Ztsi")]
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

        public int SetServiceUrl(string serviceUrl)
        {
            try
            {
                var setDesiredTask = SetDesired<string>(_componentName, _desiredServiceUrl, serviceUrl);
                setDesiredTask.Wait();
                return setDesiredTask.Result.Ac;
            }
            catch (Exception e)
            {
                Assert.Warn("Failed to set service url: {0} ", e.Message);
                return -1;
            }
        }

        public int SetEnabled(bool enabled)
        {
            try
            {
                var setDesiredTask = SetDesired<bool>(_componentName, _desiredEnabled, enabled);
                setDesiredTask.Wait();
                return setDesiredTask.Result.Ac;
            }
            catch (Exception e)
            {
                Assert.Warn("Failed to set enabled: {0} ", e.Message);
                return -1;
            }
        }

        public (int, int) SetZtsiConfiguration(string serviceUrl, bool enabled)
        {
            try
            {
                var desiredConfiguration = new DesiredZtsi
                {
                    DesiredEnabled = enabled,
                    DesiredServiceUrl = serviceUrl
                };

                var setDesiredTask = SetDesired<DesiredZtsi>(_componentName, desiredConfiguration);
                setDesiredTask.Wait();

                var desiredEnabledTask = GetReported<GenericResponse<bool>>(_componentName, _desiredEnabled, (GenericResponse<bool> response) => response.Ac == ACK_SUCCESS);
                desiredEnabledTask.Wait();
                var desiredServiceUrlTask = GetReported<GenericResponse<string>>(_componentName, _desiredServiceUrl, (GenericResponse<string> response) => response.Ac == ACK_SUCCESS);
                desiredServiceUrlTask.Wait();

                return (desiredServiceUrlTask.Result.Ac, desiredEnabledTask.Result.Ac);
            }
            catch (Exception e)
            {
                Assert.Warn("Failed to set ztsi configuration: {0} ", e.Message);
                return (-1, -1);
            }
        }

        public (string, Enabled) GetZtsiConfiguration(string expectedServiceUrl, Enabled expectedEnabled)
        {
            try
            {
                var reportedTask = GetReported<Ztsi>(_componentName, (Ztsi ztsi) => ((ztsi.ServiceUrl == expectedServiceUrl) && (ztsi.Enabled == expectedEnabled)));
                reportedTask.Wait();
                var reportedConfiguration = reportedTask.Result;
                ValidateLocalReported(reportedConfiguration, _componentName);

                return (reportedConfiguration.ServiceUrl, reportedConfiguration.Enabled);
            }
            catch (Exception e)
            {
                Assert.Warn("Failed to get ztsi configuration: {0} ", e.Message);
                return (null, Enabled.Unknown);
            }
        }

        public string GetServiceUrl(string expectedServiceUrl)
        {
            try
            {
                var reportedTask = GetReported<Ztsi>(_componentName, (Ztsi ztsi) => (ztsi.ServiceUrl == expectedServiceUrl));
                reportedTask.Wait();
                ValidateLocalReported(reportedTask.Result, _componentName);
                return reportedTask.Result.ServiceUrl;
            }
            catch (Exception e)
            {
                Assert.Warn("Failed to get service url: {0} ", e.Message);
                return null;
            }
        }

        public Enabled GetEnabled(Enabled expectedEnabled)
        {
            try
            {
                var reportedTask = GetReported<Ztsi>(_componentName, (Ztsi ztsi) => (ztsi.Enabled == expectedEnabled));
                reportedTask.Wait();
                ValidateLocalReported(reportedTask.Result, _componentName);
                return reportedTask.Result.Enabled;
            }
            catch (Exception e)
            {
                Assert.Warn("Failed to get enabled: {0} ", e.Message);
                return Enabled.Unknown;
            }
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
            int enabledAckCode = SetEnabled(false);
            int serviceUrlAckCode = SetServiceUrl(serviceUrl);
            Assert.AreEqual(ACK_ERROR, serviceUrlAckCode, "Unexpected ack code for service url");
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
            int enabledAckCode = SetEnabled(false);
            int serviceUrlAckCode = SetServiceUrl(serviceUrl);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ACK_SUCCESS, enabledAckCode, "Unexpected ack code for enabled");
                Assert.AreEqual(ACK_SUCCESS, serviceUrlAckCode, "Unexpected ack code for service url");
                Assert.AreEqual(serviceUrl, GetServiceUrl(serviceUrl));
            });
        }

        [Test]
        public void ZtsiTest_Enabled()
        {
            int serviceUrlAckCode = SetServiceUrl("http://example.com");
            int enabledAckCode = SetEnabled(true);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ACK_SUCCESS, serviceUrlAckCode, "Unexpected ack code for service url");
                Assert.AreEqual(ACK_SUCCESS, enabledAckCode, "Unexpected ack code for enabled");
                Assert.AreEqual(Enabled.Enabled, GetEnabled(Enabled.Enabled));
            });
        }


        [Test]
        [TestCase("http://example1.com", false, "http://example1.com", Enabled.Disabled)]
        [TestCase("http://example2.com", true, "http://example2.com", Enabled.Enabled)]
        [TestCase("", false, "", Enabled.Disabled)]
        public void ZtsiTest_Configuration(string serviceUrl, bool enabled, string expectedServiceUrl, Enabled expectedEnabled)
        {
            (int serviceUrl, int enabled) ackCode = SetZtsiConfiguration(serviceUrl, enabled);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ACK_SUCCESS, ackCode.serviceUrl, "Unexpected ack code for service url");
                Assert.AreEqual(ACK_SUCCESS, ackCode.enabled, "Unexpected ack code for enabled");
                Assert.AreEqual(expectedServiceUrl, GetServiceUrl(expectedServiceUrl));
                Assert.AreEqual(expectedEnabled, GetEnabled(expectedEnabled));
            });
        }

        [Test]
        public void ZtsiTest_InvalidConfiguration()
        {
            (int serviceUrl, int enabled) setupAckCode = SetZtsiConfiguration("http://test.com", true);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ACK_SUCCESS, setupAckCode.serviceUrl, "Unexpected ack code for service url during setup");
                Assert.AreEqual(ACK_SUCCESS, setupAckCode.enabled, "Unexpected ack code for enabled during setup");
            });

            (int serviceUrl, int enabled) ackCode = SetZtsiConfiguration("", true);

            (string serviceUrl, Enabled enabled) = GetZtsiConfiguration("http://test.com", Enabled.Enabled);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ACK_SUCCESS, ackCode.serviceUrl, "Unexpected ack code for service url");
                Assert.AreEqual(ACK_SUCCESS, ackCode.enabled, "Unexpected ack code for enabled");
                Assert.AreEqual("http://test.com", serviceUrl);
                Assert.AreEqual(Enabled.Enabled, enabled);
            });
        }
    }
}