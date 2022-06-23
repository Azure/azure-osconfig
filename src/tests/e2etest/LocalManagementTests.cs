// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace E2eTesting
{

    [TestFixture, Category("LocalManagement")]
    public class LocalManagementTests : E2ETest
    {
        private static readonly string _componentName = "CommandRunner";
        private static readonly string _reportedObjectName = "commandStatus";
        private static readonly string _desiredPath = "/etc/osconfig/osconfig_desired.json";

        public class CommandRunner
        {
            public CommandRunnerTests.CommandArguments commandArguments { get; set; }
        }

        public class CommandRunnerObject
        {
            public CommandRunner CommandRunner { get; set; }
        }

        [Test]
        public async Task LocalManagementTest_Set()
        {
            String commandId = String.Format("LocalManagementTest_Set_{0}", GenerateId()) ;
            // Perform a set operation on local management
            CommandRunnerObject twin = new CommandRunnerObject
            {
                CommandRunner = new CommandRunner
                {
                    commandArguments = new CommandRunnerTests.CommandArguments
                    {
                        commandId = commandId,
                        arguments = "echo \"Hello from local management!\"",
                        action = CommandRunnerTests.Action.RunCommand,
                        timeout = 0,
                        singleLineTextResult = true
                    }
                }
            };
            CommandRunnerTests.CommandStatus expectedStatus = new CommandRunnerTests.CommandStatus
            {
                commandId = commandId,
                resultCode = 0,
                textResult = "Hello from local management! ",
                currentState = CommandRunnerTests.CommandState.Succeeded
            };

            // TODO: try setting JsonSerializerOptions PropertyNamingPolicy = JsonNamingPolicy.CamelCase so we dont have to change the structure of the object
            File.WriteAllLines(_desiredPath, new string[] { JsonSerializer.Serialize(twin) });

            var reported = await GetReported<CommandRunnerTests.CommandStatus>(_componentName, _reportedObjectName, (CommandRunnerTests.CommandStatus status) => {
                return (status.commandId == expectedStatus.commandId) && (status.currentState == CommandRunnerTests.CommandState.Succeeded);
            });
            
            ValidateLocalReported(reported, _componentName, _reportedObjectName);
            JsonAssert.AreEqual(expectedStatus, reported);
        }
    }
}