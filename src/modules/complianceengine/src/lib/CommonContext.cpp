// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonContext.h"

#include "CommonUtils.h"

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
    char* output = LoadStringFromFile(filePath.c_str(), false, mLog);
    if (output == nullptr)
    {
        return Error("Failed to load file contents");
    }
    std::string result(output);
    free(output);
    return result;
}

} // namespace ComplianceEngine
