#include "../Evaluator.h"

#include <CommonUtils.h>
#include <EnsureFilesystemOption.h>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <linux/limits.h>
#include <map>
#include <mntent.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace ComplianceEngine
{
struct FstabEntry
{
    std::string device;
    std::string filesystem;
    std::vector<std::string> options;
    int dump{};
    int pass{};
    int lineno{};
};

static Result<std::map<std::string, FstabEntry>> ParseFstab(const std::string& filePath)
{
    std::map<std::string, FstabEntry> fstabMap;
    FILE* file = fopen(filePath.c_str(), "r");
    if (nullptr == file)
    {
        return Error("Failed to open file " + filePath + " with error " + strerror(errno));
    }

    int lineno = 0;
    while (!feof(file))
    {
        lineno++;
        char b = 0;
        if (fread(&b, 1, 1, file) != 1)
        {
            break;
        }
        // Preserve comments and empty lines.
        if (b == '#' || b == '\n')
        {
            while (b != '\n')
            {
                fread(&b, 1, 1, file);
            }
            continue;
        }
        else
        {
            fseek(file, -1, SEEK_CUR);
        }
        struct mntent* mnt = getmntent(file);
        if (nullptr == mnt)
        {
            continue;
        }
        FstabEntry entry;
        entry.device = mnt->mnt_fsname;
        entry.filesystem = mnt->mnt_type;
        entry.dump = mnt->mnt_freq;
        entry.pass = mnt->mnt_passno;
        entry.lineno = lineno;

        std::istringstream optionsStream(mnt->mnt_opts);
        std::string option;
        while (std::getline(optionsStream, option, ','))
        {
            entry.options.push_back(option);
        }

        fstabMap[mnt->mnt_dir] = entry;
    }
    fclose(file);
    return fstabMap;
}

static Status CheckOptions(const std::vector<std::string>& options, const std::set<std::string>& optionsSet, const std::set<std::string>& optionsNotSet,
    IndicatorsTree& indicators)
{
    for (const auto& option : optionsSet)
    {
        if (std::find(options.begin(), options.end(), option) == options.end())
        {
            return indicators.NonCompliant("Required option not set: " + option);
        }

        indicators.Compliant("Required option is set: " + option);
    }
    for (const auto& option : optionsNotSet)
    {
        if (std::find(options.begin(), options.end(), option) != options.end())
        {
            return indicators.NonCompliant("Forbidden option is set: " + option);
        }

        indicators.Compliant("Forbidden option is not set: " + option);
    }

    return indicators.Compliant("All required options are set and no forbidden options are set");
}

Result<Status> AuditEnsureFilesystemOption(const EnsureFilesystemOptionParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(context);
    assert(params.test_fstab.HasValue());
    assert(params.test_mtab.HasValue());
    assert(params.test_mount.HasValue());
    auto fstabEntries = ParseFstab(params.test_fstab.Value());
    if (!fstabEntries.HasValue())
    {
        return fstabEntries.Error();
    }

    auto mtabEntries = ParseFstab(params.test_mtab.Value());
    if (!mtabEntries.HasValue())
    {
        return mtabEntries.Error();
    }

    std::set<std::string> optionsSet, optionsNotSet;
    if (params.optionsSet.HasValue())
    {
        std::copy(params.optionsSet->items.cbegin(), params.optionsSet->items.cend(), std::inserter(optionsSet, optionsSet.begin()));
    }
    if (params.optionsNotSet.HasValue())
    {
        std::copy(params.optionsNotSet->items.cbegin(), params.optionsNotSet->items.cend(), std::inserter(optionsNotSet, optionsNotSet.begin()));
    }

    if (fstabEntries->find(params.mountpoint) != fstabEntries->end())
    {
        if (Status::NonCompliant == CheckOptions(fstabEntries.Value()[params.mountpoint].options, optionsSet, optionsNotSet, indicators))
        {
            return Status::NonCompliant;
        }
    }
    else
    {
        indicators.Compliant("Mountpoint " + params.mountpoint + " not found in /etc/fstab");
    }

    if (mtabEntries->find(params.mountpoint) != mtabEntries->end())
    {
        if (Status::NonCompliant == CheckOptions(mtabEntries.Value()[params.mountpoint].options, optionsSet, optionsNotSet, indicators))
        {
            return Status::NonCompliant;
        }
    }
    else
    {
        indicators.Compliant("Mountpoint " + params.mountpoint + " not found in /etc/mtab");
    }

    return indicators.Compliant("All /etc/fstab and /etc/mtab options are verified");
}

Result<Status> RemediateEnsureFilesystemOption(const EnsureFilesystemOptionParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    UNUSED(context);
    assert(params.test_fstab.HasValue());
    assert(params.test_mtab.HasValue());
    assert(params.test_mount.HasValue());

    auto fstabEntries = ParseFstab(params.test_fstab.Value());
    if (!fstabEntries.HasValue())
    {
        return fstabEntries.Error();
    }

    auto mtabEntries = ParseFstab(params.test_mtab.Value());
    if (!mtabEntries.HasValue())
    {
        return mtabEntries.Error();
    }

    std::set<std::string> optionsSet, optionsNotSet;
    if (params.optionsSet.HasValue())
    {
        std::copy(params.optionsSet->items.cbegin(), params.optionsSet->items.cend(), std::inserter(optionsSet, optionsSet.begin()));
    }
    if (params.optionsNotSet.HasValue())
    {
        std::copy(params.optionsNotSet->items.cbegin(), params.optionsNotSet->items.cend(), std::inserter(optionsNotSet, optionsNotSet.begin()));
    }

    if (fstabEntries->find(params.mountpoint) != fstabEntries->end())
    {
        if (Status::NonCompliant == CheckOptions(fstabEntries.Value()[params.mountpoint].options, optionsSet, optionsNotSet, indicators))
        {
            auto& entry = fstabEntries.Value()[params.mountpoint];
            std::ifstream file(params.test_fstab.Value());
            std::ofstream tempFile(params.test_fstab.Value() + ".tmp");
            std::string line;
            int lineno = 0;
            while (std::getline(file, line))
            {
                lineno++;
                if (lineno == entry.lineno)
                {
                    std::ostringstream oss;
                    oss << entry.device << " " << params.mountpoint << " " << entry.filesystem << " ";
                    auto missingOptions = optionsSet;
                    for (const auto& option : entry.options)
                    {
                        if (missingOptions.find(option) != missingOptions.end())
                        {
                            missingOptions.erase(option);
                        }
                        if (optionsNotSet.find(option) != optionsNotSet.end())
                        {
                            indicators.Compliant("Forbidden option " + option + " removed");
                        }
                        else
                        {
                            oss << option << ",";
                        }
                    }
                    for (const auto& option : missingOptions)
                    {
                        oss << option << ",";
                    }
                    oss.seekp(-1, std::ios_base::end);
                    oss << " " << entry.dump << " " << entry.pass << "\n";
                    tempFile << oss.str();
                    indicators.Compliant("Updated fstab entry for " + params.mountpoint + " with options: " + oss.str());
                }
                else
                {
                    tempFile << line << "\n";
                }
            }
            file.close();
            tempFile.close();
            char timeString[PATH_MAX] = {0};
            auto tm = time(nullptr);
            strftime(timeString, 64, "%Y%m%d%H%M%S", gmtime(&tm));
            if (0 != rename(params.test_fstab.Value().c_str(), (params.test_fstab.Value() + ".bak." + timeString).c_str()))
            {
                return Error("Failed to backup " + params.test_fstab.Value() + " with error " + std::to_string(errno));
            }
            if (0 != rename((params.test_fstab.Value() + ".tmp").c_str(), params.test_fstab.Value().c_str()))
            {
                return Error("Failed to rename " + params.test_fstab.Value() + ".tmp with error " + std::to_string(errno));
            }
        }
    }

    if (mtabEntries->find(params.mountpoint) != mtabEntries->end())
    {
        if (Status::NonCompliant == CheckOptions(mtabEntries.Value()[params.mountpoint].options, optionsSet, optionsNotSet, indicators))
        {
            std::string command = params.test_mount.Value() + " -o remount " + params.mountpoint;
            system(command.c_str());
            indicators.Compliant("Remounted " + params.mountpoint + " with options: " + command);
        }
    }

    return Status::Compliant;
}
} // namespace ComplianceEngine
