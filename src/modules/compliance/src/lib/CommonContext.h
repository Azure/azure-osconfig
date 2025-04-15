// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_COMMONCONTEXT_H
#define COMPLIANCE_COMMONCONTEXT_H

#include "ContextInterface.h"
#include "Logging.h"
#include "Result.h"

#include <sstream>
#include <string>

namespace compliance
{
class CommonContext : public ContextInterface
{
public:
    CommonContext(OsConfigLogHandle log)
        : mLog(log)
    {
    }
    ~CommonContext() override;

    Result<std::string> ExecuteCommand(const std::string& cmd) const override;
    Result<std::string> GetFileContents(const std::string& filePath) const override;

    std::ostream& GetLogstream() override;
    std::string ConsumeLogstream() override;
    OsConfigLogHandle GetLogHandle() const override
    {
        return mLog;
    }

private:
    OsConfigLogHandle mLog;
    std::ostringstream mLogstream;
};
} // namespace compliance
#endif // COMPLIANCE_COMMONCONTEXT_H
