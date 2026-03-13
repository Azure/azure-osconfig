#include <CommonContext.h>
#include <StringTools.h>
#include <Users.h>

namespace ComplianceEngine
{
Result<unsigned int> GetUidMin(ContextInterface& context)
{
    const std::string prefix = "UID_MIN ";
    auto loginDefsResult = context.GetFileContents("/etc/login.defs");
    if (!loginDefsResult.HasValue() || loginDefsResult.Value().empty())
    {
        OsConfigLogWarning(context.GetLogHandle(), "Failed to read /etc/login.defs");
        return Error("Failed to read /etc/login.defs");
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
            auto result = TryStringToUint(value);
            if (!result.HasValue())
            {
                OsConfigLogWarning(context.GetLogHandle(), "Invalid UID_MIN value in /etc/login.defs %s", result.Error().message.c_str());
            }
            return result;
        }
    }
    return Error("Could not get UID_MIN find value");
}
} // namespace ComplianceEngine
