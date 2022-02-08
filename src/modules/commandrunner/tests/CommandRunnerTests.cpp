// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

#include <CommandRunner.h>
#include <CommonUtils.h>
#include <Mmi.h>

using ::testing::_;
using ::testing::Invoke;

#define LIMITED_PAYLOAD_SIZE 83

namespace OSConfig::Platform::Tests
{
    static void SignalDoWork(int signal)
    {
        UNUSED(signal);
    }

    class CommandRunnerTests : public ::testing::Test
    {
    public:
        void SetUp() override;
        void TearDown() override;
    };

    void CommandRunnerTests::SetUp()
    {
        signal(SIGUSR1, SignalDoWork);
    }

    void CommandRunnerTests::TearDown() 
    {
        // Do nothing
    }

    TEST_F(CommandRunnerTests, Execute)
    {
        CommandRunner::CommandArguments cmdArgs{ "1", "echo test", CommandRunner::Action::RunCommand, 0, true };
        CommandRunner cmdRunner("CommandRunnerTests::Execute", nullptr);
        cmdRunner.AddCommandStatus(cmdArgs.commandId, true);
        ASSERT_EQ(MMI_OK, cmdRunner.Execute(cmdRunner, CommandRunner::Action::RunCommand, cmdArgs.commandId, cmdArgs.arguments, CommandRunner::CommandState::Unknown, cmdArgs.timeout, true));
        auto cmdStatus = cmdRunner.GetCommandStatus("1");
        ASSERT_NE(nullptr, cmdStatus);
        ASSERT_STREQ(cmdArgs.commandId.c_str(), cmdStatus->commandId.c_str());
        ASSERT_STREQ("test ", cmdStatus->textResult.c_str());   // "test " CommandRunner adds " " instead of EOL
        ASSERT_EQ(CommandRunner::CommandState::Succeeded, cmdStatus->commandState);
    }

    TEST_F(CommandRunnerTests, CommandStatusUpdated)
    {
        CommandRunner cmdRunner("CommandRunnerTests::CommandStatusUpdated", nullptr);

        // Run 1st command
        CommandRunner::CommandArguments cmdArgs1{ "1", "echo test1", CommandRunner::Action::RunCommand, 0, true };
        cmdRunner.AddCommandStatus(cmdArgs1.commandId, true);
        ASSERT_EQ(MMI_OK, cmdRunner.Execute(cmdRunner, CommandRunner::Action::RunCommand, cmdArgs1.commandId, cmdArgs1.arguments, CommandRunner::CommandState::Unknown, cmdArgs1.timeout, true));
        auto cmdStatus1 = cmdRunner.GetCommandStatus("1");
        ASSERT_NE(nullptr, cmdStatus1);
        ASSERT_STREQ(cmdArgs1.commandId.c_str(), cmdStatus1->commandId.c_str());
        ASSERT_STREQ("test1 ", cmdStatus1->textResult.c_str());
        ASSERT_EQ(CommandRunner::CommandState::Succeeded, cmdStatus1->commandState);

        // Run 2nd command
        CommandRunner::CommandArguments cmdArgs2{ "2", "echo test2", CommandRunner::Action::RunCommand, 0, true };
        cmdRunner.AddCommandStatus(cmdArgs2.commandId, true);
        ASSERT_EQ(MMI_OK, cmdRunner.Execute(cmdRunner, CommandRunner::Action::RunCommand, cmdArgs2.commandId, cmdArgs2.arguments, CommandRunner::CommandState::Unknown, cmdArgs2.timeout, true));
        auto cmdStatus2 = cmdRunner.GetCommandStatus("2");
        ASSERT_NE(nullptr, cmdStatus2);
        ASSERT_STREQ(cmdArgs2.commandId.c_str(), cmdStatus2->commandId.c_str());
        ASSERT_STREQ("test2 ", cmdStatus2->textResult.c_str());
        ASSERT_EQ(CommandRunner::CommandState::Succeeded, cmdStatus2->commandState);

        ASSERT_STREQ(cmdArgs2.commandId.c_str(), cmdRunner.GetCommandIdToRefresh().c_str());
    }

