// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <OsInfo.h>

class OsInfoTests : public OsInfo
{
public:
    OsInfoTests(const std::map<std::string, std::string> &textResults, size_t maxPayloadSizeBytes);
    ~OsInfoTests();
};

OsInfoTests::OsInfoTests(const std::map<std::string, std::string> &textResults, size_t maxPayloadSizeBytes)
    : OsInfo(maxPayloadSizeBytes), m_textResults(textResults)
{
}

OsInfoTests::~OsInfoTests()
{
}

namespace OSConfig::Platform::Tests
{

}