// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <nlohmann/json.hpp>
#include <TestUtils.h>

namespace Tests
{
    testing::AssertionResult IsJsonEq(const std::string& expectedJson, const std::string& actualJson)
    {
        try
        {
            auto expected = nlohmann::json::parse(expectedJson);
            auto actual = nlohmann::json::parse(actualJson);

            if (actual == expected)
            {
                return testing::AssertionSuccess();
            }
            else
            {
                return testing::AssertionFailure() << "expected:\n" << expectedJson << "\n but got:\n" << actualJson;
            }
        }
        catch (const nlohmann::json::parse_error& e)
        {
            return testing::AssertionFailure() << "JSON parse error: " << e.what();
        }
    }
} // namespace Tests
