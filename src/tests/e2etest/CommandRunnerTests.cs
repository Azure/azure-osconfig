// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System;
using System.Xml.Serialization;

namespace E2eTesting
{
    [TestFixture, Category("CommandRunner")]
    public class CommandRunnerTests : E2ETest
    {
        private static readonly string _componentName = "CommandRunner";
        private static readonly string _desiredObjectName = "commandArguments";
        private static readonly string _reportedObjectName = "commandStatus";

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

        public class CommandArguments
        {
            public string commandId { get; set; }
            public string arguments { get; set; }
            public Action action { get; set; }
            public int timeout { get; set; }
            public bool singleLineTextResult { get; set; }
        }

        public class CommandStatus
        {
            public string commandId { get; set; }
            public long resultCode { get; set; }
            public string textResult { get; set; }
            public CommandState currentState { get; set; }
        }

        public static CommandArguments CreateCommand(string arguments, Action action = Action.RunCommand, int timeout = 0, bool singleLineTextResult = false)
        {
            return CreateCommand(GenerateId(), arguments, action, timeout, singleLineTextResult);
        }

        public static CommandArguments CreateCommand(string newCommandId, string newArguments, Action newAction = Action.RunCommand, int newTimeout = 0, bool newSingleLinetextResult = false)
        {
            return new CommandArguments
            {
                commandId = newCommandId,
                arguments = newArguments,
                action = newAction,
                timeout = newTimeout,
                singleLineTextResult = newSingleLinetextResult
            };
        }

        public static CommandArguments CreateLongRunningCommand(string newCommandId, int newTimeout = 120)
        {
            return new CommandArguments
            {
                commandId = newCommandId,
                arguments = "sleep 1000s",
                action = Action.RunCommand,
                timeout = newTimeout
            };
        }

        public static CommandArguments CreateCancelCommand(string newCommandId)
        {
            return new CommandArguments
            {
                commandId = newCommandId,
                arguments = "",
                action = Action.CancelCommand,
                timeout = 0
            };
        }

        public static CommandStatus CreateCommandStatus(string newCommandId, string newTextResult = "", CommandState newCommandState = CommandState.Succeeded, int newResultCode = 0)
        {
            return new CommandStatus
            {
                commandId = newCommandId,
                textResult = newTextResult,
                currentState = newCommandState,
                resultCode = newResultCode
            };
        }

        public void SendCommand(CommandArguments command, int expectedAckCode = ACK_SUCCESS)
        {
            int ackCode = -1;

            Console.WriteLine($"{command.action} \"{command.commandId}\" ({command.arguments})");

            try
            {
                var setDesiredTask = SetDesired<CommandArguments>(_componentName, _desiredObjectName, command);
                setDesiredTask.Wait();
                ackCode = setDesiredTask.Result.Ac;
            }
            catch (Exception e)
            {
                Assert.Fail("Failed to send command: {0}", e.Message);
            }

            if (ackCode != expectedAckCode)
            {
                Assert.Fail("CommandRunner.CommandArguments expected ackCode {0}, but got {1}", expectedAckCode, ackCode);
            }
        }

        public void CancelCommand(string commandId)
        {
            SendCommand(CreateCancelCommand(commandId));
        }

        public void RefreshCommandStatus(string newCommandId, int expectedAckCode = ACK_SUCCESS)
        {
            var refreshCommand = new CommandArguments
            {
                commandId = newCommandId,
                arguments = "",
                action = Action.RefreshCommandStatus
            };

            SendCommand(refreshCommand, expectedAckCode);
        }

        public CommandStatus WaitForStatus(string commandId, CommandState state)
        {
            Func<CommandStatus, bool> condition = (CommandStatus status) => ((status.commandId == commandId) && (status.currentState == state));
            var reportedTask = GetReported<CommandStatus>(_componentName, _reportedObjectName, condition);
            reportedTask.Wait();
            ValidateLocalReported(reportedTask.Result, _componentName, _reportedObjectName);
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

            CommandStatus status = WaitForStatus(command.commandId, state);
            JsonAssert.AreEqual(CreateCommandStatus(command.commandId, textResult, state, resultCode), status);
        }

        [Test]
        public void CommandRunnerTest_CancelCommand()
        {
            var commandId = GenerateId();
            var command = CreateLongRunningCommand(commandId);

            SendCommand(command);
            CommandStatus runningStatus = WaitForStatus(commandId, CommandState.Running);

            CancelCommand(command.commandId);
            CommandStatus canceledStatus = WaitForStatus(commandId, CommandState.Canceled);

            Assert.Multiple(() =>
            {
                JsonAssert.AreEqual(CreateCommandStatus(commandId, "", CommandState.Running, 0), runningStatus);
                JsonAssert.AreEqual(CreateCommandStatus(commandId, "", CommandState.Canceled, 125), canceledStatus);
            });
        }

        [Test]
        public void CommandRunnerTest_RepeatCommandId()
        {
            var commandId = GenerateId();
            var command = CreateCommand(commandId, "echo 'command 1'", Action.RunCommand, 0, true);
            var commandWithDuplicateCommandId = CreateCommand(commandId, "echo 'command 2'", Action.RunCommand, 0, true);

            SendCommand(command);
            var commandStatus = WaitForStatus(commandId, CommandState.Succeeded);
            JsonAssert.AreEqual(CreateCommandStatus(commandId, "command 1 ", CommandState.Succeeded, 0), commandStatus);

            // Send a command with a duplicate command Id
            SendCommand(commandWithDuplicateCommandId, 400);
        }

        [Test]
        public void CommandRunnerTest_CommandSequence()
        {
            var command1 = CreateLongRunningCommand(GenerateId());
            var command2 = CreateCommand("echo 'command 2'");
            var command3 = CreateCommand("sleep 10s && echo 'command 3'", timeout: 1);
            var command4 = CreateCommand("blah");

            var expectedCommandStatus1 = CreateCommandStatus(command1.commandId, "", CommandState.Canceled, 125);
            var expectedCommandStatus2 = CreateCommandStatus(command2.commandId, "command 2\n", CommandState.Succeeded, 0);
            var expectedCommandStatus3 = CreateCommandStatus(command3.commandId, "", CommandState.TimedOut, 62);
            var expectedCommandStatus4 = CreateCommandStatus(command4.commandId, "sh: 1: blah: not found\n", CommandState.Failed, 127);

            SendCommand(command1);
            SendCommand(command2);
            SendCommand(command3);
            SendCommand(command4);
            SendCommand(CreateCancelCommand(command1.commandId));

            // Wait for the last command to complete before checking all command statuses
            CommandStatus actualCommandStatus4 = WaitForStatus(command4.commandId, CommandState.Failed);

            RefreshCommandStatus(command1.commandId);
            CommandStatus actualCommandStatus1 = WaitForStatus(command1.commandId, CommandState.Canceled);

            RefreshCommandStatus(command2.commandId);
            CommandStatus actualCommandStatus2 = WaitForStatus(command2.commandId, CommandState.Succeeded);

            RefreshCommandStatus(command3.commandId);
            CommandStatus actualCommandStatus3 = WaitForStatus(command3.commandId, CommandState.TimedOut);

            Assert.Multiple(() =>
            {
                JsonAssert.AreEqual(expectedCommandStatus1, actualCommandStatus1);
                JsonAssert.AreEqual(expectedCommandStatus2, actualCommandStatus2);
                JsonAssert.AreEqual(expectedCommandStatus3, actualCommandStatus3);
                JsonAssert.AreEqual(expectedCommandStatus4, actualCommandStatus4);
            });
        }
    }
}