// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <string>
namespace compliance
{

AUDIT_FN(packageInstalled)
{
    char* output = NULL;
    std::string cmdline = "dpkg -s ";
    if (args.find("packageName") == args.end())
    {
        logstream << "No package name provided";
        return Error("No package name provided");
    }
    logstream << "packageInstalled for " << args["packageName"];
    cmdline += args["packageName"];
    auto rv = ExecuteCommand(NULL, cmdline.c_str(), false, false, 0, 0, &output, NULL, NULL);
    if ((0 == rv) && (NULL != output) && (NULL != strstr(output, "\nStatus: install ok installed\n")))
    {
        free(output);
        return true;
    }
    else
    {
        free(output);
        return false;
    }
}
} // namespace compliance
