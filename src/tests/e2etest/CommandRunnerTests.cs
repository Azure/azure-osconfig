// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System;
using System.Text.Json;
using System.Threading.Tasks;

namespace E2eTesting
{
    public class CommandRunnerTests : E2eTest
    {
        private const string componentName = "CommandRunner";
        private const string commandArguments = "CommandArguments";
        private const string commandStatus = "CommandStatus";

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

        public partial class CommandArgumentsResponse
        {
            public CommandArguments value { get; set; }
            public int ac { get; set; }
            public string ad { get; set; }
            public int av { get; set; }
        }

        public partial class CommandStatus
        {
            public string CommandId { get; set; }
            public long ResultCode { get; set; }
            public string TextResult { get; set; }
            public CommandState CurrentState { get; set; }
        }
        public CommandStatus WaitForCommandStatus(string commandId, CommandState commandState, int maxWaitSeconds = 180)
        {
            var reportedCommandStatus = JsonSerializer.Deserialize<CommandStatus>(GetTwin().Properties.Reported[componentName][commandStatus].ToString());
            DateTime startTime = DateTime.Now;

            // Wait until the commandId is present in reported properties and the command is in the expected state
            while((reportedCommandStatus.CommandId != commandId) || (reportedCommandStatus.CurrentState != commandState))
            {
                if ((DateTime.Now - startTime).TotalSeconds < maxWaitSeconds)
                {
                    Console.WriteLine("[CommandRunnerTests] waiting for {0} to complete...", commandId);
                    Task.Delay(twinRefreshIntervalMs).Wait();
                    reportedCommandStatus = JsonSerializer.Deserialize<CommandStatus>(GetNewTwin().Properties.Reported[componentName][commandStatus].ToString());
                }
                else
                {
                    Assert.Fail("Timed out while waiting for '{0}' to reach a '{1}' state, recieved: {2}", commandId, commandState, JsonSerializer.Serialize<CommandStatus>(reportedCommandStatus));
                    break;
                }
            }

            return reportedCommandStatus;
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

        public void SendCommand(CommandArguments command, int responseCode = 200)
        {
            var twinPatch = CreateCommandArgumentsPropertyPatch(componentName, command);
            if (UpdateTwinBlockUntilUpdate(twinPatch))
            {
                var response = JsonSerializer.Deserialize<CommandArgumentsResponse>(GetTwin().Properties.Reported[componentName][commandArguments].ToString());
                Assert.AreEqual(response.ac, responseCode);
                AreEqualByJson(response.value, command);
            }
            else
            {
                Assert.Fail("Timed out while waiting for twin update");
            }
        }

        public void CancelCommand(string commandId)
        {
            SendCommand(CreateCancelCommand(commandId));
        }

        public void RefreshCommandStatus(string commandId, int responseCode = 200)
        {
            var refreshCommand = new CommandArguments
            {
                CommandId = commandId,
                Arguments = "",
                Action = Action.RefreshCommandStatus
            };

            SendCommand(refreshCommand, responseCode);
        }

        [Test]
        [TestCase("echo 'hello world'", 60, false, 0, "hello world\n", CommandState.Succeeded)]
        [TestCase("sleep 30s", 1, true, 62, "", CommandState.TimedOut)]
        [TestCase("sleep 10s", 0, true, 0, "", CommandState.Succeeded)]
        [TestCase("echo 'single\nline'", 0, true, 0, "single line ", CommandState.Succeeded)]
        [TestCase("echo 'multiple\nlines'", 0, false, 0, "multiple\nlines\n", CommandState.Succeeded)]
        [TestCase("blah", 0, false, 127, "sh: 1: blah: not found\n", CommandState.Failed)]
        public void CommandRunnerTest_RunCommand(string arguments, int timeout, bool singleLineTextResult, int resultCode, string textResult, CommandState commandState)
        {
            var command = CreateCommand(arguments, Action.RunCommand, timeout, singleLineTextResult);
            SendCommand(command);

            var commandStatus = WaitForCommandStatus(command.CommandId, commandState);
            AreEqualByJson(CreateCommandStatus(command.CommandId, textResult, commandState, resultCode), commandStatus);
        }

        [Test]
        public void CommandRunnerTest_CancelCommand()
        {
            var commandId = CreateCommandId();
            var command = CreateLongRunningCommand(commandId);

            SendCommand(command);
            var runningCommandStatus = WaitForCommandStatus(commandId, CommandState.Running);
            AreEqualByJson(CreateCommandStatus(commandId, "", CommandState.Running, 0), runningCommandStatus);

            CancelCommand(command.CommandId);
            var canceledCommandStatus = WaitForCommandStatus(commandId, CommandState.Canceled);
            AreEqualByJson(CreateCommandStatus(commandId, "", CommandState.Canceled, 125), canceledCommandStatus);
        }

        [Test]
        public void CommandRunnerTest_RepeatCommandId()
        {
            var commandId = CreateCommandId();
            var command = CreateCommand(commandId, "echo 'command 1'", Action.RunCommand, 0, true);
            var commandWithDuplicateCommandId = CreateCommand(commandId, "echo 'command 2'", Action.RunCommand, 0, true);

            SendCommand(command);
            var commandStatus = WaitForCommandStatus(commandId, CommandState.Succeeded);
            AreEqualByJson(CreateCommandStatus(commandId, "command 1 ", CommandState.Succeeded, 0), commandStatus);

            // Send a command with a duplicate command Id
            var twinPatch = CreateCommandArgumentsPropertyPatch(componentName, commandWithDuplicateCommandId);
            UpdateTwinBlockUntilUpdate(twinPatch);
            var response = JsonSerializer.Deserialize<CommandArgumentsResponse>(GetTwin().Properties.Reported[componentName][commandArguments].ToString());
            Assert.AreEqual(response.ac, 400);
            AreEqualByJson(response.value, commandWithDuplicateCommandId);
        }

        [Test]
        public void CommandRunnerTest_CommandSequence()
        {
            var command1 = CreateCommand("sleep 100s && echo 'command 1'");
            var command2 = CreateCommand("echo 'command 2'");
            var command3 = CreateCommand("sleep 1000s && echo 'command 3'", timeout: 1);
            var command4 = CreateCommand("blah");

            var expectedCommandStatus1 = CreateCommandStatus(command1.CommandId, "command 1\n");
            var expectedCommandStatus2 = CreateCommandStatus(command2.CommandId, "", CommandState.Canceled, 125);
            var expectedCommandStatus3 = CreateCommandStatus(command3.CommandId, "", CommandState.TimedOut, 62);
            var expectedCommandStatus4 = CreateCommandStatus(command4.CommandId, "sh: 1: blah: not found\n", CommandState.Failed, 127);

            SendCommand(command1);
            SendCommand(command2);
            SendCommand(CreateCancelCommand(command2.CommandId));
            SendCommand(command3);
            SendCommand(command4);

            // Wait for the last command to complete before checking all command statuses
            WaitForCommandStatus(command4.CommandId, CommandState.Failed);

            RefreshCommandStatus(command1.CommandId);
            var actualCommandStatus1 = WaitForCommandStatus(command1.CommandId, CommandState.Succeeded);
            AreEqualByJson(expectedCommandStatus1, actualCommandStatus1);

            RefreshCommandStatus(command2.CommandId);
            var actualCommandStatus2 = WaitForCommandStatus(command2.CommandId, CommandState.Canceled);
            AreEqualByJson(expectedCommandStatus2, actualCommandStatus2);

            RefreshCommandStatus(command3.CommandId);
            var actualCommandStatus3 = WaitForCommandStatus(command3.CommandId, CommandState.TimedOut);
            AreEqualByJson(expectedCommandStatus3, actualCommandStatus3);

            RefreshCommandStatus(command4.CommandId);
            var actualCommandStatus4 = WaitForCommandStatus(command4.CommandId, CommandState.Failed);
            AreEqualByJson(expectedCommandStatus4, actualCommandStatus4);
        }
    }
}