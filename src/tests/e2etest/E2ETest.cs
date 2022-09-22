// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Serialization;
using NUnit.Framework;
using System;
using System.Text.RegularExpressions;

namespace E2eTesting
{
    [TestFixture]
    public abstract class E2ETest
    {
        public enum DataSourceType
        {
            IotHub,
            Local
        }

        private AbstractDataSource _iotHubDataSource;
        private AbstractDataSource _localDataSource;

        public static string GenerateId()
        {
            return Convert.ToBase64String(Guid.NewGuid().ToByteArray()).Substring(0, 4);
        }

        [OneTimeSetUp]
        public void OneTimeSetUp()
        {
            JsonConvert.DefaultSettings = () => new JsonSerializerSettings
            {
                ContractResolver = new CamelCasePropertyNamesContractResolver(),
                NullValueHandling = NullValueHandling.Ignore
            };

            _iotHubDataSource = new IotHubDataSource();
            _iotHubDataSource.Initialize();
            _localDataSource = new LocalDataSource();
            _localDataSource.Initialize();
        }

        public AbstractDataSource GetDataSource(DataSourceType dataSourceType)
        {
            switch (dataSourceType)
            {
                case DataSourceType.IotHub:
                    return _iotHubDataSource;
                case DataSourceType.Local:
                    return _localDataSource;
                default:
                    throw new ArgumentException("Invalid data source type");
            }
        }

        public bool SetDesired<T>(string componentName, string objectName, T value, int maxWaitSeconds, DataSourceType dataSourceType = DataSourceType.IotHub)
        {
            return GetDataSource(dataSourceType).SetDesired(componentName, objectName, value, maxWaitSeconds);
        }
        public bool SetDesired<T>(string componentName, string objectName, T value, DataSourceType dataSourceType = DataSourceType.IotHub)
        {
            return GetDataSource(dataSourceType).SetDesired(componentName, objectName, value);
        }
        public bool SetDesired<T>(string componentName, T value, int maxWaitSeconds, DataSourceType dataSourceType = DataSourceType.IotHub)
        {
            return GetDataSource(dataSourceType).SetDesired(componentName, value, maxWaitSeconds);
        }
        public bool SetDesired<T>(string componentName, T value, DataSourceType dataSourceType = DataSourceType.IotHub)
        {
            return GetDataSource(dataSourceType).SetDesired(componentName, value);
        }

        public T GetReported<T>(string componentName, string objectName, Func<T, bool> condition, int maxWaitSeconds)
        {
            var value = _iotHubDataSource.GetReported<T>(componentName, objectName, condition, maxWaitSeconds);
            JsonAssert.AreEqual(value, _localDataSource.GetReported<T>(componentName, objectName, condition, maxWaitSeconds));
            return value;
        }
        public T GetReported<T>(string componentName, string objectName, Func<T, bool> condition)
        {
            var value = _iotHubDataSource.GetReported<T>(componentName, objectName, condition);
            JsonAssert.AreEqual(value, _localDataSource.GetReported<T>(componentName, objectName, condition));
            return value;
        }
        public T GetReported<T>(string componentName, Func<T, bool> condition, int maxWaitSeconds)
        {
            var value = _iotHubDataSource.GetReported<T>(componentName, condition, maxWaitSeconds);
            JsonAssert.AreEqual(value, _localDataSource.GetReported<T>(componentName, condition, maxWaitSeconds));
            return value;
        }
        public T GetReported<T>(string componentName, string objectName, int maxWaitSeconds)
        {
            var value = _iotHubDataSource.GetReported<T>(componentName, objectName, maxWaitSeconds);
            JsonAssert.AreEqual(value, _localDataSource.GetReported<T>(componentName, objectName, maxWaitSeconds));
            return value;
        }
        public T GetReported<T>(string componentName, Func<T, bool> condition)
        {
            var value = _iotHubDataSource.GetReported<T>(componentName, condition);;
            JsonAssert.AreEqual(value, _localDataSource.GetReported<T>(componentName, condition));
            return value;
        }
        public T GetReported<T>(string componentName, string objectName)
        {
            var value = _iotHubDataSource.GetReported<T>(componentName, objectName);
            JsonAssert.AreEqual(value, _localDataSource.GetReported<T>(componentName, objectName));
            return value;
        }
        public T GetReported<T>(string componentName, int maxWaitSeconds)
        {
            var value = _iotHubDataSource.GetReported<T>(componentName, maxWaitSeconds);
            JsonAssert.AreEqual(value, _localDataSource.GetReported<T>(componentName, maxWaitSeconds));
            return value;
        }
        public T GetReported<T>(string componentName)
        {
            var value = _iotHubDataSource.GetReported<T>(componentName);
            JsonAssert.AreEqual(value, _localDataSource.GetReported<T>(componentName));
            return value;
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