    TEST_F(CommandRunnerTests, ExecuteCommandLimitedPayload)
    {
        CommandRunner::CommandArguments cmdArgs{ "1", "echo test", CommandRunner::Action::RunCommand, 0, true };
        CommandRunner cmdRunner("CommandRunnerTests::ExecuteCommandLimitedPayload", nullptr, LIMITED_PAYLOAD_SIZE);
        cmdRunner.AddCommandStatus(cmdArgs.commandId, true);
        ASSERT_EQ(MMI_OK, cmdRunner.Execute(cmdRunner, CommandRunner::Action::RunCommand, cmdArgs.commandId, cmdArgs.arguments, CommandRunner::CommandState::Unknown, cmdArgs.timeout, true));
        auto cmdStatus = cmdRunner.GetCommandStatus("1");
        ASSERT_NE(nullptr, cmdStatus);
        ASSERT_STREQ(cmdArgs.commandId.c_str(), cmdStatus->commandId.c_str());
        ASSERT_STREQ("t", cmdStatus->textResult.c_str());
        ASSERT_EQ(CommandRunner::CommandState::Succeeded, cmdStatus->commandState);
    }

    TEST_F(CommandRunnerTests, CachedCommandStatus)
    {
        CommandRunner::CommandArguments cmdArgs{ "1", "echo 1", CommandRunner::Action::RunCommand, 0, true };
        CommandRunner cmdRunner("CommandRunnerTests::CachedCommandStatus", nullptr, LIMITED_PAYLOAD_SIZE);
        cmdRunner.AddCommandStatus(cmdArgs.commandId, true);
        ASSERT_EQ(MMI_OK, cmdRunner.Execute(cmdRunner, CommandRunner::Action::RunCommand, cmdArgs.commandId, cmdArgs.arguments, CommandRunner::CommandState::Unknown, cmdArgs.timeout, true));

        auto status = cmdRunner.GetCommandStatusToPersist();

        ASSERT_STREQ(cmdArgs.commandId.c_str(), status.commandId.c_str());
        ASSERT_STREQ("1", status.textResult.c_str());
        ASSERT_EQ(CommandRunner::CommandState::Succeeded, status.commandState);
    }

    TEST_F(CommandRunnerTests, OverwriteBufferCommandStatus)
    {
        constexpr int CommandArgumentsListSize = COMMANDSTATUS_CACHE_MAX + 1;
        std::array<CommandRunner::CommandArguments, CommandArgumentsListSize> commandArgumentList;
        CommandRunner cmdRunner("CommandRunnerTests::OverwriteBufferCommandStatus", nullptr);
        for (int i = 0; i < CommandArgumentsListSize; ++i)
        {
            char size, commandId[10], command[20];
            size = sprintf(commandId, "%d", i);
            ASSERT_GT(size, 0);
            size = sprintf(command, "echo %d", i);
            ASSERT_GT(size, 0);
            commandArgumentList[i] = { commandId, command, CommandRunner::Action::RunCommand, 0, true };
            cmdRunner.AddCommandStatus(commandId, true);
            ASSERT_EQ(MMI_OK, cmdRunner.Execute(cmdRunner, CommandRunner::Action::RunCommand, commandArgumentList[i].commandId, commandArgumentList[i].arguments, CommandRunner::CommandState::Unknown, commandArgumentList[i].timeout, true));
        }

        // 1st element should no longer be present in the buffer
        auto cmd1 = cmdRunner.GetCommandStatus(commandArgumentList[0].commandId);
        auto cmd2 = cmdRunner.GetCommandStatus(commandArgumentList[1].commandId);
        ASSERT_EQ(nullptr, cmd1);
        ASSERT_NE(nullptr, cmd2);
    }

    TEST_F(CommandRunnerTests, WorkerThreadExecution)
    {
        CommandRunner::CommandArguments cmdArgs1{ "1", "sleep 1s && echo 1", CommandRunner::Action::RunCommand, 0, true };
        CommandRunner::CommandArguments cmdArgs2{ "2", "sleep 1s && echo 2", CommandRunner::Action::RunCommand, 0, true };
        CommandRunner cmdRunner("CommandRunnerTests::WorkerThreadExecution", nullptr);

        // Dispatch both
        cmdRunner.Run(cmdArgs1);
        cmdRunner.Run(cmdArgs2);

        auto cmdStatus1 = cmdRunner.GetCommandStatus(cmdArgs1.commandId);
        auto cmdStatus2 = cmdRunner.GetCommandStatus(cmdArgs2.commandId);

        ASSERT_NE(nullptr, cmdStatus1);
        ASSERT_NE(nullptr, cmdStatus2);

        ASSERT_STREQ(cmdArgs1.commandId.c_str(), cmdStatus1->commandId.c_str());
        ASSERT_TRUE((CommandRunner::CommandState::Unknown == cmdStatus1->commandState) || (CommandRunner::CommandState::Running == cmdStatus1->commandState));
        ASSERT_STREQ(cmdArgs2.commandId.c_str(), cmdStatus2->commandId.c_str());
        ASSERT_TRUE((CommandRunner::CommandState::Unknown == cmdStatus2->commandState) || (CommandRunner::CommandState::Running == cmdStatus2->commandState));

        cmdRunner.WaitForCommandResults();

        cmdStatus1 = cmdRunner.GetCommandStatus(cmdArgs1.commandId);
        cmdStatus2 = cmdRunner.GetCommandStatus(cmdArgs2.commandId);

        ASSERT_EQ(CommandRunner::CommandState::Succeeded, cmdStatus1->commandState);
        ASSERT_STREQ("1 ", cmdStatus1->textResult.c_str());
        ASSERT_EQ(CommandRunner::CommandState::Succeeded, cmdStatus2->commandState);
        ASSERT_STREQ("2 ", cmdStatus2->textResult.c_str());
    }

