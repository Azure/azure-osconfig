#include <CommonUtils.h>
#include <Evaluator.h>
#include <grp.h>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace compliance
{
AUDIT_FN(EnsureRootPath)
{
    UNUSED(args);
    UNUSED(context);

    // Directory must not be world- or group-writable
    const int maxPerm = 0777 & ~0022;
    // We're using sudo to get the proper root login shell, with the whole environment loaded.
    auto rootEnv = context.ExecuteCommand("sudo -Hiu root env");
    if (!rootEnv.HasValue())
    {
        return indicators.NonCompliant("Failed to run sudo: " + rootEnv.Error().message);
    }

    std::string rootPath;
    std::istringstream envStream(rootEnv.Value());
    std::string line;
    while (std::getline(envStream, line))
    {
        if (line.find("PATH=") == 0)
        {
            rootPath = line.substr(strlen("PATH="));
            break;
        }
    }

    if (rootPath.empty())
    {
        return indicators.NonCompliant("root's PATH is empty");
    }

    if (rootPath[rootPath.length() - 1] == ':')
    {
        return indicators.NonCompliant("Trailing colon in root's PATH");
    }
    std::istringstream pathStream(rootPath);
    std::string path;
    while (std::getline(pathStream, path, ':'))
    {
        if (path.empty())
        {
            return indicators.NonCompliant("Empty path in root's PATH");
        }
        if ("." == path || ".." == path)
        {
            return indicators.NonCompliant("Path in root's PATH is '.' or '..'");
        }

        struct stat statbuf;
        if (stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode))
        {
            auto owner = getpwuid(statbuf.st_uid);
            if (!owner || std::string(owner->pw_name) != "root")
            {
                return indicators.NonCompliant("Directory '" + path + "' from root's PATH is not owned by root");
            }

            if ((statbuf.st_mode & 0777) & ~maxPerm)
            {
                std::ostringstream oss;
                oss << "Directory '" << path << "' from root's PATH has too permissive access - " << std::oct << (statbuf.st_mode & 0777)
                    << " should be at most " << std::oct << maxPerm;
                return indicators.NonCompliant(oss.str());
            }
        }
        else
        {
            return indicators.NonCompliant("Path '" + path + "' from root's PATH does not exist or is not a directory");
        }
    }

    return indicators.Compliant("Root's PATH does not contain dangerous entries.");
}
} // namespace compliance
