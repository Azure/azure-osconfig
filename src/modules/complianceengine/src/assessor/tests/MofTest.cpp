// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Unit tests for the strict, streaming MOF parser (MofResourceRange /
// MofResourceIterator). The parser validates the fixed field set the
// augmentation engine emits, rejects unknown/duplicate fields, enforces field
// constants and rule-name consistency, and bounds line length, total size, and
// entry count. These tests focus on the happy path and a broad set of
// adversarial / malformed inputs.

#include "Mof.hpp"

#include <algorithm>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

using ComplianceEngine::Error;
using ComplianceEngine::Result;
using ComplianceEngine::MOF::MofResourceRange;
using ComplianceEngine::MOF::Resource;

namespace
{
using Fields = std::vector<std::pair<std::string, std::string>>;

constexpr const char* kHeader = "instance of OsConfigResource as $OsConfigResource0ref";

// A complete, valid field set matching what the augmentation engine emits.
Fields DefaultFields()
{
    return {
        {"ResourceID", "1.1.1 Some Rule"},
        {"PayloadKey", "/cis/ubuntu/22.04/v2.0.0/1/1/1"},
        {"RuleId", "00000000-0000-0000-0000-000000000000"},
        {"ComponentName", "ComplianceEngine"},
        {"ProcedureObjectName", "procedureMyRule"},
        {"ProcedureObjectValue", "base64data=="},
        {"InitObjectName", "initMyRule"},
        {"ReportedObjectName", "auditMyRule"},
        {"ExpectedObjectValue", "PASS"},
        {"DesiredObjectName", "remediateMyRule"},
        {"DesiredObjectValue", ""},
        {"ModuleName", "GuestConfiguration"},
        {"ModuleVersion", "1.0.0"},
        {"ConfigurationName", "ComplianceEngine"},
        {"SourceInfo", "::4::5::OsConfigResource"},
    };
}

void SetField(Fields& fields, const std::string& key, const std::string& value)
{
    for (auto& f : fields)
    {
        if (f.first == key)
        {
            f.second = value;
            return;
        }
    }
    fields.emplace_back(key, value);
}

void EraseField(Fields& fields, const std::string& key)
{
    fields.erase(std::remove_if(fields.begin(), fields.end(), [&](const std::pair<std::string, std::string>& f) { return f.first == key; }), fields.end());
}

std::string RenderBlock(const std::string& header, const Fields& fields, bool withClose = true)
{
    std::string out = header + "\n{\n";
    for (const auto& f : fields)
    {
        out += "    " + f.first + " = \"" + f.second + "\";\n";
    }
    if (withClose)
    {
        out += "};\n";
    }
    return out;
}

std::string Render(const Fields& fields)
{
    return RenderBlock(kHeader, fields);
}

// Raw single entry with a custom body (between the braces). Used for syntactic
// tests that cannot be expressed via the field renderer.
std::string Wrap(const std::string& body)
{
    return std::string(kHeader) + "\n{\n" + body + "};\n";
}

// Parse the supplied text and return the first entry's Result. Returns an Error
// if the input contains no entries.
Result<Resource> ParseFirst(const std::string& text)
{
    std::istringstream stream(text);
    auto rangeResult = MofResourceRange::MakeFromStream(stream, nullptr);
    if (!rangeResult.HasValue())
    {
        return rangeResult.Error();
    }
    auto& range = rangeResult.Value();
    auto it = range.begin();
    if (it == range.end())
    {
        return Error("no entries parsed");
    }
    return *it;
}
} // namespace

// ---------------------------------------------------------------------------
// Happy path
// ---------------------------------------------------------------------------

TEST(MofParserTest, ParsesValidEntry)
{
    auto result = ParseFirst(Render(DefaultFields()));
    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    const auto& res = result.Value();
    EXPECT_EQ(res.resourceID, "1.1.1 Some Rule");
    EXPECT_EQ(res.ruleName, "MyRule");
    EXPECT_EQ(res.procedure, "base64data==");
    EXPECT_TRUE(res.hasInitAudit);
    EXPECT_EQ(res.benchmarkInfo.section, "1.1.1");
}

