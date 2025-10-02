// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_CONTEXTINTERFACE_H
#define COMPLIANCEENGINE_CONTEXTINTERFACE_H

#include "FilesystemScanner.h"
#include "Logging.h"
#include "Result.h"

#include <string>

namespace ComplianceEngine
{
class ContextInterface
{
public:
    virtual ~ContextInterface() = 0;
    virtual Result<std::string> ExecuteCommand(const std::string& cmd) const = 0;
    virtual Result<std::string> GetFileContents(const std::string& filePath) const = 0;

    virtual OsConfigLogHandle GetLogHandle() const = 0;
    virtual std::string GetSpecialFilePath(const std::string& path) const = 0;

    virtual FilesystemScanner& GetFilesystemScanner() = 0;
};
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_CONTEXT_H
