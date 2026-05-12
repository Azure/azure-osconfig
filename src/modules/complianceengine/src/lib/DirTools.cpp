// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "DirTools.h"

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace ComplianceEngine
{
bool mkdir_recursive(const std::string& path, mode_t mode)
{
    if (path.empty())
        return false;

    std::string tmp;
    tmp.reserve(path.size());

    for (size_t i = 0; i < path.size(); ++i)
    {
        tmp += path[i];

        // create on '/' boundaries
        if (path[i] == '/' || i == path.size() - 1)
        {
            if (tmp.size() == 0)
                continue;

            if (::mkdir(tmp.c_str(), mode) != 0)
            {
                if (errno == EEXIST)
                {
                    continue;
                }
                return false;
            }
        }
    }
    return true;
}

} // namespace ComplianceEngine
