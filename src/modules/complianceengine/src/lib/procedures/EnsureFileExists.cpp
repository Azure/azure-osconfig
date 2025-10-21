// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <EnsureFileExists.h>
#include <Evaluator.h>
#include <Optional.h>
#include <ProcedureMap.h>
#include <Regex.h>
#include <Result.h>
#include <dirent.h>
#include <fstream>

namespace ComplianceEngine
{
Result<Status> AuditEnsureFileExists(const AuditEnsureFileExistsParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto log = context.GetLogHandle();
    struct stat statbuf;
    if (0 != stat(params.filename.c_str(), &statbuf))
    {
        const int status = errno;
        if (ENOENT == status)
        {
            OsConfigLogDebug(log, "File '%s' does not exist", params.filename.c_str());
            return indicators.NonCompliant("File '" + params.filename + "' does not exist");
        }

        OsConfigLogError(log, "Stat error %s (%d)", strerror(status), status);
        return Error("Stat error '" + std::string(strerror(status)) + "'", status);
    }

    return indicators.Compliant("File '" + params.filename + "' exists");
}

} // namespace ComplianceEngine
