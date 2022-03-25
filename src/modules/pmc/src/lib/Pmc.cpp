// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Pmc.h"
#include <CommonUtils.h>
#include <Mmi.h>
#include <string>

Pmc::Pmc(unsigned int maxPayloadSizeBytes)
    : PmcBase(maxPayloadSizeBytes)
{
}

int Pmc::RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds)
{
    char* buffer = nullptr;
    int status = ExecuteCommand(nullptr, command, replaceEol, true, 0, timeoutSeconds, &buffer, nullptr, PmcLog::Get());

    if (status == MMI_OK)
    {
        if (buffer && textResult)
        {
            *textResult = buffer;
        }
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogError(PmcLog::Get(), "RunCommand failed with status: %d and output '%s'", status, buffer);
    }

    if (buffer)
    {
        free(buffer);
    }
    return status;
}