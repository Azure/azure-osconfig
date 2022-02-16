// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <Logging.h>
#include <CommonUtils.h>

#if ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 30))
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

#define MAX_COMMAND_RESULT_FILE_NAME 100
#define COMMAND_CALLBACK_INTERVAL 5 //seconds
#define COMMAND_SIGNAL_INTERVAL 25000 //microseconds
#define DEFAULT_COMMAND_TIMEOUT 60 //seconds

static const char g_commandTextResultFileTemplate[] = "/tmp/~OSConfig.TextResult%u";
static const char g_commandSeparator[] = " > ";
static const char g_commandTerminator[] = " 2>&1";

char* LoadStringFromFile(const char* fileName, bool stopAtEol)
{
    FILE* file = NULL;
    int fileSize = 0;
    int i = 0;
    int next = 0;
    char* string = NULL;

    if ((NULL == fileName) || (-1 == access(fileName, F_OK)))
    {
        return string;
    }

    file = fopen(fileName, "r");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        string = (char*)malloc(fileSize + 1);
        if (string)
        {
            memset(&string[0], 0, fileSize + 1);
            for (i = 0; i < fileSize; i++)
            {
                next = fgetc(file);
                if ((EOF == next) || (stopAtEol && (EOL == next)))
                {
                    string[i] = 0;
                    break;
                }

                string[i] = (char)next;
            }
        }

        fclose(file);
    }

    return string;
}

bool SavePayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes)
{
    FILE* file = NULL;
    int i = 0;
    bool result = false;

    if (fileName && payload && (0 < payloadSizeBytes))
    {
        file = fopen(fileName, "w");
        if (file)
        {
            result = true;
            for (i = 0; i < payloadSizeBytes; i++)
            {
                if (payload[i] != fputc(payload[i], file))
                {
                    result = false;
                }
            }
            fclose(file);
        }
    }

    return result;
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
    pid_t intermediateProcess = -1;
    pid_t workerProcess = -1;
    pid_t timerProcess = -1;
    pid_t childProcess = -1;
    int status = -1;
    int intermediateStatus = -1;
    int totalWaitSeconds = 0;
    int timeout = (timeoutSeconds > 0) ? timeoutSeconds : DEFAULT_COMMAND_TIMEOUT;
    const int callbackIntervalSeconds = COMMAND_CALLBACK_INTERVAL;
    const int signalIntervalMicroSeconds = COMMAND_SIGNAL_INTERVAL;

    bool mainProcessThread = (bool)(getpid() == gettid());

    fflush(NULL);

    if ((timeoutSeconds > 0) || (NULL != callback))
    {
        if (IsFullLoggingEnabled())
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
                if (IsFullLoggingEnabled())
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
                if (IsFullLoggingEnabled())
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
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(log, "Command execution complete with status %d", status);
                }
                KillProcess(timerProcess);
            }
            else
            {
                // Timer process is done, kill the timed out worker process
                if (IsFullLoggingEnabled())
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
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(log, "Failed forking intermediate process");
            }
        }
    }
    else
    {
        if (IsFullLoggingEnabled())
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
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(log, "Failed forking process to execute command, attempting system");
            }
            status = system(command);
        }
    }

    status = NormalizeStatus(status);
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "SystemCommand: command '%s' completed with %d", command, status);
    }
    return status;
}

