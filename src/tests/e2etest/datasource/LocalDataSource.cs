// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Serialization;
using NUnit.Framework;
using System;
using System.IO;

namespace E2eTesting
{
    public class LocalDataSource : AbstractDataSource
    {
        private readonly string _reportedPath = "/etc/osconfig/osconfig_reported.json";
        private readonly string _desiredPath = "/etc/osconfig/osconfig_desired.json";
        public override void Initialize()
        {
            if (!File.Exists(_reportedPath))
            {
                Assert.Fail("[LocalDataSource] reported path is missing: ", _reportedPath);
            }
        }

        public override bool SetDesired<T>(string componentName, string objectName, T value, int maxWaitSeconds)
        {
            JObject local = JObject.Parse(File.ReadAllText(_desiredPath));
            if (String.IsNullOrEmpty(objectName))
            {
                local[componentName] = JObject.FromObject(value);
            }
            else
            {
                local[componentName][objectName] = JObject.FromObject(value);
            }
            File.WriteAllText(_desiredPath, local.ToString());
            return true;
        }

        public override T GetReported<T>(string componentName, string objectName, Func<T, bool> condition, int maxWaitSeconds)
        {
            JObject local = JObject.Parse(File.ReadAllText(_reportedPath));
            Assert.IsTrue(local.ContainsKey(componentName), "[LocalDataSource] does not contain component: " + componentName);
            if (String.IsNullOrEmpty(objectName))
            {
                return local[componentName].ToObject<T>();
            }
            else
            {
                Assert.IsTrue(local[componentName].ToObject<JObject>().ContainsKey(objectName));
                return local[componentName][objectName].ToObject<T>();
            }
        }
    }
}