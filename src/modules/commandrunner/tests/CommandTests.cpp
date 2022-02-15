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
        EXPECT_EQ(0, command->Execute(nullptr, 0));

        Command::Status status = command->GetStatus();
        EXPECT_STREQ(id.c_str(), status.id.c_str());
        EXPECT_EQ(0, status.exitCode);
        EXPECT_STREQ("test\n", status.textResult.c_str());
        EXPECT_EQ(Command::State::Succeeded, status.state);
    }

    TEST_F(CommandTests, Cancel)
    {
        EXPECT_EQ(0, command->Cancel());
        EXPECT_TRUE(command->IsCanceled());
        EXPECT_EQ(ECANCELED, command->Cancel());
        EXPECT_EQ(ECANCELED, command->Execute(nullptr, 0));
    }

    TEST_F(CommandTests, Status)
    {
        Command::Status defaultStatus = command->GetStatus();
        EXPECT_STREQ(id.c_str(), defaultStatus.id.c_str());
        EXPECT_EQ(0, defaultStatus.exitCode);
        EXPECT_STREQ("", defaultStatus.textResult.c_str());
        EXPECT_EQ(Command::State::Unknown, defaultStatus.state);

        command->SetStatus(EXIT_SUCCESS);
        EXPECT_EQ(EXIT_SUCCESS, command->GetStatus().exitCode);
        EXPECT_EQ(Command::State::Succeeded, command->GetStatus().state);

        command->SetStatus(ECANCELED);
        EXPECT_EQ(ECANCELED, command->GetStatus().exitCode);
        EXPECT_EQ(Command::State::Canceled, command->GetStatus().state);

        command->SetStatus(ETIME);
        EXPECT_EQ(ETIME, command->GetStatus().exitCode);
        EXPECT_EQ(Command::State::TimedOut, command->GetStatus().state);

        command->SetStatus(-1);
        EXPECT_EQ(-1, command->GetStatus().exitCode);
        EXPECT_EQ(Command::State::Failed, command->GetStatus().state);

        command->SetStatus(EXIT_SUCCESS, "test");
        EXPECT_STREQ("test", command->GetStatus().textResult.c_str());
        EXPECT_EQ(Command::State::Succeeded, command->GetStatus().state);
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

        EXPECT_EQ("id", arguments.id);
        EXPECT_EQ("echo 'hello world'", arguments.command);
        EXPECT_EQ(Command::Action::RunCommand, arguments.action);
        EXPECT_EQ(123, arguments.timeout);
        EXPECT_TRUE(arguments.singleLineTextResult);
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

        EXPECT_EQ("id", status.id);
        EXPECT_EQ(123, status.exitCode);
        EXPECT_EQ("text result...", status.textResult);
        EXPECT_EQ(Command::State::Succeeded, status.state);
    }
} // namespace Tests