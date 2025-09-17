// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMONUTILS_H
#define COMMONUTILS_H

#include <Logging.h>
#include <Telemetry.h>
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

#define DEFAULT_DEVICE_MODEL_ID 16

#define MAX_COMPONENT_NAME 256

// 30 seconds
#define DEFAULT_REPORTING_INTERVAL 30

#define PROTOCOL_AUTO 0
// Uncomment next line when the PROTOCOL_MQTT macro will be needed (compiling with -Werror-unused-macros)
//#define PROTOCOL_MQTT 1
#define PROTOCOL_MQTT_WS 2

#ifdef __cplusplus
extern "C"
{
#endif

char* LoadStringFromFile(const char* fileName, bool stopAtEol, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool SavePayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool FileEndsInEol(const char* fileName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool AppendPayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool SecureSaveToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool AppendToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool ConcatenateFiles(const char* firstFileName, const char* secondFileName, bool preserveAccess, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int RenameFile(const char* original, const char* target, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

typedef int(*CommandCallback)(void* context);

int ExecuteCommand(void* context, const char* command, bool replaceEol, bool forJson, unsigned int maxTextResultBytes, unsigned int timeoutSeconds, char** textResult, CommandCallback callback, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

#ifdef TEST_CODE
void AddMockCommand(const char* expectedCommand, bool matchPrefix, const char* output, int returnCode);
void CleanupMockCommands();
#endif

int RestrictFileAccessToCurrentAccountOnly(const char* fileName);

bool IsAFile(const char* fileName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool IsADirectory(const char* fileName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool FileExists(const char* fileName);
bool DirectoryExists(const char* directoryName);
int CheckFileExists(const char* fileName, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckFileNotFound(const char* fileName, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

bool MakeFileBackupCopy(const char* fileName, const char* backupName, bool preserveAccess, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

int CheckFileAccess(const char* fileName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetFileAccess(const char* fileName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int GetFileAccess(const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int RenameFileWithOwnerAndAccess(const char* original, const char* target, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

int CheckDirectoryAccess(const char* directoryName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetDirectoryAccess(const char* directoryName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int GetDirectoryAccess(const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

int CheckFileSystemMountingOption(const char* mountFileName, const char* mountDirectory, const char* mountType, const char* desiredOption, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetFileSystemMountingOption(const char* mountDirectory, const char* mountType, const char* desiredOption, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

int IsPresent(const char* what, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int IsPackageInstalled(const char* packageName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckPackageInstalled(const char* packageName, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckPackageNotInstalled(const char* packageName, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int InstallOrUpdatePackage(const char* packageName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int InstallPackage(const char* packageName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int UninstallPackage(const char* packageName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
void PackageUtilsCleanup(void);

unsigned int GetNumberOfLinesInFile(const char* fileName);
bool CharacterFoundInFile(const char* fileName, char what);
int CheckNoLegacyPlusEntriesInFile(const char* fileName, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int ReplaceMarkedLinesInFile(const char* fileName, const char* marker, const char* newline, char commentCharacter, bool preserveAccess, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int ReplaceMarkedLinesInFilePrepend(const char* fileName, const char* marker, const char* newline, char commentCharacter, bool preserveAccess, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int FindTextInFile(const char* fileName, const char* text, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckTextIsFoundInFile(const char* fileName, const char* text, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckTextIsNotFoundInFile(const char* fileName, const char* text, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckMarkedTextNotFoundInFile(const char* fileName, const char* text, const char* marker, char commentCharacter, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckTextNotFoundInEnvironmentVariable(const char* variableName, const char* text, bool strictComparison, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckSmallFileContainsText(const char* fileName, const char* text, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int FindTextInFolder(const char* directory, const char* text, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckTextNotFoundInFolder(const char* directory, const char* text, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckTextFoundInFolder(const char* directory, const char* text, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckLineNotFoundOrCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckLineFoundNotCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckTextFoundInCommandOutput(const char* command, const char* text, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetStringOptionFromBuffer(const char* buffer, const char* option, char separator, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int GetIntegerOptionFromBuffer(const char* buffer, const char* option, char separator, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckTextNotFoundInCommandOutput(const char* command, const char* text, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetEtcConfValue(const char* file, const char* name, const char* value, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetEtcLoginDefValue(const char* name, const char* value, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckLockoutForFailedPasswordAttempts(const char* fileName, const char* pamSo, char commentCharacter, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetLockoutForFailedPasswordAttempts(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckEnsurePasswordReuseIsLimited(int remember, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetEnsurePasswordReuseIsLimited(int remember, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int EnableVirtualMemoryRandomization(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int RemoveDotsFromPath(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

char* GetStringOptionFromFile(const char* fileName, const char* option, char separator, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int GetIntegerOptionFromFile(const char* fileName, const char* option, char separator, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckIntegerOptionFromFileEqualWithAny(const char* fileName, const char* option, char separator, int* values, int numberOfValues, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckIntegerOptionFromFileLessOrEqualWith(const char* fileName, const char* option, char separator, int value, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

char* DuplicateString(const char* source);
char* ConcatenateStrings(const char* first, const char* second);
char* DuplicateStringToLowercase(const char* source);
char* FormatAllocateString(const char* format, ...);
int ConvertStringToIntegers(const char* source, char separator, int** integers, int* numIntegers, int base, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* RemoveCharacterFromString(const char* source, char what, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* ReplaceEscapeSequencesInString(const char* source, const char* escapes, unsigned int numEscapes, char replacement, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int RemoveEscapeSequencesFromFile(const char* fileName, const char* escapes, unsigned int numEscapes, char replacement, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

int DisablePostfixNetworkListening(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

size_t HashString(const char* source);
char* HashCommand(const char* source, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

bool ParseHttpProxyData(const char* proxyData, char** hostAddress, int* port, char**username, char** password, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

char* GetOsPrettyName(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetOsName(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetOsVersion(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetOsKernelName(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetOsKernelRelease(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetOsKernelVersion(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetCpuType(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetCpuVendor(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetCpuModel(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
unsigned int GetNumberOfCpuCores(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetCpuFlags(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool CheckCpuFlagSupported(const char* cpuFlag, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
long GetTotalMemory(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
long GetFreeMemory(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetProductName(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetProductVendor(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetProductVersion(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetSystemCapabilities(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetSystemConfiguration(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool CheckOsAndKernelMatchDistro(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetLoginUmask(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckLoginUmask(const char* desired, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
long GetPassMinDays(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
long GetPassMaxDays(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
long GetPassWarnAge(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetPassMinDays(long days, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetPassMaxDays(long days, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetPassWarnAge(long days, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool IsCurrentOs(const char* name, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool IsRedHatBased(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool IsCommodore(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool IsSelinuxPresent(void);
bool DetectSelinux(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckCoreDumpsHardLimitIsDisabledForAllUsers(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

void RemovePrefix(char* target, char marker);
void RemovePrefixBlanks(char* target);
void RemovePrefixUpTo(char* target, char marker);
void RemovePrefixUpToString(char* target, const char* marker);
void RemoveTrailingBlanks(char* target);
void TruncateAtFirst(char* target, char marker);

char* UrlEncode(const char* target);
char* UrlDecode(const char* target);

bool LockFile(FILE* file, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool UnlockFile(FILE* file, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

char* ReadUriFromSocket(int socketHandle, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int ReadHttpStatusFromSocket(int socketHandle, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int ReadHttpContentLengthFromSocket(int socketHandle, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

int SleepMilliseconds(long milliseconds);

bool FreeAndReturnTrue(void* value);

bool IsValidDaemonName(const char *name);
bool IsDaemonActive(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool CheckDaemonActive(const char* daemonName, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool CheckDaemonNotActive(const char* daemonName, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool StartDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool StopDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool EnableDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool DisableDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool EnableAndStartDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
void StopAndDisableDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool RestartDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
bool MaskDaemon(const char* daemonName, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

char* GetHttpProxyData(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

char* RepairBrokenEolCharactersIfAny(const char* value);

int CheckAllWirelessInterfacesAreDisabled(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int DisableAllWirelessInterfaces(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetDefaultDenyFirewallPolicy(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

typedef struct ReportedProperty
{
    char componentName[MAX_COMPONENT_NAME];
    char propertyName[MAX_COMPONENT_NAME];
    size_t lastPayloadHash;
} ReportedProperty;

bool IsIotHubManagementEnabledInJsonConfig(const char* jsonString);
LoggingLevel GetLoggingLevelFromJsonConfig(const char* jsonString, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int GetMaxLogSizeFromJsonConfig(const char* jsonString, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int GetMaxLogSizeDebugMultiplierFromJsonConfig(const char* jsonString, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int GetReportingIntervalFromJsonConfig(const char* jsonString, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int GetModelVersionFromJsonConfig(const char* jsonString, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int GetLocalManagementFromJsonConfig(const char* jsonString, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int GetIotHubProtocolFromJsonConfig(const char* jsonString, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int LoadReportedFromJsonConfig(const char* jsonString, ReportedProperty** reportedProperties, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetLoggingLevelPersistently(LoggingLevel level, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

int GetGitManagementFromJsonConfig(const char* jsonString, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetGitRepositoryUrlFromJsonConfig(const char* jsonString, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
char* GetGitBranchFromJsonConfig(const char* jsonString, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

typedef struct PerfClock
{
    struct timespec start;
    struct timespec stop;
} PerfClock;

int StartPerfClock(PerfClock* clock, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int StopPerfClock(PerfClock* clock, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
long GetPerfClockTime(PerfClock* clock, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
void LogPerfClock(PerfClock* clock, const char* componentName, const char* objectName, int objectResult, long limit, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

#ifdef __cplusplus
}
#endif

#endif // COMMONUTILS_H
