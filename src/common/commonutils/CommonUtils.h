// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMONUTILS_H
#define COMMONUTILS_H

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

char* LoadStringFromFile(const char* fileName, bool stopAtEol, void* log);
bool SavePayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log);
bool FileEndsInEol(const char* fileName, void* log);
bool AppendPayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log);
bool SecureSaveToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log);
bool AppendToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log);
bool ConcatenateFiles(const char* firstFileName, const char* secondFileName, bool preserveAccess, void* log);
int RenameFile(const char* original, const char* target, void* log);

void SetCommandLogging(bool commandLogging);
bool IsCommandLoggingEnabled(void);

typedef int(*CommandCallback)(void* context);

// If called from the main process thread the timeoutSeconds and callback arguments are ignored
int ExecuteCommand(void* context, const char* command, bool replaceEol, bool forJson, unsigned int maxTextResultBytes, unsigned int timeoutSeconds, char** textResult, CommandCallback callback, void* log);

int RestrictFileAccessToCurrentAccountOnly(const char* fileName);

bool IsAFile(const char* fileName, void* log);
bool IsADirectory(const char* fileName, void* log);
bool FileExists(const char* fileName);
bool DirectoryExists(const char* directoryName);
int CheckFileExists(const char* fileName, char** reason, void* log);
int CheckFileNotFound(const char* fileName, char** reason, void* log);

bool MakeFileBackupCopy(const char* fileName, const char* backupName, bool preserveAccess, void* log);

int CheckFileAccess(const char* fileName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, char** reason, void* log);
int SetFileAccess(const char* fileName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, void* log);
int GetFileAccess(const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, void* log);
int RenameFileWithOwnerAndAccess(const char* original, const char* target, void* log);

int CheckDirectoryAccess(const char* directoryName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, bool rootCanOverwriteOwnership, char** reason, void* log);
int SetDirectoryAccess(const char* directoryName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, void* log);
int GetDirectoryAccess(const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, void* log);

int CheckFileSystemMountingOption(const char* mountFileName, const char* mountDirectory, const char* mountType, const char* desiredOption, char** reason, void* log);
int SetFileSystemMountingOption(const char* mountDirectory, const char* mountType, const char* desiredOption, void* log);

int IsPresent(const char* what, void* log);
int IsPackageInstalled(const char* packageName, void* log);
int CheckPackageInstalled(const char* packageName, char** reason, void* log);
int CheckPackageNotInstalled(const char* packageName, char** reason, void* log);
int InstallOrUpdatePackage(const char* packageName, void* log);
int InstallPackage(const char* packageName, void* log);
int UninstallPackage(const char* packageName, void* log);

unsigned int GetNumberOfLinesInFile(const char* fileName);
bool CharacterFoundInFile(const char* fileName, char what);
int CheckNoLegacyPlusEntriesInFile(const char* fileName, char** reason, void* log);
int ReplaceMarkedLinesInFile(const char* fileName, const char* marker, const char* newline, char commentCharacter, bool preserveAccess, void* log);
int FindTextInFile(const char* fileName, const char* text, void* log);
int CheckTextIsFoundInFile(const char* fileName, const char* text, char** reason, void* log);
int CheckTextIsNotFoundInFile(const char* fileName, const char* text, char** reason, void* log);
int CheckMarkedTextNotFoundInFile(const char* fileName, const char* text, const char* marker, char commentCharacter, char** reason, void* log);
int CheckTextNotFoundInEnvironmentVariable(const char* variableName, const char* text, bool strictComparison, char** reason, void* log);
int CheckSmallFileContainsText(const char* fileName, const char* text, char** reason, void* log);
int FindTextInFolder(const char* directory, const char* text, void* log);
int CheckTextNotFoundInFolder(const char* directory, const char* text, char** reason, void* log);
int CheckTextFoundInFolder(const char* directory, const char* text, char** reason, void* log);
int CheckLineNotFoundOrCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, void* log);
int CheckLineFoundNotCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, void* log);
int CheckTextFoundInCommandOutput(const char* command, const char* text, char** reason, void* log);
char* GetStringOptionFromBuffer(const char* buffer, const char* option, char separator, void* log);
int GetIntegerOptionFromBuffer(const char* buffer, const char* option, char separator, void* log);
int CheckTextNotFoundInCommandOutput(const char* command, const char* text, char** reason, void* log);
int SetEtcConfValue(const char* file, const char* name, const char* value, void* log);
int SetEtcLoginDefValue(const char* name, const char* value, void* log);
int CheckLockoutForFailedPasswordAttempts(const char* fileName, const char* pamSo, char commentCharacter, char** reason, void* log);
int SetLockoutForFailedPasswordAttempts(void* log);
int CheckPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, char** reason, void* log);
int SetPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, void* log);
int CheckEnsurePasswordReuseIsLimited(int remember, char** reason, void* log);
int SetEnsurePasswordReuseIsLimited(int remember, void* log);
int EnableVirtualMemoryRandomization(void* log);
int RemoveDotsFromPath(void* log);

char* GetStringOptionFromFile(const char* fileName, const char* option, char separator, void* log);
int GetIntegerOptionFromFile(const char* fileName, const char* option, char separator, void* log);
int CheckIntegerOptionFromFileEqualWithAny(const char* fileName, const char* option, char separator, int* values, int numberOfValues, char** reason, void* log);
int CheckIntegerOptionFromFileLessOrEqualWith(const char* fileName, const char* option, char separator, int value, char** reason, void* log);

