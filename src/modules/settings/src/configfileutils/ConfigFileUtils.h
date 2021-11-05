// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef CONFIGFILEUTILS_H
#define CONFIGFILEUTILS_H

#define READ_CONFIG_FAILURE -1
#define WRITE_CONFIG_SUCCESS 0
#define WRITE_CONFIG_FAILURE 1

enum ConfigFileFormat
{
    ConfigFileFormatToml = 1,
    ConfigFileFormatJson = 2,
    Testing = 3
};
typedef enum ConfigFileFormat ConfigFileFormat;

typedef void* CONFIG_FILE_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

CONFIG_FILE_HANDLE OpenConfigFile(const char* name, ConfigFileFormat format);
int WriteConfigString(CONFIG_FILE_HANDLE config, const char* name, const char* value);
char* ReadConfigString(CONFIG_FILE_HANDLE config, const char* name);
void FreeConfigString(char* name);
int WriteConfigInteger(CONFIG_FILE_HANDLE config, const char* name, const int value);
int ReadConfigInteger(CONFIG_FILE_HANDLE config, const char* name);
void CloseConfigFile(CONFIG_FILE_HANDLE config);

#ifdef __cplusplus
}
#endif

#endif // CONFIGFILEUTILS_H