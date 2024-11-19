// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

#define MAX_DAEMON_NAME_LENGTH 256

// Valid systemd deamon name characters for us, not universal, add more here if necessary in the future
static bool IsValidDaemonNameCharacter(char c)
{
    return ((0 == isalnum(c)) && ('_' != c) && ('-' != c) && ('.' != c)) ? false : true;
}

bool IsValidDaemonName(const char *name)
{
    size_t length = 0, i = 0;
    bool result = true;

    if ((NULL != name) && (0 < (length = strlen(name))) && (MAX_DAEMON_NAME_LENGTH > length))
    {
        for (i = 0; i < length; i++)
        {
            if (false == (result = IsValidDaemonNameCharacter(name[i])))
            {
                break;
            }
        }
    }
    else
    {
        result = false;
    }

    return result;
}

static int ExecuteSystemctlCommand(const char* command, const char* daemonName, void* log)
{
    const char* commandTemplate = "systemctl %s %s";
    char* formattedCommand = NULL;
    int result = 0;

    if ((NULL == command) || (NULL == daemonName))
    {
        OsConfigLogError(log, "ExecuteSystemctlCommand: invalid arguments");
        return EINVAL;
    }
    else if (false == IsValidDaemonName(daemonName))
    {
        OsConfigLogError(log, "ExecuteSystemctlCommand: invalid daemon name '%s'", daemonName);
        return EINVAL;
    }
    else if (NULL == (formattedCommand = FormatAllocateString(commandTemplate, command, daemonName)))
    {
        OsConfigLogError(log, "ExecuteSystemctlCommand: out of memory");
        return ENOMEM;
    }

    result = ExecuteCommand(NULL, formattedCommand, false, false, 0, 0, NULL, NULL, log);
    FREE_MEMORY(formattedCommand);
    return result;
}

bool IsDaemonActive(const char* daemonName, void* log)
{
    return (IsValidDaemonName(daemonName) && (0 == ExecuteSystemctlCommand("is-active", daemonName, log))) ? true : false;
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

static bool CommandDaemon(const char* command, const char* daemonName, void* log)
{
    int result = 0;
    bool status = true;

    if (false == IsValidDaemonName(daemonName))
    {
        OsConfigLogError(log, "CommandDaemon: invalid daemon name '%s'", daemonName);
        return false;
    }

    if (0 == (result = ExecuteSystemctlCommand(command, daemonName, log)))
    {
        OsConfigLogInfo(log, "Succeeded to %s service '%s'", command, daemonName);
    }
    else
    {
        OsConfigLogError(log, "Failed to %s service '%s' (%d)", command, daemonName, result);
        status = false;
    }

    return status;
}

bool EnableDaemon(const char* daemonName, void* log)
{
    return CommandDaemon("enable", daemonName, log);
}

bool StartDaemon(const char* daemonName, void* log)
{
    return CommandDaemon("start", daemonName, log);
}

bool EnableAndStartDaemon(const char* daemonName, void* log)
{
    bool status = true;

    if (false == IsValidDaemonName(daemonName))
    {
        OsConfigLogError(log, "EnableAndStartDaemon: invalid daemon name '%s'", daemonName);
        return false;
    }

    if (false == IsDaemonActive(daemonName, log))
    {
        if (EnableDaemon(daemonName, log) && StartDaemon(daemonName, log))
        {
            status = true;
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
    return CommandDaemon("stop", daemonName, log);
}

bool DisableDaemon(const char* daemonName, void* log)
{
    return CommandDaemon("disable", daemonName, log);
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
    return CommandDaemon("restart", daemonName, log);
}

bool MaskDaemon(const char* daemonName, void* log)
{
    return CommandDaemon("mask", daemonName, log);
}
