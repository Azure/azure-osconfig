// Smoke tests for the extracted CLI options library. Verifies the lib links
// and that ParseCommandLine accepts the same inputs the binary does today.
// Behavioural hardening (duplicate-flag rejection, empty-optarg rejection,
// breaking renames) lands in follow-up PRs along with their own tests.

#include <CliOptions.hpp>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using ComplianceEngine::Assessor::Command;
using ComplianceEngine::Assessor::Format;
using ComplianceEngine::Assessor::ParseCommandLine;
using ComplianceEngine::Assessor::PrintHelp;

namespace
{
struct ArgvHelper
{
    std::vector<std::string> storage;
    std::vector<char*> pointers;

    explicit ArgvHelper(std::initializer_list<std::string> args)
        : storage(args)
    {
        pointers.reserve(storage.size() + 1);
        for (auto& s : storage)
        {
            pointers.push_back(&s[0]);
        }
        pointers.push_back(nullptr);
    }

    int Argc() const
    {
        return static_cast<int>(storage.size());
    }
    char** Argv()
    {
        return pointers.data();
    }
};
} // namespace

TEST(CliOptionsSmokeTest, HelpIsRecognised)
{
    ArgvHelper a{"prog", "-h"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().command, Command::Help);
}

TEST(CliOptionsSmokeTest, AuditWithInputFilename)
{
    ArgvHelper a{"prog", "audit", "/tmp/x.mof"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().command, Command::Audit);
    EXPECT_EQ(result.Value().input, "/tmp/x.mof");
}

TEST(CliOptionsSmokeTest, FormatOnAuditIsRejected)
{
    // audit/remediate always emit the canonical JSON; --format is format-only.
    ArgvHelper a{"prog", "-f", "junit", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    EXPECT_FALSE(result.HasValue());
}

TEST(CliOptionsSmokeTest, FormatSubcommandDefaultsToJunit)
{
    ArgvHelper a{"prog", "format"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().command, Command::Format);
    ASSERT_TRUE(result.Value().format.HasValue());
    EXPECT_EQ(result.Value().format.Value(), Format::Junit);
}

TEST(CliOptionsSmokeTest, FormatSubcommandWithFileAndSuiteName)
{
    ArgvHelper a{"prog", "-f", "junit", "--suite-name", "cis_ubuntu", "format", "result.json"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().command, Command::Format);
    EXPECT_EQ(result.Value().input, "result.json");
    ASSERT_TRUE(result.Value().suiteName.HasValue());
    EXPECT_EQ(result.Value().suiteName.Value(), "cis_ubuntu");
    ASSERT_TRUE(result.Value().format.HasValue());
    EXPECT_EQ(result.Value().format.Value(), Format::Junit);
}

TEST(CliOptionsSmokeTest, SuiteNameOnAuditIsRejected)
{
    ArgvHelper a{"prog", "--suite-name", "x", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    EXPECT_FALSE(result.HasValue());
}

TEST(CliOptionsSmokeTest, SectionOnFormatIsRejected)
{
    ArgvHelper a{"prog", "-s", "1.1", "format"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    EXPECT_FALSE(result.HasValue());
}

TEST(CliOptionsSmokeTest, InvalidFormatValueIsRejected)
{
    ArgvHelper a{"prog", "-f", "xml", "format"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    EXPECT_FALSE(result.HasValue());
}

TEST(CliOptionsSmokeTest, ContinueOnErrorIsParsed)
{
    ArgvHelper a{"prog", "-e", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value().continueOnError);
}

TEST(CliOptionsSmokeTest, MissingCommandIsError)
{
    ArgvHelper a{"prog"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    EXPECT_FALSE(result.HasValue());
}

TEST(CliOptionsSmokeTest, PrintHelpListsSubcommands)
{
    testing::internal::CaptureStdout();
    PrintHelp("prog");
    const std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("Commands:"), std::string::npos);
    EXPECT_NE(out.find("audit"), std::string::npos);
    EXPECT_NE(out.find("remediate"), std::string::npos);
    EXPECT_NE(out.find("format"), std::string::npos);
}

TEST(CliOptionsSmokeTest, InvalidCommandIsError)
{
    ArgvHelper a{"prog", "bogus"};
    EXPECT_FALSE(ParseCommandLine(a.Argc(), a.Argv()).HasValue());
}

TEST(CliOptionsSmokeTest, TooManyArgumentsIsError)
{
    ArgvHelper a{"prog", "audit", "a.mof", "extra"};
    EXPECT_FALSE(ParseCommandLine(a.Argc(), a.Argv()).HasValue());
}

TEST(CliOptionsSmokeTest, EmptySectionIsError)
{
    ArgvHelper a{"prog", "-s", "", "audit"};
    EXPECT_FALSE(ParseCommandLine(a.Argc(), a.Argv()).HasValue());
}

TEST(CliOptionsSmokeTest, UnknownOptionIsError)
{
    ArgvHelper a{"prog", "-z", "audit"};
    EXPECT_FALSE(ParseCommandLine(a.Argc(), a.Argv()).HasValue());
}
