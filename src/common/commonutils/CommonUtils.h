// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COMMONUTILS_H
#define COMMONUTILS_H

#include <Logging.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define UNUSED(a) (void)(a)

#define FREE_MEMORY(a) {\
    if (NULL != a) {\
        free(a);\
        a = NULL;\
    }\
}\

// Linefeed (LF) ASCII character
#ifndef EOL
#define EOL 10
#endif

// Tab ASCII character
#ifndef TAB
#define TAB 9
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// FileUtils
char* LoadStringFromFile(const char* fileName, bool stopAtEol, OsConfigLogHandle log);
bool AppendPayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log);
int RestrictFileAccessToCurrentAccountOnly(const char* fileName);
bool FileExists(const char* fileName);
bool DirectoryExists(const char* directoryName);
bool LockFile(FILE* file, OsConfigLogHandle log);
bool UnlockFile(FILE* file, OsConfigLogHandle log);
int CheckFileAccess(const char* fileName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, char** reason, OsConfigLogHandle log);
int SetFileAccess(const char* fileName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, OsConfigLogHandle log);
unsigned int GetNumberOfLinesInFile(const char* fileName);

// SocketUtils
int ReadHttpStatusFromSocket(int socketHandle, OsConfigLogHandle log);
int ReadHttpContentLengthFromSocket(int socketHandle, OsConfigLogHandle log);

// DaemonUtils / PackageUtils (used by mc adapter)
bool IsDaemonActive(const char* daemonName, OsConfigLogHandle log);
bool EnableAndStartDaemon(const char* daemonName, OsConfigLogHandle log);
bool RestartDaemon(const char* daemonName, OsConfigLogHandle log);
int IsPresent(const char* what, OsConfigLogHandle log);

// StringUtils
char* GetOsPrettyName(OsConfigLogHandle log);
char* DuplicateString(const char* source);
char* ConcatenateStrings(const char* first, const char* second);
char* FormatAllocateString(const char* format, ...);
void TruncateAtFirst(char* target, char marker);

// CommandUtils
typedef int(*CommandCallback)(void* context);
int ExecuteCommand(void* context, const char* command, bool replaceEol, bool forJson, unsigned int maxTextResultBytes, unsigned int timeoutSeconds, char** textResult, CommandCallback callback, OsConfigLogHandle log);

#ifdef TEST_CODE
void AddMockCommand(const char* expectedCommand, bool matchPrefix, const char* output, int returnCode);
void CleanupMockCommands();
#endif

// ConfigUtils
LoggingLevel GetLoggingLevelFromJsonConfig(const char* jsonString, OsConfigLogHandle log);
int GetMaxLogSizeFromJsonConfig(const char* jsonString, OsConfigLogHandle log);
int GetMaxLogSizeDebugMultiplierFromJsonConfig(const char* jsonString, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // COMMONUTILS_H
