// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MODULE_TEST_COMMON_H
#define MODULE_TEST_COMMON_H

#include <dlfcn.h>
#include <errno.h>
#include <parson.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <Asb.h>
#include <Mmi.h>
#include <version.h>

#define LOG_INFO(format, ...) OSCONFIG_LOG_INFO(NULL, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) OSCONFIG_LOG_ERROR(NULL, format, ##__VA_ARGS__)
#define LOG_TRACE(format, ...) printf(format "\n", ## __VA_ARGS__)

#endif // MODULE_TEST_COMMON_H
