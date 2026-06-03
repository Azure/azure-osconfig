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

TEST(CliOptionsSmokeTest, FormatJsonIsParsed)
{
    ArgvHelper a{"prog", "-f", "json", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value().format.HasValue());
    EXPECT_EQ(result.Value().format.Value(), Format::Json);
}

TEST(CliOptionsSmokeTest, MissingCommandIsError)
{
    ArgvHelper a{"prog"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    EXPECT_FALSE(result.HasValue());
}
