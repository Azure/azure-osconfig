// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Serialization;
using NUnit.Framework;
using System;
using System.IO;
using System.Text;

namespace E2eTesting
{
    public class LocalDataSource : AbstractDataSource
    {
        private readonly string _reportedPath = "/etc/osconfig/osconfig_reported.json";
        private readonly string _desiredPath = "/etc/osconfig/osconfig_desired.json";
        private const int POLL_INTERVAL_MS = 1000;
        
        public override void Initialize()
        {
            if (!File.Exists(_reportedPath))
            {
                Assert.Fail("[LocalDataSource] reported path is missing: " + _reportedPath);
            }
            if (!File.Exists(_desiredPath))
            {
                using (FileStream fs = File.Create(_desiredPath))
                {
                    Byte[] info = new UTF8Encoding(true).GetBytes("{}");
                    fs.Write(info, 0, info.Length);
                }
            }
        }

        public override bool SetDesired<T>(string componentName, string objectName, T value, int maxWaitSeconds)
        {
            JObject local = JObject.Parse(File.ReadAllText(_desiredPath));

            string json = $@"{{
                ""{componentName}"": {{
                    ""{objectName}"": {JsonConvert.SerializeObject(value)}
                    }}
                }}";

            local.Merge(JObject.Parse(json), new JsonMergeSettings
                {
                    MergeArrayHandling = MergeArrayHandling.Union,
                }
            );
            File.WriteAllText(_desiredPath, local.ToString());
            return true;
        }

        public override T GetReported<T>(string componentName, string objectName, Func<T, bool> condition, int maxWaitSeconds)
        {
            DateTime start = DateTime.Now;
            JObject reported = JObject.Parse(File.ReadAllText(_reportedPath));

            Assert.IsTrue(reported.ContainsKey(componentName), "[LocalDataSource] does not contain component: " + componentName);
            if (String.IsNullOrEmpty(objectName))
            {
                while((maxWaitSeconds > 0 && (DateTime.Now - start).TotalSeconds < maxWaitSeconds)
                     && !(condition(reported[componentName].ToObject<T>())))
                {
                    System.Threading.Thread.Sleep(POLL_INTERVAL_MS);
                    reported = JObject.Parse(File.ReadAllText(_reportedPath));
                }

                if ((DateTime.Now - start).TotalSeconds > maxWaitSeconds)
                {
                    JObject desired = JObject.Parse(File.ReadAllText(_desiredPath));
                    Assert.Warn("[GetReported-LocalDataSource] Time limit reached while waiting for reported update for {0}.{1} (start: {2} | end: {3}). reported: {4}. desired {5}", componentName, objectName, start, DateTime.Now, reported[componentName], desired[componentName]);
                }

                return reported[componentName].ToObject<T>();
            }
            else
            {
                Assert.IsTrue(reported[componentName].ToObject<JObject>().ContainsKey(objectName));

                while((maxWaitSeconds > 0 && (DateTime.Now - start).TotalSeconds < maxWaitSeconds) 
                    && !(condition(reported[componentName][objectName].ToObject<T>())))
                {
                    System.Threading.Thread.Sleep(POLL_INTERVAL_MS);
                    reported = JObject.Parse(File.ReadAllText(_reportedPath));
                }

                if ((DateTime.Now - start).TotalSeconds > maxWaitSeconds)
                {
                    JObject desired = JObject.Parse(File.ReadAllText(_desiredPath));
                    Assert.Warn("[GetReported-LocalDataSource] Time limit reached while waiting for reported update for {0}.{1} (start: {2} | end: {3}). reported: {4}. desired {5}", componentName, objectName, start, DateTime.Now, reported[componentName][objectName], desired[componentName][objectName]);
                }

                return reported[componentName][objectName].ToObject<T>();
            }
        }
    }
}