// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <rapidjson/document.h>
#include <TestUtils.h>

namespace Tests
{
    testing::AssertionResult IsJsonEq(const std::string& expectedJson, const std::string& actualJson)
    {
        rapidjson::Document actual;
        rapidjson::Document expected;

        if (expected.Parse(expectedJson.c_str()).HasParseError())
        {
            return testing::AssertionFailure() << "expected JSON is not valid JSON";
        }
        else if (actual.Parse(actualJson.c_str()).HasParseError())
        {
            return testing::AssertionFailure() << "actual JSON is not valid JSON";
        }
        else if (actual == expected)
        {
            return testing::AssertionSuccess();
        }
        else
        {
            return testing::AssertionFailure() << "expected:\n" << expectedJson << "\n but got:\n" << actualJson;
        }
    }
} // namespace Tests
