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
        std::shared_ptr<CommandRunner> m_commandRunner;

        static const char m_component[];
        static const char m_desiredObject[];
        static const char m_reportedObject[];

        static const std::string m_id;
        std::shared_ptr<Command> m_command;

        void SetUp() override;
        void TearDown() override;

        std::string Id();
    };

    const char CommandRunnerTests::m_component[] = "CommandRunner";
    const char CommandRunnerTests::m_desiredObject[] = "commandArguments";
    const char CommandRunnerTests::m_reportedObject[] = "commandStatus";

    const std::string CommandRunnerTests::m_id = "CommandTest_Id";

    void CommandRunnerTests::SetUp()
    {
        m_commandRunner = std::make_shared<CommandRunner>("CommandRunner_Test_Client", 0, false);
        m_command = std::make_shared<Command>(m_id, "echo 'test'", 0, false);
        EXPECT_STREQ(m_id.c_str(), this->m_command->GetId().c_str());
    }

    void CommandRunnerTests::TearDown()
    {
        m_commandRunner.reset();
        m_command.reset();
    }

    std::string CommandRunnerTests::Id()
    {
        static int id = 0;
        return std::to_string(id++);
    }

    TEST_F(CommandRunnerTests, SetInvalidComponent)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "echo 'hello world'", Command::Action::RunCommand, 0, false);
        std::string desiredPayload = Command::Arguments::Serialize(arguments);

        EXPECT_EQ(EINVAL, m_commandRunner->Set("invalid", m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        EXPECT_EQ(EINVAL, m_commandRunner->Set(m_desiredObject, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        EXPECT_EQ(EINVAL, m_commandRunner->Set(m_reportedObject, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
    }

    TEST_F(CommandRunnerTests, SetInvalidObject)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "echo 'hello world'", Command::Action::RunCommand, 0, false);
        std::string desiredPayload = Command::Arguments::Serialize(arguments);

        EXPECT_EQ(EINVAL, m_commandRunner->Set(m_component, "invalid", (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        EXPECT_EQ(EINVAL, m_commandRunner->Set(m_component, m_component, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        EXPECT_EQ(EINVAL, m_commandRunner->Set(m_component, m_reportedObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
    }

    TEST_F(CommandRunnerTests, SetInvalidPayload)
    {
        std::string invalidPayload = "InvalidPayload";
        EXPECT_EQ(EINVAL, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(invalidPayload.c_str()), invalidPayload.size()));
    }

    TEST_F(CommandRunnerTests, GetInvalidComponent)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(EINVAL, m_commandRunner->Get("invalid", m_reportedObject, &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, m_commandRunner->Get(m_desiredObject, m_reportedObject, &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, m_commandRunner->Get(m_reportedObject, m_reportedObject, &payload, &payloadSizeBytes));
    }

    TEST_F(CommandRunnerTests, GetInvalidObject)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(EINVAL, m_commandRunner->Get(m_component, "invalid", &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, m_commandRunner->Get(m_component, m_component, &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, m_commandRunner->Get(m_component, m_desiredObject, &payload, &payloadSizeBytes));
    }

    TEST_F(CommandRunnerTests, GetInvalidPayload)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(EINVAL, m_commandRunner->Get(m_component, m_desiredObject, nullptr, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, m_commandRunner->Get(m_component, m_desiredObject, &payload, nullptr));
    }

    TEST_F(CommandRunnerTests, RunCommand)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "echo 'hello world'", Command::Action::RunCommand, 0, false);
        Command::Status status(id, 0, "hello world\n", Command::State::Succeeded);

        std::string desiredPayload = Command::Arguments::Serialize(arguments);
        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        m_commandRunner->WaitForCommands();

        EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;
    }

    TEST_F(CommandRunnerTests, RunCommandTimeout)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "sleep 10s", Command::Action::RunCommand, 1, false);
        Command::Status status(id, ETIME, "", Command::State::TimedOut);

        std::string desiredPayload = Command::Arguments::Serialize(arguments);
        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        m_commandRunner->WaitForCommands();

        EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;
    }

    TEST_F(CommandRunnerTests, RunCommandSingleLineTextResult)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "echo 'single\nline'", Command::Action::RunCommand, 0, true);
        Command::Status status(id, 0, "single line ", Command::State::Succeeded);

        std::string desiredPayload = Command::Arguments::Serialize(arguments);
        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        m_commandRunner->WaitForCommands();

        EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;
    }

    TEST_F(CommandRunnerTests, RunCommandLimitedPayloadSize)
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

        EXPECT_EQ(MMI_OK, commandRunner.Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        commandRunner.WaitForCommands();

        EXPECT_EQ(MMI_OK, commandRunner.Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;
    }

    TEST_F(CommandRunnerTests, RunCommandMaximumCacheSize)
    {
        std::vector<std::pair<std::string, Command::Status>> expectedResults;

        // Fill the cache with the max number of commands
        for (unsigned int i = 0; i < CommandRunner::m_maxCacheSize; i++)
        {
            std::string id = Id();
            Command::Arguments arguments(id, "echo '" + id + "'", Command::Action::RunCommand, 0, false);
            Command::Status status(id, 0, id + "\n", Command::State::Succeeded);
            expectedResults.push_back(std::make_pair(id, status));

            std::string desiredPayload = Command::Arguments::Serialize(arguments);
            EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        }

        m_commandRunner->WaitForCommands();

        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        // Check that all of the commands are in the cache
        for (auto& expectedResult : expectedResults)
        {
            Command::Arguments arguments(expectedResult.first, "", Command::Action::RefreshCommandStatus, 0, false);
            std::string refresh = Command::Arguments::Serialize(arguments);

            EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(refresh.c_str()), refresh.size()));
            EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
            EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(expectedResult.second), std::string(reportedPayload, payloadSizeBytes)));
            delete[] reportedPayload;
        }

        // Add one more command to the cache
        std::string id = Id();
        Command::Arguments extraCommand(id, "echo '" + id + "'", Command::Action::RunCommand, 0, false);
        Command::Status lastStatus(id, 0, id + "\n", Command::State::Succeeded);

        std::string desiredPayload = Command::Arguments::Serialize(extraCommand);
        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        m_commandRunner->WaitForCommands();

        // Get the last command from the cache
        EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(lastStatus), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;

        // The first command should have been removed from the cache
        Command::Arguments refreshFirstCommand(expectedResults[0].first, "", Command::Action::RefreshCommandStatus, 0, false);
        std::string refresh = Command::Arguments::Serialize(refreshFirstCommand);
        EXPECT_EQ(EINVAL, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(refresh.c_str()), refresh.size()));

        // The last command should still be reported (set as command to report) and in the cache
        EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(lastStatus), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;
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

        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload1.c_str()), desiredPayload1.size()));
        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload2.c_str()), desiredPayload2.size()));

        m_commandRunner->WaitForCommands();

        // The last run command should be reported
        EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status2), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;

        // Refresh the command
        Command::Arguments refresh(id1, "", Command::Action::RefreshCommandStatus, 0, false);
        std::string refreshPayload = Command::Arguments::Serialize(refresh);
        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(refreshPayload.c_str()), refreshPayload.size()));

        m_commandRunner->WaitForCommands();

        // The refreshed command should be reported
        EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status1), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;
    }

    TEST_F(CommandRunnerTests, CancelCommandInProgress)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "sleep 10s", Command::Action::RunCommand, 0, false);
        Command::Arguments cancelCommand(id, "", Command::Action::CancelCommand, 0, false);
        Command::Status status(arguments.m_id, ECANCELED, "", Command::State::Canceled);

        std::string desiredPayload = Command::Arguments::Serialize(arguments);
        std::string cancelPayload = Command::Arguments::Serialize(cancelCommand);

        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(cancelPayload.c_str()), cancelPayload.size()));

        m_commandRunner->WaitForCommands();

        EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;
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

        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload1.c_str()), desiredPayload1.size()));
        m_commandRunner->WaitForCommands();
        EXPECT_EQ(EINVAL, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload2.c_str()), desiredPayload2.size()));

        EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;
    }

    TEST_F(CommandRunnerTests, RepeatCommand)
    {
        std::string id = Id();
        Command::Arguments arguments(id, "echo 'hello world'", Command::Action::RunCommand, 0, false);
        Command::Status status(id, 0, "hello world\n", Command::State::Succeeded);

        std::string desiredPayload = Command::Arguments::Serialize(arguments);

        MMI_JSON_STRING reportedPayload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));
        m_commandRunner->WaitForCommands();
        EXPECT_EQ(MMI_OK, m_commandRunner->Set(m_component, m_desiredObject, (MMI_JSON_STRING)(desiredPayload.c_str()), desiredPayload.size()));

        EXPECT_EQ(MMI_OK, m_commandRunner->Get(m_component, m_reportedObject, &reportedPayload, &payloadSizeBytes));
        EXPECT_TRUE(IsJsonEq(Command::Status::Serialize(status), std::string(reportedPayload, payloadSizeBytes)));
        delete[] reportedPayload;
    }

    TEST_F(CommandRunnerTests, ExecuteCommand)
    {
        EXPECT_EQ(0, m_command->Execute(0));

        Command::Status status = m_command->GetStatus();
        EXPECT_STREQ(m_id.c_str(), status.m_id.c_str());
        EXPECT_EQ(0, status.m_exitCode);
        EXPECT_STREQ("test\n", status.m_textResult.c_str());
        EXPECT_EQ(Command::State::Succeeded, status.m_state);
    }

    TEST_F(CommandRunnerTests, CancelCommand)
    {
        EXPECT_EQ(0, m_command->Cancel());
        EXPECT_TRUE(m_command->IsCanceled());
        EXPECT_EQ(ECANCELED, m_command->Cancel());
        EXPECT_EQ(ECANCELED, m_command->Execute(0));
    }

    TEST_F(CommandRunnerTests, CommandStatus)
    {
        Command::Status defaultStatus = m_command->GetStatus();
        EXPECT_STREQ(m_id.c_str(), defaultStatus.m_id.c_str());
        EXPECT_EQ(0, defaultStatus.m_exitCode);
        EXPECT_STREQ("", defaultStatus.m_textResult.c_str());
        EXPECT_EQ(Command::State::Unknown, defaultStatus.m_state);

        m_command->SetStatus(EXIT_SUCCESS);
        EXPECT_EQ(EXIT_SUCCESS, m_command->GetStatus().m_exitCode);
        EXPECT_EQ(Command::State::Succeeded, m_command->GetStatus().m_state);

        m_command->SetStatus(ECANCELED);
        EXPECT_EQ(ECANCELED, m_command->GetStatus().m_exitCode);
        EXPECT_EQ(Command::State::Canceled, m_command->GetStatus().m_state);

        m_command->SetStatus(ETIME);
        EXPECT_EQ(ETIME, m_command->GetStatus().m_exitCode);
        EXPECT_EQ(Command::State::TimedOut, m_command->GetStatus().m_state);

        m_command->SetStatus(-1);
        EXPECT_EQ(-1, m_command->GetStatus().m_exitCode);
        EXPECT_EQ(Command::State::Failed, m_command->GetStatus().m_state);

        m_command->SetStatus(EXIT_SUCCESS, "test");
        EXPECT_STREQ("test", m_command->GetStatus().m_textResult.c_str());
        EXPECT_EQ(Command::State::Succeeded, m_command->GetStatus().m_state);
    }

    TEST_F(CommandRunnerTests, CommandEquality)
    {
        std::shared_ptr<Command> command1 = std::make_shared<Command>(m_id, "echo 'test'", 0, false);
        std::shared_ptr<Command> command2 = std::make_shared<Command>(m_id, "echo 'test'", 0, false);
        std::shared_ptr<Command> command3 = std::make_shared<Command>(m_id, "echo 'test2'", 0, false);
        std::shared_ptr<Command> command4 = std::make_shared<Command>(m_id, "echo 'test'", 1, false);
        std::shared_ptr<Command> command5 = std::make_shared<Command>(m_id, "echo 'test'", 0, true);

        EXPECT_TRUE(*command1 == *command2);
        EXPECT_FALSE(*command1 == *command3);
        EXPECT_FALSE(*command1 == *command4);
        EXPECT_FALSE(*command1 == *command5);
    }

    TEST_F(CommandRunnerTests, DeserializeCommandStatus)
    {
        const std::string json = R"""({
            "commandId": "id",
            "arguments": "echo 'hello world'",
            "action": 3,
            "timeout": 123,
            "singleLineTextResult": true
        })""";

        rapidjson::Document document;
        document.Parse(json.c_str());
        EXPECT_FALSE(document.HasParseError());

        Command::Arguments arguments = Command::Arguments::Deserialize(document);

        EXPECT_EQ("id", arguments.m_id);
        EXPECT_EQ("echo 'hello world'", arguments.m_arguments);
        EXPECT_EQ(Command::Action::RunCommand, arguments.m_action);
        EXPECT_EQ(123, arguments.m_timeout);
        EXPECT_TRUE(arguments.m_singleLineTextResult);
    }

    TEST_F(CommandRunnerTests, Serialize)
    {
        Command::Status status("id", 123, "text result...", Command::State::Succeeded);

        const std::string expected = R"""({
            "commandId": "id",
            "resultCode": 123,
            "textResult": "text result...",
            "currentState": 2
        })""";

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        Command::Status::Serialize(writer, status);

        EXPECT_TRUE(IsJsonEq(expected, buffer.GetString()));
    }

    TEST_F(CommandRunnerTests, SerializeSkipTextResult)
    {
        Command::Status status("id", 123, "text result...", Command::State::Succeeded);

        const std::string expected = R"""({
            "commandId": "id",
            "resultCode": 123,
            "currentState": 2
        })""";

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        Command::Status::Serialize(writer, status, false);

        EXPECT_TRUE(IsJsonEq(expected, buffer.GetString()));
    }

    TEST_F(CommandRunnerTests, Deserialize)
    {
        const std::string json = R"""({
            "commandId": "id",
            "resultCode": 123,
            "textResult": "text result...",
            "currentState": 2
        })""";

        rapidjson::Document document;
        document.Parse(json.c_str());

        Command::Status status = Command::Status::Deserialize(document);

        EXPECT_EQ("id", status.m_id);
        EXPECT_EQ(123, status.m_exitCode);
        EXPECT_EQ("text result...", status.m_textResult);
        EXPECT_EQ(Command::State::Succeeded, status.m_state);
    }


} // namespace Tests
