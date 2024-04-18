// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

#define MAX_DAEMON_COMMAND_LENGTH 256

bool IsDaemonActive(const char* daemonName, void* log)
{
    const char* isActiveTemplate = "systemctl is-active %s";
    char isActiveCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    bool status = true;

    snprintf(isActiveCommand, sizeof(isActiveCommand), isActiveTemplate, daemonName);

    if (ESRCH == ExecuteCommand(NULL, isActiveCommand, false, false, 0, 0, NULL, NULL, log))
    {
        status = false;
    }

    return status;
}

bool CheckDaemonActive(const char* daemonName, char** reason, void* log)
{
    bool result = false;
    
    if (true == (result = IsDaemonActive(daemonName, log)))
    {
        OsConfigLogInfo(log, "CheckDaemonActive: service '%s' is active", daemonName);
        OsConfigCaptureSuccessReason(reason, "Service '%s' is active", daemonName);
    }
    else
    {
        OsConfigLogError(log, "CheckDaemonActive: service '%s' is inactive", daemonName);
        OsConfigCaptureReason(reason, "Service '%s' is inactive", daemonName);
    }
    
    return result;
}

bool CheckDaemonNotActive(const char* daemonName, char** reason, void* log)
{
    bool result = false;

    if (true == (result = IsDaemonActive(daemonName, log)))
    {
        OsConfigLogError(log, "CheckDaemonActive: service '%s' is active", daemonName);
        OsConfigCaptureReason(reason, "Service '%s' is active", daemonName);
        result = false;
    }
    else
    {
        OsConfigLogInfo(log, "CheckDaemonActive: service '%s' is inactive", daemonName);
        OsConfigCaptureSuccessReason(reason, "Service '%s' is inactive", daemonName);
        result = true;
    }

    return result;
}

bool EnableAndStartDaemon(const char* daemonName, void* log)
{
    const char* enableTemplate = "systemctl enable %s";
    const char* startTemplate = "systemctl start %s";
    char enableCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    char startCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    bool status = true;

    if (false == IsDaemonActive(daemonName, log))
    {
        snprintf(enableCommand, sizeof(enableCommand), enableTemplate, daemonName);
        snprintf(startCommand, sizeof(startCommand), startTemplate, daemonName);

        OsConfigLogInfo(log, "Starting service '%s'", daemonName);

        status = ((0 == ExecuteCommand(NULL, enableCommand, false, false, 0, 0, NULL, NULL, log)) &&
            (0 == ExecuteCommand(NULL, startCommand, false, false, 0, 0, NULL, NULL, log)));
    }

    return status;
}

void StopAndDisableDaemon(const char* daemonName, void* log)
{
    const char* stopTemplate = "sudo systemctl stop %s";
    const char* disableTemplate = "sudo systemctl disable %s";
    char stopCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    char disableCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};

    snprintf(stopCommand, sizeof(stopCommand), stopTemplate, daemonName);
    snprintf(disableCommand, sizeof(disableCommand), disableTemplate, daemonName);

    ExecuteCommand(NULL, stopCommand, false, false, 0, 0, NULL, NULL, log);
    ExecuteCommand(NULL, disableCommand, false, false, 0, 0, NULL, NULL, log);
}

bool RestartDaemon(const char* daemonName, void* log)
{
    const char* restartTemplate = "systemctl restart %s";
    char restartCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    bool status = true;

    if (true == IsDaemonActive(daemonName, log))
    {
        snprintf(restartCommand, sizeof(restartCommand), restartTemplate, daemonName);

        OsConfigLogInfo(log, "Restarting service '%s'", daemonName);

        status = (0 == ExecuteCommand(NULL, restartCommand, false, false, 0, 0, NULL, NULL, log));
    }

    return status;
}