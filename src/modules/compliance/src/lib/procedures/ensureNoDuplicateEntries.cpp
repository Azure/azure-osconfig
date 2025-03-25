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

namespace compliance
{
AUDIT_FN(ensureNoDuplicateEntriesExist)
{
    auto it = args.find("filename");
    if (it == args.end())
    {
        return Error("Missing 'filename' argument");
    }
    auto filename = std::move(it->second);

    it = args.find("delimiter");
    if (it == args.end())
    {
        return Error("Missing 'delimiter' argument");
    }
    auto delimiter = std::move(it->second);
    if (delimiter.size() != 1)
    {
        return Error("Delimiter must be a single character");
    }

    it = args.find("column");
    if (it == args.end())
    {
        return Error("Missing 'column' argument");
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
        return Error(std::string("Failed to parse 'column' argument: ") + e.what());
    }

    Optional<std::string> context;
    it = args.find("context");
    if (it != args.end())
    {
        context = it->second;
    }

    std::set<std::string> uniqueEntries;
    std::set<std::string> duplicateEntries;
    std::string line;
    std::ifstream file(filename);
    if (!file.is_open())
    {
        return Error("Failed to open file: " + filename);
    }

    while (std::getline(file, line))
    {
        std::istringstream iss(std::move(line));
        std::string token;
        for (int i = 0; i <= column; i++)
        {
            if (!std::getline(iss, token, delimiter[0]))
            {
                return Error("Column index out of bounds");
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
        logstream << "Duplicate " << (context.HasValue() ? context.Value() : "entries") << " found in " << filename << ": [";
        bool first = true;
        for (const auto& entry : duplicateEntries)
        {
            if (!first)
            {
                logstream << ", ";
            }
            first = false;
            logstream << entry;
        }
        logstream << "] ";
        return false;
    }

    return true;
}
} // namespace compliance
