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

char* LoadStringFromFile(const char* fileName, bool stopAtEol, OsConfigLogHandle log);
bool SavePayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log);
bool FileEndsInEol(const char* fileName, OsConfigLogHandle log);
bool AppendPayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log);
bool SecureSaveToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log);
bool AppendToFile(const char* fileName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log);
bool ConcatenateFiles(const char* firstFileName, const char* secondFileName, bool preserveAccess, OsConfigLogHandle log);
int RenameFile(const char* original, const char* target, OsConfigLogHandle log);

typedef int(*CommandCallback)(void* context);

// If called from the main process thread the timeoutSeconds and callback arguments are ignored
int ExecuteCommand(void* context, const char* command, bool replaceEol, bool forJson, unsigned int maxTextResultBytes, unsigned int timeoutSeconds, char** textResult, CommandCallback callback, OsConfigLogHandle log);

int RestrictFileAccessToCurrentAccountOnly(const char* fileName);

bool IsAFile(const char* fileName, OsConfigLogHandle log);
bool IsADirectory(const char* fileName, OsConfigLogHandle log);
bool FileExists(const char* fileName);
bool DirectoryExists(const char* directoryName);
int CheckFileExists(const char* fileName, char** reason, OsConfigLogHandle log);
int CheckFileNotFound(const char* fileName, char** reason, OsConfigLogHandle log);

bool MakeFileBackupCopy(const char* fileName, const char* backupName, bool preserveAccess, OsConfigLogHandle log);

int CheckFileAccess(const char* fileName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, char** reason, OsConfigLogHandle log);
int SetFileAccess(const char* fileName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, OsConfigLogHandle log);
int GetFileAccess(const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, OsConfigLogHandle log);
int RenameFileWithOwnerAndAccess(const char* original, const char* target, OsConfigLogHandle log);

int CheckDirectoryAccess(const char* directoryName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, bool rootCanOverwriteOwnership, char** reason, OsConfigLogHandle log);
int SetDirectoryAccess(const char* directoryName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, OsConfigLogHandle log);
int GetDirectoryAccess(const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, OsConfigLogHandle log);

int CheckFileSystemMountingOption(const char* mountFileName, const char* mountDirectory, const char* mountType, const char* desiredOption, char** reason, OsConfigLogHandle log);
int SetFileSystemMountingOption(const char* mountDirectory, const char* mountType, const char* desiredOption, OsConfigLogHandle log);

int IsPresent(const char* what, OsConfigLogHandle log);
int IsPackageInstalled(const char* packageName, OsConfigLogHandle log);
int CheckPackageInstalled(const char* packageName, char** reason, OsConfigLogHandle log);
int CheckPackageNotInstalled(const char* packageName, char** reason, OsConfigLogHandle log);
int InstallOrUpdatePackage(const char* packageName, OsConfigLogHandle log);
int InstallPackage(const char* packageName, OsConfigLogHandle log);
int UninstallPackage(const char* packageName, OsConfigLogHandle log);

unsigned int GetNumberOfLinesInFile(const char* fileName);
bool CharacterFoundInFile(const char* fileName, char what);
int CheckNoLegacyPlusEntriesInFile(const char* fileName, char** reason, OsConfigLogHandle log);
int ReplaceMarkedLinesInFile(const char* fileName, const char* marker, const char* newline, char commentCharacter, bool preserveAccess, OsConfigLogHandle log);
int FindTextInFile(const char* fileName, const char* text, OsConfigLogHandle log);
int CheckTextIsFoundInFile(const char* fileName, const char* text, char** reason, OsConfigLogHandle log);
int CheckTextIsNotFoundInFile(const char* fileName, const char* text, char** reason, OsConfigLogHandle log);
int CheckMarkedTextNotFoundInFile(const char* fileName, const char* text, const char* marker, char commentCharacter, char** reason, OsConfigLogHandle log);
int CheckTextNotFoundInEnvironmentVariable(const char* variableName, const char* text, bool strictComparison, char** reason, OsConfigLogHandle log);
int CheckSmallFileContainsText(const char* fileName, const char* text, char** reason, OsConfigLogHandle log);
int FindTextInFolder(const char* directory, const char* text, OsConfigLogHandle log);
int CheckTextNotFoundInFolder(const char* directory, const char* text, char** reason, OsConfigLogHandle log);
int CheckTextFoundInFolder(const char* directory, const char* text, char** reason, OsConfigLogHandle log);
int CheckLineNotFoundOrCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, OsConfigLogHandle log);
int CheckLineFoundNotCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, OsConfigLogHandle log);
int CheckTextFoundInCommandOutput(const char* command, const char* text, char** reason, OsConfigLogHandle log);
char* GetStringOptionFromBuffer(const char* buffer, const char* option, char separator, OsConfigLogHandle log);
int GetIntegerOptionFromBuffer(const char* buffer, const char* option, char separator, OsConfigLogHandle log);
int CheckTextNotFoundInCommandOutput(const char* command, const char* text, char** reason, OsConfigLogHandle log);
int SetEtcConfValue(const char* file, const char* name, const char* value, OsConfigLogHandle log);
int SetEtcLoginDefValue(const char* name, const char* value, OsConfigLogHandle log);
int CheckLockoutForFailedPasswordAttempts(const char* fileName, const char* pamSo, char commentCharacter, char** reason, OsConfigLogHandle log);
int SetLockoutForFailedPasswordAttempts(OsConfigLogHandle log);
int CheckPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, char** reason, OsConfigLogHandle log);
int SetPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, OsConfigLogHandle log);
int CheckEnsurePasswordReuseIsLimited(int remember, char** reason, OsConfigLogHandle log);
int SetEnsurePasswordReuseIsLimited(int remember, OsConfigLogHandle log);
int EnableVirtualMemoryRandomization(OsConfigLogHandle log);
int RemoveDotsFromPath(OsConfigLogHandle log);

