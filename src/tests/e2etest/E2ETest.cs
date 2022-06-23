// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.Azure.Devices;
using Microsoft.Azure.Devices.Shared;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Serialization;
using NUnit.Framework;
using System;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace E2eTesting
{
    [TestFixture]
    public abstract class E2ETest
    {
        private RegistryManager _registryManager;

        private readonly string _iotHubConnectionString = Environment.GetEnvironmentVariable("E2E_OSCONFIG_IOTHUB_CONNSTR")?.Trim('"');
        private readonly string _moduleId = "osconfig";
        private readonly string _deviceId = Environment.GetEnvironmentVariable("E2E_OSCONFIG_DEVICE_ID");
        private readonly string _reportedPath = "/etc/osconfig/osconfig_reported.json";

        private const int POLL_INTERVAL_MS = 1000;
        private const int DEFAULT_MAX_WAIT_SECONDS = 90;
        private int _maxWaitTimeSeconds = DEFAULT_MAX_WAIT_SECONDS;

        protected const int ACK_SUCCESS = 200;
        protected const int ACK_ERROR = 400;

        public class GenericResponse<T>
        {
            public T Value { get; set; }
            public int Ac { get; set; }
        }

        public static string GenerateId()
        {
            return Convert.ToBase64String(Guid.NewGuid().ToByteArray()).Substring(0, 4);
        }

        [OneTimeSetUp]
        public void OneTimeSetUp()
        {
            JsonConvert.DefaultSettings = () => new JsonSerializerSettings
            {
                ContractResolver = new CamelCasePropertyNamesContractResolver()
            };

            _registryManager = RegistryManager.CreateFromConnectionString(_iotHubConnectionString);

            if (null != Environment.GetEnvironmentVariable("E2E_OSCONFIG_TWIN_TIMEOUT"))
            {
                _maxWaitTimeSeconds = int.TryParse(Environment.GetEnvironmentVariable("E2E_OSCONFIG_TWIN_TIMEOUT"), out _maxWaitTimeSeconds) ? _maxWaitTimeSeconds : DEFAULT_MAX_WAIT_SECONDS;
                Console.WriteLine($"Setting max wait time for twin updates to {_maxWaitTimeSeconds} seconds");
            }
        }

        /// <summary>
        /// Execute a command via the CommandRunner. Commands are expected to succeed with exit code 0.
        /// </summary>
        /// <param name="arguments">The command to execute</param>
        /// <returns><c>true</c> if the command succeeded, <c>false</c> otherwise</returns>
        protected bool ExecuteCommandViaCommandRunner(string arguments)
        {
            bool success = true;
            var command = CommandRunnerTests.CreateCommand(arguments);

            try
            {
                var desiredResult = SetDesired<CommandRunnerTests.CommandArguments>("CommandRunner", "commandArguments", command);
                desiredResult.Wait();
                int ackCode = desiredResult.Result.Ac;

                if (ACK_SUCCESS != ackCode)
                {
                    Console.WriteLine("[ExecuteCommandViaCommandRunner] Failed to set desired state");
                    success = false;
                }
                else
                {
                    Func<CommandRunnerTests.CommandStatus, bool> condition = (CommandRunnerTests.CommandStatus status) =>
                    {
                        return (status.commandId == command.commandId) && (status.currentState == CommandRunnerTests.CommandState.Succeeded);
                    };

                    var reportedResult = GetReported<CommandRunnerTests.CommandStatus>("CommandRunner", "commandStatus", condition, 2 * _maxWaitTimeSeconds);
                    reportedResult.Wait();
                    var reportedStatus = reportedResult.Result;

                    if (!condition(reportedStatus))
                    {
                        Console.WriteLine("[ExecuteCommandViaCommandRunner] Command status not reported as succeeded for {0}: '{1}' {2} {3}", command.commandId, reportedStatus.commandId, reportedStatus.currentState, reportedStatus.textResult);
                        success = false;
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("[ExecuteCommandViaCommandRunner] Exception: {0}", e.Message);
                success = false;
            }

            return success;
        }

        private bool PropertyExists(TwinCollection twinCollection, string componentName)
        {
            return twinCollection.Contains(componentName);
        }

        private bool PropertyExists(TwinCollection twinCollection, string componentName, string objectName)
        {
            return twinCollection.Contains(componentName) && twinCollection[componentName].Contains(objectName);
        }

        private bool IsUpdated(TwinCollection twinCollection, string componentName, DateTime previousUpdate)
        {
            return PropertyExists(twinCollection, componentName) && (previousUpdate < twinCollection[componentName].GetLastUpdated());
        }

        private bool IsUpdated(TwinCollection twinCollection, string componentName, string objectName, DateTime previousUpdate)
        {
            return PropertyExists(twinCollection, componentName, objectName) && (previousUpdate < twinCollection[componentName][objectName].GetLastUpdated());
        }

        private async Task<TwinCollection> LastReported()
        {
            Twin twin = await _registryManager.GetTwinAsync(_deviceId, _moduleId);
            return twin.Properties.Reported;
        }

        private async Task<T> LastReported<T>(string componentName)
        {
            TwinCollection reported = await LastReported();
            return reported.Contains(componentName) ? Deserialize<T>(reported[componentName]) : default(T);
        }

        private async Task<T> LastReported<T>(string componentName, string objectName)
        {
            TwinCollection reported = await LastReported();
            return (reported.Contains(componentName) && reported[componentName].Contains(objectName)) ? Deserialize<T>(reported[componentName][objectName]) : default(T);
        }

        private T Deserialize<T>(TwinCollection twinCollection)
        {
            try
            {
                return JsonConvert.DeserializeObject<T>(twinCollection.ToString());
            }
            catch (Exception e)
            {
                Assert.Warn("[Deserialize] Exception: {0}", e.Message);
                return default(T);
            }
        }

        /// <summary>
        /// Validates the local reported object and fails if not in sync with the given reported object.
        /// </summary>
        /// <param name="reported">The reported twin to check the local twin against</param>
        /// <param name="componentName">The name of the component</param>
        /// <param name="objectName">The object name (Optional)</param>
        protected void ValidateLocalReported(Object reported, String componentName, String objectName = "")
        {
            if (File.Exists(_reportedPath))
            {
                JObject local = JObject.Parse(File.ReadAllText(_reportedPath));
                Assert.IsTrue(local.ContainsKey(componentName));
                JObject remote = JObject.FromObject(reported);
                if (String.IsNullOrEmpty(objectName))
                {
                    Assert.AreEqual(local[componentName], remote);
                }
                else
                {
                    Assert.IsTrue(local[componentName].ToObject<JObject>().ContainsKey(objectName));
                    Assert.AreEqual(local[componentName][objectName], remote);
                }
            }
            else
            {
                Assert.Fail("[LastReported] File does not exist: {0}, is Local Management enabled?", _reportedPath);
            }
        }

        /// <summary>
        /// Sets the desired value of the specified component and waits for a reported property update.
        /// </summary>
        /// <typeparam name="T">The type of the desired value</typeparam>
        /// <param name="componentName">The name of the component</param>
        /// <param name="desiredValue">The desired value</param>
        protected async Task SetDesired<T>(string componentName, T value, int maxWaitSeconds)
        {
            Twin twin = await _registryManager.GetTwinAsync(_deviceId, _moduleId);

            var twinPatch = new Twin();
            twinPatch.Properties.Desired[componentName] = value;

            DateTime start = DateTime.Now;
            Twin updatedTwin = await _registryManager.UpdateTwinAsync(_deviceId, _moduleId, twinPatch, twin.ETag);
            TwinCollection reported = updatedTwin.Properties.Reported;
            DateTime previousUpdate = PropertyExists(reported, componentName) ? reported[componentName].GetLastUpdated() : DateTime.MinValue;

            if (!updatedTwin.Properties.Desired.Contains(componentName))
            {
                Assert.Warn("[SetDesired] {0} not found in desired properties", componentName);
            }

            while (!PropertyExists(reported, componentName) && !IsUpdated(reported, componentName, previousUpdate))
            {
                if ((DateTime.Now - start).TotalSeconds < maxWaitSeconds)
                {
                    await Task.Delay(POLL_INTERVAL_MS);
                    updatedTwin = await _registryManager.GetTwinAsync(_deviceId, _moduleId);
                    reported = updatedTwin.Properties.Reported;
                }
                else
                {
                    Assert.Warn("[SetDesired] Time limit reached while waiting for desired update for {0} (start: {1} | end: {2} | last updated: {3})", componentName, start, DateTime.Now, reported[componentName].GetLastUpdated());
                    break;
                }
            }

            if (!reported.Contains(componentName))
            {
                Assert.Warn("[SetDesired] {0} not found in reported properties", componentName);
            }
        }

        protected Task SetDesired<T>(string componentName, T value)
        {
            return SetDesired<T>(componentName, value, _maxWaitTimeSeconds);
        }

        /// <summary>
        /// Sets the desired value of the specified object within a component and waits for a reported property update.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="componentName"></param>
        /// <param name="objectName"></param>
        /// <param name="value"></param>
        /// <param name="maxWaitSeconds"></param>
        /// <returns>The response containing the acknowledged proprty update</returns>
        protected async Task<GenericResponse<T>> SetDesired<T>(string componentName, string objectName, T value, int maxWaitSeconds)
        {
            Twin twin = await _registryManager.GetTwinAsync(_deviceId, _moduleId);

            var twinPatch = new Twin();
            twinPatch.Properties.Desired[componentName] = new { __t = 'c' };
            twinPatch.Properties.Desired[componentName][objectName] = JToken.FromObject(value);

            DateTime start = DateTime.Now;
            Twin updatedTwin = await _registryManager.UpdateTwinAsync(_deviceId, _moduleId, twinPatch, twin.ETag);
            TwinCollection reported = updatedTwin.Properties.Reported;
            DateTime previousUpdate = PropertyExists(reported, componentName, objectName) ? reported[componentName][objectName].GetLastUpdated() : DateTime.MinValue;

            if (!updatedTwin.Properties.Desired.Contains(componentName) && !updatedTwin.Properties.Desired[componentName].Contains(objectName))
            {
                Assert.Warn("[SetDesired] {0}.{1} not found in desired properties", componentName, objectName);
            }

            while (!PropertyExists(reported, componentName, objectName) || !IsUpdated(reported, componentName, objectName, previousUpdate))
            {
                if ((DateTime.Now - start).TotalSeconds < maxWaitSeconds)
                {
                    await Task.Delay(POLL_INTERVAL_MS);
                    reported = await LastReported();
                }
                else
                {
                    Assert.Warn("[SetDesired] Time limit reached while waiting for desired update for {0}.{1} (start: {2} | end: {3} | last updated: {4})", componentName, objectName, start, DateTime.Now, reported[componentName][objectName].GetLastUpdated());
                    break;
                }
            }

            if (!reported.Contains(componentName) || !reported[componentName].Contains(objectName))
            {
                Assert.Warn("[SetDesired] {0}.{1} not found in reported properties", componentName, objectName);
            }

            return Deserialize<GenericResponse<T>>(reported[componentName][objectName]);
        }

        protected Task<GenericResponse<T>> SetDesired<T>(string componentName, string objectName, T value)
        {
            return SetDesired<T>(componentName, objectName, value, _maxWaitTimeSeconds);
        }

        /// <summary>
        /// Gets the last reported value of the specified component according to the given condition callback.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="componentName"></param>
        /// <param name="condition"></param>
        /// <param name="maxWaitSeconds"></param>
        protected async Task<T> GetReported<T>(string componentName, Func<T, bool> condition, int maxWaitSeconds)
        {
            DateTime start = DateTime.Now;
            T reported = await LastReported<T>(componentName);

            while ((null == reported) || !condition(reported))
            {
                if ((DateTime.Now - start).TotalSeconds < maxWaitSeconds)
                {
                    await Task.Delay(POLL_INTERVAL_MS);
                    reported = await LastReported<T>(componentName);
                }
                else
                {
                    Assert.Warn("[GetReported] Time limit reached while waiting for reported update for {0} (start: {1} | end: {2})", componentName, start, DateTime.Now);
                    break;
                }
            }

            return reported;
        }

        protected Task<T> GetReported<T>(string componentName, Func<T, bool> condition)
        {
            return GetReported<T>(componentName, condition, _maxWaitTimeSeconds);
        }

        /// <summary>
        /// Gets the last reported value of the specified object within a component according to the given condition callback.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="componentName"></param>
        /// <param name="objectName"></param>
        /// <param name="condition"></param>
        /// <param name="maxWaitSeconds"></param>
        protected async Task<T> GetReported<T>(string componentName, string objectName, Func<T, bool> condition, int maxWaitSeconds)
        {
            DateTime start = DateTime.Now;
            T reported = await LastReported<T>(componentName, objectName);

            while ((null == reported) || !condition(reported))
            {
                if ((DateTime.Now - start).TotalSeconds < maxWaitSeconds)
                {
                    await Task.Delay(POLL_INTERVAL_MS);
                    reported = await LastReported<T>(componentName, objectName);
                }
                else
                {
                    Assert.Warn("[GetReported] Time limit reached while waiting for reported update for {0}.{1} (start: {2} | end: {3})", componentName, objectName, start, DateTime.Now);
                    break;
                }
            }

            return reported;
        }

        protected Task<T> GetReported<T>(string componentName, string objectName, Func<T, bool> condition)
        {
            return GetReported<T>(componentName, objectName, condition, _maxWaitTimeSeconds);
        }
    }

    public static class JsonAssert
    {
        public static void AreEqual(object expected, object actual)
        {
            var expectedJson = JsonConvert.SerializeObject(expected);
            var actualJson = JsonConvert.SerializeObject(actual);
            Assert.AreEqual(expectedJson, actualJson);
        }
    }

    public static class RegexAssert
    {
        public static void IsMatch(Regex pattern, object value)
        {
            string jsonValue = JsonConvert.SerializeObject(value);
            if (!pattern.IsMatch(jsonValue))
            {
                Assert.Fail("Regex does not match.\nPattern: {0},\n String: {1}", pattern, value);
            }
        }
    }
}