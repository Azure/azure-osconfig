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
#include <parson.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Telemetry.h>
#include <Mpi.h>
#include <MpiClient.h>
#include <version.h>

#define OSCONFIG_PLATFORM "osconfig-platform"

// Max number of bytes allowed to go through to Twins (4KB)
#define OSCONFIG_MAX_PAYLOAD 4096

OsConfigLogHandle GetLog();

#endif // AGENTCOMMON_H
