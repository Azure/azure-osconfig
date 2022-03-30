// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Command.h>
#include <TestUtils.h>

namespace Tests
{
    class CommandTests : public testing::Test
    {
    protected:
        static const std::string m_id;
        std::shared_ptr<Command> m_command;

        void SetUp() override;
        void TearDown() override;
    };

    void CommandTests::SetUp()
    {
        this->m_command = std::make_shared<Command>(m_id, "echo 'test'", 0, false);
        EXPECT_STREQ(m_id.c_str(), this->m_command->GetId().c_str());
    }

    void CommandTests::TearDown()
    {
        m_command.reset();
    }

    const std::string CommandTests::m_id = "CommandTest_Id";

    TEST_F(CommandTests, Execute)
    {
        EXPECT_EQ(0, m_command->Execute(0));

        Command::Status status = m_command->GetStatus();
        EXPECT_STREQ(m_id.c_str(), status.m_id.c_str());
        EXPECT_EQ(0, status.m_exitCode);
        EXPECT_STREQ("test\n", status.m_textResult.c_str());
        EXPECT_EQ(Command::State::Succeeded, status.m_state);
    }

    TEST_F(CommandTests, Cancel)
    {
        EXPECT_EQ(0, m_command->Cancel());
        EXPECT_TRUE(m_command->IsCanceled());
        EXPECT_EQ(ECANCELED, m_command->Cancel());
        EXPECT_EQ(ECANCELED, m_command->Execute(0));
    }

    TEST_F(CommandTests, Status)
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

    TEST_F(CommandTests, Equality)
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

    TEST(CommandArgumentsTests, Deserialize)
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

    TEST(CommandStatusTests, Serialize)
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

    TEST(CommandStatusTests, SerializeSkipTextResult)
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

    TEST(CommandStatusTests, Deserialize)
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