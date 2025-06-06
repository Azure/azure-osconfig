// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Optional.h>
#include <Result.h>
#include <fcntl.h>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace ComplianceEngine
{
AUDIT_FN(EnsureNoDuplicateEntriesExist, "filename:The file to be checked for duplicate entries:M",
    "delimiter:A single character used to separate entries:M", "column:Column index to check for duplicates:M",
    "context:Context for the entries used in the messages")
{
    UNUSED(context);
    auto it = args.find("filename");
    if (it == args.end())
    {
        return Error("Missing 'filename' argument", EINVAL);
    }
    auto filename = std::move(it->second);

    it = args.find("delimiter");
    if (it == args.end())
    {
        return Error("Missing 'delimiter' argument", EINVAL);
    }
    auto delimiter = std::move(it->second);
    if (delimiter.size() != 1)
    {
        return Error("Delimiter must be a single character", EINVAL);
    }

    it = args.find("column");
    if (it == args.end())
    {
        return Error("Missing 'column' argument", EINVAL);
    }

    int column = 0;
    try
    {
        column = std::stoi(it->second);
        if (column < 0)
        {
            return Error("Column must be a non-negative integer");
        }
    }
    catch (const std::exception& e)
    {
        return Error(std::string("Failed to parse 'column' argument: ") + e.what(), EINVAL);
    }

    std::string entries;
    it = args.find("context");
    if (it != args.end())
    {
        entries = it->second;
    }
    else
    {
        entries = "entries";
    }

    std::set<std::string> uniqueEntries;
    std::set<std::string> duplicateEntries;
    std::string line;
    std::ifstream file(filename);
    if (!file.is_open())
    {
        return Error("Failed to open file: " + filename, ENOENT);
    }

    while (std::getline(file, line))
    {
        std::istringstream iss(std::move(line));
        std::string token;
        for (int i = 0; i <= column; i++)
        {
            if (!std::getline(iss, token, delimiter[0]))
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

    return indicators.Compliant(std::string("No duplicate ") + entries + " found in " + filename);
}
} // namespace ComplianceEngine