TEST(MofParserTest, EmptyDesiredObjectValueYieldsAbsentPayload)
{
    auto result = ParseFirst(Render(DefaultFields()));
    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    EXPECT_FALSE(result.Value().payload.HasValue());
}

TEST(MofParserTest, NonEmptyDesiredObjectValueYieldsPayload)
{
    auto fields = DefaultFields();
    SetField(fields, "DesiredObjectValue", "mask=0600");
    auto result = ParseFirst(Render(fields));
    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    ASSERT_TRUE(result.Value().payload.HasValue());
    EXPECT_EQ(result.Value().payload.Value(), "mask=0600");
}

TEST(MofParserTest, JsonEscapedDesiredObjectValue)
{
    // Production MOFs embed JSON in DesiredObjectValue with MOF-escaped inner
    // quotes; verify they round-trip to unescaped JSON.
    auto fields = DefaultFields();
    SetField(fields, "DesiredObjectValue", "{\\\"mountPoint\\\":\\\"/tmp\\\"}");
    auto result = ParseFirst(Render(fields));
    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    ASSERT_TRUE(result.Value().payload.HasValue());
    EXPECT_EQ(result.Value().payload.Value(), "{\"mountPoint\":\"/tmp\"}");
}

TEST(MofParserTest, SectionSlashesBecomeDots)
{
    auto fields = DefaultFields();
    SetField(fields, "PayloadKey", "/cis/rhel/8/v4.0.0/5/4/2/1");
    auto result = ParseFirst(Render(fields));
    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    EXPECT_EQ(result.Value().benchmarkInfo.section, "5.4.2.1");
}

