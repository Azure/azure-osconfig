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
using System.Threading.Tasks;

namespace E2eTesting
{
    public class IotHubDataSource : AbstractDataSource
    {
        private RegistryManager _registryManager;

        private readonly string _iotHubConnectionString = Environment.GetEnvironmentVariable("E2E_OSCONFIG_IOTHUB_CONNSTR")?.Trim('"');
        private readonly string _moduleId = "osconfig";
        private readonly string _deviceId = Environment.GetEnvironmentVariable("E2E_OSCONFIG_DEVICE_ID");

        private const int POLL_INTERVAL_MS = 1000;

        protected const int ACK_SUCCESS = 200;
        protected const int ACK_ERROR = 400;

        public class GenericResponse<T>
        {
            public T Value { get; set; }
            public int Ac { get; set; }
        }

        public override void Initialize()
        {
            _registryManager = RegistryManager.CreateFromConnectionString(_iotHubConnectionString);

            if (null != Environment.GetEnvironmentVariable("E2E_OSCONFIG_TWIN_TIMEOUT"))
            {
                _maxWaitTimeSeconds = int.TryParse(Environment.GetEnvironmentVariable("E2E_OSCONFIG_TWIN_TIMEOUT"), out _maxWaitTimeSeconds) ? _maxWaitTimeSeconds : DEFAULT_MAX_WAIT_SECONDS;
                Console.WriteLine($"Setting max wait time for twin updates to {_maxWaitTimeSeconds} seconds");
            }
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
            if (String.IsNullOrEmpty(objectName))
            {
                return await LastReported<T>(componentName);
            }
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

        public override bool SetDesired<T>(string componentName, string objectName, T value, int maxWaitSeconds)
        {
            if (String.IsNullOrEmpty(objectName))
            {
                SetDesiredInternal(componentName, value, maxWaitSeconds).Wait();
                return true;
            }
            else
            {
                Task<GenericResponse<T>> response = SetDesiredInternal(componentName, objectName, value, maxWaitSeconds);
                response.Wait();
                return response.Result.Ac == ACK_SUCCESS ? true : false;
            }
        }

        private async Task SetDesiredInternal<T>(string componentName, T value, int maxWaitSeconds)
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

        private async Task<GenericResponse<T>> SetDesiredInternal<T>(string componentName, string objectName, T value, int maxWaitSeconds)
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

        public override T GetReported<T>(string componentName, string objectName, Func<T, bool> condition, int maxWaitSeconds)
        {
            Task<T> reported = GetReportedInternal<T>(componentName, objectName, condition, maxWaitSeconds);
            reported.Wait();
            return reported.Result;
        }
        
        private async Task<T> GetReportedInternal<T>(string componentName, string objectName, Func<T, bool> condition, int maxWaitSeconds)
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
    }
}