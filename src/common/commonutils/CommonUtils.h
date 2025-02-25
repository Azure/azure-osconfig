// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMONUTILS_H
#define COMMONUTILS_H

#include <Logging.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

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

char* LoadStringFromFile(const char* fileName, bool stopAtEol, OSCONFIG_LOG_HANDLE log);
bool SavePayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OSCONFIG_LOG_HANDLE log);
bool FileEndsInEol(const char* fileName, OSCONFIG_LOG_HANDLE log);
bool AppendPayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OSCONFIG_LOG_HANDLE log);
bool SecureSaveToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OSCONFIG_LOG_HANDLE log);
bool AppendToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OSCONFIG_LOG_HANDLE log);
bool ConcatenateFiles(const char* firstFileName, const char* secondFileName, bool preserveAccess, OSCONFIG_LOG_HANDLE log);
int RenameFile(const char* original, const char* target, OSCONFIG_LOG_HANDLE log);

typedef int(*CommandCallback)(void* context);

// If called from the main process thread the timeoutSeconds and callback arguments are ignored
int ExecuteCommand(void* context, const char* command, bool replaceEol, bool forJson, unsigned int maxTextResultBytes, unsigned int timeoutSeconds, char** textResult, CommandCallback callback, OSCONFIG_LOG_HANDLE log);

int RestrictFileAccessToCurrentAccountOnly(const char* fileName);

