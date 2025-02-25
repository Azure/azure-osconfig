// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef WATCHER_H
#define WATCHER_H

#ifdef __cplusplus
extern "C"
{
#endif

void InitializeWatcher(const char* jsonConfiguration, OSCONFIG_LOG_HANDLE log);
void WatcherDoWork(OSCONFIG_LOG_HANDLE log);
void WatcherCleanup(OSCONFIG_LOG_HANDLE log);
bool IsWatcherActive(void);

#ifdef __cplusplus
}
#endif

#endif // WATCHER_H
