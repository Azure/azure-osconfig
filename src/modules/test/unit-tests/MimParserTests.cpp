// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <MimParser.h>

namespace Tests
{
    TEST(MimParserTests, LoadInvalidMim)
    {
        pMimObjects mimObjects = MimParser::ParseMim("");
        ASSERT_EQ(mimObjects->size(), 0);
    }

    TEST(MimParserTests, LoadMim)
    {
        pMimObjects mimObjects = MimParser::ParseMim("./mim/commandrunner.json");

        // Should have one component and 2 MimObjects
        ASSERT_EQ(mimObjects->size(), 1);

        size_t mimCount = 0;
        for (const auto &c : *mimObjects)
        {
            mimCount += c.second->size();
        }
        ASSERT_EQ(mimCount, 2);
        
        ASSERT_EQ((*mimObjects)["CommandRunner"]->size(), 2);

        ASSERT_EQ((*mimObjects)["CommandRunner"]->at("commandStatus").m_fields->size(), 4);

        ASSERT_STREQ((*mimObjects)["CommandRunner"]->at("commandStatus").m_fields->at("commandId").name.c_str(), "commandId");
        ASSERT_STREQ((*mimObjects)["CommandRunner"]->at("commandStatus").m_fields->at("resultCode").name.c_str(), "resultCode");
        ASSERT_STREQ((*mimObjects)["CommandRunner"]->at("commandStatus").m_fields->at("textResult").name.c_str(), "textResult");
        ASSERT_STREQ((*mimObjects)["CommandRunner"]->at("commandStatus").m_fields->at("currentState").name.c_str(), "currentState");
        
        ASSERT_EQ((*mimObjects)["CommandRunner"]->at("commandStatus").m_fields->at("currentState").allowedValues->size(), 6);
    }
}