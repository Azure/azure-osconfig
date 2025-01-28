// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

static bool g_commandLoggingEnabled = false;

void SetCommandLogging(bool commandLogging)
{
    g_commandLoggingEnabled = commandLogging;
}

bool IsCommandLoggingEnabled(void)
{
    return g_commandLoggingEnabled;
}

static void KillProcess(pid_t processId)
{
    fflush(NULL);
    if (processId > 0)
    {
        kill(processId, SIGKILL);
    }
}

static int NormalizeStatus(int status)
{
    int newStatus = status;
    if ((ETIME != newStatus) && (ECANCELED != newStatus))
    {
        if (WIFEXITED(newStatus))
        {
            newStatus = WEXITSTATUS(newStatus);
        }
        else
        {
            newStatus = errno ? errno : -1;
        }
    }
    return newStatus;
}

static int SystemCommand(void* context, const char* command, int timeoutSeconds, CommandCallback callback, void* log)
{
    const int callbackIntervalSeconds = 5; //seconds
    const int defaultCommandTimeout = 60; //seconds

    pid_t intermediateProcess = -1;
    pid_t workerProcess = -1;
    pid_t timerProcess = -1;
    pid_t childProcess = -1;
    int status = -1;
    int intermediateStatus = -1;
    int totalWaitSeconds = 0;
    int timeout = (timeoutSeconds > 0) ? timeoutSeconds : defaultCommandTimeout;

    bool mainProcessThread = (bool)(getpid() == gettid());

    fflush(NULL);

    if ((timeoutSeconds > 0) || (NULL != callback))
    {
        if (IsCommandLoggingEnabled())
        {
            OsConfigLogInfo(log, "SystemCommand: executing command '%s' with timeout of %d seconds and%scancelation on %s thread",
                command, timeout, (NULL == callback) ? " no " : " ", mainProcessThread ? "main process" : "worker");
        }

        // Fork an intermediate process to act as the parent for two more forked processes:
        // one to actually execute the system command and the other to sleep and act as a timer.
        // Whichever of these two children processes finishes first, that causes the other to be killed.
        // The intermediate process exists as a parent for these (instead of the current process being
        // the parent) to avoid collision with any other children processes.

        if (0 == (intermediateProcess = fork()))
        {
            // Intermediate process

            if (0 == (workerProcess = fork()))
            {
                // Worker process
                status = execl("/bin/sh", "sh", "-c", command, (char*)NULL);
                _exit(status);
            }
            else if (workerProcess < 0)
            {
                // Intermediate process
                if (IsCommandLoggingEnabled())
                {
                    OsConfigLogError(log, "Failed forking process to execute command");
                }
                status = -1;

                // Kill the timer process if created and wait on it before exiting otherwise timer becomes a zombie process
                if (timerProcess > 0)
                {
                    KillProcess(timerProcess);
                    waitpid(timerProcess, &intermediateStatus, 0);
                }
                _exit(status);
            }

            if (0 == (timerProcess = fork()))
            {
                // Timer process
                status = ETIME;
                if (NULL == callback)
                {
                    sleep(timeout);
                }
                else
                {
                    while (totalWaitSeconds < timeout)
                    {
                        // If the callback returns non zero, cancel the command
                        if (0 != callback(context))
                        {
                            status = ECANCELED;
                            break;
                        }
                        sleep(callbackIntervalSeconds);
                        totalWaitSeconds += callbackIntervalSeconds;
                    }
                }
                _exit(status);
            }
            else if (timerProcess < 0)
            {
                // Intermediate process
                if (IsCommandLoggingEnabled())
                {
                    OsConfigLogError(log, "Failed forking timer process");
                }
                status = -1;

                // Kill the worker process if created and wait on it before exiting otherwise worker becomes a zombie process
                if (workerProcess > 0)
                {
                    KillProcess(workerProcess);
                    waitpid(workerProcess, &intermediateStatus, 0);
                }
                _exit(status);
            }

            // Wait on the child process (worker or timer) that finishes first
            childProcess = waitpid(0, &status, 0);
            status = NormalizeStatus(status);
            if (childProcess == workerProcess)
            {
                if (IsCommandLoggingEnabled())
                {
                    OsConfigLogInfo(log, "Command execution complete with status %d", status);
                }
                KillProcess(timerProcess);
            }
            else
            {
                // Timer process is done, kill the timed out worker process
                if (IsCommandLoggingEnabled())
                {
                    OsConfigLogError(log, "Command timed out or it was canceled, command process killed (%d)", status);
                }
                KillProcess(workerProcess);
            }

            // Wait on the remaining child (either worker or timer) and exit (the intermediate process)
            waitpid(0, &intermediateStatus, 0);
            _exit(status);
        }
        else if (intermediateProcess > 0)
        {
            // Wait on the intermediate process to finish
            waitpid(intermediateProcess, &status, 0);
        }
        else
        {
            status = -1;
            if (IsCommandLoggingEnabled())
            {
                OsConfigLogError(log, "Failed forking intermediate process");
            }
        }
    }
    else
    {
        if (IsCommandLoggingEnabled())
        {
            OsConfigLogInfo(log, "SystemCommand: executing command '%s' without timeout or cancelation on %s thread",
                command, mainProcessThread ? "main process" : "worker");
        }
        if (0 == (workerProcess = fork()))
        {
            // Worker process
            status = execl("/bin/sh", "sh", "-c", command, (char*)NULL);
            _exit(status);
        }
        else if (workerProcess > 0)
        {
            // Wait on the worker process to terminate
            waitpid(workerProcess, &status, 0);
        }
        else
        {
            // If our fork fails, try system(), if that also fails then the call fails
            if (IsCommandLoggingEnabled())
            {
                OsConfigLogError(log, "Failed forking process to execute command, attempting system");
            }
            status = system(command);
        }
    }

    status = NormalizeStatus(status);
    if (IsCommandLoggingEnabled())
    {
        OsConfigLogInfo(log, "SystemCommand: command '%s' completed with %d", command, status);
    }
    return status;
}

