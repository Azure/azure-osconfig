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
        static const std::string id;
        std::shared_ptr<Command> command;

        void SetUp() override;
        void TearDown() override;
    };

    void CommandTests::SetUp()
    {
        this->command = std::make_shared<Command>(id, "echo 'test'", 0, false);
        EXPECT_STREQ(id.c_str(), this->command->GetId().c_str());
    }

    void CommandTests::TearDown()
    {
        command.reset();
    }

    const std::string CommandTests::id = "CommandTests_Id";

    TEST_F(CommandTests, Execute)
    {
        EXPECT_EQ(0, command->Execute(0));

        Command::Status status = command->GetStatus();
        EXPECT_STREQ(id.c_str(), status.m_id.c_str());
        EXPECT_EQ(0, status.m_exitCode);
        EXPECT_STREQ("test\n", status.m_textResult.c_str());
        EXPECT_EQ(Command::State::Succeeded, status.m_state);
    }

    TEST_F(CommandTests, Cancel)
    {
        EXPECT_EQ(0, command->Cancel());
        EXPECT_TRUE(command->IsCanceled());
        EXPECT_EQ(ECANCELED, command->Cancel());
        EXPECT_EQ(ECANCELED, command->Execute(0));
    }

    TEST_F(CommandTests, Status)
    {
        Command::Status defaultStatus = command->GetStatus();
        EXPECT_STREQ(id.c_str(), defaultStatus.m_id.c_str());
        EXPECT_EQ(0, defaultStatus.m_exitCode);
        EXPECT_STREQ("", defaultStatus.m_textResult.c_str());
        EXPECT_EQ(Command::State::Unknown, defaultStatus.m_state);

        command->SetStatus(EXIT_SUCCESS);
        EXPECT_EQ(EXIT_SUCCESS, command->GetStatus().m_exitCode);
        EXPECT_EQ(Command::State::Succeeded, command->GetStatus().m_state);

        command->SetStatus(ECANCELED);
        EXPECT_EQ(ECANCELED, command->GetStatus().m_exitCode);
        EXPECT_EQ(Command::State::Canceled, command->GetStatus().m_state);

        command->SetStatus(ETIME);
        EXPECT_EQ(ETIME, command->GetStatus().m_exitCode);
        EXPECT_EQ(Command::State::TimedOut, command->GetStatus().m_state);

        command->SetStatus(-1);
        EXPECT_EQ(-1, command->GetStatus().m_exitCode);
        EXPECT_EQ(Command::State::Failed, command->GetStatus().m_state);

        command->SetStatus(EXIT_SUCCESS, "test");
        EXPECT_STREQ("test", command->GetStatus().m_textResult.c_str());
        EXPECT_EQ(Command::State::Succeeded, command->GetStatus().m_state);
    }

    TEST(CommandArgumentsTests, Deserialize)
    {
        const std::string json = R"""({
            "CommandId": "id",
            "Arguments": "echo 'hello world'",
            "Action": 3,
            "Timeout": 123,
            "SingleLineTextResult": true
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
            "CommandId": "id",
            "ResultCode": 123,
            "TextResult": "text result...",
            "CurrentState": 2
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
            "CommandId": "id",
            "ResultCode": 123,
            "CurrentState": 2
        })""";

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        Command::Status::Serialize(writer, status, false);

        EXPECT_TRUE(IsJsonEq(expected, buffer.GetString()));
    }

    TEST(CommandStatusTests, Deserialize)
    {
        const std::string json = R"""({
            "CommandId": "id",
            "ResultCode": 123,
            "TextResult": "text result...",
            "CurrentState": 2
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