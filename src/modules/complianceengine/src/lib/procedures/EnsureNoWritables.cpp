// Ensures there are no world-writable regular files and that all world-writable directories have the sticky bit set.
// World-writable: (mode & 0002). Sticky bit required on dirs: (mode & S_ISVTX) when (S_IWOTH) is set.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <FilesystemScanner.h>
#include <sys/stat.h>

namespace ComplianceEngine
{

AUDIT_FN(EnsureNoWritables)
{
    UNUSED(args);

    FilesystemScanner& scanner = context.GetFilesystemScanner();
    auto fsRes = scanner.GetFullFilesystem();
    if (!fsRes)
    {
        return Result<Status>(fsRes.Error());
    }
    const auto& entries = fsRes.Value()->entries;

    std::vector<std::string> violations;
    violations.reserve(3);
    for (const auto& kv : entries)
    {
        if (violations.size() >= 3)
        {
            break;
        }
        const std::string& path = kv.first;
        const auto& st = kv.second.st;
        mode_t mode = st.st_mode;
        if (S_ISREG(mode) && (mode & S_IWOTH))
        {
            violations.push_back("file: " + path);
            continue;
        }
        if (S_ISDIR(mode) && (mode & S_IWOTH) && !(mode & S_ISVTX))
        {
            violations.push_back("dir-no-sticky: " + path);
            continue;
        }
    }
    if (!violations.empty())
    {
        std::string msg = "World-writable issues (up to 3): ";
        for (size_t i = 0; i < violations.size(); ++i)
        {
            if (i)
                msg += "; ";
            msg += violations[i];
        }
        return indicators.NonCompliant(msg);
    }
    return indicators.Compliant("No world-writable files; all world-writable directories have sticky bit");
}

} // namespace ComplianceEngine
