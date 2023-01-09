// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef WATCHER_H
#define WATCHER_H

#ifdef __cplusplus
extern "C"
{
#endif

void SaveReportedConfigurationToFile(const char* fileName, size_t* hash);
void ProcessDesiredConfigurationFromFile(const char* fileName, size_t* hash);

int RefreshDcGitRepositoryClone(const char* gitRepositoryUrl, const char* gitBranch, const char* gitClonePath, const chat* gitClonedDcFile);

#ifdef __cplusplus
}
#endif

#endif // WATCHER_H
