// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PLATFORMCOMMON_H
#define PLATFORMCOMMON_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <Mpi.h>
#include <parson.h>
#include <version.h>

#ifdef __cplusplus
extern "C"
{
#endif

OSCONFIG_LOG_HANDLE GetPlatformLog(void);

#ifdef __cplusplus
}
#endif

#endif // PLATFORMCOMMON_H
