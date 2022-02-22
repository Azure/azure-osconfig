// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.Azure.Devices;
using Microsoft.Azure.Devices.Shared;
using NUnit.Framework;
using System;
using System.Text.Json;
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

        private readonly string _sasToken = Environment.GetEnvironmentVariable("E2E_OSCONFIG_SAS_TOKEN");
        private readonly string _uploadUrl = Environment.GetEnvironmentVariable("E2E_OSCONFIG_UPLOAD_URL")?.Trim('\"');
        private readonly string _resourceGroupName = Environment.GetEnvironmentVariable("E2E_OSCONFIG_RESOURCE_GROUP_NAME")?.Trim('\"');

        // TODO: use this in Get/Set to simplify and reduce repeated arguments
        // public abstract string componentName { get; }

        public partial class GenericResponse<T>
        {
            public T value { get; set; }
            public int ac { get; set; }
        }

        [OneTimeSetUp]
        public void OneTimeSetUp()
        {
            _registryManager = RegistryManager.CreateFromConnectionString(_iotHubConnectionString);

            if ((null == _sasToken) || (null == _uploadUrl) || (null == _resourceGroupName))
            {
                Assert.Warn("Missing environment vars required for log upload to blob store");
            }
        }

        [OneTimeTearDown]
        public void OneTimeTearDown()
        {
            if ((null == _sasToken) || (null == _uploadUrl) || (null == _resourceGroupName))
            {
                // TODO: Upload logs to blobstore
            }
        }

        protected async Task<int> SetDesired<T>(string componentName, T value, int ackCode = 200, int maxWaitSeconds = 90)
        {
            Twin twin = await _registryManager.GetTwinAsync(_deviceId, _moduleId);

            var twinPatch = new Twin();
            twinPatch.Properties.Desired[componentName] = value;

            DateTime beforeUpdate = DateTime.Now;
            Twin updatedTwin = await _registryManager.UpdateTwinAsync(_deviceId, _moduleId, twinPatch, twin.ETag);
            TwinCollection reported = updatedTwin.Properties.Reported;

            if (!updatedTwin.Properties.Desired.Contains(componentName))
            {
                Assert.Warn("{0} not found in desired properties", componentName);
            }

            DateTime currentTime = DateTime.Now;
            while (!reported.Contains(componentName) ||
                   (reported.Contains(componentName) && (reported.GetLastUpdated() < beforeUpdate) && (reported[componentName].GetLastUpdated() < beforeUpdate)))
            {
                currentTime = DateTime.Now;
                if ((currentTime - beforeUpdate).TotalSeconds < maxWaitSeconds)
                {
                    await Task.Delay(1000);
                    updatedTwin = await _registryManager.GetTwinAsync(_deviceId, _moduleId);
                    reported = updatedTwin.Properties.Reported;
                }
                else
                {
                    Assert.Warn("Time limit reached while waiting for update to {0} (start: {2} | end: {3} | last updated: {4})", componentName, beforeUpdate, currentTime, reported[componentName].GetLastUpdated());
                }
            }

            if (!reported.Contains(componentName))
            {
                Assert.Warn("{0} not found in reported properties", componentName);
            }

            GenericResponse<T> response = JsonSerializer.Deserialize<GenericResponse<T>>(reported[componentName].ToString());
            return response.ac;
        }

        protected async Task<int> SetDesired<T>(string componentName, string objectName, T value, int ackCode = 200, int maxWaitSeconds = 90)
        {
            Twin twin = await _registryManager.GetTwinAsync(_deviceId, _moduleId);

            var twinPatch = new Twin();
            twinPatch.Properties.Desired[componentName] = new { __t = 'c' };
            twinPatch.Properties.Desired[componentName][objectName] = Newtonsoft.Json.Linq.JToken.FromObject(value);

            DateTime beforeUpdate = DateTime.Now;
            await Task.Delay(100);

            Twin updatedTwin = await _registryManager.UpdateTwinAsync(_deviceId, _moduleId, twinPatch, twin.ETag);
            TwinCollection reported = updatedTwin.Properties.Reported;

            if (!updatedTwin.Properties.Desired.Contains(componentName) && !updatedTwin.Properties.Desired[componentName].Contains(objectName))
            {
                Assert.Warn("{0}.{1} not found in desired properties", componentName, objectName);
            }

            DateTime currentTime = DateTime.Now;
            while (((!reported.Contains(componentName) && !reported[componentName].Contains(objectName)) ||
                    (reported.Contains(componentName) && reported[componentName].Contains(objectName) && (reported[componentName][objectName].GetLastUpdated() < beforeUpdate))))
            {
                currentTime = DateTime.Now;
                if ((currentTime - beforeUpdate).TotalSeconds < maxWaitSeconds)
                {
                    await Task.Delay(1000);
                    updatedTwin = await _registryManager.GetTwinAsync(_deviceId, _moduleId);
                    reported = updatedTwin.Properties.Reported;
                }
                else
                {
                    Assert.Warn("Time limit reached while waiting for update to {0}.{1} (start: {2} | end: {3} | last updated: {4})", componentName, objectName, beforeUpdate, currentTime, reported[componentName][objectName].GetLastUpdated());
                }
            }

            if (!reported.Contains(componentName) && !reported[componentName].Contains(objectName))
            {
                Assert.Fail("{0}.{1} not found in reported properties", componentName, objectName);
            }

            GenericResponse<T> response = JsonSerializer.Deserialize<GenericResponse<T>>(reported[componentName][objectName].ToString());
            Console.WriteLine(reported[componentName][objectName].ToString());
            return response.ac;
        }

        protected async Task<T> GetReported<T>(string componentName)
        {
            Twin twin = await _registryManager.GetTwinAsync(_deviceId, _moduleId);
            TwinCollection reported = twin.Properties.Reported;

            if (reported.Contains(componentName))
            {
                return JsonSerializer.Deserialize<T>(reported[componentName].ToString());
            }
            else
            {
                return default(T);
            }
        }

        protected async Task<T> GetReported<T>(string componentName, string objectName)
        {
            Twin twin = await _registryManager.GetTwinAsync(_deviceId, _moduleId);
            TwinCollection reported = twin.Properties.Reported;

            if (reported.Contains(componentName) && reported[componentName].Contains(objectName))
            {
                return JsonSerializer.Deserialize<T>(reported[componentName][objectName].ToString());
            }
            else
            {
                return default(T);
            }
        }

        protected async Task<T> GetReported<T>(string componentName, Func<T, bool> condition, int maxWaitSeconds = 90)
        {
            T reported = await GetReported<T>(componentName);
            DateTime start = DateTime.Now;

            while (!condition(reported) && (DateTime.Now - start).TotalSeconds < maxWaitSeconds)
            {
                await Task.Delay(1000);
                reported = await GetReported<T>(componentName);
            }

            return reported;
        }

        protected async Task<T> GetReported<T>(string componentName, string objectName, Func<T, bool> condition, int maxWaitSeconds = 90)
        {
            T reported = await GetReported<T>(componentName, objectName);
            DateTime start = DateTime.Now;

            while (((null == reported) || !condition(reported)) && (DateTime.Now - start).TotalSeconds < maxWaitSeconds)
            {
                await Task.Delay(1000);
                reported = await GetReported<T>(componentName, objectName);
            }

            return reported;
        }

        public static bool JsonEq(object expected, object actual)
        {
            var expectedJson = JsonSerializer.Serialize(expected);
            var actualJson = JsonSerializer.Serialize(actual);
            return expectedJson == actualJson;
        }

        public static void AssertJsonEq(object expected, object actual)
        {
            var expectedJson = JsonSerializer.Serialize(expected);
            var actualJson = JsonSerializer.Serialize(actual);
            Assert.AreEqual(expectedJson, actualJson);
        }
    }

    public class CommandRunnerTests : E2ETest
    {
        public enum Action
        {
            None = 0,
            Reboot,
            Shutdown,
            RunCommand,
            RefreshCommandStatus,
            CancelCommand
        }

        public enum CommandState
        {
            Unknown = 0,
            Running,
            Succeeded,
            Failed,
            TimedOut,
            Canceled
        }

        public partial class CommandArguments
        {
            public string CommandId { get; set; }
            public string Arguments { get; set; }
            public Action Action { get; set; }
            public int Timeout { get; set; }
            public bool SingleLineTextResult { get; set; }
        }

        public partial class CommandStatus
        {
            public string CommandId { get; set; }
            public long ResultCode { get; set; }
            public string TextResult { get; set; }
            public CommandState CurrentState { get; set; }
        }

        public CommandArguments CreateCommand(string arguments, Action action = Action.RunCommand, int timeout = 0, bool singleLineTextResult = false)
        {
            return CreateCommand(CreateCommandId(), arguments, action, timeout, singleLineTextResult);
        }

        public CommandArguments CreateCommand(string commandId, string arguments, Action action = Action.RunCommand, int timeout = 0, bool singleLinetextResult = false)
        {
            return new CommandArguments
            {
                CommandId = commandId,
                Arguments = arguments,
                Action = action,
                Timeout = timeout,
                SingleLineTextResult = singleLinetextResult
            };
        }

        public CommandArguments CreateLongRunningCommand(string commandId, int timeout = 120)
        {
            return new CommandArguments
            {
                CommandId = commandId,
                Arguments = "sleep 1000s",
                Action = Action.RunCommand,
                Timeout = timeout
            };
        }

        public CommandArguments CreateCancelCommand(string commandId)
        {
            return new CommandArguments
            {
                CommandId = commandId,
                Arguments = "",
                Action = Action.CancelCommand,
                Timeout = 0
            };
        }

        public CommandStatus CreateCommandStatus(string commandId, string textResult = "", CommandState commandState = CommandState.Succeeded, int resultCode = 0)
        {
            return new CommandStatus
            {
                CommandId = commandId,
                TextResult = textResult,
                CurrentState = commandState,
                ResultCode = resultCode
            };
        }

        public string CreateCommandId()
        {
            return Convert.ToBase64String(Guid.NewGuid().ToByteArray()).Substring(0, 4);
        }

        public void SendCommand(CommandArguments command, int ackCode = 200)
        {
            Console.WriteLine($"{command.Action} \"{command.CommandId}\" ({command.Arguments})");
            var setDesiredTask = SetDesired<CommandArguments>("CommandRunner", "CommandArguments", command);

            try
            {
                setDesiredTask.Wait();
                int responseCode = setDesiredTask.Result;
                if (responseCode != ackCode)
                {
                    Assert.Fail("CommandRunner.CommandArguments expected ackCode {0}, but got {1}", ackCode, responseCode);
                }
            }
            catch (Exception e)
            {
                Assert.Fail("Failed to send command: {0}", e.Message);
            }
        }

        public void CancelCommand(string commandId)
        {
            SendCommand(CreateCancelCommand(commandId));
        }

        public void RefreshCommandStatus(string commandId, int ackCode = 200)
        {
            var refreshCommand = new CommandArguments
            {
                CommandId = commandId,
                Arguments = "",
                Action = Action.RefreshCommandStatus
            };

            SendCommand(refreshCommand, ackCode);
        }

        public CommandStatus WaitForStatus(string commandId, CommandState state, int maxWaitSeconds = 90)
        {
            Func<CommandStatus, bool> condition = (CommandStatus status) => (status.CommandId == commandId) && (status.CurrentState == state);
            var reportedTask = GetReported<CommandStatus>("CommandRunner", "CommandStatus", condition, maxWaitSeconds);
            reportedTask.Wait();
            return reportedTask.Result;
        }

        [Test]
        [TestCase("echo 'hello world'", 0, false, 0, "hello world\n", CommandState.Succeeded)]
        [TestCase("sleep 10s", 1, true, 62, "", CommandState.TimedOut)]
        [TestCase("sleep 10s", 60, true, 0, "", CommandState.Succeeded)]
        [TestCase("echo 'single\nline'", 0, true, 0, "single line ", CommandState.Succeeded)]
        [TestCase("echo 'multiple\nlines'", 0, false, 0, "multiple\nlines\n", CommandState.Succeeded)]
        [TestCase("blah", 0, false, 127, "sh: 1: blah: not found\n", CommandState.Failed)]
        public void CommandRunnerTest_RunCommand(string arguments, int timeout, bool singleLineTextResult, int resultCode, string textResult, CommandState state)
        {
            var command = CreateCommand(arguments, Action.RunCommand, timeout, singleLineTextResult);
            SendCommand(command);

            CommandStatus status = WaitForStatus(command.CommandId, state);
            AssertJsonEq(CreateCommandStatus(command.CommandId, textResult, state, resultCode), status);
        }

        [Test]
        public void CommandRunnerTest_CancelCommand()
        {
            var commandId = CreateCommandId();
            var command = CreateLongRunningCommand(commandId);

            SendCommand(command);
            CommandStatus runningStatus = WaitForStatus(commandId, CommandState.Running);
            AssertJsonEq(CreateCommandStatus(commandId, "", CommandState.Running, 0), runningStatus);

            CancelCommand(command.CommandId);
            CommandStatus canceledStatus = WaitForStatus(commandId, CommandState.Canceled);
            AssertJsonEq(CreateCommandStatus(commandId, "", CommandState.Canceled, 125), canceledStatus);
        }

        [Test]
        public void CommandRunnerTest_RepeatCommandId()
        {
            var commandId = CreateCommandId();
            var command = CreateCommand(commandId, "echo 'command 1'", Action.RunCommand, 0, true);
            var commandWithDuplicateCommandId = CreateCommand(commandId, "echo 'command 2'", Action.RunCommand, 0, true);

            SendCommand(command);
            var commandStatus = WaitForStatus(commandId, CommandState.Succeeded);
            AssertJsonEq(CreateCommandStatus(commandId, "command 1 ", CommandState.Succeeded, 0), commandStatus);

            // Send a command with a duplicate command Id
            SendCommand(commandWithDuplicateCommandId, 400);
        }

        [Test]
        public void CommandRunnerTest_CommandSequence()
        {
            var command1 = CreateLongRunningCommand(CreateCommandId());
            var command2 = CreateCommand("echo 'command 2'");
            var command3 = CreateCommand("sleep 10s && echo 'command 3'", timeout: 1);
            var command4 = CreateCommand("blah");

            var expectedCommandStatus1 = CreateCommandStatus(command1.CommandId, "", CommandState.Canceled, 125);
            var expectedCommandStatus2 = CreateCommandStatus(command2.CommandId, "command 2\n", CommandState.Succeeded, 0);
            var expectedCommandStatus3 = CreateCommandStatus(command3.CommandId, "", CommandState.TimedOut, 62);
            var expectedCommandStatus4 = CreateCommandStatus(command4.CommandId, "sh: 1: blah: not found\n", CommandState.Failed, 127);

            SendCommand(command1);
            SendCommand(command2);
            SendCommand(command3);
            SendCommand(command4);
            SendCommand(CreateCancelCommand(command1.CommandId));

            // Wait for the last command to complete before checking all command statuses
            CommandStatus actualCommandStatus4 = WaitForStatus(command4.CommandId, CommandState.Failed);

            RefreshCommandStatus(command1.CommandId);
            CommandStatus actualCommandStatus1 = WaitForStatus(command1.CommandId, CommandState.Succeeded);

            RefreshCommandStatus(command2.CommandId);
            CommandStatus actualCommandStatus2 = WaitForStatus(command2.CommandId, CommandState.Canceled);

            RefreshCommandStatus(command3.CommandId);
            CommandStatus actualCommandStatus3 = WaitForStatus(command3.CommandId, CommandState.TimedOut);

            Assert.Multiple(() =>
            {
                AssertJsonEq(expectedCommandStatus1, actualCommandStatus1);
                AssertJsonEq(expectedCommandStatus2, actualCommandStatus2);
                AssertJsonEq(expectedCommandStatus3, actualCommandStatus3);
                AssertJsonEq(expectedCommandStatus4, actualCommandStatus4);
            });
        }
    }
}