// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <rapidjson/document.h>
#include <CommonTests.h>

namespace Tests
{
    testing::AssertionResult JSON_EQ(std::string const &leftString, std::string const &rightString)
    {
        rapidjson::Document jL;
        rapidjson::Document jR;
        jL.Parse(leftString.c_str());
        jR.Parse(rightString.c_str());

        if (jL == jR)
        {
            return testing::AssertionSuccess();
        }
        else
        {
            return testing::AssertionFailure() << "expected JSON is:\n"
                                               << "'" << leftString << "'\n"
                                               << "but got:\n"
                                               << "'" << rightString << "'" << "\n";
        }
    }
} // namespace Tests
