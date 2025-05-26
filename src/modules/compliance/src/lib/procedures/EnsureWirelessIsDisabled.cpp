
#include "EnsureKernelModule.h"
#include "Result.h"

#include <CommonUtils.h>
#include <Evaluator.h>
#include <cerrno>
#include <climits>
#include <fts.h>
#include <grp.h>
#include <iostream>
#include <pwd.h>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace compliance
{

AUDIT_FN(EnsureWirelessIsDisabled)
{
    UNUSED(args);
    auto log = context.GetLogHandle();

    std::string sysfs = "/sys/class/net";
    char* const paths[] = {&sysfs[0], nullptr};
    std::vector<std::string> devices;
    int status = 0;
    {
        FTS* ftsp = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
        if (!ftsp)
        {
            status = errno;
            OsConfigLogInfo(log, "Directory '%s' does not exist errno %d %s", sysfs.c_str(), status, strerror(status));
            return indicators.NonCompliant("Directory '" + sysfs + "' does not exist errno " + std::to_string(status) + " " + strerror(status));
        }
        auto ftspDeleter = std::unique_ptr<FTS, int (*)(FTS*)>(ftsp, fts_close);
        FTSENT* entry = nullptr;
        while (nullptr != (entry = fts_read(ftsp)))
        {
            if (FTS_F != entry->fts_info && sysfs != entry->fts_path)
            {
                devices.push_back(entry->fts_name);
            }
        }
    }

    std::set<std::string> wirelessKernelModules;
    for (const auto& d : devices)
    {
        std::string sysfs_dev = sysfs + "/" + d + "/";
        char* const sysfs_dev_paths[] = {&sysfs_dev[0], nullptr};
        std::string wireless = "wireless";

        FTS* ftsp = fts_open(sysfs_dev_paths, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
        auto ftspDeleter = std::unique_ptr<FTS, int (*)(FTS*)>(ftsp, fts_close);
        FTSENT* entry = nullptr;
        while (nullptr != (entry = fts_read(ftsp)))
        {
            if (FTS_F != entry->fts_info && wireless == entry->fts_name)
            {
                auto modulePath = sysfs_dev + "device/driver/module";
                char sysfModule[PATH_MAX];
                auto ret = readlink(modulePath.c_str(), sysfModule, PATH_MAX);
                if (!ret)
                {
                    ret = errno;
                    OsConfigLogInfo(log, "Readlink '%s' resolution errror %ld %s", modulePath.c_str(), ret, strerror(ret));
                    return indicators.NonCompliant("Readlink '" + modulePath + "' resolution error " + std::to_string(ret) + " " + strerror(ret));
                }
                std::string moduleName = std::string(sysfModule);
                moduleName = moduleName.substr(moduleName.find_last_of("/\\") + 1);
                wirelessKernelModules.insert(moduleName);
            }
        }
    }
    for (const auto& module : wirelessKernelModules)
    {
        auto result = EnsureKernelModuleUnavailable(module, indicators, context);
        if (result.Value() != Status::NonCompliant)
        {
            return result;
        }
    }
    return indicators.Compliant("");
}
} // namespace compliance
