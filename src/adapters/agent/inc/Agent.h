// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef AGENT_H
#define AGENT_H

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

#ifdef __cplusplus
extern "C"
{
#endif

OsConfigLogHandle GetLog(void);
void ScheduleRefreshConnection(void);
bool RefreshMpiClientSession(bool* platformAlreadyRunning);

#ifdef __cplusplus
}
#endif

#endif // AGENT_H