char* GetStringOptionFromFile(const char* fileName, const char* option, char separator, OsConfigLogHandle log);
int GetIntegerOptionFromFile(const char* fileName, const char* option, char separator, OsConfigLogHandle log);
int CheckIntegerOptionFromFileEqualWithAny(const char* fileName, const char* option, char separator, int* values, int numberOfValues, char** reason, OsConfigLogHandle log);
int CheckIntegerOptionFromFileLessOrEqualWith(const char* fileName, const char* option, char separator, int value, char** reason, OsConfigLogHandle log);

char* DuplicateString(const char* source);
char* ConcatenateStrings(const char* first, const char* second);
char* DuplicateStringToLowercase(const char* source);
char* FormatAllocateString(const char* format, ...);
int ConvertStringToIntegers(const char* source, char separator, int** integers, int* numIntegers, int base, OsConfigLogHandle log);
char* RemoveCharacterFromString(const char* source, char what, OsConfigLogHandle log);
char* ReplaceEscapeSequencesInString(const char* source, const char* escapes, unsigned int numEscapes, char replacement, OsConfigLogHandle log);
int RemoveEscapeSequencesFromFile(const char* fileName, const char* escapes, unsigned int numEscapes, char replacement, OsConfigLogHandle log);

int DisablePostfixNetworkListening(OsConfigLogHandle log);

size_t HashString(const char* source);
char* HashCommand(const char* source, OsConfigLogHandle log);

bool ParseHttpProxyData(const char* proxyData, char** hostAddress, int* port, char**username, char** password, OsConfigLogHandle log);

char* GetOsPrettyName(OsConfigLogHandle log);
char* GetOsName(OsConfigLogHandle log);
char* GetOsVersion(OsConfigLogHandle log);
char* GetOsKernelName(OsConfigLogHandle log);
char* GetOsKernelRelease(OsConfigLogHandle log);
char* GetOsKernelVersion(OsConfigLogHandle log);
char* GetCpuType(OsConfigLogHandle log);
char* GetCpuVendor(OsConfigLogHandle log);
char* GetCpuModel(OsConfigLogHandle log);
unsigned int GetNumberOfCpuCores(OsConfigLogHandle log);
char* GetCpuFlags(OsConfigLogHandle log);
bool CheckCpuFlagSupported(const char* cpuFlag, char** reason, OsConfigLogHandle log);
long GetTotalMemory(OsConfigLogHandle log);
long GetFreeMemory(OsConfigLogHandle log);
char* GetProductName(OsConfigLogHandle log);
char* GetProductVendor(OsConfigLogHandle log);
char* GetProductVersion(OsConfigLogHandle log);
char* GetSystemCapabilities(OsConfigLogHandle log);
char* GetSystemConfiguration(OsConfigLogHandle log);
bool CheckOsAndKernelMatchDistro(char** reason, OsConfigLogHandle log);
char* GetLoginUmask(char** reason, OsConfigLogHandle log);
int CheckLoginUmask(const char* desired, char** reason, OsConfigLogHandle log);
long GetPassMinDays(OsConfigLogHandle log);
long GetPassMaxDays(OsConfigLogHandle log);
long GetPassWarnAge(OsConfigLogHandle log);
int SetPassMinDays(long days, OsConfigLogHandle log);
int SetPassMaxDays(long days, OsConfigLogHandle log);
int SetPassWarnAge(long days, OsConfigLogHandle log);
bool IsCurrentOs(const char* name, OsConfigLogHandle log);
bool IsRedHatBased(OsConfigLogHandle log);
bool IsCommodore(OsConfigLogHandle log);
bool IsSelinuxPresent(void);
bool DetectSelinux(OsConfigLogHandle log);