    TEST_F(CommandRunnerTests, CommandTimeoutSeconds)
    {
        // Sleep for 10s, timeout in 1s
        // Actual timeout in 5s due to COMMAND_CALLBACK_INTERVAL = 5
        CommandRunner::CommandArguments cmdArgs{ "1", "sleep 10s", CommandRunner::Action::RunCommand, 4, true };
        CommandRunner cmdRunner("CommandRunnerTests::CommandTimeoutSeconds", nullptr);

        cmdRunner.Run(cmdArgs);
        cmdRunner.WaitForCommandResults();

        auto cmdStatus = cmdRunner.GetCommandStatus(cmdArgs.commandId);

        ASSERT_NE(nullptr, cmdStatus);
        ASSERT_EQ(CommandRunner::CommandState::TimedOut, cmdStatus->commandState);
    }

    TEST_F(CommandRunnerTests, CancelRunningCommand)
    {
        CommandRunner::CommandArguments cmdArgs{ "1", "sleep 10s", CommandRunner::Action::RunCommand, 15, true };
        CommandRunner cmdRunner("CommandRunnerTests::CancelRunningCommand", nullptr);

        cmdRunner.Run(cmdArgs);
        auto cmdStatus = cmdRunner.GetCommandStatus(cmdArgs.commandId);

        ASSERT_NE(nullptr, cmdStatus);
        ASSERT_STREQ(cmdArgs.commandId.c_str(), cmdStatus->commandId.c_str());
        ASSERT_TRUE((CommandRunner::CommandState::Unknown == cmdStatus->commandState) || (CommandRunner::CommandState::Running == cmdStatus->commandState));

        // Cancel command
        cmdRunner.Cancel(cmdArgs.commandId);
        cmdRunner.WaitForCommandResults();
        cmdStatus = cmdRunner.GetCommandStatus(cmdArgs.commandId);

        ASSERT_EQ(CommandRunner::CommandState::Canceled, cmdStatus->commandState);
    }

    TEST_F(CommandRunnerTests, CancelQueuedCommand)
    {
        CommandRunner::CommandArguments cmdArgs1{ "1", "sleep 10s", CommandRunner::Action::RunCommand, 20, true };
        CommandRunner::CommandArguments cmdArgs2{ "2", "sleep 10s", CommandRunner::Action::RunCommand, 20, true };
        CommandRunner cmdRunner("CommandRunnerTests::CancelQueuedCommand", nullptr);

        cmdRunner.Run(cmdArgs1);
        cmdRunner.Run(cmdArgs2);
        auto cmdStatus1 = cmdRunner.GetCommandStatus(cmdArgs1.commandId);
        auto cmdStatus2 = cmdRunner.GetCommandStatus(cmdArgs2.commandId);

        ASSERT_NE(nullptr, cmdStatus1);
        ASSERT_NE(nullptr, cmdStatus2);
        ASSERT_STREQ(cmdArgs1.commandId.c_str(), cmdStatus1->commandId.c_str());
        ASSERT_STREQ(cmdArgs2.commandId.c_str(), cmdStatus2->commandId.c_str());
        ASSERT_TRUE((CommandRunner::CommandState::Unknown == cmdStatus1->commandState) || (CommandRunner::CommandState::Running == cmdStatus1->commandState));
        ASSERT_TRUE((CommandRunner::CommandState::Unknown == cmdStatus2->commandState) || (CommandRunner::CommandState::Running == cmdStatus2->commandState));

        // Cancel second command while the first is still running
        cmdRunner.Cancel(cmdArgs2.commandId);
        cmdStatus1 = cmdRunner.GetCommandStatus(cmdArgs1.commandId);
        cmdStatus2 = cmdRunner.GetCommandStatus(cmdArgs2.commandId);

        ASSERT_TRUE(cmdRunner.IsCanceled(cmdArgs2.commandId));
        ASSERT_EQ(CommandRunner::CommandState::Canceled, cmdStatus2->commandState);
        ASSERT_TRUE((CommandRunner::CommandState::Unknown == cmdStatus1->commandState) || (CommandRunner::CommandState::Running == cmdStatus1->commandState));

        // Cancel first command
        cmdRunner.Cancel(cmdArgs1.commandId);
        cmdRunner.WaitForCommandResults();
        cmdStatus1 = cmdRunner.GetCommandStatus(cmdArgs1.commandId);

        ASSERT_EQ(CommandRunner::CommandState::Canceled, cmdStatus1->commandState);
    }

