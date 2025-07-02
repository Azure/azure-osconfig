// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../Evaluator.h"

#include <CommonUtils.h>
#include <iostream>
#include <string>

namespace ComplianceEngine
{

AUDIT_FN(EnsureApparmorProfiles, "enforce:Set for enforce (L2) mode, complain (L1) mode by default")
{
    std::string apparmorStatusCommand = "apparmor_status";

    bool enforce = (args.find("enforce") != args.end());

    Result<std::string> commandOutput = context.ExecuteCommand(apparmorStatusCommand);
    if (!commandOutput.HasValue())
    {
        return indicators.NonCompliant("Failed to execute apparmor_status: " + commandOutput.Error().message);
    }

    int profilesEnforce = 0;
    int profilesComplain = 0;
    int profilesLoaded = 0;
    int processesProfileUndefined = 0;
    int profilesModeTotal = 0;
    std::istringstream iss(commandOutput.Value());
    std::string line;
    while (std::getline(iss, line))
    {
        if (line.find("profiles are in enforce mode") != std::string::npos)
        {
            profilesEnforce = std::stoi(line.substr(0, line.find(' ')));
        }
        else if (line.find("profiles are in complain mode") != std::string::npos)
        {
            profilesComplain = std::stoi(line.substr(0, line.find(' ')));
        }
        else if (line.find("profiles are loaded") != std::string::npos)
        {
            profilesLoaded = std::stoi(line.substr(0, line.find(' ')));
        }
        else if (line.find("processes are unconfined but have a profile defined") != std::string::npos)
        {
            processesProfileUndefined = std::stoi(line.substr(0, line.find(' ')));
        }
    }

    profilesModeTotal = profilesEnforce + profilesComplain;

    if (0 == profilesLoaded)
    {
        return indicators.NonCompliant("No AppArmor profiles are loaded");
    }

    if (processesProfileUndefined > 0)
    {
        return indicators.NonCompliant("There are " + std::to_string(processesProfileUndefined) + " unconfined processes with a profile defined");
    }

    if (enforce)
    {
        if (profilesEnforce != profilesLoaded)
        {
            return indicators.NonCompliant("Not all loaded AppArmor profiles are in enforcing mode");
        }
    }
    else
    {
        if (profilesModeTotal != profilesLoaded)
        {
            return indicators.NonCompliant("Not all loaded AppArmor profiles are in complain or enforcing mode");
        }
    }
    return indicators.Compliant("AppArmor status command executed successfully");
}

} // namespace ComplianceEngine
