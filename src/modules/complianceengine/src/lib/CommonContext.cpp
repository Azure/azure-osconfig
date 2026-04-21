// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonContext.h"

#include "CommonUtils.h"
#include "ContextInterface.h"

namespace ComplianceEngine
{
CommonContext::~CommonContext() = default;

Result<std::string> CommonContext::ExecuteCommand(const std::string& cmd) const
{
    char* output = nullptr;
    int err = ::ExecuteCommand(NULL, cmd.c_str(), false, false, 0, 0, &output, NULL, mLog);
    if (err != 0 || output == nullptr)
    {
        std::string outStr = output == NULL ? "Failed to execute command" : output;
        free(output);
        return Error(outStr, err);
    }
    std::string result(output);
    free(output);
    return result;
}

Result<std::string> CommonContext::GetFileContents(const std::string& filePath) const
{
    struct stat st;
    if (stat(filePath.c_str(), &st) != 0)
    {
        int status = errno;
        if (ENOENT == status)
        {
            return Error("File not found: " + filePath, status);
        }

        return Error("Failed to stat file: " + std::string(strerror(status)), status);
    }

    char* output = LoadStringFromFile(filePath.c_str(), false, mLog);
    if (output == nullptr)
    {
        return Error("Failed to load file contents");
    }
    std::string result(output);
    free(output);
    return result;
}

std::string CommonContext::GetSpecialFilePath(const std::string& path) const
{
    return path;
}

} // namespace ComplianceEngine
