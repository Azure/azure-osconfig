// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CliOptions.hpp>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>

using ComplianceEngine::Assessor::Command;
using ComplianceEngine::Assessor::Format;
using ComplianceEngine::Assessor::Options;
using ComplianceEngine::Assessor::ParseCommandLine;
using ComplianceEngine::Assessor::PrintHelp;

namespace
{

// Argv helper: takes ownership of a vector of strings and exposes a writable
// Argv. Lifetime of the returned Argv equals the lifetime of the helper.
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

TEST(CliOptionsTest, DefensiveArgcZero)
{
    char* dummy[] = {nullptr};
    auto result = ParseCommandLine(0, dummy);
    ASSERT_FALSE(result.HasValue());
}

TEST(CliOptionsTest, DefensiveNullArgv)
{
    auto result = ParseCommandLine(1, nullptr);
    ASSERT_FALSE(result.HasValue());
}

TEST(CliOptionsTest, HelpShortCircuitsBeforePositional)
{
    ArgvHelper a{"prog", "-h"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().command, Command::Help);
}

TEST(CliOptionsTest, LongHelpShortCircuits)
{
    ArgvHelper a{"prog", "--help"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().command, Command::Help);
}

TEST(CliOptionsTest, VersionShortCircuits)
{
    ArgvHelper a{"prog", "-V"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().command, Command::Version);
}

TEST(CliOptionsTest, MissingCommandIsError)
{
    ArgvHelper a{"prog"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_FALSE(result.HasValue());
    EXPECT_NE(result.Error().message.find("audit"), std::string::npos);
}

TEST(CliOptionsTest, UnknownCommandIsError)
{
    ArgvHelper a{"prog", "stomp"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_FALSE(result.HasValue());
    EXPECT_NE(result.Error().message.find("stomp"), std::string::npos);
}

TEST(CliOptionsTest, AuditCommand)
{
    ArgvHelper a{"prog", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().command, Command::Audit);
    EXPECT_TRUE(result.Value().input.empty());
}

TEST(CliOptionsTest, RemediateCommandWithInput)
{
    ArgvHelper a{"prog", "remediate", "/tmp/x.mof"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().command, Command::Remediate);
    EXPECT_EQ(result.Value().input, "/tmp/x.mof");
}

TEST(CliOptionsTest, TooManyPositionalsIsError)
{
    ArgvHelper a{"prog", "audit", "/tmp/a", "/tmp/b"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_FALSE(result.HasValue());
}

TEST(CliOptionsTest, ShortLogFile)
{
    ArgvHelper a{"prog", "-L", "/tmp/log", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value().logFile.HasValue());
    EXPECT_EQ(result.Value().logFile.Value(), "/tmp/log");
}

TEST(CliOptionsTest, LongLogFile)
{
    ArgvHelper a{"prog", "--log-file", "/tmp/log", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value().logFile.HasValue());
    EXPECT_EQ(result.Value().logFile.Value(), "/tmp/log");
}

TEST(CliOptionsTest, OldLowercaseLIsRejected)
{
    // -l used to mean --log-file. The flag was renamed to -L during the
    // production hardening pass. Scripted callers should fail loudly rather
    // than silently misinterpret the next token.
    ArgvHelper a{"prog", "-l", "/tmp/log", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_FALSE(result.HasValue());
}

TEST(CliOptionsTest, DuplicateLogFileIsError)
{
    ArgvHelper a{"prog", "-L", "/tmp/a", "-L", "/tmp/b", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_FALSE(result.HasValue());
}

TEST(CliOptionsTest, EmptyLogFileValueIsError)
{
    ArgvHelper a{"prog", "--log-file=", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_FALSE(result.HasValue());
}

TEST(CliOptionsTest, FormatJson)
{
    ArgvHelper a{"prog", "-f", "json", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value().format.HasValue());
    EXPECT_EQ(result.Value().format.Value(), Format::Json);
}

TEST(CliOptionsTest, FormatNestedListCaseInsensitive)
{
    ArgvHelper a{"prog", "--format", "Nested-List", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value().format.HasValue());
    EXPECT_EQ(result.Value().format.Value(), Format::NestedList);
}

TEST(CliOptionsTest, FormatCompactList)
{
    ArgvHelper a{"prog", "-f", "compact-list", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().format.Value(), Format::CompactList);
}

TEST(CliOptionsTest, FormatDebug)
{
    ArgvHelper a{"prog", "-f", "DEBUG", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().format.Value(), Format::Debug);
}

TEST(CliOptionsTest, FormatInvalidIsError)
{
    ArgvHelper a{"prog", "-f", "yaml", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_FALSE(result.HasValue());
}

TEST(CliOptionsTest, DuplicateFormatIsError)
{
    ArgvHelper a{"prog", "-f", "json", "-f", "debug", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_FALSE(result.HasValue());
}

TEST(CliOptionsTest, SectionShort)
{
    ArgvHelper a{"prog", "-s", "1.1", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value().section.HasValue());
    EXPECT_EQ(result.Value().section.Value(), "1.1");
}

TEST(CliOptionsTest, DuplicateSectionIsError)
{
    ArgvHelper a{"prog", "-s", "1.1", "-s", "1.2", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_FALSE(result.HasValue());
}

TEST(CliOptionsTest, VerboseAndDebugFlags)
{
    ArgvHelper a{"prog", "-v", "-d", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value().verbose);
    EXPECT_TRUE(result.Value().debug);
}

TEST(CliOptionsTest, UnknownOptionIsError)
{
    ArgvHelper a{"prog", "-Z", "audit"};
    auto result = ParseCommandLine(a.Argc(), a.Argv());
    ASSERT_FALSE(result.HasValue());
}

TEST(CliOptionsTest, RepeatedCallsAreIndependent)
{
    {
        ArgvHelper a{"prog", "-v", "audit", "/tmp/x"};
        auto result = ParseCommandLine(a.Argc(), a.Argv());
        ASSERT_TRUE(result.HasValue());
        EXPECT_EQ(result.Value().command, Command::Audit);
        EXPECT_TRUE(result.Value().verbose);
        EXPECT_EQ(result.Value().input, "/tmp/x");
    }
    {
        ArgvHelper a{"prog", "remediate"};
        auto result = ParseCommandLine(a.Argc(), a.Argv());
        ASSERT_TRUE(result.HasValue());
        EXPECT_EQ(result.Value().command, Command::Remediate);
        EXPECT_FALSE(result.Value().verbose);
        EXPECT_TRUE(result.Value().input.empty());
    }
    {
        ArgvHelper a{"prog", "-h"};
        auto result = ParseCommandLine(a.Argc(), a.Argv());
        ASSERT_TRUE(result.HasValue());
        EXPECT_EQ(result.Value().command, Command::Help);
    }
}

TEST(CliOptionsTest, PrintHelpHandlesNullProgramName)
{
    std::ostringstream os;
    PrintHelp(os, nullptr);
    EXPECT_NE(os.str().find("compliance-engine-assessor"), std::string::npos);
    EXPECT_NE(os.str().find("--format"), std::string::npos);
    EXPECT_NE(os.str().find("--log-file"), std::string::npos);
}

TEST(CliOptionsTest, PrintHelpHandlesEmptyProgramName)
{
    std::ostringstream os;
    PrintHelp(os, "");
    EXPECT_NE(os.str().find("compliance-engine-assessor"), std::string::npos);
}

TEST(CliOptionsTest, PrintHelpUsesProgramName)
{
    std::ostringstream os;
    PrintHelp(os, "/usr/local/bin/my-asses");
    EXPECT_NE(os.str().find("/usr/local/bin/my-asses"), std::string::npos);
}
