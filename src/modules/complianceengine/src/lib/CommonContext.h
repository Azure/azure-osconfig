// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_COMMONCONTEXT_H
#define COMPLIANCEENGINE_COMMONCONTEXT_H

#include "ContextInterface.h"
#include "Logging.h"
#include "Result.h"

#include <string>

namespace ComplianceEngine
{

namespace
{
constexpr char fsCachePath[] = "ComplianceEngineFSCache";
constexpr char lockPath[] = "/run/complianceengine-fsscanner.lock";
constexpr int softTimeout = 3600;
constexpr int hardTimeout = 86400;
constexpr int scanWaitTime = 30;
} // namespace
class CommonContext : public ContextInterface
{
public:
    CommonContext(OsConfigLogHandle log, const std::string& statePath)
        : mLog(log),
          mStatePath(statePath),
          mFsScanner("/", mStatePath + "/" + fsCachePath, lockPath, softTimeout, hardTimeout, scanWaitTime)
    {
    }
    ~CommonContext() override;
    CommonContext(const CommonContext&) = delete;
    CommonContext& operator=(const CommonContext&) = delete;
    CommonContext(CommonContext&&) = delete;
    CommonContext& operator=(CommonContext&&) = delete;

    Result<std::string> ExecuteCommand(const std::string& cmd) const override;
    Result<std::string> GetFileContents(const std::string& filePath) const override;
    OsConfigLogHandle GetLogHandle() const override
    {
        return mLog;
    }

    std::string GetSpecialFilePath(const std::string& path) const override;
    FilesystemScanner& GetFilesystemScanner() override
    {
        return mFsScanner;
    }

    std::string GetStatePath() const override
    {
        return mStatePath;
    }

private:
    OsConfigLogHandle mLog;
    std::string mStatePath;
    FilesystemScanner mFsScanner;
};
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_COMMONCONTEXT_H
