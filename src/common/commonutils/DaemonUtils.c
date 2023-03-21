// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

#define MAX_DAEMON_COMMAND_LENGTH 256

bool IsDaemonActive(const char* name, void* log)
{
    const char* isActiveTemplate = "systemctl is-active %s";
    char isActiveCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    bool status = true;

    snprintf(isActiveCommand, sizeof(isActiveCommand), isActiveTemplate, name);

    if (ESRCH == ExecuteCommand(NULL, isActiveCommand, false, false, 0, 0, NULL, NULL, log))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "IsDaemonActive: '%s' appears inactive", name);
        }
        status = false;
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "IsDaemonActive: '%s' appears active", name);
    }

    return status;
}

bool EnableAndStartDaemon(const char* name, void* log)
{
    const char* enableTemplate = "systemctl enable %s";
    const char* startTemplate = "systemctl start %s";
    char enableCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    char startCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    bool status = true;

    if (false == IsDaemonActive(name, log))
    {
        snprintf(enableCommand, sizeof(enableCommand), enableTemplate, name);
        snprintf(startCommand, sizeof(startCommand), startTemplate, name);

        OsConfigLogInfo(log, "Starting %s", name);

        status = ((0 == ExecuteCommand(NULL, enableCommand, false, false, 0, 0, NULL, NULL, log)) &&
            (0 == ExecuteCommand(NULL, startCommand, false, false, 0, 0, NULL, NULL, log)));
    }

    return status;
}

void StopAndDisableDaemon(const char* name, void* log)
{
    const char* stopTemplate = "sudo systemctl stop %s";
    const char* disableTemplate = "sudo systemctl disable %s";
    char stopCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    char disableCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};

    snprintf(stopCommand, sizeof(stopCommand), stopTemplate, name);
    snprintf(disableCommand, sizeof(disableCommand), disableTemplate, name);

    ExecuteCommand(NULL, stopCommand, false, false, 0, 0, NULL, NULL, log);
    ExecuteCommand(NULL, disableCommand, false, false, 0, 0, NULL, NULL, log);
}

bool RestartDaemon(const char* name, void* log)
{
    const char* restartTemplate = "systemctl restart %s";
    char restartCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    bool status = true;

    if (true == IsDaemonActive(name, log))
    {
        snprintf(restartCommand, sizeof(restartCommand), restartTemplate, name);

        OsConfigLogInfo(log, "Restarting %s", name);

        status = (0 == ExecuteCommand(NULL, restartCommand, false, false, 0, 0, NULL, NULL, log));
    }

    return status;
}