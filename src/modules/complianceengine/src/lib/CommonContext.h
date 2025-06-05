// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_COMMONCONTEXT_H
#define COMPLIANCEENGINE_COMMONCONTEXT_H

#include "ContextInterface.h"
#include "DirectoryEntry.h"
#include "Logging.h"
#include "Result.h"

#include <memory>
#include <sstream>
#include <string>

namespace ComplianceEngine
{
class CommonContext : public ContextInterface
{
public:
    CommonContext(OsConfigLogHandle log)
        : mLog(log),
          mDirectoryIterator(new FtsDirectoryIterator())
    {
    }
    ~CommonContext() override;

    Result<std::string> ExecuteCommand(const std::string& cmd) const override;
    Result<std::string> GetFileContents(const std::string& filePath) const override;
    Result<DirectoryEntries> GetDirectoryEntries(const std::string& directoryPath, bool recursive) const override;
    OsConfigLogHandle GetLogHandle() const override
    {
        return mLog;
    }

private:
    OsConfigLogHandle mLog;
    std::unique_ptr<DirectoryIteratorInterface> mDirectoryIterator;
};
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_COMMONCONTEXT_H
