// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using System;
using System.Linq;
using NUnit.Framework;
using Microsoft.Azure.Devices;
using Microsoft.Azure.Devices.Shared;
using Microsoft.Extensions.Logging;
using System.Threading.Tasks;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace E2eTesting
{
    [TestFixture]
    public class E2eTest
    {
        // Local testing: populate and uncomment string below
        // private string iotHubConnectionString = "HostName=<iot-hub>.azure-devices.net;SharedAccessKeyName=iothubowner;SharedAccessKey=<secret>";
        private string iotHubConnectionString = Environment.GetEnvironmentVariable("E2E_OSCONFIG_IOTHUB_CONNSTR");
        // Local testing: populate and uncomment the string below
        // private string _deviceId = "ubuntu2004";
        private string deviceId = Environment.GetEnvironmentVariable("E2E_OSCONFIG_DEVICE_ID");
        private readonly string moduleId = "osconfig";
        protected int twinTimeoutSeconds = 0;
        private int twinTimeoutSecondsDefault = 45;
        public readonly int twinRefreshIntervalMs = 2000;

        private ServiceClient _serviceClient;
        protected RegistryManager _registryManager;
        // vars needed to upload to blob store
        private string sas_token = null;
        private string upload_url = null;
        private string resource_group_name = null;

        private Twin twin;

        public String DeviceId
        {
            get { return deviceId; }
        }

        public void SetTwinTimeoutSeconds(int seconds)
        {
            twinTimeoutSeconds = seconds;
        }

        [OneTimeSetUp]
        public void Setup()
        {
            Console.WriteLine("[Setup] Setting up connection to IoTHub");

            if (null == iotHubConnectionString)
            {
                Assert.Fail("Missing required environment variable 'E2E_OSCONFIG_IOTHUB_CONNSTR'");

            }
            if (null == deviceId)
            {
                Assert.Fail("Missing required environment variable 'E2E_OSCONFIG_DEVICE_ID'");
            }

            sas_token = Environment.GetEnvironmentVariable("E2E_OSCONFIG_SAS_TOKEN");
            upload_url = Environment.GetEnvironmentVariable("E2E_OSCONFIG_UPLOAD_URL")?.Trim('\"');
            resource_group_name = Environment.GetEnvironmentVariable("E2E_OSCONFIG_RESOURCE_GROUP_NAME")?.Trim('\"');

            int.TryParse(Environment.GetEnvironmentVariable("E2E_OSCONFIG_TWIN_TIMEOUT"), out twinTimeoutSeconds);
            if (twinTimeoutSeconds == 0)
            {
                twinTimeoutSeconds = twinTimeoutSecondsDefault;
            }

            if (!UploadLogsToBlobStore())
            {
                Assert.Warn("Missing environment vars required for log upload to blob store");
            }

            iotHubConnectionString = iotHubConnectionString.TrimStart('"');
            iotHubConnectionString = iotHubConnectionString.TrimEnd('"');
            _serviceClient = ServiceClient.CreateFromConnectionString(iotHubConnectionString);
            _registryManager = RegistryManager.CreateFromConnectionString(iotHubConnectionString);
            GetNewTwin();
        }

        [OneTimeTearDown]
        public void TearDown()
        {
            Console.WriteLine("[TearDown] UploadLogsToBlobStore():{0}", UploadLogsToBlobStore());
            if (UploadLogsToBlobStore())
            {
                string fileName = String.Format("{0}-{1}.tar.gz", resource_group_name, deviceId);
                string fullURL = String.Format("{0}{1}?{2}", upload_url, fileName, sas_token);

                string tempFileCommand = "temp_file=`sudo mktemp`";
                string tarCommand = "sudo tar -cvzf $temp_file osconfig*.log";
                string curlCommand = String.Format("curl -X PUT -T $temp_file -H \"x-ms-date: $(date -u)\" -H \"x-ms-blob-type: BlockBlob\" \"{1}\"", fileName, fullURL);
                string commandToPerform = String.Format("cd /var/log && {0} && {1} && {2}", tempFileCommand, tarCommand, curlCommand);

                var result = PerformCommandViaCommandRunner(commandToPerform);
                if (!result.Item1)
                {
                    Assert.Warn("Failed to upload logs to blob storage");
                }
            }
        }

        /// <summary>
        /// Retreives the current twin (local)
        /// </summary>
        /// <returns>The current twin</returns>
        public Twin GetTwin()
        {
            return twin == null ? GetNewTwin() : twin;
        }

        /// <summary>
        /// Retreives a new Twin from the hub
        /// </summary>
        /// <returns>The new twin</returns>
        public Twin GetNewTwin()
        {
            Task<Twin> newTwin = _registryManager.GetTwinAsync(DeviceId, moduleId);
            newTwin.Wait();
            twin = newTwin.Result;
            return twin;
        }

        /// <summary>
        /// Updates the desired property contained in the Twin and blocks until the agent has reported
        /// </summary>
        /// <param name="patch">Patch containing the twin changes</param>
        /// <returns>True if the update succeeds and false if there is a failure</returns>
        public bool UpdateTwinBlockUntilUpdate(Twin patch)
        {
            var beforeUpdate = DateTime.UtcNow;
            var twinTask = _registryManager.UpdateTwinAsync(DeviceId, moduleId, patch, twin.ETag);
            twinTask.Wait();
            twin = twinTask.Result;

            var patchJson = Newtonsoft.Json.Linq.JObject.Parse(patch.Properties.Desired.ToJson());
            if (patchJson.Count == 0)
            {
                Assert.Fail("Invalid patch for twin");
            }
            var componentName = patchJson.Properties().First().Name;

            // If this is a new device (no reported properties will exist yet for that ComponentName)
            Console.WriteLine("[UpdateTwinBlockUntilUpdate] start:{0}", beforeUpdate);
            while ( ((IsComponentNameReported(componentName) && (twin.Properties.Reported[componentName].GetLastUpdated() < beforeUpdate)) 
                        || !IsComponentNameReported(componentName, false))
                     && ((DateTime.UtcNow - beforeUpdate).TotalSeconds < twinTimeoutSeconds))
            {
                // Keep polling twin until updated
                Console.WriteLine("[UpdateTwinBlockUntilUpdate] waiting for twin...");
                Task.Delay(twinRefreshIntervalMs).Wait();
            }
            bool timeout = (DateTime.UtcNow - beforeUpdate).TotalSeconds >= twinTimeoutSeconds;
            Console.WriteLine("[UpdateTwinBlockUntilUpdate] {0}! end:{1}, elapsed:{2} sec", timeout ? "Timeout" : "Success", DateTime.UtcNow, (DateTime.UtcNow - beforeUpdate).TotalSeconds);

            if (timeout)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Performs a remote operation on the target device using the CommandRunner module
        /// </summary>
        /// <param name="command">The command (and arguments) to execute</param>
        /// <returns>Tuple</returns>
        /// <exception cref="ArgumentNullException"></exception>
        public (bool, string) PerformCommandViaCommandRunner(string command)
        {
            if (null == command)
            {
                throw new ArgumentNullException("command");
            }

            var random = Convert.ToBase64String(Guid.NewGuid().ToByteArray()).Substring(0, 4);
            var desiredCommand = new CommandRunnerTests.CommandArguments
            {
                CommandId = random,
                Arguments = command,
                Action = CommandRunnerTests.Action.RunCommand,
                Timeout = 60,
                SingleLineTextResult = true
            };
            var desiredCommandRefresh = new CommandRunnerTests.CommandArguments
            {
                CommandId = random,
                Arguments = command,
                Action = CommandRunnerTests.Action.RefreshCommandStatus,
                Timeout = 60,
                SingleLineTextResult = true
            };

            var twinPatch = CreateCommandArgumentsPropertyPatch("CommandRunner", desiredCommand);

            if (!UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Assert.Fail("Timeout for updating twin - ActionRunCommand");
            }

            twinPatch = CreateCommandArgumentsPropertyPatch("CommandRunner", desiredCommandRefresh);

            if (UpdateTwinBlockUntilUpdate(twinPatch))
            {
                var deserializedObject = JsonSerializer.Deserialize<CommandRunnerTests.CommandStatus>(GetTwin().Properties.Reported["CommandRunner"]["CommandStatus"].ToString());
                // Wait until commandId is equivalent
                DateTime startTime = DateTime.Now;
                while(deserializedObject.CommandId != random && (DateTime.Now - startTime).TotalSeconds < 30)
                {
                    Console.WriteLine("[PerformCommandViaCommandRunner] waiting for commandId to be equivalent...");
                    Task.Delay(twinRefreshIntervalMs).Wait();
                    deserializedObject = JsonSerializer.Deserialize<CommandRunnerTests.CommandStatus>(GetNewTwin().Properties.Reported["CommandRunner"]["CommandStatus"].ToString());
                }

                if ((DateTime.Now - startTime).TotalSeconds >= 30)
                {
                    return (false,"");
                }

                if (deserializedObject.CurrentState != CommandRunnerTests.CommandState.Succeeded)
                {
                    Console.WriteLine("[PerformCommandViaCommandRunner] Unable to execute commandId:{0}", deserializedObject.CommandId);
                    return (false,"");
                }

                return (true, deserializedObject.TextResult);
            }
            else
            {
                // Timeout executing command
                return (false, "");
            }
        }

        protected static Twin CreateTwinPatch(string componentName, object propertyValue)
        {
            var twinPatch = new Twin();
            twinPatch.Properties.Desired[componentName] = propertyValue;
            return twinPatch;
        }

        protected static Twin CreateCommandArgumentsPropertyPatch(string componentName, object propertyValue)
        {
            var twinPatch = new Twin();
            twinPatch.Properties.Desired[componentName] = new
            {
                __t = "c",
                CommandArguments = propertyValue
            };
            return twinPatch;
        }
        public static void AreEqualByJson(object expected, object actual)
        {
            var expectedJson = JsonSerializer.Serialize(expected);
            var actualJson = JsonSerializer.Serialize(actual);
            Assert.AreEqual(expectedJson, actualJson);
        }

        public static bool IsSameByJson(object expected, object actual)
        {
            var expectedJson = JsonSerializer.Serialize(expected);
            var actualJson = JsonSerializer.Serialize(actual);
            return expectedJson == actualJson;
        }

        public static bool IsRegexMatch(Regex expected, object actual)
        {
            string actualJson = JsonSerializer.Serialize(actual);
            return expected.IsMatch(actualJson);
        }

        private bool IsComponentNameReported(string componentName, bool refreshTwin = true)
        {
            return refreshTwin ? GetNewTwin().Properties.Reported.Contains(componentName) : twin.Properties.Reported.Contains(componentName);
        }
        public bool UploadLogsToBlobStore()
        {
            return ((null != sas_token)  &&
                    (null != upload_url) &&
                    (null != resource_group_name));
        }
    }
}