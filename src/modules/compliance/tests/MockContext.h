// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ContextInterface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct MockContext : public compliance::ContextInterface
{
    MOCK_METHOD(compliance::Result<std::string>, ExecuteCommand, (const std::string& cmd), (const, override));
    MOCK_METHOD(compliance::Result<std::string>, GetFileContents, (const std::string& filePath), (const, override));

    std::ostream& GetLogstream() override
    {
        return mLogstream;
    }

    std::stringstream& GetLogstreamRef()
    {
        return mLogstream;
    }

    std::string ConsumeLogstream() override
    {
        std::string result = mLogstream.str();
        mLogstream.str("");
        return result;
    }

    OsConfigLogHandle GetLogHandle() const override
    {
        return nullptr;
    }

    ~MockContext() override = default;
    std::stringstream mLogstream;
};