TEST(MofParserTest, MultipleEntriesAreStreamed)
{
    std::string text = Render(DefaultFields()) + Render(DefaultFields());
    std::istringstream stream(text);
    auto rangeResult = MofResourceRange::MakeFromStream(stream, nullptr);
    ASSERT_TRUE(rangeResult.HasValue());
    auto& range = rangeResult.Value();
    int count = 0;
    for (const auto& entry : range)
    {
        ASSERT_TRUE(entry.HasValue()) << entry.Error().message;
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST(MofParserTest, BlankLinesBetweenEntriesAreSkipped)
{
    std::string text = "\n\n" + Render(DefaultFields()) + "\n\n\n" + Render(DefaultFields()) + "\n";
    std::istringstream stream(text);
    auto rangeResult = MofResourceRange::MakeFromStream(stream, nullptr);
    ASSERT_TRUE(rangeResult.HasValue());
    auto& range = rangeResult.Value();
    int count = 0;
    for (const auto& entry : range)
    {
        ASSERT_TRUE(entry.HasValue()) << entry.Error().message;
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST(MofParserTest, EmptyInputHasNoEntries)
{
    std::istringstream stream("");
    auto rangeResult = MofResourceRange::MakeFromStream(stream, nullptr);
    ASSERT_TRUE(rangeResult.HasValue());
    auto& range = rangeResult.Value();
    EXPECT_TRUE(range.begin() == range.end());
}

TEST(MofParserTest, WhitespaceOnlyInputHasNoEntries)
{
    std::istringstream stream("\n   \n\t\n");
    auto rangeResult = MofResourceRange::MakeFromStream(stream, nullptr);
    ASSERT_TRUE(rangeResult.HasValue());
    auto& range = rangeResult.Value();
    EXPECT_TRUE(range.begin() == range.end());
}

TEST(MofParserTest, IterationStopsAfterError)
{
    // A malformed first entry must surface an error and halt iteration rather
    // than silently skipping to the next entry.
    auto fields = DefaultFields();
    EraseField(fields, "ResourceID");
    std::string text = Render(fields) + Render(DefaultFields());
    std::istringstream stream(text);
    auto rangeResult = MofResourceRange::MakeFromStream(stream, nullptr);
    ASSERT_TRUE(rangeResult.HasValue());
    auto& range = rangeResult.Value();
    auto it = range.begin();
    ASSERT_TRUE(it != range.end());
    EXPECT_FALSE((*it).HasValue());
    ++it;
    EXPECT_TRUE(it == range.end());
}

// ---------------------------------------------------------------------------
// Missing / unknown / duplicate fields
// ---------------------------------------------------------------------------

TEST(MofParserTest, MissingRequiredFieldIsRejected)
{
    auto fields = DefaultFields();
    EraseField(fields, "ProcedureObjectValue");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, UnknownFieldKeyIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "BogusField", "x");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, DuplicateFieldKeyIsRejected)
{
    auto fields = DefaultFields();
    fields.emplace_back("ResourceID", "duplicate");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

// ---------------------------------------------------------------------------
// Field constant / consistency validation
// ---------------------------------------------------------------------------

TEST(MofParserTest, WrongComponentNameIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "ComponentName", "SomethingElse");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, WrongConfigurationNameIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "ConfigurationName", "SomethingElse");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, WrongExpectedObjectValueIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "ExpectedObjectValue", "FAIL");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, ProcedureObjectNameWrongPrefixIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "ProcedureObjectName", "fooMyRule");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, InitObjectNameWrongPrefixIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "InitObjectName", "fooMyRule");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, ReportedObjectNameWrongPrefixIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "ReportedObjectName", "fooMyRule");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, DesiredObjectNameWrongPrefixIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "DesiredObjectName", "fooMyRule");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, RuleNameMismatchIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "ReportedObjectName", "auditOtherRule");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, EmptyRuleNameIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "ProcedureObjectName", "procedure");
    SetField(fields, "InitObjectName", "init");
    SetField(fields, "ReportedObjectName", "audit");
    SetField(fields, "DesiredObjectName", "remediate");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, EmptyResourceIdIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "ResourceID", "");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, EmptyProcedureObjectValueIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "ProcedureObjectValue", "");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, InvalidPayloadKeyIsRejected)
{
    auto fields = DefaultFields();
    SetField(fields, "PayloadKey", "garbage");
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

// ---------------------------------------------------------------------------
// Structural / syntactic errors
// ---------------------------------------------------------------------------

TEST(MofParserTest, MalformedHeaderIsRejected)
{
    EXPECT_FALSE(ParseFirst("not a header\n{\n};\n").HasValue());
}

TEST(MofParserTest, HeaderMissingSuffixIsRejected)
{
    std::string text = "instance of OsConfigResource as $OsConfigResource0\n{\n};\n";
    EXPECT_FALSE(ParseFirst(text).HasValue());
}

TEST(MofParserTest, MissingOpeningBraceIsRejected)
{
    std::string text = std::string(kHeader) + "\n    ResourceID = \"x\";\n";
    EXPECT_FALSE(ParseFirst(text).HasValue());
}

TEST(MofParserTest, MissingClosingBraceIsRejected)
{
    std::string text = RenderBlock(kHeader, DefaultFields(), /*withClose=*/false);
    EXPECT_FALSE(ParseFirst(text).HasValue());
}

TEST(MofParserTest, TruncatedMidEntryIsRejected)
{
    std::string text = std::string(kHeader) + "\n{\n    ResourceID = \"x\";\n";
    EXPECT_FALSE(ParseFirst(text).HasValue());
}

TEST(MofParserTest, FieldLineMissingEqualsIsRejected)
{
    EXPECT_FALSE(ParseFirst(Wrap("    GarbageLineWithoutEquals\n")).HasValue());
}

TEST(MofParserTest, FieldValueMissingQuotesIsRejected)
{
    EXPECT_FALSE(ParseFirst(Wrap("    ResourceID = novalue;\n")).HasValue());
}

TEST(MofParserTest, FieldValueMissingClosingQuoteIsRejected)
{
    EXPECT_FALSE(ParseFirst(Wrap("    ResourceID = \"unterminated;\n")).HasValue());
}

// ---------------------------------------------------------------------------
// Resource limits
// ---------------------------------------------------------------------------

TEST(MofParserTest, OverlongLineIsRejected)
{
    // A single line beyond the parser's per-line cap (4 MiB) must be refused
    // rather than buffered without bound.
    auto fields = DefaultFields();
    SetField(fields, "ProcedureObjectValue", std::string(4 * 1024 * 1024 + 16, 'A'));
    EXPECT_FALSE(ParseFirst(Render(fields)).HasValue());
}

TEST(MofParserTest, ByteCapPreemptsUnreachableEntryCountCap)
{
    // With the strict 15-field format, 100 001 valid entries cannot fit under
    // the 8 MiB input cap. The byte cap is the tighter security boundary and
    // must stop the stream before unbounded parsing can reach the entry cap.
    std::string entry = Render(DefaultFields());
    std::string text;
    constexpr size_t kMaxInputBytesForTest = static_cast<size_t>(8) * 1024 * 1024;
    const size_t entriesNeededToExceedByteCap = (kMaxInputBytesForTest / entry.size()) + 2;
    text.reserve(entry.size() * entriesNeededToExceedByteCap);
    for (size_t i = 0; i < entriesNeededToExceedByteCap; ++i)
    {
        text += entry;
    }
    std::istringstream stream(text);
    auto rangeResult = MofResourceRange::MakeFromStream(stream, nullptr);
    ASSERT_TRUE(rangeResult.HasValue());
    auto& range = rangeResult.Value();
    bool sawError = false;
    int count = 0;
    for (const auto& entry : range)
    {
        if (!entry.HasValue())
        {
            sawError = true;
            EXPECT_EQ(entry.Error().code, E2BIG);
            break;
        }
        ++count;
    }
    EXPECT_TRUE(sawError) << "expected an E2BIG error after the byte cap, but saw " << count << " valid entries";
    EXPECT_LT(static_cast<size_t>(count), entriesNeededToExceedByteCap);
}

TEST(MofParserTest, BlankLineInsideFieldSectionIsSkipped)
{
    // A blank line between field lines inside a valid block must be silently
    // skipped, not treated as an error.
    std::string body = "\n    ResourceID = \"1.1 rule\";\n\n    PayloadKey = \"/cis/ubuntu/22.04/v2.0.0/1/1\";\n";
    // Build a full valid entry manually so the blank-line path in the field
    // collection loop is exercised, then wrap the rest using DefaultFields().
    auto fields = DefaultFields();
    // Render manually inserting a blank line in the middle of the fields.
    std::string text = std::string(kHeader) + "\n{\n";
    bool firstHalf = true;
    int emitted = 0;
    for (const auto& f : fields)
    {
        text += "    " + f.first + " = \"" + f.second + "\";\n";
        ++emitted;
        if (firstHalf && emitted == 7)
        {
            text += "\n"; // inject blank line halfway through
            firstHalf = false;
        }
    }
    text += "};\n";
    auto result = ParseFirst(text);
    EXPECT_TRUE(result.HasValue()) << result.Error().message;
}

TEST(MofParserTest, FieldLineEmptyKeyIsRejected)
{
    // A line that starts with '=' has an empty key, which must be rejected.
    EXPECT_FALSE(ParseFirst(Wrap("    = \"value\";\n")).HasValue());
}

TEST(MofParserTest, TruncatedNoNewlineAtEofIsRejected)
{
    // The line terminator is missing from the very last byte of input (no \n
    // at the end). This is distinct from the EOF-before-}; path: here the
    // stream ends inside a line that has no \n, triggering EIO.
    std::string text = std::string(kHeader) + "\n{\n    ResourceID = \"x\";"; // no trailing \n
    auto result = ParseFirst(text);
    EXPECT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EIO);
}

