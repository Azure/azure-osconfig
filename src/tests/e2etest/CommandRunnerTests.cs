// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using Microsoft.Azure.Devices.Shared;
using System.Text.Json;
using System;
using System.Collections;
using System.Threading.Tasks;

namespace E2eTesting
{
    [TestFixture, Category("CommandRunner")]
    public class CommandRunnerTests : E2eTest
    {
        const string ComponentName = "CommandRunner";
        const string PropertyCommandStatus = "CommandStatus";
        const string PerpertyCommandArguments = "CommandArguments";
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
            TimeOut,
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

        public partial class ResponseCode
        {
            public int ac { get; set; }
        }

        const int responseStatusSuccess = 200;
        const int responseStatusFailed = 400;

        const string argumentsPingBing = "ping bing.com";
        const string argumentsEchoHelloWorld = "echo Hello World";
        const string helloWorld = "Hello World ";
        const string argumentsReboot = "reboot";

        public string GenerateCommandId()
        {
            return Convert.ToBase64String(Guid.NewGuid().ToByteArray()).Substring(0, 4);
        }
        public CommandArguments GenerateCommandArgumentsObject(string commandId, string arguments)
        {
            return new CommandArguments
            {
                CommandId = commandId,
                Arguments = arguments,
                Action = Action.RunCommand,
                Timeout = 60,
                SingleLineTextResult = true
            };
        }

        public CommandStatus GenerateCommandStatusObject(string commandId)
        {
            return new CommandStatus
            {
                CommandId = commandId,
                ResultCode = 0,
                TextResult = "",
                CurrentState = CommandState.Succeeded
            };
        }

        public (CommandArguments desiredCommand, CommandStatus expectedCommandStatus) GenerateLongRunningCommand()
        {
            string commandId = GenerateCommandId();
            CommandArguments desiredCommand = GenerateCommandArgumentsObject(commandId, argumentsPingBing);
            CommandStatus expectedCommandStatus = GenerateCommandStatusObject(commandId);
            expectedCommandStatus.CurrentState = CommandState.Running;
            return (desiredCommand, expectedCommandStatus);
        }

        public (CommandArguments desiredCommand, CommandStatus expectedCommandStatus) GenerateEchoHelloWorldCommand()
        {
            string commandId = GenerateCommandId();
            CommandArguments desiredCommand = GenerateCommandArgumentsObject(commandId, argumentsEchoHelloWorld);
            CommandStatus expectedCommandStatus = GenerateCommandStatusObject(commandId);
            expectedCommandStatus.TextResult = helloWorld;
            return (desiredCommand, expectedCommandStatus);
        }

        public (CommandArguments desiredCommand, CommandStatus expectedCommandStatus) GenerateCommand(string arguments)
        {
            string commandId = GenerateCommandId();
            CommandArguments desiredCommand = GenerateCommandArgumentsObject(commandId, arguments);
            CommandStatus expectedCommandStatus = GenerateCommandStatusObject(commandId);
            return (desiredCommand, expectedCommandStatus);
        }

        public void UpdateDesiredProperties(string ComponentName, CommandArguments desiredCommand)
        {
            Twin twinPatch = CreateCommandArgumentsPropertyPatch(ComponentName, desiredCommand);
            if (!UpdateTwinBlockUntilUpdate(twinPatch))
            {
                Assert.Fail("Timeout for updating twin - RunCommand");
            }
        }

        public void AreReportedPropertiesUpdated(Object expectedCommandStatus, int expectedResponseCode, bool ignoreTextResult)
        {
            Console.WriteLine("[CommandRunnerTests_AreReportedPropertiesUpdated] reported properties are not as expected");
            Assert.True(ReportedPropertiesUpdated(expectedCommandStatus, expectedResponseCode, ignoreTextResult));
        }

        public bool ReportedPropertiesUpdated(Object expectedCommandStatus, int expectedResponseCode, bool ignoreTextResult)
        {
            DateTime startTime = DateTime.Now;
            int timeoutSeconds = 30;
            bool timeOut = ((DateTime.Now - startTime).TotalSeconds > timeoutSeconds);
            while (!timeOut)
            {
                if (IsCommandStatusUpdated(expectedCommandStatus, ignoreTextResult) && IsResponseCodeUpdated(expectedResponseCode))
                {
                    break;
                }

                Task.Delay(twinRefreshIntervalMs).Wait();
                timeOut = ((DateTime.Now - startTime).TotalSeconds > timeoutSeconds);
            }

            return IsCommandStatusUpdated(expectedCommandStatus, ignoreTextResult) && IsResponseCodeUpdated(expectedResponseCode);
        }

        public bool IsCommandStatusUpdated(Object expectedCommandStatus, bool ignoreTextResult)
        {
            var deserializedTwin = JsonSerializer.Deserialize<CommandStatus>(GetNewTwin().Properties.Reported[ComponentName][PropertyCommandStatus].ToString());
            deserializedTwin.TextResult = ignoreTextResult ? "" : deserializedTwin.TextResult;
            return AreJsonObjectsEqual(expectedCommandStatus, deserializedTwin);
        }

        public bool IsResponseCodeUpdated(int expectedResponseCode)
        {
            var responseObject = JsonSerializer.Deserialize<ResponseCode>(GetTwin().Properties.Reported[ComponentName][PerpertyCommandArguments].ToString());
            return responseObject.ac == expectedResponseCode;
        }

