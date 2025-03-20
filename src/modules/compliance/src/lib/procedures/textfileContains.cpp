// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>
#include <fstream>
#include <regex>

namespace compliance
{
AUDIT_FN(textfileContains)
{
    UNUSED(args);
    UNUSED(logstream);
    auto it = args.find("filePath");
    if (it == args.end())
    {
        return Error("Missing 'filePath' parameter");
    }
    auto path = it->second;

    it = args.find("matchPattern");
    if (it == args.end())
    {
        return Error("Missing 'matchPattern' parameter");
    }
    auto pattern = it->second;

    it = args.find("matchOperation");
    auto matchOperation = it != args.end() ? it->second : std::string("pattern match");

    std::ifstream file(path);
    if (!file.is_open())
    {
        logstream << "Failed to open file" << std::endl;
        return false;
    }

    if (matchOperation == "pattern match")
    {
        try
        {
            std::regex regex(pattern, std::regex_constants::ECMAScript);

            std::string line;
            while (std::getline(file, line))
            {
                if (std::regex_search(line, regex))
                {
                    logstream << "pattern '" << std::move(pattern) << "' found in '" << path << "'";
                    return true;
                }
            }

            logstream << "pattern '" << std::move(pattern) << "' not found in '" << path << "'";
            return false;
        }
        catch (const std::exception& e)
        {
            return Error(std::string("Failed to match pattern: '") + pattern + "' " + e.what());
        }
    }

    return Error(std::string("Unsupported operation '") + matchOperation + std::string("'"));
}
} // namespace compliance