bool IsAFile(const char* fileName, OSCONFIG_LOG_HANDLE log);
bool IsADirectory(const char* fileName, OSCONFIG_LOG_HANDLE log);
bool FileExists(const char* fileName);
bool DirectoryExists(const char* directoryName);
int CheckFileExists(const char* fileName, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckFileNotFound(const char* fileName, char** reason, OSCONFIG_LOG_HANDLE log);

bool MakeFileBackupCopy(const char* fileName, const char* backupName, bool preserveAccess, OSCONFIG_LOG_HANDLE log);

int CheckFileAccess(const char* fileName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, char** reason, OSCONFIG_LOG_HANDLE log);
int SetFileAccess(const char* fileName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, OSCONFIG_LOG_HANDLE log);
int GetFileAccess(const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, OSCONFIG_LOG_HANDLE log);
int RenameFileWithOwnerAndAccess(const char* original, const char* target, OSCONFIG_LOG_HANDLE log);

int CheckDirectoryAccess(const char* directoryName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, bool rootCanOverwriteOwnership, char** reason, OSCONFIG_LOG_HANDLE log);
int SetDirectoryAccess(const char* directoryName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, OSCONFIG_LOG_HANDLE log);
int GetDirectoryAccess(const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, OSCONFIG_LOG_HANDLE log);

int CheckFileSystemMountingOption(const char* mountFileName, const char* mountDirectory, const char* mountType, const char* desiredOption, char** reason, OSCONFIG_LOG_HANDLE log);
int SetFileSystemMountingOption(const char* mountDirectory, const char* mountType, const char* desiredOption, OSCONFIG_LOG_HANDLE log);

int IsPresent(const char* what, OSCONFIG_LOG_HANDLE log);
int IsPackageInstalled(const char* packageName, OSCONFIG_LOG_HANDLE log);
int CheckPackageInstalled(const char* packageName, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckPackageNotInstalled(const char* packageName, char** reason, OSCONFIG_LOG_HANDLE log);
int InstallOrUpdatePackage(const char* packageName, OSCONFIG_LOG_HANDLE log);
int InstallPackage(const char* packageName, OSCONFIG_LOG_HANDLE log);
int UninstallPackage(const char* packageName, OSCONFIG_LOG_HANDLE log);

unsigned int GetNumberOfLinesInFile(const char* fileName);
bool CharacterFoundInFile(const char* fileName, char what);
int CheckNoLegacyPlusEntriesInFile(const char* fileName, char** reason, OSCONFIG_LOG_HANDLE log);
int ReplaceMarkedLinesInFile(const char* fileName, const char* marker, const char* newline, char commentCharacter, bool preserveAccess, OSCONFIG_LOG_HANDLE log);
int FindTextInFile(const char* fileName, const char* text, OSCONFIG_LOG_HANDLE log);
int CheckTextIsFoundInFile(const char* fileName, const char* text, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckTextIsNotFoundInFile(const char* fileName, const char* text, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckMarkedTextNotFoundInFile(const char* fileName, const char* text, const char* marker, char commentCharacter, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckTextNotFoundInEnvironmentVariable(const char* variableName, const char* text, bool strictComparison, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckSmallFileContainsText(const char* fileName, const char* text, char** reason, OSCONFIG_LOG_HANDLE log);
int FindTextInFolder(const char* directory, const char* text, OSCONFIG_LOG_HANDLE log);
int CheckTextNotFoundInFolder(const char* directory, const char* text, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckTextFoundInFolder(const char* directory, const char* text, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckLineNotFoundOrCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckLineFoundNotCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckTextFoundInCommandOutput(const char* command, const char* text, char** reason, OSCONFIG_LOG_HANDLE log);
char* GetStringOptionFromBuffer(const char* buffer, const char* option, char separator, OSCONFIG_LOG_HANDLE log);
int GetIntegerOptionFromBuffer(const char* buffer, const char* option, char separator, OSCONFIG_LOG_HANDLE log);
int CheckTextNotFoundInCommandOutput(const char* command, const char* text, char** reason, OSCONFIG_LOG_HANDLE log);
int SetEtcConfValue(const char* file, const char* name, const char* value, OSCONFIG_LOG_HANDLE log);
int SetEtcLoginDefValue(const char* name, const char* value, OSCONFIG_LOG_HANDLE log);
int CheckLockoutForFailedPasswordAttempts(const char* fileName, const char* pamSo, char commentCharacter, char** reason, OSCONFIG_LOG_HANDLE log);
int SetLockoutForFailedPasswordAttempts(OSCONFIG_LOG_HANDLE log);
int CheckPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, char** reason, OSCONFIG_LOG_HANDLE log);
int SetPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, OSCONFIG_LOG_HANDLE log);
int CheckEnsurePasswordReuseIsLimited(int remember, char** reason, OSCONFIG_LOG_HANDLE log);
int SetEnsurePasswordReuseIsLimited(int remember, OSCONFIG_LOG_HANDLE log);
int EnableVirtualMemoryRandomization(OSCONFIG_LOG_HANDLE log);
int RemoveDotsFromPath(OSCONFIG_LOG_HANDLE log);

char* GetStringOptionFromFile(const char* fileName, const char* option, char separator, OSCONFIG_LOG_HANDLE log);
int GetIntegerOptionFromFile(const char* fileName, const char* option, char separator, OSCONFIG_LOG_HANDLE log);
int CheckIntegerOptionFromFileEqualWithAny(const char* fileName, const char* option, char separator, int* values, int numberOfValues, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckIntegerOptionFromFileLessOrEqualWith(const char* fileName, const char* option, char separator, int value, char** reason, OSCONFIG_LOG_HANDLE log);

char* DuplicateString(const char* source);
char* ConcatenateStrings(const char* first, const char* second);
char* DuplicateStringToLowercase(const char* source);
char* FormatAllocateString(const char* format, ...);
int ConvertStringToIntegers(const char* source, char separator, int** integers, int* numIntegers, int base, OSCONFIG_LOG_HANDLE log);
char* RemoveCharacterFromString(const char* source, char what, OSCONFIG_LOG_HANDLE log);
char* ReplaceEscapeSequencesInString(const char* source, const char* escapes, unsigned int numEscapes, char replacement, OSCONFIG_LOG_HANDLE log);
int RemoveEscapeSequencesFromFile(const char* fileName, const char* escapes, unsigned int numEscapes, char replacement, OSCONFIG_LOG_HANDLE log);

int DisablePostfixNetworkListening(OSCONFIG_LOG_HANDLE log);

size_t HashString(const char* source);
char* HashCommand(const char* source, OSCONFIG_LOG_HANDLE log);

bool ParseHttpProxyData(const char* proxyData, char** hostAddress, int* port, char**username, char** password, OSCONFIG_LOG_HANDLE log);

char* GetOsPrettyName(OSCONFIG_LOG_HANDLE log);
char* GetOsName(OSCONFIG_LOG_HANDLE log);
char* GetOsVersion(OSCONFIG_LOG_HANDLE log);
char* GetOsKernelName(OSCONFIG_LOG_HANDLE log);
char* GetOsKernelRelease(OSCONFIG_LOG_HANDLE log);
char* GetOsKernelVersion(OSCONFIG_LOG_HANDLE log);
char* GetCpuType(OSCONFIG_LOG_HANDLE log);
char* GetCpuVendor(OSCONFIG_LOG_HANDLE log);
char* GetCpuModel(OSCONFIG_LOG_HANDLE log);
unsigned int GetNumberOfCpuCores(OSCONFIG_LOG_HANDLE log);
char* GetCpuFlags(OSCONFIG_LOG_HANDLE log);
bool CheckCpuFlagSupported(const char* cpuFlag, char** reason, OSCONFIG_LOG_HANDLE log);
long GetTotalMemory(OSCONFIG_LOG_HANDLE log);
long GetFreeMemory(OSCONFIG_LOG_HANDLE log);
char* GetProductName(OSCONFIG_LOG_HANDLE log);
char* GetProductVendor(OSCONFIG_LOG_HANDLE log);
char* GetProductVersion(OSCONFIG_LOG_HANDLE log);
char* GetSystemCapabilities(OSCONFIG_LOG_HANDLE log);
char* GetSystemConfiguration(OSCONFIG_LOG_HANDLE log);
bool CheckOsAndKernelMatchDistro(char** reason, OSCONFIG_LOG_HANDLE log);
char* GetLoginUmask(char** reason, OSCONFIG_LOG_HANDLE log);
int CheckLoginUmask(const char* desired, char** reason, OSCONFIG_LOG_HANDLE log);
long GetPassMinDays(OSCONFIG_LOG_HANDLE log);
long GetPassMaxDays(OSCONFIG_LOG_HANDLE log);
long GetPassWarnAge(OSCONFIG_LOG_HANDLE log);
int SetPassMinDays(long days, OSCONFIG_LOG_HANDLE log);
int SetPassMaxDays(long days, OSCONFIG_LOG_HANDLE log);
int SetPassWarnAge(long days, OSCONFIG_LOG_HANDLE log);
bool IsCurrentOs(const char* name, OSCONFIG_LOG_HANDLE log);
bool IsRedHatBased(OSCONFIG_LOG_HANDLE log);
bool IsCommodore(OSCONFIG_LOG_HANDLE log);
bool IsSelinuxPresent(void);
bool DetectSelinux(OSCONFIG_LOG_HANDLE log);

void RemovePrefix(char* target, char marker);
void RemovePrefixBlanks(char* target);
void RemovePrefixUpTo(char* target, char marker);
void RemovePrefixUpToString(char* target, const char* marker);
void RemoveTrailingBlanks(char* target);
void TruncateAtFirst(char* target, char marker);

char* UrlEncode(const char* target);
char* UrlDecode(const char* target);

bool LockFile(FILE* file, OSCONFIG_LOG_HANDLE log);
bool UnlockFile(FILE* file, OSCONFIG_LOG_HANDLE log);

char* ReadUriFromSocket(int socketHandle, OSCONFIG_LOG_HANDLE log);
int ReadHttpStatusFromSocket(int socketHandle, OSCONFIG_LOG_HANDLE log);
int ReadHttpContentLengthFromSocket(int socketHandle, OSCONFIG_LOG_HANDLE log);

int SleepMilliseconds(long milliseconds);

bool FreeAndReturnTrue(void* value);

bool IsValidDaemonName(const char *name);
bool IsDaemonActive(const char* daemonName, OSCONFIG_LOG_HANDLE log);
bool CheckDaemonActive(const char* daemonName, char** reason, OSCONFIG_LOG_HANDLE log);
bool CheckDaemonNotActive(const char* daemonName, char** reason, OSCONFIG_LOG_HANDLE log);
bool StartDaemon(const char* daemonName, OSCONFIG_LOG_HANDLE log);
bool StopDaemon(const char* daemonName, OSCONFIG_LOG_HANDLE log);
bool EnableDaemon(const char* daemonName, OSCONFIG_LOG_HANDLE log);
bool DisableDaemon(const char* daemonName, OSCONFIG_LOG_HANDLE log);
bool EnableAndStartDaemon(const char* daemonName, OSCONFIG_LOG_HANDLE log);
void StopAndDisableDaemon(const char* daemonName, OSCONFIG_LOG_HANDLE log);
bool RestartDaemon(const char* daemonName, OSCONFIG_LOG_HANDLE log);
bool MaskDaemon(const char* daemonName, OSCONFIG_LOG_HANDLE log);

char* GetHttpProxyData(OSCONFIG_LOG_HANDLE log);

char* RepairBrokenEolCharactersIfAny(const char* value);

int CheckAllWirelessInterfacesAreDisabled(char** reason, OSCONFIG_LOG_HANDLE log);
int DisableAllWirelessInterfaces(OSCONFIG_LOG_HANDLE log);
int SetDefaultDenyFirewallPolicy(OSCONFIG_LOG_HANDLE log);

typedef struct REPORTED_PROPERTY
{
    char componentName[MAX_COMPONENT_NAME];
    char propertyName[MAX_COMPONENT_NAME];
    size_t lastPayloadHash;
} REPORTED_PROPERTY;

bool IsCommandLoggingEnabledInJsonConfig(const char* jsonString);
bool IsDebugLoggingEnabledInJsonConfig(const char* jsonString);
bool IsIotHubManagementEnabledInJsonConfig(const char* jsonString);
int GetReportingIntervalFromJsonConfig(const char* jsonString, OSCONFIG_LOG_HANDLE log);
int GetModelVersionFromJsonConfig(const char* jsonString, OSCONFIG_LOG_HANDLE log);
int GetLocalManagementFromJsonConfig(const char* jsonString, OSCONFIG_LOG_HANDLE log);
int GetIotHubProtocolFromJsonConfig(const char* jsonString, OSCONFIG_LOG_HANDLE log);
int LoadReportedFromJsonConfig(const char* jsonString, REPORTED_PROPERTY** reportedProperties, OSCONFIG_LOG_HANDLE log);

int GetGitManagementFromJsonConfig(const char* jsonString, OSCONFIG_LOG_HANDLE log);
char* GetGitRepositoryUrlFromJsonConfig(const char* jsonString, OSCONFIG_LOG_HANDLE log);
char* GetGitBranchFromJsonConfig(const char* jsonString, OSCONFIG_LOG_HANDLE log);

typedef struct PERF_CLOCK
{
    struct timespec start;
    struct timespec stop;
} PERF_CLOCK;

int StartPerfClock(PERF_CLOCK* clock, OSCONFIG_LOG_HANDLE log);
int StopPerfClock(PERF_CLOCK* clock, OSCONFIG_LOG_HANDLE log);
long GetPerfClockTime(PERF_CLOCK* clock, OSCONFIG_LOG_HANDLE log);
void LogPerfClock(PERF_CLOCK* clock, const char* componentName, const char* objectName, int objectResult, long limit, OSCONFIG_LOG_HANDLE log);

#ifdef __cplusplus
}
#endif

#endif // COMMONUTILS_H
