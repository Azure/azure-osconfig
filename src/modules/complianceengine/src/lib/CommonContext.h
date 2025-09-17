// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_COMMONCONTEXT_H
#define COMPLIANCEENGINE_COMMONCONTEXT_H

#include "ContextInterface.h"
#include "Logging.h"
#include "Result.h"
#include "Telemetry.h"

#include <sstream>
#include <string>

namespace ComplianceEngine
{
class CommonContext : public ContextInterface
{
public:
    CommonContext(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
        : mLog(log),
          mTelemetry(telemetry)
    {
    }
    ~CommonContext() override;

    Result<std::string> ExecuteCommand(const std::string& cmd) const override;
    Result<std::string> GetFileContents(const std::string& filePath) const override;
    OsConfigLogHandle GetLogHandle() const override
    {
        return mLog;
    }
    OSConfigTelemetryHandle GetTelemetryHandle() const override
    {
        return mTelemetry;
    }

    std::string GetSpecialFilePath(const std::string& path) const override;

private:
    OsConfigLogHandle mLog;
    OSConfigTelemetryHandle mTelemetry;
};
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_COMMONCONTEXT_H
