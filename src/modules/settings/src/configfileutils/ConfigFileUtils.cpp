// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include <fstream>
#include <string>
#include <system_error>
#include <stdexcept>
#include "BaseUtils.h"
#include "ConfigFileUtils.h"

using namespace std;

CONFIG_FILE_HANDLE OpenConfigFileInternal(const char* name, ConfigFileFormat format);
int WriteConfigStringInternal(CONFIG_FILE_HANDLE config, const char* name, const char* value);
char* ReadConfigStringInternal(CONFIG_FILE_HANDLE config, const char* name);
void FreeConfigStringInternal(char* name);
int WriteConfigIntegerInternal(CONFIG_FILE_HANDLE config, const char* name, const int value);
int ReadConfigIntegerInternal(CONFIG_FILE_HANDLE config, const char* name);
void CloseConfigFileInternal(CONFIG_FILE_HANDLE config);

CONFIG_FILE_HANDLE OpenConfigFile(const char* name, ConfigFileFormat format)
{
    try
    {
        return OpenConfigFileInternal(name, format);
    }
    catch (system_error& error)
    {
        printf("OpenConfigFile system error: %s (%d)\n", error.what(), error.code().value());
        return nullptr;
    }
    catch (runtime_error& e)
    {
        printf("OpenConfigFile runtime error: %s\n", e.what());
        return nullptr;
    }
    catch (exception& e)
    {
        printf("OpenConfigFile exception: %s\n", e.what());
        return nullptr;
    }
    catch (...)
    {
        printf ("OpenConfigFile unknown exception was thrown!\n");
        return nullptr;
    }
}

int WriteConfigString(CONFIG_FILE_HANDLE config, const char* name, const char* value)
{
    try
    {
        return WriteConfigStringInternal(config, name, value);
    }
    catch (system_error& error)
    {
        printf("WriteConfigString system error: %s (%d)\n", error.what(), error.code().value());
        return WRITE_CONFIG_FAILURE;
    }
    catch (runtime_error& e)
    {
        printf("WriteConfigString runtime error: %s\n", e.what());
        return WRITE_CONFIG_FAILURE;
    }
    catch (exception& e)
    {
        printf("WriteConfigString exception: %s\n", e.what());
        return WRITE_CONFIG_FAILURE;
    }
    catch (...)
    {
        printf ("WriteConfigString unknown exception was thrown!\n");
        return WRITE_CONFIG_FAILURE;
    }
}

char* ReadConfigString(CONFIG_FILE_HANDLE config, const char* name)
{
    try
    {
        return ReadConfigStringInternal(config, name);
    }
    catch (system_error& error)
    {
        printf("ReadConfigString system error: %s (%d)\n", error.what(), error.code().value());
        return nullptr;
    }
    catch (runtime_error& e)
    {
        printf("ReadConfigString runtime error: %s\n", e.what());
        return nullptr;
    }
    catch (exception& e)
    {
        printf("ReadConfigString exception: %s\n", e.what());
        return nullptr;
    }
    catch (...)
    {
        printf ("ReadConfigString unknown exception was thrown!\n");
        return nullptr;
    }
}

void FreeConfigString(char* name)
{
    try
    {
        return FreeConfigStringInternal(name);
    }
    catch (system_error& error)
    {
        printf("FreeConfigString system error: %s (%d)\n", error.what(), error.code().value());
        return;
    }
    catch (runtime_error& e)
    {
        printf("FreeConfigString runtime error: %s\n", e.what());
        return;
    }
    catch (exception& e)
    {
        printf("FreeConfigString exception: %s\n", e.what());
        return;
    }
    catch (...)
    {
        printf ("FreeConfigString unknown exception was thrown!\n");
        return;
    }
}

int WriteConfigInteger(CONFIG_FILE_HANDLE config, const char* name, const int value)
{
    try
    {
        return WriteConfigIntegerInternal(config, name, value);
    }
    catch (system_error& error)
    {
        printf("WriteConfigInteger system error: %s (%d)\n", error.what(), error.code().value());
        return WRITE_CONFIG_FAILURE;
    }
    catch (runtime_error& e)
    {
        printf("WriteConfigInteger runtime error: %s\n", e.what());
        return WRITE_CONFIG_FAILURE;
    }
    catch (exception& e)
    {
        printf("WriteConfigInteger exception: %s\n", e.what());
        return WRITE_CONFIG_FAILURE;
    }
    catch (...)
    {
        printf ("WriteConfigInteger unknown exception was thrown!\n");
        return WRITE_CONFIG_FAILURE;
    }
}