// ---------------------------------------------------------------------------
// Make(istream&) overload and operator->
// ---------------------------------------------------------------------------

TEST(MofParserTest, MakeFromIstreamOverloadParsesValidEntry)
{
    // Make(istream&, log) is a distinct code path from MakeFromStream; verify
    // it produces the same result on a well-formed single entry.
    std::istringstream stream(Render(DefaultFields()));
    auto rangeResult = MofResourceRange::Make(stream, nullptr);
    ASSERT_TRUE(rangeResult.HasValue());
    auto& range = rangeResult.Value();
    auto it = range.begin();
    ASSERT_TRUE(it != range.end());
    ASSERT_TRUE((*it).HasValue()) << (*it).Error().message;
    EXPECT_EQ((*it).Value().ruleName, "MyRule");
}

TEST(MofParserTest, ArrowOperatorDereferencesCurrentEntry)
{
    // operator-> must return a pointer to the current Result<Resource>.
    std::istringstream stream(Render(DefaultFields()));
    auto rangeResult = MofResourceRange::MakeFromStream(stream, nullptr);
    ASSERT_TRUE(rangeResult.HasValue());
    auto& range = rangeResult.Value();
    auto it = range.begin();
    ASSERT_TRUE(it != range.end());
    ASSERT_TRUE(it->HasValue()) << it->Error().message;
    EXPECT_EQ(it->Value().ruleName, "MyRule");
}

