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

static SaveReportedConfigurationToFile(const char* fileName, size_t* hash)
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

static int RefreshDcGitRepositoryClone(const char* gitRepositoryUrl, const char* gitBranch, const char* gitClonePath, const char* gitClonedDcFile, void* log)
{
    const char pullCommand[] = "git pull";    
    char* cloneCommand = NULL;
    char* checkoutCommand = NULL;
    char* currentDirectory = NULL;
    int error = 0;

    if ((NULL == gitRepositoryUrl) || (NULL == gitBranch) || (NULL == gitClonePath) || (NULL == gitClonedDcFile))
    {
        OsConfigLogError(log, "RefreshDcGitRepositoryClone: invalid arguments");
        return EINVAL;
    }

    // Do not log gitRepositoryUrl as it may contain Git account credentials

    if (NULL == (cloneCommand = FormatAllocateString("git clone -q %s %s", gitRepositoryUrl, gitClonePath)))
    {
        OsConfigLogError(log, "RefreshDcGitRepositoryClone: FormatAllocateString for the clone command failed");
        error = EFAULT;
    }
    else if (NULL == (checkoutCommand = FormatAllocateString("git checkout -q %s", gitBranch)))
    {
        OsConfigLogError(log, "RefreshDcGitRepositoryClone: FormatAllocateString for the checkout command failed");
        error = EFAULT;
    }
    else if (NULL == (currentDirectory = getcwd(NULL, 0)))
    {
        OsConfigLogError(log, "RefreshDcGitRepositoryClone: getcwd failed");
        error = EFAULT;
    }
    else if (false == FileExists(gitClonedDcFile))
    {
        if (0 != (error = ExecuteCommand(NULL, cloneCommand, false, false, 0, 0, NULL, NULL, GetLog())))
        {
            OsConfigLogError(log, "RefreshDcGitRepositoryClone: failed cloning Git repository to %s (%d)", gitClonePath, error);
        }
    }
    else if (0 != (error = chdir(gitClonePath)))
    {
        OsConfigLogError(log, "RefreshDcGitRepositoryClone: failed changing current directory %s (%d)", gitClonePath, error);
    }
    else if (0 != (error = ExecuteCommand(NULL, checkoutCommand, false, false, 0, 0, NULL, NULL, GetLog())))
    {
        OsConfigLogError(log, "RefreshDcGitRepositoryClone: failed checking out Git branch %s (%d)", gitBranch, error);
    }
    else if (0 == (error = ExecuteCommand(NULL, pullCommand, false, false, 0, 0, NULL, NULL, GetLog())))
    {
        OsConfigLogError(log, "RefreshDcGitRepositoryClone: failed Git pull from branch %s to local clone %s (%d)", gitBranch, gitClonePath, error);
    }
    else if (!FileExists(gitClonedDcFile))
    {
        OsConfigLogError(log, "RefreshDcGitRepositoryClone: bad Git clone, DC file %s not found", gitClonedDcFile);
    }
    else if (0 != (error = chdir(currentDirectory)))
    {
        OsConfigLogError(log, "RefreshDcGitRepositoryClone: failed restoring current directory to %s (%d)", currentDirectory, error);
    }

    if (FileExists(gitClonedDcFile))
    {
        RestrictFileAccessToCurrentAccountOnly(gitClonedDcFile);
    }

    FREE_MEMORY(cloneCommand);
    FREE_MEMORY(checkoutCommand);
    FREE_MEMORY(currentDirectory);

    if (0 == error)
    {
        OsConfigLogInfo(log, "Watcher: successfully refreshed Git clone for branch %s and DC file %s", gitBranch, gitClonePath);
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

    if (g_gitManagement && (0 != RefreshDcGitRepositoryClone(g_gitRepositoryUrl, g_gitBranch, GIT_DC_CLONE, GIT_DC_FILE)))
    {
        OsConfigLogError(log, "Watcher failed cloning from the configured Git repository");
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

    if (g_gitManagement && (0 == RefreshDcGitRepositoryClone(g_gitRepositoryUrl, g_gitBranch, GIT_DC_CLONE, GIT_DC_FILE, log)))
    {
        ProcessDesiredConfigurationFromFile(GIT_DC_FILE, &g_gitDesiredHash, log);
    }

    if (g_localManagement)
    {
        SaveReportedConfigurationToFile(RC_FILE, &g_reportedHash);
    }
}

void ShutdownWatcher(void)
{
    g_localManagement = false;
    g_gitManageremt = false;

    FREE_MEMORY(g_gitRepositoryUrl);
    FREE_MEMORY(g_gitBranch);
}

bool IsWatcherActive(void)
{
    return (g_localManagement || g_gitManagement) ? true : false;
}