int ReadConfigInteger(CONFIG_FILE_HANDLE config, const char* name)
{
    try
    {
        return ReadConfigIntegerInternal(config, name);
    }
    catch (system_error& error)
    {
        printf("ReadConfigInteger system error: %s (%d)\n", error.what(), error.code().value());
        return READ_CONFIG_FAILURE;
    }
    catch (runtime_error& e)
    {
        printf("ReadConfigInteger runtime error: %s\n", e.what());
        return READ_CONFIG_FAILURE;
    }
    catch (exception& e)
    {
        printf("ReadConfigInteger exception: %s\n", e.what());
        return READ_CONFIG_FAILURE;
    }
    catch (...)
    {
        printf ("ReadConfigInteger unknown exception was thrown!\n");
        return READ_CONFIG_FAILURE;
    }
}

void CloseConfigFile(CONFIG_FILE_HANDLE config)
{
    try
    {
        return CloseConfigFileInternal(config);
    }
    catch (system_error& error)
    {
        printf("CloseConfigFile system error: %s (%d)\n", error.what(), error.code().value());
        return;
    }
    catch (runtime_error& e)
    {
        printf("CloseConfigFile runtime error: %s\n", e.what());
        return;
    }
    catch (exception& e)
    {
        printf("CloseConfigFile exception: %s\n", e.what());
        return;
    }
    catch (...)
    {
        printf ("CloseConfigFile unknown exception was thrown.\n");
        return;
    }
}

CONFIG_FILE_HANDLE OpenConfigFileInternal(const char* name, ConfigFileFormat format)
{
    if (nullptr == name)
    {
        printf("OpenConfigFile: Invalid argument\n");
        return nullptr;
    }
    else
    {
        CONFIG_FILE_HANDLE handle = BaseUtilsFactory::CreateInstance(name, format);
        if (nullptr == handle)
        {
            printf("OpenConfigFile: BaseUtilsFactory::CreateInstance failed\n");
        }
        return handle;
    }
}

int WriteConfigStringInternal(CONFIG_FILE_HANDLE config, const char* name, const char* value)
{
    if ((nullptr == config) || (nullptr == name) || (nullptr == value))
    {
        printf("WriteConfigString: Invalid argument\n");
        return WRITE_CONFIG_FAILURE;
    }
    else
    {
        BaseUtils* utils = static_cast<BaseUtils*>(config);
        if (!(utils->SetValueString(name, value)))
        {
            printf("WriteConfigString: BaseUtils::SetValueString failed\n");
            return WRITE_CONFIG_FAILURE;
        }
        config = static_cast<void*>(utils);
        return WRITE_CONFIG_SUCCESS;
    }
}

char* ReadConfigStringInternal(CONFIG_FILE_HANDLE config, const char* name)
{
    if ((nullptr == config) || (nullptr == name))
    {
        printf("ReadConfigString: Invalid argument\n");
        return nullptr;
    }
    else
    {
        BaseUtils* utils = static_cast<BaseUtils*>(config);
        char* value = utils->GetValueString(name);
        if (nullptr == value)
        {
            printf("ReadConfigString: BaseUtils::GetValueString failed\n");
        }

        config = static_cast<void*>(utils);
        return value;
    }
}

void FreeConfigStringInternal(char* name)
{
    if (nullptr != name)
    {
        delete[] name;
    }
}

int WriteConfigIntegerInternal(CONFIG_FILE_HANDLE config, const char* name, const int value)
{
    if ((nullptr == config) || (nullptr == name))
    {
        printf("WriteConfigInteger: Invalid argument\n");
        return WRITE_CONFIG_FAILURE;
    }
    else
    {
        int result = WRITE_CONFIG_FAILURE;
        BaseUtils* utils = static_cast<BaseUtils*>(config);
        if (!(utils->SetValueInteger(name, value)))
        {
            printf("WriteConfigInteger: BaseUtils::SetValueInteger failed\n");
        }
        else
        {
            result = WRITE_CONFIG_SUCCESS;
        }

        config = static_cast<void*>(utils);
        return result;
    }
}

int ReadConfigIntegerInternal(CONFIG_FILE_HANDLE config, const char* name)
{
    if ((nullptr == config) || (nullptr == name))
    {
        printf("ReadConfigInteger: Invalid argument\n");
        return READ_CONFIG_FAILURE;
    }
    else
    {
        BaseUtils* utils = static_cast<BaseUtils*>(config);
        int value = utils->GetValueInteger(name);
        if (READ_CONFIG_FAILURE == value)
        {
            printf("ReadConfigInteger: BaseUtils::GetValueInteger failed\n");
        }

        config = static_cast<void*>(utils);
        return value;
    }
}

void CloseConfigFileInternal(CONFIG_FILE_HANDLE config)
{
    if (nullptr != config)
    {
        delete static_cast<BaseUtils*>(config);
    }
}