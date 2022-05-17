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

#include <CommonUtils.h>
#include <Logging.h>
#include <Mpi.h>
#include <Mmi.h>

#include <parson.h>

#ifdef __cplusplus

#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <vector>
#include <algorithm>
#include <cinttypes>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <thread>
#include <tuple>
#include <dlfcn.h>
#include <iostream>
#include <unordered_set>
#include <sstream>
#include <future>
#include <ctime>
#include <chrono>

#include <ScopeGuard.h>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#endif //#ifdef __cplusplus

#ifdef __cplusplus
extern "C"
{
#endif

OSCONFIG_LOG_HANDLE GetPlatformLog();

void AreModulesLoadedAndLoadIfNot(void);
void UnloadModules(void);

#ifdef __cplusplus
}
#endif

#endif // PLATFORMCOMMON_H
