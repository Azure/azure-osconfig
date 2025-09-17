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

static int ExecuteSystemctlCommand(const char* command, const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(telemetry);
    const char* commandTemplate = "systemctl %s %s";
    char* formattedCommand = NULL;
    int result = 0;

    if ((NULL == command) || (NULL == daemonName))
    {
        OSConfigTelemetryStatusTrace(telemetry, "command", EINVAL);
        OsConfigLogError(log, "ExecuteSystemctlCommand: invalid arguments");
        return EINVAL;
    }
    else if (false == IsValidDaemonName(daemonName))
    {
        OSConfigTelemetryStatusTrace(telemetry, "IsValidDaemonName", EINVAL);
        OsConfigLogError(log, "ExecuteSystemctlCommand: invalid daemon name '%s'", daemonName);
        return EINVAL;
    }
    else if (NULL == (formattedCommand = FormatAllocateString(commandTemplate, command, daemonName)))
    {
        OSConfigTelemetryStatusTrace(telemetry, "FormatAllocateString", ENOMEM);
        OsConfigLogError(log, "ExecuteSystemctlCommand: out of memory");
        return ENOMEM;
    }

    result = ExecuteCommand(NULL, formattedCommand, false, false, 0, 0, NULL, NULL, log, telemetry);
    FREE_MEMORY(formattedCommand);
    return result;
}

bool IsDaemonActive(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    return (IsValidDaemonName(daemonName) && (0 == ExecuteSystemctlCommand("is-active", daemonName, log, telemetry))) ? true : false;
}

bool CheckDaemonActive(const char* daemonName, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    bool result = false;

    if (true == (result = IsDaemonActive(daemonName, log, telemetry)))
    {
        OsConfigLogInfo(log, "CheckDaemonActive: service '%s' is active", daemonName);
        OsConfigCaptureSuccessReason(reason, "Service '%s' is active", daemonName);
    }
    else
    {
        OsConfigLogInfo(log, "CheckDaemonActive: service '%s' is inactive", daemonName);
        OsConfigCaptureReason(reason, "Service '%s' is inactive", daemonName);
    }

    return result;
}

bool CheckDaemonNotActive(const char* daemonName, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    bool result = false;

    if (true == IsDaemonActive(daemonName, log, telemetry))
    {
        OsConfigLogInfo(log, "CheckDaemonNotActive: service '%s' is active", daemonName);
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

static bool CommandDaemon(const char* command, const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    int result = 0;
    bool status = true;

    if (false == IsValidDaemonName(daemonName))
    {
        OSConfigTelemetryStatusTrace(telemetry, "IsValidDaemonName", EINVAL);
        OsConfigLogError(log, "CommandDaemon: invalid daemon name '%s'", daemonName);
        return false;
    }

    if (0 == (result = ExecuteSystemctlCommand(command, daemonName, log, telemetry)))
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

bool EnableDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    return CommandDaemon("enable", daemonName, log, telemetry);
}

bool StartDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    return CommandDaemon("start", daemonName, log, telemetry);
}

bool EnableAndStartDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    bool status = false;

    if (false == IsValidDaemonName(daemonName))
    {
        OSConfigTelemetryStatusTrace(telemetry, "IsValidDaemonName", EINVAL);
        OsConfigLogError(log, "EnableAndStartDaemon: invalid daemon name '%s'", daemonName);
        return false;
    }

    if (false == EnableDaemon(daemonName, log, telemetry))
    {
        OSConfigTelemetryStatusTrace(telemetry, "EnableDaemon", EINVAL);
        OsConfigLogError(log, "EnableAndStartDaemon: failed to enable service '%s'", daemonName);
    }
    else
    {
        if (false == IsDaemonActive(daemonName, log, telemetry))
        {
            if (false == StartDaemon(daemonName, log, telemetry))
            {
                OSConfigTelemetryStatusTrace(telemetry, "StartDaemon", EINVAL);
                OsConfigLogError(log, "EnableAndStartDaemon: failed to start service '%s'", daemonName);
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

bool StopDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    return CommandDaemon("stop", daemonName, log, telemetry);
}

bool DisableDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    return CommandDaemon("disable", daemonName, log, telemetry);
}

void StopAndDisableDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    if (true == StopDaemon(daemonName, log, telemetry))
    {
        DisableDaemon(daemonName, log, telemetry);
    }
}

bool RestartDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    return CommandDaemon("restart", daemonName, log, telemetry);
}

bool MaskDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    return CommandDaemon("mask", daemonName, log, telemetry);
}
