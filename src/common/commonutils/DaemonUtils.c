// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Internal.h"

#define MAX_DAEMON_NAME_LENGTH 256
#define g_packageManagerTimeoutSeconds 1800

static bool IsValidDaemonNameCharacter(char c)
{
    return ((0 == isalnum(c)) && ('_' != c) && ('-' != c) && ('.' != c)) ? false : true;
}

static bool IsValidDaemonName(const char *name)
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

static int ExecuteSystemctlCommand(const char* command, const char* daemonName, OsConfigLogHandle log)
{
    const char* commandTemplate = "systemctl %s %s";
    char* formattedCommand = NULL;
    int result = 0;

    if ((NULL == command) || (NULL == daemonName))
    {
        OsConfigLogError(log, "ExecuteSystemctlCommand: invalid arguments");
        OSConfigTelemetryStatusTrace("command", EINVAL);
        return EINVAL;
    }
    else if (false == IsValidDaemonName(daemonName))
    {
        OsConfigLogError(log, "ExecuteSystemctlCommand: invalid daemon name '%s'", daemonName);
        OSConfigTelemetryStatusTrace("IsValidDaemonName", EINVAL);
        return EINVAL;
    }
    else if (NULL == (formattedCommand = FormatAllocateString(commandTemplate, command, daemonName)))
    {
        OsConfigLogError(log, "ExecuteSystemctlCommand: out of memory");
        OSConfigTelemetryStatusTrace("FormatAllocateString", ENOMEM);
        return ENOMEM;
    }

    result = ExecuteCommand(NULL, formattedCommand, false, false, 0, 0, NULL, NULL, log);
    FREE_MEMORY(formattedCommand);
    return result;
}

static bool CommandDaemon(const char* command, const char* daemonName, OsConfigLogHandle log)
{
    int result = 0;
    bool status = true;

    if (false == IsValidDaemonName(daemonName))
    {
        OsConfigLogError(log, "CommandDaemon: invalid daemon name '%s'", daemonName);
        OSConfigTelemetryStatusTrace("IsValidDaemonName", EINVAL);
        return false;
    }

    if (0 == (result = ExecuteSystemctlCommand(command, daemonName, log)))
    {
        OsConfigLogInfo(log, "Succeeded to %s service '%s'", command, daemonName);
    }
    else
    {
        OsConfigLogInfo(log, "Cannot %s service '%s' (%d, errno: %d)", command, daemonName, result, errno);
        status = false;
    }

    return status;
}

bool IsDaemonActive(const char* daemonName, OsConfigLogHandle log)
{
    return (IsValidDaemonName(daemonName) && (0 == ExecuteSystemctlCommand("is-active", daemonName, log))) ? true : false;
}

static bool EnableDaemon(const char* daemonName, OsConfigLogHandle log)
{
    return CommandDaemon("enable", daemonName, log);
}

static bool StartDaemon(const char* daemonName, OsConfigLogHandle log)
{
    return CommandDaemon("start", daemonName, log);
}

bool EnableAndStartDaemon(const char* daemonName, OsConfigLogHandle log)
{
    bool status = false;

    if (false == IsValidDaemonName(daemonName))
    {
        OsConfigLogError(log, "EnableAndStartDaemon: invalid daemon name '%s'", daemonName);
        OSConfigTelemetryStatusTrace("IsValidDaemonName", EINVAL);
        return false;
    }

    if (false == EnableDaemon(daemonName, log))
    {
        OsConfigLogError(log, "EnableAndStartDaemon: failed to enable service '%s'", daemonName);
        OSConfigTelemetryStatusTrace("EnableDaemon", EINVAL);
    }
    else
    {
        if (false == IsDaemonActive(daemonName, log))
        {
            if (false == StartDaemon(daemonName, log))
            {
                OsConfigLogError(log, "EnableAndStartDaemon: failed to start service '%s'", daemonName);
                OSConfigTelemetryStatusTrace("StartDaemon", EINVAL);
            }
            else
            {
                status = true;
            }
        }
        else
        {
            OsConfigLogInfo(log, "Service '%s' is already running", daemonName);
            status = true;
        }
    }

    return status;
}

bool RestartDaemon(const char* daemonName, OsConfigLogHandle log)
{
    return CommandDaemon("restart", daemonName, log);
}

int IsPresent(const char* what, OsConfigLogHandle log)
{
    const char* commandTemplate = "command -v %s";
    char* command = NULL;
    int status = ENOENT;

    if (NULL == what)
    {
        OsConfigLogError(log, "IsPresent called with invalid argument");
        OSConfigTelemetryStatusTrace("what", EINVAL);
        return EINVAL;
    }

    if (NULL != (command = FormatAllocateString(commandTemplate, what)))
    {
        if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, g_packageManagerTimeoutSeconds, NULL, NULL, log)))
        {
            OsConfigLogInfo(log, "'%s' is locally present", what);
        }
    }
    else
    {
        OsConfigLogError(log, "IsPresent: FormatAllocateString failed");
        OSConfigTelemetryStatusTrace("FormatAllocateString", ENOMEM);
        status = ENOMEM;
    }

    FREE_MEMORY(command);

    return status;
}
