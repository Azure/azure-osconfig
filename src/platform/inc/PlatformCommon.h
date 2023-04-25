// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PLATFORMCOMMON_H
#define PLATFORMCOMMON_H

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <parson.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <MmiClient.h>
#include <Mpi.h>
#include <ModulesManager.h>

#include <version.h>

OSCONFIG_LOG_HANDLE GetPlatformLog(void);

#endif // PLATFORMCOMMON_H