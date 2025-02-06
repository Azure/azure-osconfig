// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <parson.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mpi.h>
#include <MpiClient.h>

#include "MI.h"
#include "MSFT_Credential.h"
#include "OMI_BaseResource.h"
#include "ReasonClass.h"
#include "OsConfigResource.h"

#define LogWithMiContext(context, miResult, log, FORMAT, ...) {\
    {\
        char message[512] = {0};\
        if (0 < snprintf(message, ARRAY_SIZE(message), FORMAT, ##__VA_ARGS__)) {\
            if (MI_RESULT_OK == miResult) {\
                MI_Context_WriteVerbose(context, message);\
            } else{\
                MI_Context_PostError(context, miResult, MI_RESULT_TYPE_MI, message);\
            }\
        }\
    }\
}\

#define LogInfo(context, log, FORMAT, ...) {\
    OsConfigLogInfo(log, FORMAT, ##__VA_ARGS__);\
    LogWithMiContext(context, MI_RESULT_OK, log, FORMAT, ##__VA_ARGS__);\
}\

#define LogError(context, miResult, log, FORMAT, ...) {\
    OsConfigLogError(log, FORMAT, ##__VA_ARGS__);\
    LogWithMiContext(context, miResult, log, FORMAT, ##__VA_ARGS__);\
}\

OSCONFIG_LOG_HANDLE GetLog(void);

#endif
