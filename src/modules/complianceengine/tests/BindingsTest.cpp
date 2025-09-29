// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Bindings.h>
#include <Regex.h>
#include <gtest/gtest.h>
#include <map>
#include <string>

using ComplianceEngine::Optional;
using ComplianceEngine::Pattern;
using ComplianceEngine::Separated;
using ComplianceEngine::BindingsImpl::ParseArguments;
using std::map;
using std::string;

class BindingsTest : public ::testing::Test
{
};

struct BuiltinTypesParams
{
    int intValue;
    bool boolValue;
    string stringValue;
    regex regexValue;
    Pattern patternValue;
    mode_t octalValue;
    Separated<string, ','> separatedValue;
    Optional<int> optionalIntValue;
};

namespace ComplianceEngine
{
template <>
struct Bindings<BuiltinTypesParams>
{
    using T = BuiltinTypesParams;
    static constexpr size_t size = 8;
    static const char* names[];
    static constexpr auto members = std::make_tuple(&T::intValue, &T::boolValue, &T::stringValue, &T::regexValue, &T::patternValue, &T::octalValue,
        &T::separatedValue, &T::optionalIntValue);
};

const char* Bindings<BuiltinTypesParams>::names[] = {
    "intValue", "boolValue", "stringValue", "regexValue", "patternValue", "octalValue", "separatedValue", "optionalIntValue"};
} // namespace ComplianceEngine

TEST_F(BindingsTest, ValidInput_1)
{
    map<string, string> args;
    args["intValue"] = "42";
    args["boolValue"] = "true";
    args["stringValue"] = "test";
    args["regexValue"] = "te.*";
    args["patternValue"] = "te.*";
    args["octalValue"] = "0755";
    args["separatedValue"] = "foo,bar,baz";
    args["optionalIntValue"] = "100";

    auto result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_TRUE(result.HasValue());
    auto params = result.Value();
    EXPECT_EQ(params.intValue, 42);
    EXPECT_EQ(params.boolValue, true);
    EXPECT_EQ(params.stringValue, "test");
    EXPECT_TRUE(regex_match("test", params.regexValue));
    EXPECT_EQ(params.patternValue.GetPattern(), string("te.*"));
    EXPECT_TRUE(regex_match("test", params.patternValue.GetRegex()));
    EXPECT_EQ(params.octalValue, 0755);
    ASSERT_EQ(params.separatedValue.items.size(), 3);
    EXPECT_EQ(params.separatedValue.items[0], "foo");
    EXPECT_EQ(params.separatedValue.items[1], "bar");
    EXPECT_EQ(params.separatedValue.items[2], "baz");
    ASSERT_TRUE(params.optionalIntValue.HasValue());
    EXPECT_EQ(params.optionalIntValue.Value(), 100);
}

TEST_F(BindingsTest, MissingValues_1)
{
    map<string, string> args;
    auto result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["intValue"] = "42";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["boolValue"] = "true";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["stringValue"] = "test";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["regexValue"] = "te.*";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["patternValue"] = "te.*";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["octalValue"] = "0755";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["separatedValue"] = "foo,bar,baz";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_TRUE(result.HasValue());
    auto params = result.Value();
    EXPECT_EQ(params.intValue, 42);
    EXPECT_EQ(params.boolValue, true);
    EXPECT_EQ(params.stringValue, "test");
    EXPECT_TRUE(regex_match("test", params.regexValue));
    EXPECT_EQ(params.patternValue.GetPattern(), string("te.*"));
    EXPECT_TRUE(regex_match("test", params.patternValue.GetRegex()));
    EXPECT_EQ(params.octalValue, 0755);
    ASSERT_EQ(params.separatedValue.items.size(), 3);
    EXPECT_EQ(params.separatedValue.items[0], "foo");
    EXPECT_EQ(params.separatedValue.items[1], "bar");
    EXPECT_EQ(params.separatedValue.items[2], "baz");
    EXPECT_FALSE(params.optionalIntValue.HasValue());
}

TEST_F(BindingsTest, InvalidValues_1)
{
    map<string, string> args;
    args["intValue"] = "42";
    args["boolValue"] = "true";
    args["stringValue"] = "test";
    args["regexValue"] = "te.*";
    args["patternValue"] = "te.*";
    args["octalValue"] = "0755";
    args["separatedValue"] = "foo,bar,baz";
    args["optionalIntValue"] = "100";

    args["intValue"] = "foo";
    auto result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["intValue"] = "0";
    args["boolValue"] = "foo";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["boolValue"] = "true";
    args["regexValue"] = "[";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["regexValue"] = "test";
    args["patternValue"] = "(";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["patternValue"] = "te.*";
    args["octalValue"] = "999";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["octalValue"] = "0755";
    args["optionalIntValue"] = "foo";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_FALSE(result.HasValue());

    args["optionalIntValue"] = "-3";
    result = ParseArguments<BuiltinTypesParams>(args);
    ASSERT_TRUE(result.HasValue());
    auto params = result.Value();
    EXPECT_EQ(params.intValue, 0);
    EXPECT_EQ(params.boolValue, true);
    EXPECT_EQ(params.stringValue, "test");
    EXPECT_TRUE(regex_match("test", params.regexValue));
    EXPECT_EQ(params.patternValue.GetPattern(), string("te.*"));
    EXPECT_TRUE(regex_match("test", params.patternValue.GetRegex()));
    EXPECT_EQ(params.octalValue, 0755);
    ASSERT_EQ(params.separatedValue.items.size(), 3);
    EXPECT_EQ(params.separatedValue.items[0], "foo");
    EXPECT_EQ(params.separatedValue.items[1], "bar");
    EXPECT_EQ(params.separatedValue.items[2], "baz");
    ASSERT_TRUE(params.optionalIntValue.HasValue());
    EXPECT_EQ(params.optionalIntValue.Value(), -3);
}
