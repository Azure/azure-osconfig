// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Command.h>
#include <CommandRunner.h>
#include <Mmi.h>
#include <TestUtils.h>

namespace Tests
{
    class CommandRunnerTests : public ::testing::Test
    {
    protected:
        std::shared_ptr<CommandRunner> commandRunner;

        static const char component[];
        static const char desiredObject[];
        static const char reportedObject[];
        static const std::chrono::milliseconds timeout;

        void SetUp() override;
        void TearDown() override;

        std::string Id();
    };

    const char CommandRunnerTests::component[] = "CommandRunner";
    const char CommandRunnerTests::desiredObject[] = "CommandArguments";
    const char CommandRunnerTests::reportedObject[] = "CommandStatus";
    const std::chrono::milliseconds CommandRunnerTests::timeout(5000);

    void CommandRunnerTests::SetUp()
    {
        this->commandRunner = std::make_shared<CommandRunner>("CommandRunner_Test_Client", 0, false);
    }

    void CommandRunnerTests::TearDown()
    {
        this->commandRunner.reset();
    }

    std::string CommandRunnerTests::Id()
    {
        static int id = 0;
        return std::to_string(id++);
    }

    TEST_F(CommandRunnerTests, Set_InvalidComponent)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "echo 'hello world'", Command::Action::RunCommand, 0, false);
        std::string desiredPayload = Command::Arguments::Serialize(arguments);

        EXPECT_EQ(EINVAL, commandRunner->Set("invalid", desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        EXPECT_EQ(EINVAL, commandRunner->Set(desiredObject, desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        EXPECT_EQ(EINVAL, commandRunner->Set(reportedObject, desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
    }

    TEST_F(CommandRunnerTests, Set_InvalidObject)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "echo 'hello world'", Command::Action::RunCommand, 0, false);
        std::string desiredPayload = Command::Arguments::Serialize(arguments);

        EXPECT_EQ(EINVAL, commandRunner->Set(component, "invalid", (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        EXPECT_EQ(EINVAL, commandRunner->Set(component, component, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        EXPECT_EQ(EINVAL, commandRunner->Set(component, reportedObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
    }

    TEST_F(CommandRunnerTests, Set_InvalidPayload)
    {
        std::string invalidPayload = "InvalidPayload";
        EXPECT_EQ(EINVAL, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(invalidPayload.c_str()), invalidPayload.size()));
    }

    TEST_F(CommandRunnerTests, Get_InvalidComponent)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(EINVAL, commandRunner->Get("invalid", reportedObject, &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, commandRunner->Get(desiredObject, reportedObject, &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, commandRunner->Get(reportedObject, reportedObject, &payload, &payloadSizeBytes));
    }

    TEST_F(CommandRunnerTests, Get_InvalidObject)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(EINVAL, commandRunner->Get(component, "invalid", &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, commandRunner->Get(component, component, &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, commandRunner->Get(component, desiredObject, &payload, &payloadSizeBytes));
    }

    TEST_F(CommandRunnerTests, Get_InvalidPayload)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(EINVAL, commandRunner->Get(component, desiredObject, nullptr, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, commandRunner->Get(component, desiredObject, &payload, nullptr));
    }

    TEST_F(CommandRunnerTests, RunCommand)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "echo 'hello world'", Command::Action::RunCommand, 0, false);
        Command::Status status(id, 0, "hello world\n", Command::State::Succeeded);

        std::string desiredPayload = Command::Arguments::Serialize(arguments);
        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        commandRunner->WaitForCommands();

        EXPECT_EQ(MMI_OK, commandRunner->Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
    }

    TEST_F(CommandRunnerTests, RunCommand_Timeout)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "sleep 10s", Command::Action::RunCommand, 1, false);
        Command::Status status(id, ETIME, "", Command::State::TimedOut);

        std::string desiredPayload = Command::Arguments::Serialize(arguments);
        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        commandRunner->WaitForCommands();

        EXPECT_EQ(MMI_OK, commandRunner->Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
    }

    TEST_F(CommandRunnerTests, RunCommand_SingleLineTextResult)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "echo 'single\nline'", Command::Action::RunCommand, 0, true);
        Command::Status status(id, 0, "single line ", Command::State::Succeeded);

        std::string desiredPayload = Command::Arguments::Serialize(arguments);
        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        commandRunner->WaitForCommands();

        EXPECT_EQ(MMI_OK, commandRunner->Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
    }

    TEST_F(CommandRunnerTests, RunCommand_LimitedPayloadSize)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "echo 'hello world'", Command::Action::RunCommand, 0, false);

        std::string expectedTextResult = "hello";
        Command::Status status(id, 0, expectedTextResult, Command::State::Succeeded);
        std::string desiredPayload = Command::Arguments::Serialize(arguments);

        // Create an empty status to estimate the size of a serialized status
        Command::Status emptyStatus(id, 0, "", Command::State::Succeeded);

        // Add expected text result length and null terminator to the size
        unsigned int limitedPayloadSize = Command::Status::Serialize(emptyStatus).length() + expectedTextResult.size() + 1;

        CommandRunner commandRunner("Limited_Payload_Client", limitedPayloadSize, false);
        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, commandRunner.Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        commandRunner.WaitForCommands();

        EXPECT_EQ(MMI_OK, commandRunner.Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
    }

    TEST_F(CommandRunnerTests, RunCommand_MaximumCacheSize)
    {
        std::vector<std::pair<std::string, Command::Status>> expectedResults;

        // Fill the cache with the max number of commands
        for (unsigned int i = 0; i < CommandRunner::MAX_CACHE_SIZE; i++)
        {
            std::string id = Id();
            Command::Arguments arguments(id, "echo '" + id + "'", Command::Action::RunCommand, 0, false);
            Command::Status status(id, 0, id + "\n", Command::State::Succeeded);
            expectedResults.push_back(std::make_pair(id, status));

            std::string desiredPayload = Command::Arguments::Serialize(arguments);
            EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        }

        commandRunner->WaitForCommands();

        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        // Check that all of the commands are in the cache
        for (auto& expectedResult : expectedResults)
        {
            Command::Arguments arguments(expectedResult.first, "", Command::Action::RefreshCommandStatus, 0, false);
            std::string refresh = Command::Arguments::Serialize(arguments);

            EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(refresh.c_str()), refresh.size()));
            EXPECT_EQ(MMI_OK, commandRunner->Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
            EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(expectedResult.second), std::string(reportedPayload, payloadSizeBytes)));
        }

        // Add one more command to the cache
        std::string id = Id();
        Command::Arguments extraCommand(id, "echo '" + id + "'", Command::Action::RunCommand, 0, false);
        Command::Status lastStatus(id, 0, id + "\n", Command::State::Succeeded);

        std::string desiredPayload = Command::Arguments::Serialize(extraCommand);
        EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        commandRunner->WaitForCommands();

        // Get the last command from the cache
        EXPECT_EQ(MMI_OK, commandRunner->Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(lastStatus), std::string(reportedPayload, payloadSizeBytes)));

        // The first command should have been removed from the cache
        Command::Arguments refreshFirstCommand(expectedResults[0].first, "", Command::Action::RefreshCommandStatus, 0, false);
        std::string refresh = Command::Arguments::Serialize(refreshFirstCommand);
        EXPECT_EQ(EINVAL, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(refresh.c_str()), refresh.size()));

        // The last command should still be reported (set as command to report) and in the cache
        EXPECT_EQ(MMI_OK, commandRunner->Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(lastStatus), std::string(reportedPayload, payloadSizeBytes)));
    }

    TEST_F(CommandRunnerTests, RefreshCommand)
    {
        std::string id1 = Id();
        std::string id2 = Id();

        Command::Arguments arguments1(id1, "echo 'command 1'", Command::Action::RunCommand, 0, false);
        Command::Arguments arguments2(id2, "echo 'command 2'", Command::Action::RunCommand, 0, false);

        Command::Status status1(id1, 0, "command 1\n", Command::State::Succeeded);
        Command::Status status2(id2, 0, "command 2\n", Command::State::Succeeded);

        std::string desiredPayload1 = Command::Arguments::Serialize(arguments1);
        std::string desiredPayload2 = Command::Arguments::Serialize(arguments2);

        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload1.c_str()), desiredPayload1.size()));
        EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload2.c_str()), desiredPayload2.size()));

        commandRunner->WaitForCommands();

        // The last run command should be reported
        EXPECT_EQ(MMI_OK, commandRunner->Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status2), std::string(reportedPayload, payloadSizeBytes)));

        // Refresh the command
        Command::Arguments refresh(id1, "", Command::Action::RefreshCommandStatus, 0, false);
        std::string refreshPayload = Command::Arguments::Serialize(refresh);
        EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(refreshPayload.c_str()), refreshPayload.size()));

        commandRunner->WaitForCommands();

        // The refreshed command should be reported
        EXPECT_EQ(MMI_OK, commandRunner->Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status1), std::string(reportedPayload, payloadSizeBytes)));
    }

    TEST_F(CommandRunnerTests, CancelCommand)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "sleep 10s", Command::Action::RunCommand, 0, false);
        Command::Arguments cancelCommand(id, "", Command::Action::CancelCommand, 0, false);
        Command::Status status(arguments.id, ECANCELED, "", Command::State::Canceled);

        std::string desiredPayload = Command::Arguments::Serialize(arguments);
        std::string cancelPayload = Command::Arguments::Serialize(cancelCommand);

        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(cancelPayload.c_str()), cancelPayload.size()));

        commandRunner->WaitForCommands();

        EXPECT_EQ(MMI_OK, commandRunner->Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
    }

    TEST_F(CommandRunnerTests, RepeatCommandId)
    {
        std::string id = Id();
        Command::Arguments arguments1(id, "echo 'hello world'", Command::Action::RunCommand, 0, false);
        Command::Arguments arguments2(id, "echo 'repeated command id'", Command::Action::RunCommand, 0, false);
        Command::Status status(id, 0, "hello world\n", Command::State::Succeeded);

        std::string desiredPayload1 = Command::Arguments::Serialize(arguments1);
        std::string desiredPayload2 = Command::Arguments::Serialize(arguments2);

        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload1.c_str()), desiredPayload1.size()));
        commandRunner->WaitForCommands();
        EXPECT_EQ(EINVAL, commandRunner->Set(component, desiredObject, (MMI_JSON_STRING)(desiredPayload2.c_str()), desiredPayload2.size()));

        EXPECT_EQ(MMI_OK, commandRunner->Get(component, reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
    }
} // namespace Tests