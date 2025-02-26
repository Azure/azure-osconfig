// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Evaluator.h>

namespace compliance
{

AUDIT_FN(packageInstalled)
{
    if (args.find("packageName") == args.end())
    {
        logstream << "No package name provided";
        return Error("No package name provided");
    }
    logstream << "packageInstalled for " << args["packageName"];
    char buf[256];
    snprintf(buf, 256, "dpkg -L %s > /dev/null 2>&1", args["packageName"].c_str());
    int rv = system(buf);
    return rv == 0;
}
} // namespace compliance
