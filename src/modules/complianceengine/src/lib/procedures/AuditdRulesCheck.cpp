// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <AuditdRulesCheck.h>
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Optional.h>
#include <Regex.h>
#include <StringTools.h>
#include <fstream>
#include <fts.h>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace ComplianceEngine
{
namespace
{
int GetUidMin(ContextInterface& context)
{
    const int defaultUidMin = 1000;
    const std::string prefix = "UID_MIN ";
    auto loginDefsResult = context.GetFileContents("/etc/login.defs");
    if (!loginDefsResult.HasValue() || loginDefsResult.Value().empty())
    {
        OsConfigLogWarning(context.GetLogHandle(), "Failed to read /etc/login.defs, using default UID_MIN");
        return defaultUidMin;
    }
    std::stringstream ss(loginDefsResult.Value());
    std::string line;
    while (std::getline(ss, line))
    {
        line = TrimWhiteSpaces(line);
        if (line.length() > prefix.length() && line.substr(0, prefix.length()) == prefix)
        {
            auto value = line.substr(prefix.length());
            value = TrimWhiteSpaces(value);
            try
            {
                return std::stoi(value);
            }
            catch (const std::exception&)
            {
                OsConfigLogWarning(context.GetLogHandle(), "Invalid UID_MIN value in /etc/login.defs, using default");
                return defaultUidMin;
            }
        }
    }
    OsConfigLogWarning(context.GetLogHandle(), "UID_MIN not found in /etc/login.defs, using default");
    return defaultUidMin;
}

std::string ReplaceAuidPlaceholder(const std::string& option, int uidMin)
{
    regex auidRegex(R"(-F auid>=[0-9]+\b)");
    smatch m;
    std::string replaced = option;
    // Replace all matches of auidRegex with the new value
    size_t offset = 0;
    while (offset < replaced.size())
    {
        std::string sub = std::string(replaced.begin() + offset, replaced.end());
        if (!regex_search(sub, m, auidRegex))
        {
            break;
        }
        auto pos = m.position(0) + offset;
        auto len = m.length(0);
        replaced.replace(pos, len, "-F auid>=" + std::to_string(uidMin));
        offset = pos + std::string("-F auid>=").length() + std::to_string(uidMin).length();
    }
    return replaced;
}

Result<std::vector<std::string>> GetRulesFromRunningConfig(ContextInterface& context)
{
    std::vector<std::string> rules;
    auto commandResult = context.ExecuteCommand("auditctl -l");
    if (!commandResult.HasValue())
    {
        return Error("auditctl command failed: " + commandResult.Error().message, commandResult.Error().code);
    }

    std::stringstream ss(commandResult.Value());
    std::string line;

    while (std::getline(ss, line))
    {
        if (line == "No rules")
        {
            return rules;
        }
        auto pos = line.find_first_of('#');
        if (pos != std::string::npos)
        {
            line = line.substr(0, pos);
        }
        line = TrimWhiteSpaces(line);
        if (!line.empty())
        {
            rules.push_back(line);
        }
    }
    return rules;
}

Result<std::vector<std::string>> GetRulesFromFilesAtPath(ContextInterface& context, const std::string& directory)
{
    std::vector<std::string> rules;

    // First check if directory exists
    struct stat statbuf;
    if (stat(directory.c_str(), &statbuf) != 0 || !S_ISDIR(statbuf.st_mode))
    {
        OsConfigLogWarning(context.GetLogHandle(), "Directory does not exist or is not accessible: %s", directory.c_str());
        return Error("Failed to access audit rules directory: " + directory, errno);
    }

    char* paths[] = {const_cast<char*>(directory.c_str()), nullptr};
    FTS* fts = fts_open(paths, FTS_NOCHDIR | FTS_PHYSICAL, nullptr);
    if (fts == nullptr)
    {
        OsConfigLogWarning(context.GetLogHandle(), "Failed to open %s directory", directory.c_str());
        return Error("Failed to open audit rules directory: " + directory, errno);
    }

    FTSENT* ent = nullptr;
    while ((ent = fts_read(fts)) != nullptr)
    {
        if (ent->fts_info == FTS_F)
        {
            std::string filename = ent->fts_name;
            if (filename.size() >= 6 && filename.substr(filename.size() - 6) == ".rules")
            {
                std::ifstream file(ent->fts_path);
                if (!file)
                {
                    OsConfigLogWarning(context.GetLogHandle(), "Failed to open audit rule file: %s", ent->fts_path);
                    continue;
                }

                std::string line;
                while (std::getline(file, line))
                {
                    auto pos = line.find_first_of('#');
                    if (pos != std::string::npos)
                    {
                        line = line.substr(0, pos);
                    }
                    line = TrimWhiteSpaces(line);
                    if (line.empty())
                    {
                        continue;
                    }
                    rules.push_back(line);
                }
            }
        }
    }
    fts_close(fts);
    return rules;
}

Result<std::string> FindSudoLogfile(ContextInterface& context)
{
    auto sudoersResult = context.ExecuteCommand("grep -E '^[[:space:]]*[Dd]efaults.*logfile' /etc/sudoers 2>/dev/null | tail -1");
    if (sudoersResult.HasValue() && !sudoersResult.Value().empty())
    {
        regex logfileRegex(R"(logfile[[:space:]]*=[[:space:]]*([^,[[:space:]]+))");
        smatch match;
        if (regex_search(sudoersResult.Value(), match, logfileRegex))
        {
            std::string logfile = match[1].str();
            if (logfile.front() == '"' && logfile.back() == '"')
            {
                logfile = logfile.substr(1, logfile.length() - 2);
            }
            return logfile;
        }
    }

    auto sudoersDResult = context.ExecuteCommand("grep -h -E '^[[:space:]]*[Dd]efaults.*logfile' /etc/sudoers.d/* 2>/dev/null | tail -1");
    if (sudoersDResult.HasValue() && !sudoersDResult.Value().empty())
    {
        regex logfileRegex(R"(logfile[[:space:]]*=[[:space:]]*([^,[[:space:]]+))");
        smatch match;
        if (regex_search(sudoersDResult.Value(), match, logfileRegex))
        {
            std::string logfile = match[1].str();
            if (logfile.length() > 2 && logfile.front() == '"' && logfile.back() == '"')
            {
                logfile = logfile.substr(1, logfile.length() - 2);
            }
            return logfile;
        }
    }

    return Error("Sudo logfile setting not found", ENOENT);
}

Status CheckRuleInList(const std::vector<std::string>& rules, const std::string& searchItem, const Optional<regex>& excludeRegex,
    const std::vector<regex>& requiredRegexes, IndicatorsTree& indicators)
{
    for (const auto& rule : rules)
    {
        if (rule.find(searchItem) == std::string::npos)
        {
            continue;
        }
        if (excludeRegex.HasValue() && regex_search(rule, excludeRegex.Value()))
        {
            continue;
        }
        for (const auto& req : requiredRegexes)
        {
            if (!regex_search(rule, req))
            {
                return indicators.NonCompliant("Rule is missing required options: " + rule);
            }
        }
        return indicators.Compliant("Rule found: " + rule + " and is properly configured");
    }
    return indicators.NonCompliant("Rule not found: " + searchItem);
}
} // namespace

Result<Status> AuditAuditdRulesCheck(const AuditAuditdRulesCheckParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    Optional<regex> excludeOption;
    if (params.excludeOption.HasValue())
    {
        try
        {
            excludeOption = regex(params.excludeOption.Value(), std::regex_constants::icase | std::regex_constants::extended);
        }
        catch (const regex_error& e)
        {
            return Error("Invalid excludeOptions regex: " + std::string(e.what()), EINVAL);
        }
    }

    auto uidMin = GetUidMin(context);
    std::vector<regex> requiredOptions;
    for (auto option : params.requiredOptions.items)
    {
        option = TrimWhiteSpaces(option);
        if (!option.empty())
        {
            option = ReplaceAuidPlaceholder(option, uidMin);
            try
            {
                requiredOptions.push_back(regex(option, std::regex_constants::icase | std::regex_constants::extended));
            }
            catch (const regex_error& e)
            {
                return Error("Invalid requiredOptions regex: " + std::string(e.what()), EINVAL);
            }
        }
    }

    auto runningRulesResult = GetRulesFromRunningConfig(context);
    if (!runningRulesResult.HasValue())
    {
        return Error("Failed to get running audit rules: " + runningRulesResult.Error().message, runningRulesResult.Error().code);
    }
    auto runningRules = runningRulesResult.Value();

    // Determine rules directory (default or test override)
    std::string rulesDirectory = context.GetSpecialFilePath("/etc/audit/rules.d");

    auto filesRulesResult = GetRulesFromFilesAtPath(context, rulesDirectory);
    if (!filesRulesResult.HasValue())
    {
        return Error("Failed to get audit rules from files: " + filesRulesResult.Error().message, filesRulesResult.Error().code);
    }
    auto filesRules = filesRulesResult.Value();

    if (params.searchItem.find("-S ") == 0)
    {
        std::vector<std::string> syscalls;
        std::stringstream ss(params.searchItem.substr(3));
        std::string syscall;
        while (std::getline(ss, syscall, ','))
        {
            auto runningResult = CheckRuleInList(runningRules, "-S " + syscall, excludeOption, requiredOptions, indicators);
            if (runningResult != Status::Compliant)
            {
                return runningResult;
            }
            auto fileResult = CheckRuleInList(filesRules, "-S " + syscall, excludeOption, requiredOptions, indicators);
            if (fileResult != Status::Compliant)
            {
                return fileResult;
            }
        }
        return indicators.Compliant("All syscall rules found and properly configured");
    }
    else if (params.searchItem.find("SUDOLOGFILE") == 0)
    {
        auto logfileResult = FindSudoLogfile(context);
        if (!logfileResult.HasValue())
        {
            return logfileResult.Error();
        }
        std::string actualSearchItem = "-w " + logfileResult.Value();
        auto runningResult = CheckRuleInList(runningRules, actualSearchItem, excludeOption, requiredOptions, indicators);
        if (runningResult != Status::Compliant)
        {
            return runningResult;
        }
        auto fileResult = CheckRuleInList(filesRules, actualSearchItem, excludeOption, requiredOptions, indicators);
        if (fileResult != Status::Compliant)
        {
            return fileResult;
        }
        return indicators.Compliant("Sudo logfile rule found and properly configured");
    }
    else if (params.searchItem.find("-e 2") == 0)
    {
        return CheckRuleInList(filesRules, "-e 2", excludeOption, requiredOptions, indicators);
    }
    else
    {
        auto runningResult = CheckRuleInList(runningRules, params.searchItem, excludeOption, requiredOptions, indicators);
        if (runningResult != Status::Compliant)
        {
            return runningResult;
        }
        auto fileResult = CheckRuleInList(filesRules, params.searchItem, excludeOption, requiredOptions, indicators);
        if (fileResult != Status::Compliant)
        {
            return fileResult;
        }
        return indicators.Compliant("Rule found: " + params.searchItem + " and is properly configured");
    }
}

} // namespace ComplianceEngine