        [Test]
        public void CommandRunnerTest_RunCommand_Echo_HelloWorld()
        {
            AssertModuleConnected();
            // Run an echo Hello World command
            (CommandArguments desiredCommand, CommandStatus expectedCommandStatus) = GenerateEchoHelloWorldCommand();
            UpdateDesiredProperties(ComponentName, desiredCommand);
            // Verify the command status (should be succeeded)
            AreReportedPropertiesUpdated(expectedCommandStatus, responseStatusSuccess, false);
        }

        [Test]
        public void CommandRunnerTest_Same_CommandId()
        {
            AssertModuleConnected();
            // Run a command
            (CommandArguments desiredCommand, CommandStatus expectedCommandStatus) = GenerateEchoHelloWorldCommand();
            UpdateDesiredProperties(ComponentName, desiredCommand);
            // Verify the command status (should be succeeded)
            AreReportedPropertiesUpdated(expectedCommandStatus, responseStatusSuccess, false);
            // Run another command with the same commandId and different arguments
            (CommandArguments desiredSleepCommand, CommandStatus expectedSleepCommandStatus) = GenerateCommand("sleep 1");
            desiredSleepCommand.CommandId = desiredCommand.CommandId;
            UpdateDesiredProperties(ComponentName, desiredSleepCommand);
            // Reported response code should be failed (400)
            Assert.True(IsResponseCodeUpdated(responseStatusFailed));
            //Refresh command status
            desiredSleepCommand.Action = Action.RefreshCommandStatus;
            UpdateDesiredProperties(ComponentName, desiredSleepCommand);
            AreReportedPropertiesUpdated(expectedCommandStatus, responseStatusSuccess, false);
        }

        [Test]
        public void CommandRunnerTest_Truncated_Payload()
        {
            AssertModuleConnected();
            // Run a command that generated return payload that exceed the maximum limit
            (CommandArguments desiredCommand, CommandStatus expectedCommandStatus) = GenerateCommand("man curl");
            UpdateDesiredProperties(ComponentName, desiredCommand);
            // Verify the command status (should be succeeded)
            AreReportedPropertiesUpdated(expectedCommandStatus, responseStatusSuccess, true);
        }

        [Test]
        public void CommandRunnerTest_Cancel_Command()
        {
            AssertModuleConnected();
            // Run a long running command
            (CommandArguments desiredCommand, CommandStatus expectedCommandStatus) = GenerateLongRunningCommand();
            UpdateDesiredProperties(ComponentName, desiredCommand);
            // Cancel the long running command
            desiredCommand.Action = Action.CancelCommand;
            UpdateDesiredProperties(ComponentName, desiredCommand);
            // Refresh the long running command status
            desiredCommand.Action = Action.RefreshCommandStatus;
            UpdateDesiredProperties(ComponentName, desiredCommand);
            // Verify the long running command status (should be cancelled)
            expectedCommandStatus.ResultCode = 125;
            expectedCommandStatus.CurrentState = CommandState.Canceled;
            AreReportedPropertiesUpdated(expectedCommandStatus, responseStatusSuccess, true);
        }

        [Test]
        public void CommandRunnerTest_TimeOut_Command()
        {
            AssertModuleConnected();
            // Run a long running command
            (CommandArguments desiredCommand, CommandStatus expectedCommandStatus) = GenerateLongRunningCommand();
            desiredCommand.Timeout = 10;
            UpdateDesiredProperties(ComponentName, desiredCommand);
            // Verify the long running command status (should be Timeout)
            expectedCommandStatus.ResultCode = 62;
            expectedCommandStatus.CurrentState = CommandState.TimeOut;
            AreReportedPropertiesUpdated(expectedCommandStatus, responseStatusSuccess, true);
        }

        [Test]
        public void CommandRunnerTest_Repeat_Command()
        {
            AssertModuleConnected();
            // Run a command
            (CommandArguments desiredCommand, CommandStatus expectedCommandStatus) = GenerateEchoHelloWorldCommand();
            UpdateDesiredProperties(ComponentName, desiredCommand);
            // Verify the command status (should be succeeded)
            AreReportedPropertiesUpdated(expectedCommandStatus, responseStatusSuccess, false);
            // Run the same command again
            UpdateDesiredProperties(ComponentName, desiredCommand);
            // Verify the command status (should be failed)
            AreReportedPropertiesUpdated(expectedCommandStatus, responseStatusFailed, false);
        }

        [Test]
        public void CommandRunnerTest_RefreshCommandStatus()
        {
            AssertModuleConnected();
            string[] commandArguments = { "echo Hello", "date", "ls", "ip route get 1.1.1.1 | awk '{print $7}'", "ls -a", "ping -c 4 example.com", "ip address", "curl -l -s https://example.com", "pwd", "clear" };
            string commandIdPrefix = GenerateCommandId();
            ArrayList commandList = new ArrayList();
            for (int i = 0; i < commandArguments.Length; i++)
            {
                // Run ten commands
                string commandId = commandIdPrefix + i.ToString();
                CommandArguments desiredCommand = GenerateCommandArgumentsObject(commandId, commandArguments[i]);
                commandList.Add(desiredCommand);
                UpdateDesiredProperties(ComponentName, desiredCommand);
            }
            foreach (CommandArguments command in commandList)
            {
                // Refresh command status should work for the last ten commands
                command.Action = Action.RefreshCommandStatus;
                UpdateDesiredProperties(ComponentName, command);
                CommandStatus expectedCommandStatus = GenerateCommandStatusObject(command.CommandId);
                AreReportedPropertiesUpdated(expectedCommandStatus, responseStatusSuccess, true);
            }
        }
    }
}