// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

#include <sys/select.h>

static long MonotonicTime()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        return (long)(ts.tv_sec);
    }
    return -1;
}

#ifdef TEST_CODE
struct MockCommand
{
    const char* expectedCommand;
    bool matchPrefix;
    const char* output;
    int returnCode;
    struct MockCommand* next;
};

static struct MockCommand* g_mockCommand;

void AddMockCommand(const char* expectedCommand, bool matchPrefix, const char* output, int returnCode)
{
    struct MockCommand* mock = malloc(sizeof(struct MockCommand));
    // Handling memory error in a brutal way as it's only for tests, not for production usage.
    if (NULL == mock)
    {
        abort();
    }
    mock->expectedCommand = expectedCommand;
    mock->matchPrefix = matchPrefix;
    mock->output = output;
    mock->returnCode = returnCode;
    mock->next = g_mockCommand;
    g_mockCommand = mock;
}

void CleanupMockCommands()
{
    while (NULL != g_mockCommand)
    {
	struct MockCommand* next = g_mockCommand->next;
        free(g_mockCommand);
        g_mockCommand = next;
    }
}
#endif

#define BUFFER_SIZE 1024

int ExecuteCommand(void* context, const char* command, bool replaceEol, bool forJson, unsigned int maxTextResultBytes, unsigned int timeoutSeconds,
    char** textResult, CommandCallback callback, OsConfigLogHandle log)
{
    int workerPid = -1;
    int pipefd[2] = {0};
    long startTime = 0;

    if (NULL == command)
    {
        OsConfigLogDebug(log, "Command cannot be NULL");
        return -1;
    }
    if (strlen(command) > (size_t)sysconf(_SC_ARG_MAX))
    {
        OsConfigLogError(log, "Command '%.40s...' is too long, %lu characters (maximum %lu characters)", command, strlen(command), (size_t)sysconf(_SC_ARG_MAX));
        return E2BIG;
    }

#ifdef TEST_CODE
    // Allow mocked call for unit testing of things that execute commands.
    struct MockCommand* mock = g_mockCommand;
    while (NULL != mock) {
        size_t stringLen = strlen(mock->expectedCommand);
        if (!mock->matchPrefix && (strlen(command) > stringLen))
        {
            stringLen = strlen(command);
        }
        if (0 == strncmp(mock->expectedCommand, command, stringLen))
        {
            *textResult = DuplicateString(mock->output);
            return mock->returnCode;
        }
	mock = mock->next;
    }
#endif

    // Create a pipe, then fork. Forked process duplicates the write pipe end to stdout and stderr,
    // then execs the shell with the given command. The main process uses select() with a timeout.
    // to read from the pipe, and keep track of both command  timeout and callbacks. The read loop
    // ends when the read() returns EOF or when the command times out or the callback returns a non-zero value.
    // The read loop also replaces the EOL characters with spaces if requested, and replaces all special
    // characters with spaces if requested. The output is returned in the textResult buffer, which is
    // allocated by this function. The caller is responsible for freeing the buffer when done.

    startTime = MonotonicTime();
    if (startTime < 0)
    {
        OsConfigLogError(log, "Cannot get time for command '%s', clock_gettime() failed with %d (%s)", command, errno, strerror(errno));
        return errno;
    }

    if (0 != pipe(pipefd))
    {
        OsConfigLogError(log, "Cannot create pipe for command '%s', pipe() failed with %d (%s)", command, errno, strerror(errno));
        return errno;
    }

    workerPid = fork();
    if (workerPid < 0)
    {
        OsConfigLogError(log, "Cannot fork for command '%s', fork() failed with %d (%s)", command, errno, strerror(errno));
        close(pipefd[0]);
        close(pipefd[1]);
        return errno;
    }

    if (0 == workerPid)
    {
        // Child process
        close(pipefd[0]);
        if (STDOUT_FILENO != dup2(pipefd[1], STDOUT_FILENO))
        {
            exit(errno);
        }
        if (STDERR_FILENO != dup2(pipefd[1], STDERR_FILENO))
        {
            exit(errno);
        }
        execl("/bin/sh", "sh", "-c", command, (char*)NULL);
        // If execl() fails, exit with the error code
        exit(errno);
    }
    else
    {
        // Main process
        const int defaultCommandTimeout = 60;  // seconds
        const int callbackIntervalSeconds = 5; // seconds
        long lastCallbackTime = 0;
        int status = -1;
        int childStatus = 0;
        unsigned int outputBufferPos = 0;
        unsigned int outputBufferSize = 0;

        close(pipefd[1]);

        if ((NULL != callback) && (timeoutSeconds == 0))
        {
            timeoutSeconds = defaultCommandTimeout;
        }

        for (;;)
        {
            const struct timeval selectInterval = {0, 100 * 1000}; // 100 ms, accuracy of timeouts.
            struct timeval tv;
            int bytesRead = 0;
            int inputBufferPos = 0;
            long currentTime = 0;
            char buffer[BUFFER_SIZE] = {0};
            fd_set fdset;
            int ret = 0;
            char* tmp = NULL;

            FD_ZERO(&fdset);
            FD_SET(pipefd[0], &fdset);

            tv = selectInterval;
            ret = select(pipefd[0] + 1, &fdset, NULL, NULL, &tv);
            if (ret < 0)
            {
                if (EINTR == errno)
                {
                    continue;
                }
                OsConfigLogError(log, "Error doing select for command '%s', select() failed with %d (%s)", command, errno, strerror(errno));
                status = errno;
                break;
            }

            currentTime = MonotonicTime();
            if (currentTime < 0)
            {
                OsConfigLogError(log, "Error getting time for command '%s', clock_gettime() failed with %d (%s)", command, errno, strerror(errno));
                status = errno;
                break;
            }
            if ((timeoutSeconds > 0) && (currentTime - startTime >= timeoutSeconds))
            {
                OsConfigLogError(log, "Timeout reading from pipe for command '%s', %d seconds", command, (int)(currentTime - startTime));
                status = ETIME;
                break;
            }
            if ((NULL != callback) && (currentTime - lastCallbackTime >= callbackIntervalSeconds))
            {
                if (0 != callback(context))
                {
                    OsConfigLogError(log, "Canceled reading from pipe for command '%s'", command);
                    status = ECANCELED;
                    break;
                }
                lastCallbackTime = currentTime;
            }

            if (!FD_ISSET(pipefd[0], &fdset))
            {
                // It was a timeout, nothing to read, loop.
                continue;
            }

            bytesRead = read(pipefd[0], buffer, BUFFER_SIZE);
            if (bytesRead == 0)
            {
                // Child closed the pipe, we are done.
                status = 0;
                break;
            }
            if (bytesRead < 0)
            {
                if (EINTR == errno)
                {
                    continue;
                }
                OsConfigLogError(log, "Error reading from pipe for command '%s', read() failed with %d (%s)", command, errno, strerror(errno));
                status = errno;
                break;
            }

            if (((maxTextResultBytes > 0) && (outputBufferPos == maxTextResultBytes)) || textResult == NULL)
            {
                // We don't want any more data, loop to read the rest of the output.
                continue;
            }

            outputBufferSize = bytesRead + outputBufferPos;
            if ((maxTextResultBytes > 0) && (outputBufferSize > maxTextResultBytes - 1))
            {
                outputBufferSize = maxTextResultBytes - 1;
            }

            tmp = realloc(*textResult, outputBufferSize + 1);
            if (NULL == tmp)
            {
                OsConfigLogError(log, "Cannot allocate buffer for command '%s'", command);
                status = ENOMEM;
                FREE_MEMORY(*textResult);
                break;
            }
            *textResult = tmp;

            for (inputBufferPos = 0; (inputBufferPos < bytesRead) && (outputBufferPos < outputBufferSize); inputBufferPos++, outputBufferPos++)
            {
                // Copy the data. Following characters are replaced with spaces:
                // all special characters from 0x00 to 0x1F except 0x0A (LF) when replaceEol is false
                // plus 0x22 (") and 0x5C (\) characters that break the JSON envelope when forJson is true
                const char c = buffer[inputBufferPos];
                if ((replaceEol && (EOL == c)) || ((c < 0x20) && (EOL != c)) || (0x7F == c) || (forJson && (('"' == c) || ('\\' == c))))
                {
                    (*textResult)[outputBufferPos] = ' ';
                }
                else
                {
                    (*textResult)[outputBufferPos] = c;
                }
            }
        }

        if ((NULL != textResult) && (NULL != *textResult))
        {
            (*textResult)[outputBufferPos] = '\0';
        }

        close(pipefd[0]);
        kill(workerPid, SIGKILL);
        waitpid(workerPid, &childStatus, 0);
        if (status == 0)
        {
            // The command was successful, but we need to check the child process status.
            if (WIFEXITED(childStatus))
            {
                status = WEXITSTATUS(childStatus);
            }
            else
            {
                status = childStatus;
            }
        }

        OsConfigLogDebug(log, "Context: '%p'", context);
        OsConfigLogDebug(log, "Command: '%s'", command);
        OsConfigLogDebug(log, "Status: %d (errno: %d)", status, errno);
        OsConfigLogDebug(log, "Text result: '%s'", (NULL != textResult && NULL != *textResult) ? (*textResult) : "");

        return status;
    }
}

char* HashCommand(const char* source, OsConfigLogHandle log)
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
