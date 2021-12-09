// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include <Mmi.h>
#include <Sample.h>

namespace OSConfig::Platform::Tests
{
    TEST(SampleTests, SampleTest)
    {
        std::string value = "Sample C++ Module";

        Sample* sample = new Sample(0);
        ASSERT_NE(sample, nullptr);
        ASSERT_EQ(MMI_OK, sample->SetValue(value));
        ASSERT_STREQ(value.c_str(), sample->GetValue().c_str());
    }
}