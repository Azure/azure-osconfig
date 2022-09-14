// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MODULESTESTCOMMON_H
#define MODULESTESTCOMMON_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <map>
#include <memory>
#include <parson.h>
#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <sstream>
#include <string.h>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <CommonUtils.h>
#include <Mmi.h>
#include <Logging.h>
#include <version.h>

// Use <filesystem> or <experimental/filesystem> based on gcc lib availability
#if __has_include(<filesystem>)
    #include <filesystem>
#else
    #include <experimental/filesystem>
    
    // Alias namespace
    namespace std 
    {
        namespace filesystem = experimental::filesystem;
    }
#endif

#include "ManagementModule.h"
#include "RecipeModuleSessionLoader.h"
#include "TestRecipeParser.h"
#include "RecipeInvoker.h"
#include "MimParser.h"

#define TestLogInfo(format, ...) {printf(format, ##__VA_ARGS__); std::cout << std::endl;}
#define TestLogError(format, ...) {fprintf(stderr, format, ##__VA_ARGS__); std::cerr << std::endl;}

#define DEFAULT_CLIENT_NAME "Azure OSConfig"
#define MODEL_VERSION_NAME "ModelVersion"
#define CONFIG_FILE "/etc/osconfig/osconfig.json"
#define DEFAULT_DEVICE_MODEL_ID 7
#define DEFAULT_FULL_CLIENT_NAME_TEMPLATE DEFAULT_CLIENT_NAME " %d;%s"
#define CLIENT_NAME_MAX_SIZE 128

static inline std::string GetFullClientName()
{
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    int modelNumber = DEFAULT_DEVICE_MODEL_ID;

    char* jsonConfiguration = LoadStringFromFile(CONFIG_FILE, false, NULL);
    if (NULL != jsonConfiguration)
    {
        if (NULL != (rootValue = json_parse_string(jsonConfiguration)))
        {
            if (NULL != (rootObject = json_value_get_object(rootValue)))
            {
                modelNumber = (int)json_object_get_number(rootObject, MODEL_VERSION_NAME);
            }
            else
            {
                TestLogError("GetModelVersionFromJsonConfig: Failed to reveive value, using default (%d)", DEFAULT_DEVICE_MODEL_ID);
            }
            json_value_free(rootValue);
        }
        else
        {
            TestLogError("GetModelVersionFromJsonConfig: Failed to read json, using default (%d)", DEFAULT_DEVICE_MODEL_ID);
        }
    }
    else
    {
        TestLogError("GetModelVersionFromJsonConfig: No configuration data, using default (%d)", DEFAULT_DEVICE_MODEL_ID);
    }

    char fullClientName[CLIENT_NAME_MAX_SIZE];
    snprintf(fullClientName, sizeof(fullClientName), DEFAULT_FULL_CLIENT_NAME_TEMPLATE, modelNumber, OSCONFIG_VERSION);
    return fullClientName;
}

#endif // MODULESTESTCOMMON_H