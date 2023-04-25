// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef LOG_H
#define LOG_H

#include <errno.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <version.h>

OSCONFIG_LOG_HANDLE GetPlatformLog();

#endif // LOG_H