    TEST_F(CommandRunnerTests, SingleLineTextResult)
    {
        CommandRunner::CommandArguments cmdArgs1{ "1", "sleep 1s && echo 'single\\nline'", CommandRunner::Action::RunCommand, 0, true };
        CommandRunner::CommandArguments cmdArgs2{ "2", "sleep 1s && echo 'multiple\\nlines'", CommandRunner::Action::RunCommand, 0, false };
        CommandRunner cmdRunner("CommandRunnerTests::SingleLineTextResult", nullptr);

        cmdRunner.Run(cmdArgs1);
        cmdRunner.Run(cmdArgs2);

        cmdRunner.WaitForCommandResults();

        auto cmdStatus1 = cmdRunner.GetCommandStatus(cmdArgs1.commandId);
        auto cmdStatus2 = cmdRunner.GetCommandStatus(cmdArgs2.commandId);

        ASSERT_NE(nullptr, cmdStatus1);
        ASSERT_NE(nullptr, cmdStatus2);

        ASSERT_EQ(CommandRunner::CommandState::Succeeded, cmdStatus1->commandState);
        ASSERT_STREQ("single line ", cmdStatus1->textResult.c_str());
        ASSERT_EQ(CommandRunner::CommandState::Succeeded, cmdStatus2->commandState);
        ASSERT_STREQ("multiple\nlines\n", cmdStatus2->textResult.c_str());
    }

    TEST_F(CommandRunnerTests, PersistedCommandStatus)
    {
        CommandRunner cmdRunner("CommandRunnerTests::PersistedCommandStatus", nullptr);
        CommandRunner::CommandArguments cmdArgs1{ "1", "sleep 10s", CommandRunner::Action::RunCommand, 10, true };
        CommandRunner::CommandArguments cmdArgs2{ "2", "sleep 10s", CommandRunner::Action::RunCommand, 10, true };

        cmdRunner.Run(cmdArgs1);
        auto persistedStatus = cmdRunner.GetCommandStatusToPersist();

        // Check 1st command is persisted
        ASSERT_STREQ(persistedStatus.commandId.c_str(), cmdArgs1.commandId.c_str());
        ASSERT_EQ(persistedStatus.resultCode, 0);
        ASSERT_EQ(persistedStatus.commandState, CommandRunner::CommandState::Unknown);

        cmdRunner.Run(cmdArgs2);
        persistedStatus = cmdRunner.GetCommandStatusToPersist();

        // Check 2nd command is persisted while 1st command is running
        ASSERT_STREQ(persistedStatus.commandId.c_str(), cmdArgs2.commandId.c_str());
        ASSERT_EQ(persistedStatus.resultCode, 0);
        ASSERT_EQ(persistedStatus.commandState, CommandRunner::CommandState::Unknown);

        // Cancel 1st command
        cmdRunner.Cancel(cmdArgs1.commandId);
        persistedStatus = cmdRunner.GetCommandStatusToPersist();

        // Check 2nd command is still persisted after canceling 1st command
        ASSERT_STREQ(persistedStatus.commandId.c_str(), cmdArgs2.commandId.c_str());
        ASSERT_EQ(persistedStatus.resultCode, 0);
        ASSERT_EQ(persistedStatus.commandState, CommandRunner::CommandState::Unknown);

        // Cancel 2nd command
        cmdRunner.Cancel(cmdArgs2.commandId);
        persistedStatus = cmdRunner.GetCommandStatusToPersist();

        // Check 2nd command is still persisted after cancel
        ASSERT_STREQ(persistedStatus.commandId.c_str(), cmdArgs2.commandId.c_str());
        ASSERT_EQ(persistedStatus.resultCode, ECANCELED);
        ASSERT_EQ(persistedStatus.commandState, CommandRunner::CommandState::Canceled);

        cmdRunner.WaitForCommandResults();
    }
} // namespace OSConfig::Platform::Tests