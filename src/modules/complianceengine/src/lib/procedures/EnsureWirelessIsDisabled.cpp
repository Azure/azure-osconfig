
#include "KernelModuleTools.h"
#include "Result.h"

#include <CommonUtils.h>
#include <EnsureWirelessIsDisabled.h>
#include <cerrno>
#include <climits>
#include <fts.h>
#include <grp.h>
#include <iostream>
#include <pwd.h>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace ComplianceEngine
{

Result<Status> AuditEnsureWirelessIsDisabled(const EnsureWirelessIsDisabledParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.test_sysfs_class_net.HasValue());
    auto log = context.GetLogHandle();
    std::string sysfs = params.test_sysfs_class_net.Value();
    // normalize sysfs - remove trailing slash if necessary
    if (sysfs.back() == '/')
        sysfs = sysfs.substr(sysfs.length() - 1);

    char* const paths[] = {&sysfs[0], nullptr};
    std::set<std::string> devices;
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
                std::string is_wireless(entry->fts_path);
                auto pos = is_wireless.rfind("/");
                if (pos == std::string::npos)
                {
                    continue;
                }
                is_wireless = is_wireless.substr(pos + 1);
                if (is_wireless == "wireless")
                {
                    std::string wireles_path(entry->fts_path);
                    // remove sysfs and trailing slash
                    wireles_path = wireles_path.substr(sysfs.length() + 1);
                    pos = wireles_path.find("/");
                    if (pos == std::string::npos)
                    {
                        continue;
                    }
                    auto deviceName = wireles_path.substr(0, pos);
                    devices.insert(deviceName);
                }
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
                char sysfsModule[PATH_MAX] = {0};
                auto ret = readlink(modulePath.c_str(), sysfsModule, PATH_MAX);
                if (ret == -1)
                {
                    ret = errno;
                    OsConfigLogInfo(log, "Readlink '%s' resolution errror %ld %s", modulePath.c_str(), ret, strerror(ret));
                    return indicators.NonCompliant("Readlink '" + modulePath + "' resolution error " + std::to_string(ret) + " " + strerror(ret));
                }
                auto moduleName = std::string(sysfsModule, ret);
                auto pos = moduleName.rfind("/");
                if (pos == std::string::npos)
                {
                    OsConfigLogInfo(log, "Error parsing module name '%s' ignoring it", moduleName.c_str());
                    continue;
                }
                moduleName = moduleName.substr(pos + 1);
                wirelessKernelModules.insert(moduleName);
            }
        }
    }
    for (const auto& module : wirelessKernelModules)
    {
        auto isModuleLoaded = IsKernelModuleLoaded(module, context);
        if (!isModuleLoaded.HasValue())
        {
            return Result<Status>(isModuleLoaded.Error());
        }
        else if (isModuleLoaded.Value() == true)
        {
            return indicators.NonCompliant("Kernel module loaded '" + module + "'");
        }
        auto isModuleBlocked = IsKernelModuleBlocked(module, indicators, context);
        if (!isModuleBlocked.HasValue() || isModuleBlocked.Value() == Status::NonCompliant)
        {
            return isModuleBlocked;
        }
    }
    return indicators.Compliant("No wireless kernel module found");
}
} // namespace ComplianceEngine