void RemovePrefix(char* target, char marker);
void RemovePrefixBlanks(char* target);
void RemovePrefixUpTo(char* target, char marker);
void RemovePrefixUpToString(char* target, const char* marker);
void RemoveTrailingBlanks(char* target);
void TruncateAtFirst(char* target, char marker);

char* UrlEncode(const char* target);
char* UrlDecode(const char* target);

bool LockFile(FILE* file, OsConfigLogHandle log);
bool UnlockFile(FILE* file, OsConfigLogHandle log);

char* ReadUriFromSocket(int socketHandle, OsConfigLogHandle log);
int ReadHttpStatusFromSocket(int socketHandle, OsConfigLogHandle log);
int ReadHttpContentLengthFromSocket(int socketHandle, OsConfigLogHandle log);

int SleepMilliseconds(long milliseconds);

bool FreeAndReturnTrue(void* value);

bool IsValidDaemonName(const char *name);
bool IsDaemonActive(const char* daemonName, OsConfigLogHandle log);
bool CheckDaemonActive(const char* daemonName, char** reason, OsConfigLogHandle log);
bool CheckDaemonNotActive(const char* daemonName, char** reason, OsConfigLogHandle log);
bool StartDaemon(const char* daemonName, OsConfigLogHandle log);
bool StopDaemon(const char* daemonName, OsConfigLogHandle log);
bool EnableDaemon(const char* daemonName, OsConfigLogHandle log);
bool DisableDaemon(const char* daemonName, OsConfigLogHandle log);
bool EnableAndStartDaemon(const char* daemonName, OsConfigLogHandle log);
void StopAndDisableDaemon(const char* daemonName, OsConfigLogHandle log);
bool RestartDaemon(const char* daemonName, OsConfigLogHandle log);
bool MaskDaemon(const char* daemonName, OsConfigLogHandle log);

char* GetHttpProxyData(OsConfigLogHandle log);

char* RepairBrokenEolCharactersIfAny(const char* value);

int CheckAllWirelessInterfacesAreDisabled(char** reason, OsConfigLogHandle log);
int DisableAllWirelessInterfaces(OsConfigLogHandle log);
int SetDefaultDenyFirewallPolicy(OsConfigLogHandle log);

typedef struct ReportedProperty
{
    char componentName[MAX_COMPONENT_NAME];
    char propertyName[MAX_COMPONENT_NAME];
    size_t lastPayloadHash;
} ReportedProperty;

bool IsDebugLoggingEnabledInJsonConfig(const char* jsonString);
bool IsIotHubManagementEnabledInJsonConfig(const char* jsonString);
int GetLoggingLevelFromJsonConfig(const char* jsonString, OsConfigLogHandle log);
int GetReportingIntervalFromJsonConfig(const char* jsonString, OsConfigLogHandle log);
int GetModelVersionFromJsonConfig(const char* jsonString, OsConfigLogHandle log);
int GetLocalManagementFromJsonConfig(const char* jsonString, OsConfigLogHandle log);
int GetIotHubProtocolFromJsonConfig(const char* jsonString, OsConfigLogHandle log);
int LoadReportedFromJsonConfig(const char* jsonString, ReportedProperty** reportedProperties, OsConfigLogHandle log);

int GetGitManagementFromJsonConfig(const char* jsonString, OsConfigLogHandle log);
char* GetGitRepositoryUrlFromJsonConfig(const char* jsonString, OsConfigLogHandle log);
char* GetGitBranchFromJsonConfig(const char* jsonString, OsConfigLogHandle log);

typedef struct PerfClock
{
    struct timespec start;
    struct timespec stop;
} PerfClock;

int StartPerfClock(PerfClock* clock, OsConfigLogHandle log);
int StopPerfClock(PerfClock* clock, OsConfigLogHandle log);
long GetPerfClockTime(PerfClock* clock, OsConfigLogHandle log);
void LogPerfClock(PerfClock* clock, const char* componentName, const char* objectName, int objectResult, long limit, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // COMMONUTILS_H
