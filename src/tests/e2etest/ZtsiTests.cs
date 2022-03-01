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
        private static readonly string _desiredEnabled = "DesiredEnabled";
        private static readonly string _desiredServiceUrl = "DesiredServiceUrl";

        private static readonly Regex _urlPattern = new Regex("((http|https)://)(www.)?[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|]");

        public enum Enabled
        {
            Unknown = 0,
            Enabled,
            Disabled
        }
        public partial class Ztsi
        {
            public Enabled Enabled { get; set; }
            public string ServiceUrl { get; set; }
        }

        public partial class DesiredZtsi
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
                return setDesiredTask.Result.ac;
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
                return setDesiredTask.Result.ac;
            }
            catch (Exception e)
            {
                Assert.Warn("Failed to set enabled: {0} ", e.Message);
                return -1;
            }
        }

        // TODO: doc string summary
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

                var desiredEnabledTask = GetReported<GenericResponse<bool>>(_componentName, _desiredEnabled, (GenericResponse<bool> response) => response.ac == ACK_SUCCESS);
                desiredEnabledTask.Wait();
                var desiredServiceUrlTask = GetReported<GenericResponse<string>>(_componentName, _desiredServiceUrl, (GenericResponse<string> response) => response.ac == ACK_SUCCESS);
                desiredServiceUrlTask.Wait();

                return (desiredServiceUrlTask.Result.ac, desiredEnabledTask.Result.ac);
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
                var reportedTask = GetReported<Ztsi>(_componentName, (Ztsi ztsi) => (ztsi.Enabled != expectedEnabled));
                reportedTask.Wait();
                return reportedTask.Result.Enabled;
            }
            catch (Exception e)
            {
                Assert.Warn("Failed to get enabled: {0} ", e.Message);
                return Enabled.Unknown;
            }
        }

        private void RemoveConfigurationFile()
        {
            try
            {
                ExecuteCommandViaCommandRunner("rm /etc/ztsi/config.json");
            }
            catch
            {
                // Ignore warnings
            }
        }

        [Test]
        [TestCase(true, "", true, "", ACK_SUCCESS, Enabled.Unknown, ACK_SUCCESS)]
        [TestCase(true, "", false, "", ACK_SUCCESS, Enabled.Disabled, ACK_SUCCESS)]
        [TestCase(true, "Invalid url", true, "", ACK_ERROR, Enabled.Unknown, ACK_SUCCESS)]
        [TestCase(true, "Invalid url", false, "", ACK_ERROR, Enabled.Disabled, ACK_SUCCESS)]
        [TestCase(true, "https://www.test.com/", true, "https://www.test.com/", ACK_SUCCESS, Enabled.Unknown, ACK_SUCCESS)]
        [TestCase(true, "https://www.test.com/", false, "https://www.test.com/", ACK_SUCCESS, Enabled.Disabled, ACK_SUCCESS)]
        [TestCase(false, "https://www.example.com/", true, "https://www.example.com/", ACK_SUCCESS, Enabled.Enabled, ACK_SUCCESS)]
        public void ZtsiTest_Enabled_ServiceUrl(bool removeConfigurationFile, string serviceUrl, bool enabled, string expectedServiceUrl, int expectedServiceUrlAckCode, Enabled expectedEnabled, int expectedEnabledAckCode)
        {
            if (removeConfigurationFile)
            {
                RemoveConfigurationFile();
            }

            int enabledAckCode = SetEnabled(enabled);
            int serviceUrlAckCode = SetServiceUrl(serviceUrl);
            (string serviceUrl, Enabled enabled) reportedConfiguration = GetZtsiConfiguration(expectedServiceUrl, expectedEnabled);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(expectedEnabledAckCode, enabledAckCode, "Unexpected ack code for setting enabled");
                Assert.AreEqual(expectedServiceUrlAckCode, serviceUrlAckCode, "Unexpected ack code for setting service url");
                Assert.AreEqual(expectedEnabled, reportedConfiguration.enabled);
                Assert.AreEqual(expectedServiceUrl, reportedConfiguration.serviceUrl);
            });
        }

        [Test]
        [TestCase(true, "", true, "", ACK_SUCCESS, Enabled.Unknown, ACK_SUCCESS)]
        [TestCase(true, "", false, "", ACK_SUCCESS, Enabled.Unknown, ACK_SUCCESS)]
        [TestCase(true, "Invalid url", true, "", ACK_ERROR, Enabled.Unknown, ACK_SUCCESS)]
        [TestCase(true, "Invalid url", false, "", ACK_ERROR, Enabled.Unknown, ACK_SUCCESS)]
        [TestCase(true, "https://www.test.com/", true, "https://www.test.com/", ACK_SUCCESS, Enabled.Enabled, ACK_SUCCESS)]
        [TestCase(true, "https://www.test.com/", false, "https://www.test.com/", ACK_SUCCESS, Enabled.Disabled, ACK_SUCCESS)]
        [TestCase(false, "https://www.example.com/", true, "https://www.example.com/", ACK_SUCCESS, Enabled.Enabled, ACK_SUCCESS)]
        public void ZtsiTest_ServiceUrl_Enabled(bool removeConfigurationFile, string serviceUrl, bool enabled, string expectedServiceUrl, int expectedServiceUrlAckCode, Enabled expectedEnabled, int expectedEnabledAckCode)
        {
            if (removeConfigurationFile)
            {
                RemoveConfigurationFile();
            }

            int enabledAckCode = SetServiceUrl(serviceUrl);
            string reportedServiceUrl = GetServiceUrl(expectedServiceUrl);
            int serviceUrlAckCode = SetEnabled(enabled);
            Enabled reportedEnabled = GetEnabled(expectedEnabled);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(expectedServiceUrlAckCode, serviceUrlAckCode, "Unexpected ack code for setting service url");
                Assert.AreEqual(expectedEnabledAckCode, enabledAckCode, "Unexpected ack code for setting enabled");
                Assert.AreEqual(expectedServiceUrl, reportedServiceUrl);
                Assert.AreEqual(expectedEnabled, reportedEnabled);
            });
        }

        [Test]
        [TestCase(true, "", true, "", ACK_ERROR, Enabled.Unknown, ACK_SUCCESS)]
        [TestCase(true, "", false, "", ACK_SUCCESS, Enabled.Unknown, ACK_SUCCESS)]
        [TestCase(true, "Invalid url", true, "", ACK_ERROR, Enabled.Unknown, ACK_SUCCESS)]
        [TestCase(true, "Invalid url", false, "", ACK_ERROR, Enabled.Disabled, ACK_SUCCESS)]
        [TestCase(true, "https://www.test.com/", true, "https://www.test.com/", ACK_SUCCESS, Enabled.Enabled, ACK_SUCCESS)]
        [TestCase(true, "https://www.test.com/", false, "https://www.test.com/", ACK_SUCCESS, Enabled.Disabled, ACK_SUCCESS)]
        [TestCase(false, "https://www.example.com/", true, "https://www.example.com/", ACK_SUCCESS, Enabled.Enabled, ACK_SUCCESS)]
        public void ZtsiTest_Configuration(bool removeConfigurationFile, string serviceUrl, bool enabled, string expectedServiceUrl, int expectedServiceUrlAckCode, Enabled expectedEnabled, int expectedEnabledAckCode)
        {
            if (removeConfigurationFile)
            {
                RemoveConfigurationFile();
            }

            (int serviceUrl, int enabled) ackCode = SetZtsiConfiguration(serviceUrl, enabled);
            (string serviceUrl, Enabled enabled) configuration = GetZtsiConfiguration(expectedServiceUrl, expectedEnabled);

            Assert.Multiple(() =>
            {
                Assert.AreEqual(expectedServiceUrlAckCode, ackCode.serviceUrl, "Unexpected ack code for setting service url");
                Assert.AreEqual(expectedEnabledAckCode, ackCode.enabled, "Unexpected ack code for setting enabled");
                Assert.AreEqual(expectedServiceUrl, configuration.serviceUrl);
                Assert.AreEqual(expectedEnabled, configuration.enabled);
            });
        }
    }
}