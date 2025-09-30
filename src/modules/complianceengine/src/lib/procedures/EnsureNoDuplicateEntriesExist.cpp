// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <EnsureNoDuplicateEntriesExist.h>
#include <Evaluator.h>
#include <Optional.h>
#include <Result.h>
#include <fstream>
#include <set>
#include <string>

namespace ComplianceEngine
{
Result<Status> AuditEnsureNoDuplicateEntriesExist(const EnsureNoDuplicateEntriesExistParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(context);
    if (params.delimiter.size() != 1)
    {
        return Error("Delimiter must be a single character", EINVAL);
    }

    if (params.column < 0)
    {
        return Error("Column must be a non-negative integer", EINVAL);
    }

    std::string entries;
    if (params.context.HasValue())
    {
        entries = params.context.Value();
    }
    else
    {
        entries = "entries";
    }

    std::set<std::string> uniqueEntries;
    std::set<std::string> duplicateEntries;
    std::string line;
    std::ifstream file(params.filename);
    if (!file.is_open())
    {
        return Error("Failed to open file: " + params.filename, ENOENT);
    }

    while (std::getline(file, line))
    {
        std::istringstream iss(std::move(line));
        std::string token;
        for (int i = 0; i <= params.column; i++)
        {
            if (!std::getline(iss, token, params.delimiter[0]))
            {
                return Error("Column index out of bounds", EINVAL);
            }
        }

        if (uniqueEntries.find(token) != uniqueEntries.end())
        {
            duplicateEntries.insert(std::move(token));
        }
        else
        {
            uniqueEntries.insert(std::move(token));
        }
    }

    if (!duplicateEntries.empty())
    {
        for (const auto& entry : duplicateEntries)
        {
            indicators.NonCompliant("Duplicate entry: '" + entry + "'");
        }
        return Status::NonCompliant;
    }

    return indicators.Compliant(std::string("No duplicate ") + entries + " found in " + params.filename);
}
} // namespace ComplianceEngine
