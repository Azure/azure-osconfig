// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <EnsureXdmcp.h>
#include <Regex.h>
#include <fstream>
#include <string>

namespace ComplianceEngine
{
Result<Status> AuditEnsureXdmcp(IndicatorsTree& indicators, ContextInterface& context)
{
    auto log = context.GetLogHandle();
    regex xdmcpSection = regex(R"(\[xdmcp\])");
    regex xdmcpEnable = regex(R"(\s*Enable\s*=\s*true)");
    bool found = false;
    size_t lineNum = 0;

    std::vector<std::string> files = {"/etc/gdm3/custom.conf", "/etc/gdm3/daemon.conf", "/etc/gdm/custom.conf", "/etc/gdm/daemon.conf"};
    for (const auto& cfg : files)
    {
        auto file = context.GetFileContents(cfg);
        if (!file.HasValue())
            continue;

        std::istringstream fileStream(file.Value());
        std::string line;

        bool inXdmcpSection = false;
        lineNum = 0;
        while (std::getline(fileStream, line))
        {
            lineNum++;

            if (regex_search(line, xdmcpSection))
            {
                inXdmcpSection = true;
                continue;
            }
            if (inXdmcpSection)
            {
                if (line.find('[') != std::string::npos)
                {
                    inXdmcpSection = false;
                    continue;
                }
                if (regex_search(line, xdmcpEnable))
                {
                    OsConfigLogDebug(log, "Found xdcmp in %s at line %lu line %s", cfg.c_str(), lineNum, line.c_str());
                    found = true;
                    break;
                }
            }
        }
    }
    if (found)
    {
        return indicators.NonCompliant("Found xdmcp Enabled block");
    }

    return indicators.Compliant("Did not found xdmcp Enabled block");
}

} // namespace ComplianceEngine
