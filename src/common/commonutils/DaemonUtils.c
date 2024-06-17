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
        OsConfigLogError(log, "CheckDaemonNotActive: service '%s' is active", daemonName);
        OsConfigCaptureReason(reason, "Service '%s' is active", daemonName);
        result = false;
    }
    else
    {
        OsConfigLogInfo(log, "CheckDaemonNotActive: service '%s' is inactive", daemonName);
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
    int commandResult = 0;
    bool status = true;

    if (false == IsDaemonActive(daemonName, log))
    {
        snprintf(enableCommand, sizeof(enableCommand), enableTemplate, daemonName);
        snprintf(startCommand, sizeof(startCommand), startTemplate, daemonName);

        OsConfigLogInfo(log, "Starting service '%s'", daemonName);

        if (0 == (commandResult == ExecuteCommand(NULL, enableCommand, false, false, 0, 0, NULL, NULL, log)))
        {
            if (0 == (commandResult == ExecuteCommand(NULL, startCommand, false, false, 0, 0, NULL, NULL, log)))
            {
                status = true;
            }
            else
            {
                OsConfigLogError(log, "Cannot start service '%s' (%d)", daemonName, commandResult);
                status = false;
            }
        }
        else
        {
            OsConfigLogError(log, "Failed to enable service '%s' (%d)", daemonName, commandResult);
            status = false;
        }
    }
    else
    {
        OsConfigLogInfo(log, "Service '%s' is already running", daemonName);
    }

    return status;
}

bool StopDaemon(const char* daemonName, void* log)
{
    const char* stopTemplate = "sudo systemctl stop -f %s";
    char stopCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    int commandResult = 0;
    bool status = true;

    snprintf(stopCommand, sizeof(stopCommand), stopTemplate, daemonName);

    if (0 != (commandResult = ExecuteCommand(NULL, stopCommand, false, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogError(log, "Failed to stop service '%s' (%d)", daemonName, commandResult);
        status = false;
    }

    return status;
}

bool DisableDaemon(const char* daemonName, void* log)
{
    const char* disableTemplate = "sudo systemctl disable %s";
    char disableCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    int commandResult = 0;
    bool status = true;

    snprintf(disableCommand, sizeof(disableCommand), disableTemplate, daemonName);

    if (0 != (commandResult = ExecuteCommand(NULL, disableCommand, false, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogError(log, "Failed to disable service '%s' (%d)", daemonName, commandResult);
        return false;
    }

    return status;
}

void StopAndDisableDaemon(const char* daemonName, void* log)
{
    if (true == StopDaemon(daemonName, log))
    {
        DisableDaemon(daemonName, log);
    }
}

bool RestartDaemon(const char* daemonName, void* log)
{
    const char* restartTemplate = "systemctl restart %s";
    char restartCommand[MAX_DAEMON_COMMAND_LENGTH] = {0};
    int commandResult = 0;
    bool status = true;

    if (true == IsDaemonActive(daemonName, log))
    {
        snprintf(restartCommand, sizeof(restartCommand), restartTemplate, daemonName);

        OsConfigLogInfo(log, "Restarting service '%s'", daemonName);

        if (0 != (commandResult = ExecuteCommand(NULL, restartCommand, false, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogError(log, "Failed to restart service '%s' (%d)", daemonName, commandResult);
            status = false;
        }
    }

    return status;
}