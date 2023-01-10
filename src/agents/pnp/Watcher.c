// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/AgentCommon.h"
#include "inc/PnpAgent.h"
#include "inc/AisUtils.h"

// The local Desired Configuration (DC) and Reported Configuration (RC) files
#define DC_FILE "/etc/osconfig/osconfig_desired.json"
#define RC_FILE "/etc/osconfig/osconfig_reported.json"

// The local clone for Git Desired Configuration (DC) 
#define GIT_DC_CLONE "/etc/osconfig/gitops/"
#define GIT_DC_FILE GIT_DC_CLONE "osconfig_desired.json"

static int g_localManagement = 0;
static size_t g_reportedHash = 0;
static size_t g_desiredHash = 0;

static int g_gitManagement = 0;
static char* g_gitRepositoryUrl = NULL;
static char* g_gitBranch = NULL;
static size_t g_gitDesiredHash = 0;

static void SaveReportedConfigurationToFile(const char* fileName, size_t* hash)
{
    char* payload = NULL;
    int payloadSizeBytes = 0;
    size_t payloadHash = 0;
    bool platformAlreadyRunning = true;
    int mpiResult = MPI_OK;
    
    if (fileName && hash)
    {
        mpiResult = CallMpiGetReported((MPI_JSON_STRING*)&payload, &payloadSizeBytes, GetLog());
        if ((MPI_OK != mpiResult) && RefreshMpiClientSession(&platformAlreadyRunning) && (false == platformAlreadyRunning))
        {
            CallMpiFree(payload);

            mpiResult = CallMpiGetReported((MPI_JSON_STRING*)&payload, &payloadSizeBytes, GetLog());
        }
        
        if ((MPI_OK == mpiResult) && (NULL != payload) && (0 < payloadSizeBytes))
        {
            if (*hash != (payloadHash = HashString(payload)))
            {
                if (SavePayloadToFile(fileName, payload, payloadSizeBytes, GetLog()))
                {
                    RestrictFileAccessToCurrentAccountOnly(fileName);
                    *hash = payloadHash;
                }
            }
        }
        
        CallMpiFree(payload);
    }
}

static void ProcessDesiredConfigurationFromFile(const char* fileName, size_t* hash, void* log)
{
    size_t payloadHash = 0;
    int payloadSizeBytes = 0;
    char* payload = NULL; 
    bool platformAlreadyRunning = true;
    int mpiResult = MPI_OK;

    if (fileName && hash)
    {
        RestrictFileAccessToCurrentAccountOnly(fileName);

        payload = LoadStringFromFile(fileName, false, GetLog());
        if (payload && (0 != (payloadSizeBytes = strlen(payload))))
        {
            // Do not call MpiSetDesired unless this desired configuration is different from previous
            if (*hash != (payloadHash = HashString(payload)))
            {
                OsConfigLogInfo(log, "Watcher processing DC payload from %s", fileName);

                mpiResult = CallMpiSetDesired((MPI_JSON_STRING)payload, payloadSizeBytes, GetLog());
                if ((MPI_OK != mpiResult) && RefreshMpiClientSession(&platformAlreadyRunning) && (false == platformAlreadyRunning))
                {
                    mpiResult = CallMpiSetDesired((MPI_JSON_STRING)payload, payloadSizeBytes, GetLog());
                }
            
                if (MPI_OK == mpiResult)
                {
                    *hash = payloadHash;
                }
            }
        }
        FREE_MEMORY(payload);
    }
}

static int InitializeGitClone(const char* gitRepositoryUrl, const char* gitClonePath, void* log)
{
    const char* g_gitCloneCleanUpTemplate "rm -r %s";
    const char* g_gitCloneTemplate "git clone -q %s %s";
    const char* g_gitConfigTemplate "git config --global --add safe.directory %s";
    
    char* cleanUpCommand = NULL;
    char* configCommand = NULL;
    char* cloneCommand = NULL;
    int error = 0;

    if ((NULL == gitRepositoryUrl) || (NULL == gitClonePath))
    {
        OsConfigLogError(log, "InitializeGitClone: invalid arguments");
        return EINVAL;
    }

    // Do not log gitRepositoryUrl as it may contain Git account credentials

    cleanUpCommand = FormatAllocateString(g_gitCloneCleanUpTemplate, gitClonePath);
    cloneCommand = FormatAllocateString(g_gitCloneTemplate, gitRepositoryUrl, gitClonePath);
    configCommand = FormatAllocateString(g_gitConfigTemplate, gitClonePath);
    
    ExecuteCommand(NULL, cleanUpCommand, false, false, 0, 0, NULL, NULL, log);

    if (0 != (error = ExecuteCommand(NULL, cloneCommand, false, false, 0, 0, NULL, NULL, GetLog())))
    {
        OsConfigLogError(log, "Watcher: failed making a new Git clone at %s (%d)", gitClonePath, error);
    }
    else if (0 != (error = ExecuteCommand(NULL, configCommand, false, false, 0, 0, NULL, NULL, GetLog())))
    {
        OsConfigLogError(log, "Watcher: failed configuring the new Git clone at %s (%d)", gitClonePath, error);
    }

    FREE_MEMORY(cleanUpCommand);
    FREE_MEMORY(cloneCommand);
    FREE_MEMORY(configCommand);

    if (0 == error)
    {
        OsConfigLogInfo(log, "Watcher: successfully initialized Git clone at %s", gitClonePath);
    }

    return error;
}

