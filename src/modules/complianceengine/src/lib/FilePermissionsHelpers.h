// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <fts.h>
#include <grp.h>
#include <iostream>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace ComplianceEngine
{
// For given filename, audit the values (owner, group, permissions, mask) given in args.
Result<Status> AuditEnsureFilePermissionsHelper(const std::string& filename, const std::map<std::string, std::string>& args, IndicatorsTree& indicators,
    ContextInterface& context);
// Remediate, as above.
Result<Status> RemediateEnsureFilePermissionsHelper(const std::string& filename, const std::map<std::string, std::string>& args,
    IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine 