int ExecuteCommand(void* context, const char* command, bool replaceEol, bool forJson, unsigned int maxTextResultBytes, unsigned int timeoutSeconds, char** textResult, CommandCallback callback, void* log)
{
    int status = -1;
    FILE* resultsFile = NULL;
    int fileSize = 0;
    int next = 0;
    int i = 0;
    char* commandLine = NULL;
    size_t commandLineLength = 0;
    size_t maximumCommandLine = 0;
    char commandTextResultFile[MAX_COMMAND_RESULT_FILE_NAME] = {0};
    bool redirectedOutputCommand = false;

    if ((NULL == command) || (0 == system(NULL)))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "Cannot run command '%s'", command);
        }
        return -1;
    }

    redirectedOutputCommand = (bool)(strchr(command, '>'));

    commandLineLength = strlen(command) + 1;
    if (!redirectedOutputCommand)
    {
        // Append a random number to the results file to prevent parallel commands overwriting each other results
        snprintf(commandTextResultFile, sizeof(commandTextResultFile), g_commandTextResultFileTemplate, rand());

        commandLineLength += strlen(g_commandSeparator) + strlen(commandTextResultFile) + strlen(g_commandTerminator) ;
    }

    maximumCommandLine = (size_t)sysconf(_SC_ARG_MAX);
    if (commandLineLength > maximumCommandLine)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "Cannot run command '%s', command too long (%u), ARG_MAX: %u", command, (unsigned)commandLineLength, (unsigned)maximumCommandLine);
        }
        return E2BIG;
    }

    commandLine = (char*)malloc(commandLineLength);
    if (NULL == commandLine)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "Cannot run command '%s', cannot allocate %u bytes for command, out of memory", command, (unsigned)commandLineLength);
        }
        return ENOMEM;
    }

    // If the command includes a redirector ('>') skip redirecting for text results (as there won't be any)
    if (redirectedOutputCommand)
    {
        snprintf(commandLine, commandLineLength, "%s", command);
    }
    else
    {
        snprintf(commandLine, commandLineLength, "%s%s%s%s", command, g_commandSeparator, commandTextResultFile, g_commandTerminator);
    }

    // Execute the command with the requested timeout: error ETIME (62) means the command timed out
    status = SystemCommand(context, commandLine, timeoutSeconds, callback, log);

    free(commandLine);

    // Read the text result from the output of the command, if any, whether command succeeded or failed
    if ((NULL != textResult) && (!redirectedOutputCommand))
    {
        resultsFile = fopen(commandTextResultFile, "r");
        if (resultsFile)
        {
            fseek(resultsFile, 0, SEEK_END);
            fileSize = ftell(resultsFile);
            fseek(resultsFile, 0, SEEK_SET);

            if (fileSize > 0)
            {
                // Truncate to desired maximum, if any
                if ((maxTextResultBytes > 0) && ((fileSize + 1) > maxTextResultBytes))
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

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Context: '%p'", context);
        OsConfigLogInfo(log, "Command: '%s'", command);
        OsConfigLogInfo(log, "Status: %d (errno: %d)", status, errno);
        OsConfigLogInfo(log, "Text result: '%s'", (NULL != textResult) ? (*textResult) : "");
    }

    return status;
}

int RestrictFileAccessToCurrentAccountOnly(const char* fileName)
{
    // S_ISUID (0x04000): Set user ID on execution
    // S_ISGID (0x02000): Set group ID on execution
    // S_IRUSR (0x00400): Read permission, owner
    // S_IWUSR (0x00200): Write permission, owner
    // S_IRGRP (0x00040): Read permission, group
    // S_IWGRP (0x00020): Write permission, group.
    // S_IXUSR (0x00100): Execute/search permission, owner
    // S_IXGRP (0x00010): Execute/search permission, group

    return chmod(fileName, S_ISUID | S_ISGID | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IXUSR | S_IXGRP);
}

bool FileExists(const char* name)
{
    return ((NULL != name) && (-1 != access(name, F_OK))) ? true : false;
}

static void RemoveProxyStringEscaping(char* value)
{
    int i = 0;
    int j = 0;

    if (NULL == value)
    {
        return;
    }

    int length = strlen(value);

    for (i = 0; i < length - 1; i++)
    {
        if (('\\' == value[i]) && ('@' == value[i + 1]))
        {
            for (j = i; j < length - 1; j++)
            {
                value[j] = value[j + 1];
            }
            length -= 1;
            value[length] = 0;
        }
    }
}

bool ParseHttpProxyData(const char* proxyData, char** proxyHostAddress, int* proxyPort, char** proxyUsername, char** proxyPassword, void* log)
{
    // We accept the proxy data string to be in one of two following formats:
    //
    // "http://server:port"
    // "http://username:password@server:port"
    //
    // ..where the prefix must be either lowercase "http" or uppercase "HTTP"
    // ..and username and password can contain '@' characters escaped as "\\@"
    //
    // For example:
    //
    // "http://username\\@mail.foo:p\\@ssw\\@rd@server:port" where username is "username@mail.foo" and password is "p@ssw@rd"

    const char httpPrefix[] = "http://";
    const char httpUppercasePrefix[] = "HTTP://";

    int proxyDataLength = 0;
    int prefixLength = 0;
    bool isBadAlphaNum = false;
    int credentialsSeparatorCounter = 0;
    int columnCounter = 0;

    char* credentialsSeparator = NULL;
    char* firstColumn = NULL;
    char* lastColumn = NULL;

    char* afterPrefix = NULL;
    char* hostAddress = NULL;
    char* port = NULL;
    char* username = NULL;
    char* password = NULL;

    int hostAddressLength = 0;
    int portLength = 0;
    int portNumber = 0;
    int usernameLength = 0;
    int passwordLength = 0;

    int i = 0;

    bool result = false;

    if ((NULL == proxyData) || (NULL == proxyHostAddress) || (NULL == proxyPort))
    {
        OsConfigLogError(log, "ParseHttpProxyData called with invalid arguments");
        return result;
    }

    // Initialize output arguments

    *proxyHostAddress = NULL;
    *proxyPort = 0;

    if (proxyUsername)
    {
        *proxyUsername = NULL;
    }

    if (proxyPassword)
    {
        *proxyPassword = NULL;
    }

    // Check for required prefix and invalid characters and if any found then immediately fail

    proxyDataLength = strlen(proxyData);
    prefixLength = strlen(httpPrefix);
    if (proxyDataLength <= prefixLength)
    {
        OsConfigLogError(log, "Unsupported proxy data (%s), too short", proxyData);
        return NULL;
    }

    if (strncmp(proxyData, httpPrefix, prefixLength) && strncmp(proxyData, httpUppercasePrefix, strlen(httpUppercasePrefix)))
    {
        OsConfigLogError(log, "Unsupported proxy data (%s), no %s prefix", proxyData, httpPrefix);
        return NULL;
    }
    
    for (i = 0; i < proxyDataLength; i++)
    {
        if (('.' == proxyData[i]) || ('/' == proxyData[i]) || ('\\' == proxyData[i]) || ('_' == proxyData[i]) || 
            ('-' == proxyData[i]) || ('$' == proxyData[i]) || ('!' == proxyData[i]) || (isalnum(proxyData[i])))
        {
            continue;
        }
        else if ('@' == proxyData[i])
        {
            // not valid as first character
            if (0 == i)
            {
                OsConfigLogError(log, "Unsupported proxy data (%s), invalid '@' prefix", proxyData);
                isBadAlphaNum = true;
                break;
            }
            // '\@' can be used to insert '@' characters in username or password
            else if ((i > 0) && ('\\' != proxyData[i - 1]))
            {
                if (NULL == credentialsSeparator)
                {
                    credentialsSeparator = (char*)&(proxyData[i]);
                }
                credentialsSeparatorCounter += 1;
                if (credentialsSeparatorCounter > 1)
                {
                    OsConfigLogError(log, "Unsupported proxy data (%s), too many '@' characters", proxyData);
                    isBadAlphaNum = true;
                    break;
                }
            }
        }
        else if (':' == proxyData[i])
        {
            columnCounter += 1;
            if (columnCounter > 3)
            {
                OsConfigLogError(log, "Unsupported proxy data (%s), too many ':' characters", proxyData);
                isBadAlphaNum = true;
                break;
            }
        }
        else
        {
            OsConfigLogError(log, "Unsupported proxy data (%s), unsupported character '%c' at position %d", proxyData, proxyData[i], i);
            isBadAlphaNum = true;
            break;
        }
    }

    if ((0 == columnCounter) && (false == isBadAlphaNum))
    {
        OsConfigLogError(log, "Unsupported proxy data (%s), missing ':'", proxyData);
        isBadAlphaNum = true;
    }

    if (isBadAlphaNum)
    {
        return NULL;
    }

    afterPrefix = (char*)(proxyData + prefixLength);
    firstColumn = strchr(afterPrefix, ':');
    lastColumn = strrchr(afterPrefix, ':');
    
    // If the '@' credentials separator is not already found, try the first one if any
    if (NULL == credentialsSeparator)
    {
        credentialsSeparator = strchr(afterPrefix, '@');
    }

    // If found, bump over the first character that is the separator itself

    if (firstColumn && (strlen(firstColumn) > 0))
    {
        firstColumn += 1;
    }

    if (lastColumn && (strlen(lastColumn) > 0))
    {
        lastColumn += 1;
    }

    if (credentialsSeparator && (strlen(credentialsSeparator) > 0))
    {
        credentialsSeparator += 1;
    }

    if ((proxyData >= firstColumn) ||
        (afterPrefix >= firstColumn) ||
        (firstColumn > lastColumn) ||
        (credentialsSeparator && (firstColumn >= credentialsSeparator)) ||
        (credentialsSeparator && (credentialsSeparator >= lastColumn)) ||
        (credentialsSeparator && (firstColumn == lastColumn)) ||
        (credentialsSeparator && (0 == strlen(credentialsSeparator))) ||
        ((credentialsSeparator ? strlen("A:A@A:A") : strlen("A:A")) > strlen(afterPrefix)) ||
        (1 > strlen(lastColumn)) ||
        (1 > strlen(firstColumn)))
    {
        OsConfigLogError(log, "Unsupported proxy data (%s) format", afterPrefix);
    }
    else
    {
        {
            if (credentialsSeparator)
            {
                // username:password@server:port
                usernameLength = (int)(firstColumn - afterPrefix - 1);
                if (usernameLength > 0)
                {
                    if (NULL != (username = (char*)malloc(usernameLength + 1)))
                    {
                        memcpy(username, afterPrefix, usernameLength);
                        username[usernameLength] = 0;

                        RemoveProxyStringEscaping(username);
                        usernameLength = strlen(username);
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.username: %d", errno);
                    }
                }

                passwordLength = (int)(credentialsSeparator - firstColumn - 1);
                if (passwordLength > 0)
                {
                    if (NULL != (password = (char*)malloc(passwordLength + 1)))
                    {
                        memcpy(password, firstColumn, passwordLength);
                        password[passwordLength] = 0;

                        RemoveProxyStringEscaping(password);
                        passwordLength = strlen(password);
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.password: %d", errno);
                    }
                }

                hostAddressLength = (int)(prefixLength + lastColumn - credentialsSeparator - 1);
                if (hostAddressLength > 0)
                {
                    if (NULL != (hostAddress = (char*)malloc(hostAddressLength + 1)))
                    {
                        memcpy(hostAddress, httpPrefix, prefixLength);
                        memcpy(hostAddress + prefixLength, credentialsSeparator, hostAddressLength - prefixLength);
                        hostAddress[hostAddressLength] = 0;
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.host_address: %d", errno);
                    }
                }

                portLength = (int)strlen(lastColumn);
                if (portLength > 0)
                {
                    if (NULL != (port = (char*)malloc(portLength + 1)))
                    {
                        memcpy(port, lastColumn, portLength);
                        port[portLength] = 0;
                        portNumber = strtol(port, NULL, 10);
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.port string copy: %d", errno);
                    }
                }
            }
            else
            {
                // server:port
                hostAddressLength = (int)(firstColumn - /*proxyData*/afterPrefix - 1);
                if (hostAddressLength > 0)
                {
                    if (NULL != (hostAddress = (char*)malloc(hostAddressLength + 1)))
                    {
                        memcpy(hostAddress, /*proxyData*/afterPrefix, hostAddressLength);
                        hostAddress[hostAddressLength] = 0;
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.host_address: %d", errno);
                    }
                }

                portLength = (int)strlen(firstColumn);
                if (portLength > 0)
                {
                    if (NULL != (port = (char*)malloc(portLength + 1)))
                    {
                        memcpy(port, firstColumn, portLength);
                        port[portLength] = 0;
                        portNumber = strtol(port, NULL, 10);
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.port string copy: %d", errno);
                    }
                }
            }

            *proxyHostAddress = hostAddress;
            *proxyPort = portNumber;
                        
            if (proxyUsername && proxyPassword)
            {
                *proxyUsername = username;
                *proxyPassword = password;
                
            }

            OsConfigLogInfo(log, "HTTP proxy host|address: %s (%d)", *proxyHostAddress, hostAddressLength);
            OsConfigLogInfo(log, "HTTP proxy port: %d", *proxyPort);
            OsConfigLogInfo(log, "HTTP proxy username: %s (%d)", *proxyUsername, usernameLength);
            OsConfigLogInfo(log, "HTTP proxy password: %s (%d)", *proxyPassword, passwordLength);

            FREE_MEMORY(port);

            result = true;
        }
    }

    return result;
}