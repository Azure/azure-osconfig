// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ListValidShells.h>
#include <fstream>

namespace ComplianceEngine
{
using std::set;
using std::string;
constexpr const char* etcShellsPath = "/etc/shells";
Result<set<string>> ListValidShells(ContextInterface& context)
{
    set<string> validShells;
    OsConfigLogDebug(context.GetLogHandle(), "Listing valid shells from %s", context.GetSpecialFilePath(etcShellsPath).c_str());
    std::ifstream shellsFile(context.GetSpecialFilePath(etcShellsPath));
    if (!shellsFile.is_open())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to open %s file", etcShellsPath);
        return Error(std::string("Failed to open ") + etcShellsPath + " file", EINVAL);
    }

    string line;
    while (std::getline(shellsFile, line))
    {
        auto index = line.find('#');
        if (index != string::npos)
        {
            line = line.substr(0, index);
        }

        index = line.rfind('/');
        if (index == string::npos)
        {
            index = 0;
        }
        const auto pos = line.find("nologin", index);
        if (pos != std::string::npos)
        {
            OsConfigLogDebug(context.GetLogHandle(), "Ignoring %s entry: %s", etcShellsPath, line.c_str());
            continue;
        }

        validShells.insert(line);
    }
    return validShells;
}
} // namespace ComplianceEngine
