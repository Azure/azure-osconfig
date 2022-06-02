// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MODULESTESTCOMMON_H
#define MODULESTESTCOMMON_H

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
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
#include <ManagementModule.h>

#include "MimParser.h"
#include "TestRecipeParser.h"

constexpr const size_t g_lineLength = 256;

inline void TestLogInfo(const char* format, const char* args...)
{
    char buf[g_lineLength] = {0};
    std::snprintf(buf, g_lineLength, format, args);
    std::cout << buf << std::endl;
}

inline void TestLogError(const char* log)
{
    std::cerr << log;
}

inline void TestLogError(const char* format, const char* args...)
{
    char buf[g_lineLength] = {0};
    std::snprintf(buf, g_lineLength, format, args);
    std::cerr << buf;
}

// Use <filesystem> or <experimental/filesystem> based on gcc lib availability
#if __has_include(<filesystem>)
#   include <filesystem>
#else
#   include <experimental/filesystem>
// Alias namespace
    namespace std {
        namespace filesystem = experimental::filesystem;
    }
#endif

#endif // MODULESTESTCOMMON_H