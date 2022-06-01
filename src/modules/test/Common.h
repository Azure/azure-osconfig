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
#include <Logging.h>
#include <Mmi.h>
#include <ManagementModule.h>

#define TEST_LOG_INFO(format, ...) OSCONFIG_LOG_INFO(NULL, format, ## __VA_ARGS__)
#define TEST_LOG_ERROR(format, ...) {\
    size_t s = std::snprintf(NULL, 0, format, ## __VA_ARGS__);\
    std::unique_ptr<char[]> buf( new char[ s + 1 ] );\
    std::snprintf(buf.get(), s + 1, format, ## __VA_ARGS__);\
    ADD_FAILURE() << buf.get();\
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