// ---------------------------------------------------------------------------
// Make(path) — file-based factory
// ---------------------------------------------------------------------------

namespace
{
struct TempMofFile
{
    std::string directory;
    std::string path;
};

TempMofFile WriteTempMofInSafeParent(const std::string& content)
{
    TempMofFile result;
    char dirTmpl[] = "/tmp/moftest_safe_XXXXXX";
    char* dir = ::mkdtemp(dirTmpl);
    if (dir == nullptr)
    {
        return result;
    }
    result.directory = dir;
    // Make the immediate parent acceptable to InputSecurity. This still leaves
    // /tmp itself writable, but RefuseWritableParentDir intentionally validates
    // the immediate parent that can rename-swap the file entry.
    ::chmod(result.directory.c_str(), 0700);

    result.path = result.directory + "/test.mof";
    int fd = ::open(result.path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fd < 0)
    {
        ::rmdir(result.directory.c_str());
        return TempMofFile();
    }
    const ssize_t written = ::write(fd, content.data(), content.size());
    ::close(fd);
    if (written != static_cast<ssize_t>(content.size()))
    {
        ::unlink(result.path.c_str());
        ::rmdir(result.directory.c_str());
        return TempMofFile();
    }
    return result;
}
} // namespace

TEST(MofParserTest, MakeFromPathParsesValidEntry)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "Make(path) requires a root-owned file in a root-owned non-writable parent";
    }
    TempMofFile file = WriteTempMofInSafeParent(Render(DefaultFields()));
    if (file.path.empty())
    {
        GTEST_SKIP() << "could not create temporary file";
    }
    auto rangeResult = MofResourceRange::Make(file.path, nullptr);
    ::unlink(file.path.c_str());
    ::rmdir(file.directory.c_str());
    ASSERT_TRUE(rangeResult.HasValue()) << rangeResult.Error().message;
    auto& range = rangeResult.Value();
    auto it = range.begin();
    ASSERT_TRUE(it != range.end());
    ASSERT_TRUE((*it).HasValue()) << (*it).Error().message;
    EXPECT_EQ((*it).Value().ruleName, "MyRule");
}

TEST(MofParserTest, MakeFromPathRejectsPathTraversal)
{
    auto result = MofResourceRange::Make("/tmp/../etc/passwd", nullptr);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EACCES);
}

TEST(MofParserTest, MakeFromPathRejectsWritableParentDir)
{
    // Create a world-writable subdirectory under /tmp, then a file inside it.
    // InputSecurity must refuse the file because its parent is writable.
    char dirTmpl[] = "/tmp/moftest_dir_XXXXXX";
    char* dir = ::mkdtemp(dirTmpl);
    if (dir == nullptr)
    {
        GTEST_SKIP() << "could not create temporary directory";
    }
    ::chmod(dir, 0777); // world-writable — triggers the refusal

    std::string path = std::string(dir) + "/test.mof";
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fd >= 0)
    {
        const std::string content = Render(DefaultFields());
        ::write(fd, content.data(), content.size());
        ::close(fd);
    }

    auto result = MofResourceRange::Make(path, nullptr);
    ::unlink(path.c_str());
    ::rmdir(dir);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EACCES);
}
