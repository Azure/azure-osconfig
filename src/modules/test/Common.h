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
#include <string.h>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <CommonUtils.h>
#include <Mmi.h>
#include <Logging.h>

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
#include "TestRecipeParser.h"
#include "RecipeInvoker.h"
#include "MimParser.h"

#define TestLogInfo(format, ...) {printf(format, ##__VA_ARGS__); std::cout << std::endl;}
#define TestLogError(format, ...) {fprintf(stderr, format, ##__VA_ARGS__); std::cerr << std::endl;}

#endif // MODULESTESTCOMMON_H