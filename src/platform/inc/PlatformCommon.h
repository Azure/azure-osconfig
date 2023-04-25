// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PLATFORMCOMMON_H
#define PLATFORMCOMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <version.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <parson.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <Mpi.h>
#include <ModulesManager.h>

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