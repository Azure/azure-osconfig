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
#include <string.h>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <ManagementModule.h>

#undef OsConfigLogInfo
#define OsConfigLogInfo(log, FORMAT, ...) {\
    printf("[%s] [%s:%d]%s" FORMAT "\n", GetFormattedTime(), __SHORT_FILE__, __LINE__, " ", ## __VA_ARGS__);\
}\

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