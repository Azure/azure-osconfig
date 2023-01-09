// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/AgentCommon.h"

void SaveReportedConfigurationToFile(const char* fileName, size_t* hash)
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

void ProcessDesiredConfigurationFromFile(const char* fileName, size_t* hash)
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
                OsConfigLogInfo(GetLog(), "Processing DC payload from %s", fileName);
            
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

int RefreshDcGitRepositoryClone(const char* gitRepositoryUrl, const char* gitBranch, const char* gitClonePath, const char* gitClonedDcFile)
{
    const char pullCommand[] = "git pull";    
    char* cloneCommand = NULL;
    char* checkoutCommand = NULL;
    char* currentDirectory = NULL;
    int error = 0;

    if ((NULL == gitRepositoryUrl) || (NULL == gitBranch) || (NULL == gitClonePath) || (NULL == gitClonedDcFile))
    {
        OsConfigLogError(GetLog(), "RefreshDcGitRepositoryClone: invalid arguments");
        return EINVAL;
    }

    // Do not log gitRepositoryUrl as it may contain Git account credentials

    if (NULL == (cloneCommand = FormatAllocateString("git clone -q %s %s", gitRepositoryUrl, gitClonePath)))
    {
        OsConfigLogError(GetLog(), "RefreshDcGitRepositoryClone: FormatAllocateString for the clone command failed");
        error = EFAULT;
    }
    else if (NULL == (checkoutCommand = FormatAllocateString("git checkout -q %s", gitBranch)))
    {
        OsConfigLogError(GetLog(), "RefreshDcGitRepositoryClone: FormatAllocateString for the checkout command failed");
        error = EFAULT;
    }
    else if (NULL == (currentDirectory = getcwd(NULL, 0)))
    {
        OsConfigLogError(GetLog(), "RefreshDcGitRepositoryClone: getcwd failed");
        error = EFAULT;
    }
    else if (false == FileExists(gitClonedDcFile))
    {
        if (0 != (error = ExecuteCommand(NULL, cloneCommand, false, false, 0, 0, NULL, NULL, GetLog())))
        {
            OsConfigLogError(GetLog(), "Failed cloning Git repository to %s (%d)", gitClonePath, error);
        }
    }
    else if (0 != (error = chdir(GIT_DC_CLONE)))
    {
        OsConfigLogError(GetLog(), "Failed changing current directory %s (%d)", gitClonePath, error);
    }
    else if (0 != (error = ExecuteCommand(NULL, checkoutCommand, false, false, 0, 0, NULL, NULL, GetLog())))
    {
        OsConfigLogError(GetLog(), "Failed checking out Git branch %s (%d)", gitBranch, error);
    }
    else if (0 == (error = ExecuteCommand(NULL, pullCommand, false, false, 0, 0, NULL, NULL, GetLog())))
    {
        OsConfigLogError(GetLog(), "Failed Git pull from branch %s to local clone %s (%d)", gitBranch, gitClonePath, error);
    }
    else if (!FileExists(GIT_DC_FILE))
    {
        OsConfigLogError(GetLog(), "Bad Git clone, DC file %s not found", gitClonedDcFile);
    }
    else if (0 != (error = chdir(currentDirectory)))
    {
        OsConfigLogError(GetLog(), "Failed restoring current directory to %s (%d)", currentDirectory, error);
    }

    FREE_MEMORY(cloneCommand);
    FREE_MEMORY(checkoutCommand);
    FREE_MEMORY(currentDirectory);

    if (0 == error)
    {
        OsConfigLogInfo(GetLog(), "Successfully refreshed Git clone for branch %s and DC file %s", gitBranch, gitClonePath);
    }

    return error;
}