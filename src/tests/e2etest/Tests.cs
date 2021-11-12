// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using Microsoft.Azure.Devices.Shared;
using System.Text.Json;
using System;
using System.Threading.Tasks;

namespace e2etesting
{
    public class CommandRunnerTests : E2eTest
    {
        public enum Action
        {
            ActionNone = 0,
            ActionReboot,
            ActionShutdown,
            ActionRunCommand,
            ActionRefreshCommandStatus
        }

        public enum CommandState
        {
            Unknown = 0,
            Running,
            Succeeded,
            Failed
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
            public long ExtendedResultCode { get; set; }
            public string TextResult { get; set; }
            public CommandState CurrentState { get; set; }
        }

        [Test]
        public void CommandRunnerTest_ActionRunCommand_echo_HelloWorld()
        {
            var random = Convert.ToBase64String(Guid.NewGuid().ToByteArray()).Substring(0, 4);
            var desiredCommand = new CommandArguments
            {
                CommandId = random,
                Arguments = "echo HelloWorld",
                Action = Action.ActionRunCommand,
                Timeout = 60,
                SingleLineTextResult = true
            };
            var expectedCommandStatus = new CommandStatus
            {
                CommandId = random,
                ResultCode = 0,
                ExtendedResultCode = 0,
                TextResult = "HelloWorld ",
                CurrentState = CommandState.Succeeded,
            };

            var twinPatch = CreateCommandArgumentsPropertyPatch("CommandRunner", desiredCommand);

            if (UpdateTwinBlockUntilUpdate(twinPatch))
            {
                var deserializedTwin = JsonSerializer.Deserialize<CommandStatus>(GetTwin().Properties.Reported["CommandRunner"]["CommandStatus"].ToString());
                // Wait until our random commandId is present in reported properties
                DateTime startTime = DateTime.Now;
                while(deserializedTwin.CommandId != random && (DateTime.Now - startTime).TotalSeconds < 30)
                {
                    Console.WriteLine("[CommandRunnerTests] waiting for commandId to be equivalent...");
                    Task.Delay(twinRefreshIntervalMs).Wait();
                    deserializedTwin = JsonSerializer.Deserialize<CommandStatus>(GetNewTwin().Properties.Reported["CommandRunner"]["CommandStatus"].ToString());
                }

                AreEqualByJson(expectedCommandStatus, deserializedTwin);
            }
            else
            {
                Assert.Fail("Timeout for updating twin - ActionRunCommand");
            }
        }
    }

    public class FirewallTests : E2eTest
    {
        string ComponentName = "Firewall";
        public enum FirewallStateCode
        {
            Unknown = 0,
            Enabled,
            Disabled,
        }

        public partial class Firewall
        {
        public FirewallStateCode FirewallState { get; set; }
        public string FirewallFingerprint { get; set; }
        }
        public partial class ExpectedFirewallPattern
        {
            public string FirewallState { get; set; }
            public string FirewallFingerprint { get; set; }
        }

        [Test]
        public void FirewallTest_Regex()
        {
            var expectedFirewallPattern = new ExpectedFirewallPattern
            {
                FirewallState = "[0-2]",
                FirewallFingerprint = "[0-9a-z]{64}"
            };

            var deserializedReportedObject = JsonSerializer.Deserialize<Firewall>(GetTwin().Properties.Reported[ComponentName].ToString());
            Assert.True(IsRegexMatch(deserializedReportedObject.FirewallState, expectedFirewallPattern.FirewallState));
        }
    }
}