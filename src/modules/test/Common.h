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

#include <Mmi.h>
#include <ManagementModule.h>

#define UNUSED(a) (void)(a)
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define OsConfigLogInfo(log, FORMAT, ...) {\
    printf("[          ] [%s:%d]%s" FORMAT "\n", __SHORT_FILE__, __LINE__, " ", ## __VA_ARGS__);\
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

bool IsValidMimObjectPayload(const char *payload, const int payloadSizeBytes, void *log);

#endif // MODULESTESTCOMMON_H