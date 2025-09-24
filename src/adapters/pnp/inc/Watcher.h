// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef WATCHER_H
#define WATCHER_H

#ifdef __cplusplus
extern "C"
{
#endif

void InitializeWatcher(const char* jsonConfiguration, OsConfigLogHandle log);
void WatcherDoWork(OsConfigLogHandle log);
void WatcherCleanup(OsConfigLogHandle log);
bool IsWatcherActive(void);

#ifdef __cplusplus
}
#endif

#endif // WATCHER_H
