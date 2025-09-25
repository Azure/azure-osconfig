// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_COMMONCONTEXT_H
#define COMPLIANCEENGINE_COMMONCONTEXT_H

#include "ContextInterface.h"
#include "Logging.h"
#include "Result.h"

#include <sstream>
#include <string>

namespace ComplianceEngine
{

namespace
{
constexpr char fsCachePath[] = "/var/lib/GuestConfig/ComplianceEngineFSCache";
constexpr char lockPath[] = "/run/complianceengine-fsscanner.lock";
constexpr int softTimeout = 3600;
constexpr int hardTimeout = 86400;
constexpr int scanWaitTime = 30;
} // namespace
class CommonContext : public ContextInterface
{
public:
    CommonContext(OsConfigLogHandle log)
        : mLog(log),
          mFsScanner("/", fsCachePath, lockPath, softTimeout, hardTimeout, scanWaitTime)
    {
    }
    ~CommonContext() override;

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

private:
    OsConfigLogHandle mLog;
    FilesystemScanner mFsScanner;
};
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_COMMONCONTEXT_H