static int RefreshGitClone(const char* gitBranch, const char* gitClonePath, const char* gitClonedDcFile, void* log)
{
    const char* g_gitCheckoutTemplate "git checkout %s";
    const char* g_gitPullCommand "git pull";
    
    char* checkoutCommand = NULL;
    char* currentDirectory = NULL;
    int error = 0;

    if ((NULL == gitClonePath) || (NULL == gitClonedDcFile) || (NULL == gitBranch))
    {
        OsConfigLogError(log, "RefreshGitClone: invalid arguments");
        return EINVAL;
    }

    checkoutCommand = FormatAllocateString(g_gitCheckoutTemplate, gitBranch);
    currentDirectory = getcwd(NULL, 0);

    if (0 != (error = chdir(gitClonePath)))
    {
        OsConfigLogError(log, "Watcher: failed changing current directory to %s (%d)", gitClonePath, error);
    }
    else if (0 != (error = ExecuteCommand(NULL, checkoutCommand, false, false, 0, 0, NULL, NULL, GetLog())))
    {
        OsConfigLogError(log, "Watcher: failed checking out Git branch %s (%d)", gitBranch, error);
    }
    else if (0 != (error = ExecuteCommand(NULL, g_gitPullCommand, false, false, 0, 0, NULL, NULL, GetLog())))
    {
        OsConfigLogError(log, "Watcher: failed Git pull from branch %s to local clone %s (%d)", gitBranch, gitClonePath, error);
    }
    else if (!FileExists(gitClonedDcFile))
    {
        OsConfigLogError(log, "Watcher: bad Git clone, DC file %s not found", gitClonedDcFile);
    }
    else if (0 != (error = chdir(currentDirectory)))
    {
        OsConfigLogError(log, "Watcher: failed restoring current directory to %s (%d)", currentDirectory, error);
    }
    else
    {
        RestrictFileAccessToCurrentAccountOnly(gitClonedDcFile);
    }

    FREE_MEMORY(checkoutCommand);
    FREE_MEMORY(currentDirectory);

    if ((0 == error) && (IsFullLoggingEnabled()))
    {
        OsConfigLogInfo(log, "Watcher: successfully refreshed the Git clone at %s for branch %s", gitClonePath, gitBranch);
    }

    return error;
}

void InitializeWatcher(const char* jsonConfiguration, void* log)
{
    if (NULL != jsonConfiguration)
    {
        g_localManagement = GetLocalManagementFromJsonConfig(jsonConfiguration, log);
        g_gitManagement = GetGitManagementFromJsonConfig(jsonConfiguration, log);
        g_gitRepositoryUrl = GetGitRepositoryUrlFromJsonConfig(jsonConfiguration, log);
        g_gitBranch = GetGitBranchFromJsonConfig(jsonConfiguration, log);
    }

    if (g_gitManagement)
    {
        InitializeGitClone(g_gitRepositoryUrl, GIT_DC_CLONE, log);
    }

    RestrictFileAccessToCurrentAccountOnly(DC_FILE);
    RestrictFileAccessToCurrentAccountOnly(RC_FILE);
    RestrictFileAccessToCurrentAccountOnly(GIT_DC_FILE);
}

void WatcherDoWork(void* log)
{
    if (g_localManagement)
    {
        ProcessDesiredConfigurationFromFile(DC_FILE, &g_desiredHash, log);
    }

    if (g_gitManagement && (0 == RefreshGitClone(g_gitBranch, GIT_DC_CLONE, GIT_DC_FILE, log)))
    {
        ProcessDesiredConfigurationFromFile(GIT_DC_FILE, &g_gitDesiredHash, log);
    }

    if (g_localManagement)
    {
        SaveReportedConfigurationToFile(RC_FILE, &g_reportedHash);
    }
}

void WatcherCleanup(void)
{
    FREE_MEMORY(g_gitRepositoryUrl);
    FREE_MEMORY(g_gitBranch);
}

bool IsWatcherActive(void)
{
    return (g_localManagement || g_gitManagement) ? true : false;
}