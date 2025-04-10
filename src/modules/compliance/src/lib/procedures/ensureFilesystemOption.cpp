#include "../Evaluator.h"

#include <CommonUtils.h>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <linux/limits.h>
#include <map>
#include <mntent.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace compliance
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
    Indicators& indicators)
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
};

AUDIT_FN(EnsureFilesystemOption, "mountpoint:Filesystem mount point:M", "optionsSet:Comma-separated list of options that must be set",
    "optionsNotSet:Comma-separated list of options that must not be set", "test_fstab:Location of the fstab file", "test_mtab:Location of the mtab file")
{
    UNUSED(context);
    if (args.find("mountpoint") == args.end())
    {
        return Error("No mountpoint provided");
    }
    std::string mountpoint = args["mountpoint"];
    std::string fstab = "/etc/fstab";
    std::string mtab = "/etc/mtab";
    if (args.find("test_fstab") != args.end())
    {
        fstab = args["test_fstab"];
    }
    if (args.find("test_mtab") != args.end())
    {
        mtab = args["test_mtab"];
    }
    auto fstabEntries = ParseFstab(fstab);
    if (!fstabEntries.HasValue())
    {
        return fstabEntries.Error();
    }

    auto mtabEntries = ParseFstab(mtab);
    if (!mtabEntries.HasValue())
    {
        return mtabEntries.Error();
    }

    std::set<std::string> optionsSet, optionsNotSet;
    if (args.find("optionsSet") != args.end())
    {
        std::istringstream iss(args.at("optionsSet"));
        std::string option;
        while (std::getline(iss, option, ','))
        {
            optionsSet.insert(option);
        }
    }
    if (args.find("optionsNotSet") != args.end())
    {
        std::istringstream iss(args.at("optionsNotSet"));
        std::string option;
        while (std::getline(iss, option, ','))
        {
            optionsNotSet.insert(option);
        }
    }

    if (fstabEntries->find(mountpoint) != fstabEntries->end())
    {
        if (Status::NonCompliant == CheckOptions(fstabEntries.Value()[mountpoint].options, optionsSet, optionsNotSet, indicators))
        {
            return Status::NonCompliant;
        }
    }
    else
    {
        indicators.Compliant("Mountpoint " + mountpoint + " not found in /etc/fstab");
    }

    if (mtabEntries->find(mountpoint) != mtabEntries->end())
    {
        if (Status::NonCompliant == CheckOptions(mtabEntries.Value()[mountpoint].options, optionsSet, optionsNotSet, indicators))
        {
            return Status::NonCompliant;
        }
    }
    else
    {
        indicators.Compliant("Mountpoint " + mountpoint + " not found in /etc/mtab");
    }

    return indicators.Compliant("All /etc/fstab and /etc/mtab options are verified");
}

REMEDIATE_FN(EnsureFilesystemOption, "mountpoint:Filesystem mount point:M", "optionsSet:Comma-separated list of options that must be set",
    "optionsNotSet:Comma-separated list of options that must not be set", "test_fstab:Location of the fstab file",
    "test_mtab:Location of the mtab file", "test_mount:Location of the mount binary")
{
    UNUSED(context);
    if (args.find("mountpoint") == args.end())
    {
        return Error("No mountpoint provided");
    }
    std::string mountpoint = args["mountpoint"];
    std::string fstab = "/etc/fstab";
    std::string mtab = "/etc/mtab";
    if (args.find("test_fstab") != args.end())
    {
        fstab = args["test_fstab"];
    }
    if (args.find("test_mtab") != args.end())
    {
        mtab = args["test_mtab"];
    }
    std::string mount = "/sbin/mount";
    if (args.find("test_mount") != args.end())
    {
        mount = args["test_mount"];
    }

    auto fstabEntries = ParseFstab(fstab);
    if (!fstabEntries.HasValue())
    {
        return fstabEntries.Error();
    }

    auto mtabEntries = ParseFstab(mtab);
    if (!mtabEntries.HasValue())
    {
        return mtabEntries.Error();
    }

    std::set<std::string> optionsSet, optionsNotSet;
    if (args.find("optionsSet") != args.end())
    {
        std::istringstream iss(args.at("optionsSet"));
        std::string option;
        while (std::getline(iss, option, ','))
        {
            optionsSet.insert(option);
        }
    }
    if (args.find("optionsNotSet") != args.end())
    {
        std::istringstream iss(args.at("optionsNotSet"));
        std::string option;
        while (std::getline(iss, option, ','))
        {
            optionsNotSet.insert(option);
        }
    }

    if (fstabEntries->find(mountpoint) != fstabEntries->end())
    {
        if (Status::NonCompliant == CheckOptions(fstabEntries.Value()[mountpoint].options, optionsSet, optionsNotSet, indicators))
        {
            auto& entry = fstabEntries.Value()[mountpoint];
            std::ifstream file(fstab);
            std::ofstream tempFile(fstab + ".tmp");
            std::string line;
            int lineno = 0;
            while (std::getline(file, line))
            {
                lineno++;
                if (lineno == entry.lineno)
                {
                    std::ostringstream oss;
                    oss << entry.device << " " << mountpoint << " " << entry.filesystem << " ";
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
                    indicators.Compliant("Updated fstab entry for " + mountpoint + " with options: " + oss.str());
                }
                else
                {
                    tempFile << line << "\n";
                }
            }
            file.close();
            tempFile.close();
            char timeString[PATH_MAX];
            auto tm = time(nullptr);
            strftime(timeString, 64, "%Y%m%d%H%M%S", gmtime(&tm));
            if (0 != rename(fstab.c_str(), (fstab + ".bak." + timeString).c_str()))
            {
                return Error("Failed to backup " + fstab + " with error " + std::to_string(errno));
            }
            if (0 != rename((fstab + ".tmp").c_str(), fstab.c_str()))
            {
                return Error("Failed to rename " + fstab + ".tmp with error " + std::to_string(errno));
            }
        }
    }

    if (mtabEntries->find(mountpoint) != mtabEntries->end())
    {
        if (Status::NonCompliant == CheckOptions(mtabEntries.Value()[mountpoint].options, optionsSet, optionsNotSet, indicators))
        {
            std::string command = mount + " -o remount " + mountpoint;
            system(command.c_str());
            indicators.Compliant("Remounted " + mountpoint + " with options: " + command);
        }
    }

    return Status::Compliant;
}
} // namespace compliance
