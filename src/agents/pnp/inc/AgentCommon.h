// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef AGENTCOMMON_H
#define AGENTCOMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "iothub.h"
#include "parson.h"
#include "iothub_device_client.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "iothubtransportmqtt_websockets.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/socketio.h"
#include "azure_uhttp_c/uhttp.h"

#include "tracelogging/TraceLoggingProvider.h"

#include <CommonUtils.h>
#include <Logging.h>
#include <Mpi.h>
#include <version.h>

// Max number of bytes allowed to go through to Twins (4KB)
#define OSCONFIG_MAX_PAYLOAD 4096

#define LogErrorJustTelemetry(log, FORMAT, ...) {\
    {\
        char errorMessage[512] = {0};\
        if (0 < snprintf(errorMessage, ARRAY_SIZE(errorMessage), FORMAT, ##__VA_ARGS__))\
        {\
            TraceLoggingWrite(g_providerHandle, "LogError", TraceLoggingString(errorMessage, "ErrorMessage"));\
        }\
    }\
}\

#define LogErrorWithTelemetry(log, FORMAT, ...) {\
    OsConfigLogError(log, FORMAT, ##__VA_ARGS__);\
    LogErrorJustTelemetry(log, FORMAT, ##__VA_ARGS__);\
}\

OSCONFIG_LOG_HANDLE GetLog();

TRACELOGGING_DECLARE_PROVIDER(g_providerHandle);

#endif // AGENTCOMMON_H