#define MAX_COMMAND_RESULT_FILE_NAME 100

int ExecuteCommand(void* context, const char* command, bool replaceEol, bool forJson, unsigned int maxTextResultBytes, unsigned int timeoutSeconds, char** textResult, CommandCallback callback, void* log)
{
    const char commandTextResultFileTemplate[] = "/tmp/~OSConfig.TextResult%u";
    const char commandSeparator[] = " > ";
    const char commandTerminator[] = " 2>&1";

    int status = -1;
    FILE* resultsFile = NULL;
    int fileSize = 0;
    int next = 0;
    int i = 0;
    char* commandLine = NULL;
    size_t commandLineLength = 0;
    size_t maximumCommandLine = 0;
    char commandTextResultFile[MAX_COMMAND_RESULT_FILE_NAME] = {0};
    bool wrappedCommand = false;

    if ((NULL == command) || (0 == system(NULL)))
    {
        if (IsCommandLoggingEnabled())
        {
            OsConfigLogError(log, "Cannot run command '%s'", command);
        }
        return -1;
    }

    commandLineLength = strlen(command);
    wrappedCommand = (('(' == command[0]) || (')' == command[commandLineLength]));

    // Append a random number to the results file to prevent parallel commands overwriting each other results
    snprintf(commandTextResultFile, sizeof(commandTextResultFile), commandTextResultFileTemplate, rand());

    commandLineLength += strlen(commandSeparator) + strlen(commandTextResultFile) + strlen(commandTerminator) + (wrappedCommand ? 0 : 2) + 1;

    maximumCommandLine = (size_t)sysconf(_SC_ARG_MAX);
    if (commandLineLength > maximumCommandLine)
    {
        if (IsCommandLoggingEnabled())
        {
            OsConfigLogError(log, "Cannot run command '%s', command too long (%u), ARG_MAX: %u", command, (unsigned)commandLineLength, (unsigned)maximumCommandLine);
        }
        return E2BIG;
    }

    commandLine = (char*)malloc(commandLineLength);
    if (NULL == commandLine)
    {
        if (IsCommandLoggingEnabled())
        {
            OsConfigLogError(log, "Cannot run command '%s', cannot allocate %u bytes for command, out of memory", command, (unsigned)commandLineLength);
        }
        return ENOMEM;
    }

    snprintf(commandLine, commandLineLength, wrappedCommand ? "%s%s%s%s" : "(%s)%s%s%s", command, commandSeparator, commandTextResultFile, commandTerminator);

    // Execute the command with the requested timeout: error ETIME (62) means the command timed out
    status = SystemCommand(context, commandLine, timeoutSeconds, callback, log);

    free(commandLine);

    // Read the text result from the output of the command, if any, whether command succeeded or failed
    if (NULL != textResult)
    {
        *textResult = NULL;
        resultsFile = fopen(commandTextResultFile, "r");
        if (resultsFile)
        {
            fseek(resultsFile, 0, SEEK_END);
            fileSize = ftell(resultsFile);
            fseek(resultsFile, 0, SEEK_SET);

            if (fileSize > 0)
            {
                // Truncate to desired maximum, if any
                if ((maxTextResultBytes > 0) && (((size_t)fileSize + 1) > maxTextResultBytes))
                {
                    fileSize = (maxTextResultBytes > 1) ? (maxTextResultBytes - 1) : 0;
                }

                *textResult = (char*)malloc(fileSize + 1);
                if (NULL != *textResult)
                {
                    memset(*textResult, 0, fileSize + 1);
                    for (i = 0; i < fileSize; i++)
                    {
                        next = fgetc(resultsFile);

                        if (EOF == next)
                        {
                            break;
                        }

                        // Copy the data. Following characters are replaced with spaces:
                        // all special characters from 0x00 to 0x1F except 0x0A (LF) when replaceEol is false
                        // plus 0x22 (") and 0x5C (\) characters that break the JSON envelope when forJson is true
                        if ((replaceEol && (EOL == next)) || ((next < 0x20) && (EOL != next)) || (0x7F == next) || (forJson && (('"' == next) || ('\\' == next))))
                        {
                            (*textResult)[i] = ' ';
                        }
                        else
                        {
                            (*textResult)[i] = (char)next;
                        }
                    }
                }
            }

            fclose(resultsFile);
        }
    }

    remove(commandTextResultFile);

    if (IsCommandLoggingEnabled())
    {
        OsConfigLogInfo(log, "Context: '%p'", context);
        OsConfigLogInfo(log, "Command: '%s'", command);
        OsConfigLogInfo(log, "Status: %d (errno: %d)", status, errno);
        OsConfigLogInfo(log, "Text result: '%s'", (NULL != textResult && NULL != *textResult) ? (*textResult) : "");
    }

    return status;
}

char* HashCommand(const char* source, void* log)
{
    static const char hashCommandTemplate[] = "%s | sha256sum | head -c 64";

    char* command = NULL;
    char* hash = NULL;
    int length = 0;
    int status = -1;

    if (NULL == source)
    {
        return NULL;
    }

    length = (int)(strlen(source) + strlen(hashCommandTemplate));
    command = (char*)malloc(length);
    if (NULL != command)
    {
        memset(command, 0, length);
        snprintf(command, length, hashCommandTemplate, source);

        status = ExecuteCommand(NULL, command, false, false, 0, 0, &hash, NULL, log);
        if (0 != status)
        {
            FREE_MEMORY(hash);
        }
    }
    else
    {
        OsConfigLogError(log, "HashCommand: out of memory");
    }

    FREE_MEMORY(command);
    return (0 == status) ? hash : NULL;
}
