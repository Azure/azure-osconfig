// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ContextInterface.h"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <unistd.h>
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
        // Remove files tracked explicitly
        for (const auto& file : mTempfiles)
        {
            if (0 != remove(file.c_str()))
            {
                std::cerr << "Failed to remove temporary file: " << file << ", error: " << std::strerror(errno) << std::endl;
            }
        }

        // Recursively remove any directories created under the temp root (e.g., modulesRoot tree)
        RecursiveRemove(mTempdir);
        if (0 != rmdir(mTempdir))
        {
            // If directory not empty (race), best-effort second pass
            RecursiveRemove(mTempdir);
            if (0 != rmdir(mTempdir))
            {
                std::cerr << "Failed to remove temporary directory: " << mTempdir << ", error: " << std::strerror(errno) << std::endl;
            }
        }
    }

    void RecursiveRemove(const std::string& path) const
    {
        DIR* dir = opendir(path.c_str());
        if (!dir)
        {
            return;
        }
        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr)
        {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            {
                continue;
            }
            std::string child = path + "/" + ent->d_name;
            struct stat st
            {
            };
            if (0 == lstat(child.c_str(), &st))
            {
                if (S_ISDIR(st.st_mode))
                {
                    RecursiveRemove(child);
                    if (0 != rmdir(child.c_str()))
                    {
                        std::cerr << "Failed to remove directory: " << child << ", error: " << std::strerror(errno) << std::endl;
                    }
                }
                else
                {
                    if (0 != remove(child.c_str()))
                    {
                        std::cerr << "Failed to remove file: " << child << ", error: " << std::strerror(errno) << std::endl;
                    }
                }
            }
        }
        closedir(dir);
    }

    std::string MakeTempfile(const std::string& content, const std::string& extension = "")
    {
        std::string filename = std::string(mTempdir) + "/" + std::to_string(mTempfiles.size() + 1) + extension;
        std::ofstream file(filename);
        file << content;
        mTempfiles.push_back(filename);
        return filename;
    }

    std::string GetTempdirPath() const
    {
        return mTempdir;
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
