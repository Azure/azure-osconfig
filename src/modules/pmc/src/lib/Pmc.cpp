// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include <CommonUtils.h>
#include <Mmi.h>
#include <Pmc.h>

Pmc::Pmc(unsigned int maxPayloadSizeBytes)
    : PmcBase(maxPayloadSizeBytes)
{
}

int Pmc::RunCommand(const char* command, std::string* textResult, bool isLongRunning)
{
    char* buffer = nullptr;
    int status;

    if (isLongRunning)
    {
        status = ExecuteCommand(nullptr, command, true, true, 0, TIMEOUT_LONG_RUNNING, &buffer, nullptr, PmcLog::Get());
    }
    else
    {
        status = ExecuteCommand(nullptr, command, true, true, 0, 0, &buffer, nullptr, PmcLog::Get());
    }

    if (status == MMI_OK)
    {
        if (buffer && textResult)
        {
            *textResult = buffer;
        }
    }

    FREE_MEMORY(buffer);

    return status;
}