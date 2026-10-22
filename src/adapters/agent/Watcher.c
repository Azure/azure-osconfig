// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/Agent.h"

#define MPI_CLIENT_NAME "OSConfig Watcher"
#define MAX_PAYLOAD_LENGTH 0

// The local Desired Configuration (DC) and Reported Configuration (RC) files
#define DC_FILE "/etc/osconfig/osconfig_desired.json"
#define RC_FILE "/etc/osconfig/osconfig_reported.json"

static int g_localManagement = 0;
static size_t g_reportedHash = 0;
static size_t g_desiredHash = 0;

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
            if ((*hash != (payloadHash = HashString(payload))) && payloadHash)
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

static void ProcessDesiredConfigurationFromFile(const char* fileName, size_t* hash, OsConfigLogHandle log)
{
    size_t payloadHash = 0;
    int payloadSizeBytes = 0;
    char* payload = NULL;
    bool platformAlreadyRunning = true;
    int mpiResult = MPI_OK;

    if (fileName && hash)
    {
        RestrictFileAccessToCurrentAccountOnly(fileName);

        payload = LoadStringFromFile(fileName, false, log);
        if (payload && (0 != (payloadSizeBytes = strlen(payload))))
        {
            // Do not call MpiSetDesired unless this desired configuration is different from previous
            if ((*hash != (payloadHash = HashString(payload))) && payloadHash)
            {
                OsConfigLogInfo(log, "Watcher: processing DC payload from '%s'", fileName);

                mpiResult = CallMpiSetDesired((MPI_JSON_STRING)payload, payloadSizeBytes, log);
                if ((MPI_OK != mpiResult) && RefreshMpiClientSession(&platformAlreadyRunning) && (false == platformAlreadyRunning))
                {
                    mpiResult = CallMpiSetDesired((MPI_JSON_STRING)payload, payloadSizeBytes, log);
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

static int ProtectDcFile(const char* gitClonedDcFile, OsConfigLogHandle log)
{
    int error = 0;

    if (NULL == gitClonedDcFile)
    {
        OsConfigLogError(log, "ProtectDcFile: invalid argument");
        error = EINVAL;
    }
    else if (false == FileExists(gitClonedDcFile))
    {
        OsConfigLogError(log, "Watcher: bad Git clone, DC file %s not found", gitClonedDcFile);
        error = EACCES;
    }
    else if (0 != (error = RestrictFileAccessToCurrentAccountOnly(gitClonedDcFile)))
    {
        OsConfigLogError(log, "Watcher: failed protecting Git DC file %s", gitClonedDcFile);
    }

    return error;
}

void InitializeWatcher(const char* jsonConfiguration, OsConfigLogHandle log)
{
    if (NULL != jsonConfiguration)
    {
        g_localManagement = GetLocalManagementFromJsonConfig(jsonConfiguration, log);
    }

    RestrictFileAccessToCurrentAccountOnly(DC_FILE);
    RestrictFileAccessToCurrentAccountOnly(RC_FILE);
}

void WatcherDoWork(OsConfigLogHandle log)
{
    if (g_localManagement)
    {
        ProcessDesiredConfigurationFromFile(DC_FILE, &g_desiredHash, log);
        SaveReportedConfigurationToFile(RC_FILE, &g_reportedHash);
    }
}

void WatcherCleanup(OsConfigLogHandle log)
{
    OsConfigLogInfo(log, "Watcher shutting down");
}

bool IsWatcherActive(void)
{
    return g_localManagement ? true : false;
}
