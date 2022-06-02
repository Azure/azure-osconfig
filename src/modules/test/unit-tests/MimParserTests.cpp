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
        pMimObjects mimObjects = MimParser::ParseMim("./mim/sample.json");

        // Should have one component and 10 MimObjects
        ASSERT_EQ(mimObjects->size(), 1);

        size_t mimCount = 0;
        for (const auto &c : *mimObjects)
        {
            mimCount += c.second->size();
        }
        ASSERT_EQ(mimCount, 10);
        ASSERT_EQ((*mimObjects)["SampleComponent"]->size(), 10);

        ASSERT_EQ((*mimObjects)["SampleComponent"]->at("ReportedObject").m_fields->size(), 8);
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("ReportedObject").m_fields->at("stringSetting").name.c_str(), "stringSetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("ReportedObject").m_fields->at("integerSetting").name.c_str(), "integerSetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("ReportedObject").m_fields->at("booleanSetting").name.c_str(), "booleanSetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("ReportedObject").m_fields->at("integerEnumerationSetting").name.c_str(), "integerEnumerationSetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("ReportedObject").m_fields->at("stringsArraySetting").name.c_str(), "stringsArraySetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("ReportedObject").m_fields->at("integerArraySetting").name.c_str(), "integerArraySetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("ReportedObject").m_fields->at("stringMapSetting").name.c_str(), "stringMapSetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("ReportedObject").m_fields->at("integerMapSetting").name.c_str(), "integerMapSetting");

        ASSERT_EQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_fields->size(), 8);
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_fields->at("stringSetting").name.c_str(), "stringSetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_fields->at("integerSetting").name.c_str(), "integerSetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_fields->at("booleanSetting").name.c_str(), "booleanSetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_fields->at("integerEnumerationSetting").name.c_str(), "integerEnumerationSetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_fields->at("stringsArraySetting").name.c_str(), "stringsArraySetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_fields->at("integerArraySetting").name.c_str(), "integerArraySetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_fields->at("stringMapSetting").name.c_str(), "stringMapSetting");
        ASSERT_STREQ((*mimObjects)["SampleComponent"]->at("desiredArrayObject").m_fields->at("integerMapSetting").name.c_str(), "integerMapSetting");
    }
}