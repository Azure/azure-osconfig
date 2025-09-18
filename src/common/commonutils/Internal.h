// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef INTERNAL_H
#define INTERNAL_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <mntent.h>
#include <dirent.h>
#include <math.h>
#include <libgen.h>
#include <shadow.h>
#include <parson.h>
#include <Logging.h>
#include <Reasons.h>
#include <CommonUtils.h>
#include <Telemetry.h>
#include <version.h>

#include "../asb/Asb.h"

#if ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 30))
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

#define INT_ENOENT -999

#define MAX_STRING_LENGTH 512

// Buffer sizes for string conversion of numeric values
// Based on maximum possible digits for each type plus sign and null terminator
#define MAX_INT_STRING_LENGTH 16    // Accommodates 32-bit int values
#define MAX_LONG_STRING_LENGTH 32   // Accommodates 64-bit long values

#define OSConfigTelemetryStatusTrace(handle, callingFunctionName, status) do { char status_str[MAX_INT_STRING_LENGTH] = {0}; snprintf(status_str, sizeof(status_str), "%d", (status)); char line_str[MAX_INT_STRING_LENGTH] = {0}; snprintf(line_str, sizeof(line_str), "%d", __LINE__); const char* keyValuePairs[] = { "Filename", __FILE__, "LineNumber", line_str, "FunctionName", __func__, "CallingFunctionName", (callingFunctionName) ? (callingFunctionName) : "-", "ResultCode", status_str, "DistroName", GetOsName(NULL), "CorrelationId", getenv("activityId") ? getenv("activityId") : "", "Version", OSCONFIG_VERSION }; OSConfigTelemetryLogEvent((handle), "StatusTrace", keyValuePairs, sizeof(keyValuePairs) / sizeof(keyValuePairs[0]) / 2); } while(0)

#define OSConfigTelemetryBaselineRun(handle, baselineName, mode, durationSeconds) do { char durationSeconds_str[MAX_LONG_STRING_LENGTH] = {0}; snprintf(durationSeconds_str, sizeof(durationSeconds_str), "%.2f", (double)(durationSeconds)); const char* keyValuePairs[] = { "BaselineName", (baselineName) ? (baselineName) : "N/A", "Mode", (mode) ? (mode) : "N/A", "DurationSeconds", durationSeconds_str, "DistroName", GetOsName(NULL), "CorrelationId", getenv("activityId") ? getenv("activityId") : "", "Version", OSCONFIG_VERSION }; OSConfigTelemetryLogEvent((handle), "BaselineRun", keyValuePairs, sizeof(keyValuePairs) / sizeof(keyValuePairs[0]) / 2); } while(0)

#define OSConfigTelemetryRuleComplete(handle, componentName, objectName, objectResult, microseconds) do { char objectResult_str[MAX_INT_STRING_LENGTH] = {0}; snprintf(objectResult_str, sizeof(objectResult_str), "%d", (objectResult)); char microseconds_str[MAX_LONG_STRING_LENGTH] = {0}; snprintf(microseconds_str, sizeof(microseconds_str), "%ld", (long)(microseconds)); const char* keyValuePairs[] = { "ComponentName", (componentName) ? (componentName) : "N/A", "ObjectName", (objectName) ? (objectName) : "N/A", "ObjectResult", objectResult_str, "Microseconds", microseconds_str, "DistroName", g_prettyName ? g_prettyName : "unknown", "CorrelationId", getenv("activityId") ? getenv("activityId") : "", "Version", OSCONFIG_VERSION }; OSConfigTelemetryLogEvent((handle), "RuleComplete", keyValuePairs, sizeof(keyValuePairs) / sizeof(keyValuePairs[0]) / 2); } while(0)

#endif // INTERNAL_H
