// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ContextInterface.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <vector>

struct MockContext : public ComplianceEngine::ContextInterface
{
    MOCK_METHOD(ComplianceEngine::Result<std::string>, ExecuteCommand, (const std::string& cmd), (const, override));
    MOCK_METHOD(ComplianceEngine::Result<std::string>, GetFileContents, (const std::string& filePath), (const, override));

    OsConfigLogHandle GetLogHandle() const override
    {
        return nullptr;
    }

    MockContext()
    {
        strcpy(mTempdir, "/tmp/ComplianceEngineTest.XXXXXX");
        if (mkdtemp(mTempdir) == nullptr)
        {
            throw std::runtime_error("Failed to create temporary directory");
        }
    }

    ~MockContext() override
    {
        for (const auto& file : mTempfiles)
        {
            if (0 != remove(file.c_str()))
            {
                std::cerr << "Failed to remove temporary file: " << file << ", error: " << std::strerror(errno) << std::endl;
            }
        }
        if (0 != remove(mTempdir))
        {
            std::cerr << "Failed to remove temporary directory: " << mTempdir << ", error: " << std::strerror(errno) << std::endl;
        }
    }

    std::string MakeTempfile(const std::string& content)
    {
        std::string filename = std::string(mTempdir) + "/" + std::to_string(mTempfiles.size() + 1);
        std::ofstream file(filename);
        file << content;
        mTempfiles.push_back(filename);
        return filename;
    }

    std::string GetSpecialFilePath(const std::string& path) const override
    {
        auto it = mSpecialFilesMap.find(path);
        if (it != mSpecialFilesMap.end())
        {
            return it->second;
        }

        return path;
    }

    void SetSpecialFilePath(const std::string& path, const std::string& overridden)
    {
        mSpecialFilesMap[path] = overridden;
    }

private:
    char mTempdir[PATH_MAX];
    std::vector<std::string> mTempfiles;
    std::map<std::string, std::string> mSpecialFilesMap;
};
