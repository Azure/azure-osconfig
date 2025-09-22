// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_COMMONCONTEXT_H
#define COMPLIANCEENGINE_COMMONCONTEXT_H

#include "ContextInterface.h"
#include "Logging.h"
#include "Result.h"

#include <sstream>
#include <string>

static constexpr char FS_CACHE_PATH[] = "/var/lib/GuestConfig/ComplianceEngineFSCache";
static constexpr char LOCK_PATH[] = "/run/complianceengine-fsscanner.lock";

namespace ComplianceEngine
{
class CommonContext : public ContextInterface
{
public:
    CommonContext(OsConfigLogHandle log)
        : mLog(log),
          mFsScanner("/", FS_CACHE_PATH, LOCK_PATH, 3600, 86400, 20)
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
