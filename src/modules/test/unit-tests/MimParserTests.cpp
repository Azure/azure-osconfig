// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Common.h>
namespace Tests
{
    TEST(MimParserTests, LoadInvalidMim)
    {
        pMimObjects mimObjects = MimParser::ParseMim("");
        ASSERT_EQ(mimObjects, nullptr);
    }

    TEST(MimParserTests, LoadMim)
    {
        pMimObjects mimObjects = MimParser::ParseMim("./mim/sample.json");

        // Should have one component and 10 MimObjects
        ASSERT_EQ(mimObjects->size(), 1);

        size_t mimCount = 0;
        for (const auto &c : *mimObjects)
        {
            mimCount += c.second->size();
        }
        EXPECT_EQ(mimCount, 10);
        EXPECT_EQ((*mimObjects)["SampleComponent"]->size(), 10);

        EXPECT_EQ((*mimObjects)["SampleComponent"]->at("reportedObject").m_settings->size(), 9);
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("reportedObject").m_settings->at("stringSetting").name.c_str(), "stringSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("reportedObject").m_settings->at("integerSetting").name.c_str(), "integerSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("reportedObject").m_settings->at("booleanSetting").name.c_str(), "booleanSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("reportedObject").m_settings->at("integerEnumerationSetting").name.c_str(), "integerEnumerationSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("reportedObject").m_settings->at("stringEnumerationSetting").name.c_str(), "stringEnumerationSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("reportedObject").m_settings->at("stringsArraySetting").name.c_str(), "stringsArraySetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("reportedObject").m_settings->at("integerArraySetting").name.c_str(), "integerArraySetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("reportedObject").m_settings->at("stringMapSetting").name.c_str(), "stringMapSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("reportedObject").m_settings->at("integerMapSetting").name.c_str(), "integerMapSetting");

        EXPECT_EQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_settings->size(), 9);
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_type.c_str(), "array");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_settings->at("stringSetting").name.c_str(), "stringSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_settings->at("integerSetting").name.c_str(), "integerSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_settings->at("booleanSetting").name.c_str(), "booleanSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_settings->at("integerEnumerationSetting").name.c_str(), "integerEnumerationSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_settings->at("stringEnumerationSetting").name.c_str(), "stringEnumerationSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_settings->at("stringsArraySetting").name.c_str(), "stringsArraySetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_settings->at("integerArraySetting").name.c_str(), "integerArraySetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_settings->at("stringMapSetting").name.c_str(), "stringMapSetting");
        EXPECT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_settings->at("integerMapSetting").name.c_str(), "integerMapSetting");
    }
}