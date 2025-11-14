// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <ListValidShells.h>
#include <Result.h>
#include <StringTools.h>
#include <Telemetry.h>
#include <UsersIterator.h>
#include <fstream>
#include <set>
#include <shadow.h>
#include <sstream>
#include <sys/stat.h>

namespace ComplianceEngine
{
using std::set;
using std::string;

namespace
{
static const set<string> GetWhitelistedAccounts()
{
    static const set<string> sAccounts = {"root", "halt", "sync", "shutdown", "nfsnobody"};
    return sAccounts;
}

Result<uid_t> LoadMinUID(ContextInterface& context)
{
    static const uid_t defaultUID = 1000;
    const auto filename = context.GetSpecialFilePath("/etc/login.defs");

    struct stat st;
    if (0 != stat(filename.c_str(), &st))
    {
        int status = errno;
        if (ENOENT == status)
        {
            OsConfigLogInfo(context.GetLogHandle(), "/etc/login.defs file is missing, assuming minimum user UID is %d", defaultUID);
            return defaultUID;
        }

        return Error(string("Failed to read ") + filename + " file: " + strerror(status), status);
    }

    std::ifstream file(filename);
    if (!file.is_open())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to read %s", filename.c_str());
        OSConfigTelemetryStatusTrace("fopen", EINVAL);
        return Error(string("Failed to read ") + filename, EINVAL);
    }

    string line;
    const string keyword = "UID_MIN";
    while (std::getline(file, line))
    {
        auto index = line.find('#');
        if (string::npos != index)
        {
            line = line.substr(0, index);
        }

        index = line.find(keyword);
        if (string::npos == index)
        {
            continue;
        }

        string value;
        line = line.substr(index + keyword.size());

        std::stringstream stream(line);
        stream >> value;

        auto uid = TryStringToInt(value);
        if (!uid.HasValue())
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to parse UID_MIN value: %s", uid.Error().message.c_str());
            OSConfigTelemetryStatusTrace("TryStringToInt", uid.Error().code);
            return uid.Error();
        }

        if (uid.Value() < 0)
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to parse UID_MIN value: must not be negative");
            OSConfigTelemetryStatusTrace("UID_MIN", EINVAL);
            return Error("Failed to parse UID_MIN value: must not be negative", EINVAL);
        }

        return static_cast<uid_t>(uid.Value());
    }

    return defaultUID;
}
} // anonymous namespace

Result<Status> AuditEnsureSystemAccountsDoNotHaveValidShell(IndicatorsTree& indicators, ContextInterface& context)
{
    const auto& whitelistedAccounts = GetWhitelistedAccounts();
    const auto validShells = ListValidShells(context);
    if (!validShells.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to get valid shells: %s", validShells.Error().message.c_str());
        OSConfigTelemetryStatusTrace("ListValidShells", validShells.Error().code);
        return validShells.Error();
    }

    const auto minUID = LoadMinUID(context);
    if (!minUID.HasValue())
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to get minimum user UID: %s", minUID.Error().message.c_str());
        OSConfigTelemetryStatusTrace("LoadMinUID", minUID.Error().code);
        return minUID.Error();
    }

    auto users = UsersRange::Make(context.GetSpecialFilePath("/etc/passwd"), context.GetLogHandle());
    if (!users.HasValue())
    {
        return users.Error();
    }

    for (const auto& user : users.Value())
    {
        OsConfigLogInfo(context.GetLogHandle(), "User: %s, UID: %d, shell: %s, min: %d", user.pw_name, user.pw_uid, user.pw_shell, minUID.Value());
        if (user.pw_uid >= minUID.Value())
        {
            continue;
        }

        OsConfigLogInfo(context.GetLogHandle(), "User: %s, UID: %d, shell: %s", user.pw_name, user.pw_uid, user.pw_shell);
        if (whitelistedAccounts.end() != whitelistedAccounts.find(user.pw_name))
        {
            // Skip whitelisted account
            OsConfigLogDebug(context.GetLogHandle(), "Skipping whitelisted account '%s'", user.pw_name);
            continue;
        }

        OsConfigLogInfo(context.GetLogHandle(), "User: %s, UID: %d, shell: %s", user.pw_name, user.pw_uid, user.pw_shell);
        const auto shell = string(user.pw_shell);
        const auto it = validShells->find(shell);
        if (it != validShells->end())
        {
            OsConfigLogInfo(context.GetLogHandle(), "System user %d has a valid login shell '%s'", user.pw_uid, user.pw_shell);
            return indicators.NonCompliant(string("System user ") + std::to_string(user.pw_uid) + " has a valid login shell");
        }

        OsConfigLogInfo(context.GetLogHandle(), "User: %s, UID: %d, shell: %s", user.pw_name, user.pw_uid, user.pw_shell);
        OsConfigLogDebug(context.GetLogHandle(), "System user %d does not have a valid login shell: '%s'", user.pw_uid, user.pw_shell);
        indicators.Compliant(string("System user ") + std::to_string(user.pw_uid) + " does not have a valid login shell");
    }

    return Status::Compliant;
}
} // namespace ComplianceEngine
