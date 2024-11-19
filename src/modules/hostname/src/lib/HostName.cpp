// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "HostName.h"
#include <CommonUtils.h>
#include <Mmi.h>
#include <string>

HostName::HostName(size_t maxPayloadSizeBytes)
    : HostNameBase(maxPayloadSizeBytes)
{
}

HostName::~HostName()
{
}

int HostName::RunCommand(const char* command, bool replaceEol, std::string* textResult)
{
    char* buffer = nullptr;
    int status = ExecuteCommand(nullptr, command, replaceEol, true, 0, 0, &buffer, nullptr, HostNameLog::Get());

    if (status == MMI_OK)
    {
        if (buffer && textResult)
        {
            *textResult = buffer;
        }
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogError(HostNameLog::Get(), "Failed to run command: %d, '%s'", status, buffer);
    }

    if (buffer)
    {
        free(buffer);
    }
    return status;
}
