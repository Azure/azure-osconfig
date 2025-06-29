// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ContextInterface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct MockContext : public ComplianceEngine::ContextInterface
{
    MOCK_METHOD(ComplianceEngine::Result<std::string>, ExecuteCommand, (const std::string& cmd), (const, override));
    MOCK_METHOD(ComplianceEngine::Result<std::string>, GetFileContents, (const std::string& filePath), (const, override));
    MOCK_METHOD(ComplianceEngine::Result<ComplianceEngine::DirectoryEntries>, GetDirectoryEntries, (const std::string& directoryPath, bool recursive),
        (const, override));

    OsConfigLogHandle GetLogHandle() const override
    {
        return nullptr;
    }

    ~MockContext() override = default;
    std::stringstream mLogstream;
};
