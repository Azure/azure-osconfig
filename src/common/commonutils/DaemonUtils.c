// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "internal.h"

#define MAX_DAEMON_COMMAND_LENGTH 256

bool IsDaemonActive(const char* name, void* log)
{
    const char* isActiveTemplate = "systemctl is-active %s";
    char command[MAX_DAEMON_COMMAND_LENGTH] = {0};
    bool status = true;

    snprintf(command, sizeof(command), isActiveTemplate, name);

    if (ESRCH == ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log))
    {
        status = false;
    }

    return status;
}

static bool EnableAndStartDaemon(const char* name, void* log)
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

static void StopAndDisableDaemon(const char* name, void* log)
{
    const char* stopTemplate = "sudo systemctl stop %s";
    const char* disableTemplate = "sudo systemctl disable %s";
    char stopCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    char disableCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    bool status = true;

    snprintf(stopCommand, sizeof(stopCommand), stopTemplate, name);
    snprintf(disableCommand, sizeof(disableCommand), disableTemplate, name);

    ExecuteCommand(NULL, stopCommand, false, false, 0, 0, NULL, NULL, log);
    ExecuteCommand(NULL, startCommand, false, false, 0, 0, NULL, NULL, log);
}