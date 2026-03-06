#include <CommonContext.h>
#include <StringTools.h>
#include <Users.h>

namespace ComplianceEngine
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
} // namespace ComplianceEngine
