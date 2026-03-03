// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <StringTools.h>
#include <algorithm>
#include <sstream>

namespace ComplianceEngine
{

Result<Status> AuditEnsureFirewalldActiveZoneTargets(IndicatorsTree& indicators, ContextInterface& context)
{
    // Step 1: Check if firewalld.service is active
    auto activeResult = context.ExecuteCommand("systemctl is-active firewalld.service");
    if (!activeResult.HasValue())
    {
        return indicators.NonCompliant("firewalld.service is not active on the system. ");
    }

    std::string activeOutput = TrimWhiteSpaces(activeResult.Value());
    if (activeOutput.find("active") != 0)
    {
        return indicators.NonCompliant("firewalld.service is not active on the system. ");
    }

    // Step 2: Get active zones
    auto zonesResult = context.ExecuteCommand("firewall-cmd --get-active-zones");
    if (!zonesResult.HasValue())
    {
        return indicators.NonCompliant("Failed to execute firewall-cmd --get-active-zones: " + zonesResult.Error().message);
    }

    // Parse zone names: non-indented lines contain zone names as the first word
    std::vector<std::string> zones;
    {
        std::istringstream stream(zonesResult.Value());
        std::string line;
        while (std::getline(stream, line))
        {
            if (line.empty() || line[0] == ' ' || line[0] == '\t')
            {
                continue;
            }

            std::istringstream iss(line);
            std::string zone;
            if (iss >> zone)
            {
                zones.push_back(zone);
            }
        }
    }

    if (zones.empty())
    {
        return indicators.NonCompliant("No active firewalld zones found");
    }

    for (const auto& zone : zones)
    {
        // Get interfaces for this zone
        auto ifResult = context.ExecuteCommand("firewall-cmd --zone=" + zone + " --list-interfaces");
        if (!ifResult.HasValue())
        {
            return indicators.NonCompliant("Failed to get interfaces for zone \"" + zone + "\": " + ifResult.Error().message);
        }

        std::string interfaces = TrimWhiteSpaces(ifResult.Value());

        // Skip zones with only loopback or virtual bridge interfaces
        {
            bool skipZone = !interfaces.empty();
            std::istringstream iss(interfaces);
            std::string iface;
            while (iss >> iface)
            {
                if (iface != "lo" && iface.find("virbr") != 0)
                {
                    skipZone = false;
                    break;
                }
            }
            if (skipZone)
            {
                continue;
            }
        }

        // Get the permanent target
        auto ptargetResult = context.ExecuteCommand("firewall-cmd --permanent --zone=" + zone + " --get-target");
        if (!ptargetResult.HasValue())
        {
            return indicators.NonCompliant("Failed to get permanent target for zone \"" + zone + "\": " + ptargetResult.Error().message);
        }
        std::string permanentTarget = TrimWhiteSpaces(ptargetResult.Value());

        // Get the runtime target from --list-all output by parsing the "target:" line
        auto listAllResult = context.ExecuteCommand("firewall-cmd --list-all --zone=" + zone);
        if (!listAllResult.HasValue())
        {
            return indicators.NonCompliant("Failed to execute firewall-cmd --list-all for zone \"" + zone + "\": " + listAllResult.Error().message);
        }

        std::string target;
        {
            std::istringstream stream(listAllResult.Value());
            std::string line;
            while (std::getline(stream, line))
            {
                std::string trimmed = TrimWhiteSpaces(line);
                static const std::string prefix = "target:";
                if (trimmed.find(prefix) == 0)
                {
                    target = TrimWhiteSpaces(trimmed.substr(prefix.size()));
                    break;
                }
            }
        }

        // Check conditions matching the SCE script logic
        std::string targetLower = target;
        std::transform(targetLower.begin(), targetLower.end(), targetLower.begin(), [](unsigned char c) { return std::tolower(c); });

        if (target.empty() || targetLower == "accept")
        {
            return indicators.NonCompliant("Active zone: \"" + zone + "\" Target is: \"" + target + "\" for interfaces: \"" + interfaces + "\"");
        }

        std::string permanentTargetLower = permanentTarget;
        std::transform(permanentTargetLower.begin(), permanentTargetLower.end(), permanentTargetLower.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (targetLower != permanentTargetLower)
        {
            return indicators.NonCompliant("Active zone: \"" + zone + "\" Target is: \"" + target + "\" is not a permanent firewall rule");
        }

        indicators.Compliant("Active zone: \"" + zone + "\" Target is: \"" + target + "\" for interfaces: \"" + interfaces + "\"");
    }

    return indicators.Compliant("All active firewalld zones have valid targets");
}

} // namespace ComplianceEngine