char* DuplicateString(const char* source);
char* ConcatenateStrings(const char* first, const char* second);
char* DuplicateStringToLowercase(const char* source);
char* FormatAllocateString(const char* format, ...);
int ConvertStringToIntegers(const char* source, char separator, int** integers, int* numIntegers, void* log);
char* RemoveCharacterFromString(const char* source, char what, void* log);
char* ReplaceEscapeSequencesInString(const char* source, const char* escapes, unsigned int numEscapes, char replacement, void* log);
int RemoveEscapeSequencesFromFile(const char* fileName, const char* escapes, unsigned int numEscapes, char replacement, void* log);

int DisablePostfixNetworkListening(void* log);

size_t HashString(const char* source);
char* HashCommand(const char* source, void* log);

bool ParseHttpProxyData(const char* proxyData, char** hostAddress, int* port, char**username, char** password, void* log);

char* GetOsPrettyName(void* log);
char* GetOsName(void* log);
char* GetOsVersion(void* log);
char* GetOsKernelName(void* log);
char* GetOsKernelRelease(void* log);
char* GetOsKernelVersion(void* log);
char* GetCpuType(void* log);
char* GetCpuVendor(void* log);
char* GetCpuModel(void* log);
unsigned int GetNumberOfCpuCores(void* log);
char* GetCpuFlags(void* log);
bool CheckCpuFlagSupported(const char* cpuFlag, char** reason, void* log);
long GetTotalMemory(void* log);
long GetFreeMemory(void* log);
char* GetProductName(void* log);
char* GetProductVendor(void* log);
char* GetProductVersion(void* log);
char* GetSystemCapabilities(void* log);
char* GetSystemConfiguration(void* log);
bool CheckOsAndKernelMatchDistro(char** reason, void* log);
char* GetLoginUmask(char** reason, void* log);
int CheckLoginUmask(const char* desired, char** reason, void* log);
long GetPassMinDays(void* log);
long GetPassMaxDays(void* log);
long GetPassWarnAge(void* log);
int SetPassMinDays(long days, void* log);
int SetPassMaxDays(long days, void* log);
int SetPassWarnAge(long days, void* log);
bool IsCurrentOs(const char* name, void* log);
bool IsRedHatBased(void* log);
bool IsCommodore(void* log);
bool IsSelinuxPresent(void);
bool DetectSelinux(void* log);

void RemovePrefix(char* target, char marker);
void RemovePrefixBlanks(char* target);
void RemovePrefixUpTo(char* target, char marker);
void RemovePrefixUpToString(char* target, const char* marker);
void RemoveTrailingBlanks(char* target);
void TruncateAtFirst(char* target, char marker);

char* UrlEncode(const char* target);
char* UrlDecode(const char* target);

bool LockFile(FILE* file, void* log);
bool UnlockFile(FILE* file, void* log);

char* ReadUriFromSocket(int socketHandle, void* log);
int ReadHttpStatusFromSocket(int socketHandle, void* log);
int ReadHttpContentLengthFromSocket(int socketHandle, void* log);

int SleepMilliseconds(long milliseconds);

bool FreeAndReturnTrue(void* value);

bool IsValidDaemonName(const char *name);
bool IsDaemonActive(const char* daemonName, void* log);
bool CheckDaemonActive(const char* daemonName, char** reason, void* log);
bool CheckDaemonNotActive(const char* daemonName, char** reason, void* log);
bool StartDaemon(const char* daemonName, void* log);
bool StopDaemon(const char* daemonName, void* log);
bool EnableDaemon(const char* daemonName, void* log);
bool DisableDaemon(const char* daemonName, void* log);
bool EnableAndStartDaemon(const char* daemonName, void* log);
void StopAndDisableDaemon(const char* daemonName, void* log);
bool RestartDaemon(const char* daemonName, void* log);
bool MaskDaemon(const char* daemonName, void* log);

char* GetHttpProxyData(void* log);

char* RepairBrokenEolCharactersIfAny(const char* value);

int CheckAllWirelessInterfacesAreDisabled(char** reason, void* log);
int DisableAllWirelessInterfaces(void* log);
int SetDefaultDenyFirewallPolicy(void* log);

typedef struct REPORTED_PROPERTY
{
    char componentName[MAX_COMPONENT_NAME];
    char propertyName[MAX_COMPONENT_NAME];
    size_t lastPayloadHash;
} REPORTED_PROPERTY;

bool IsCommandLoggingEnabledInJsonConfig(const char* jsonString);
bool IsFullLoggingEnabledInJsonConfig(const char* jsonString);
bool IsIotHubManagementEnabledInJsonConfig(const char* jsonString);
int GetReportingIntervalFromJsonConfig(const char* jsonString, void* log);
int GetModelVersionFromJsonConfig(const char* jsonString, void* log);
int GetLocalManagementFromJsonConfig(const char* jsonString, void* log);
int GetIotHubProtocolFromJsonConfig(const char* jsonString, void* log);
int LoadReportedFromJsonConfig(const char* jsonString, REPORTED_PROPERTY** reportedProperties, void* log);

int GetGitManagementFromJsonConfig(const char* jsonString, void* log);
char* GetGitRepositoryUrlFromJsonConfig(const char* jsonString, void* log);
char* GetGitBranchFromJsonConfig(const char* jsonString, void* log);

typedef struct PERF_CLOCK
{
    struct timespec start;
    struct timespec stop;
} PERF_CLOCK;

int StartPerfClock(PERF_CLOCK* clock, void* log);
int StopPerfClock(PERF_CLOCK* clock, void* log);
long GetPerfClockTime(PERF_CLOCK* clock, void* log);
void LogPerfClock(PERF_CLOCK* clock, const char* componentName, const char* objectName, int objectResult, long limit, void* log);

#ifdef __cplusplus
}
#endif

#endif // COMMONUTILS_H
