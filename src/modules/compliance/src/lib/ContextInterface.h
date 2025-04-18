// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_CONTEXTINTERFACE_H
#define COMPLIANCE_CONTEXTINTERFACE_H

#include "Logging.h"
#include "Result.h"

#include <string>

namespace compliance
{
class ContextInterface
{
public:
    virtual ~ContextInterface() = 0;
    virtual Result<std::string> ExecuteCommand(const std::string& cmd) const = 0;
    virtual Result<std::string> GetFileContents(const std::string& filePath) const = 0;

    virtual OsConfigLogHandle GetLogHandle() const = 0;
};
} // namespace compliance
#endif // COMPLIANCE_CONTEXT_H
