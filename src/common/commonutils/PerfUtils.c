// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

int StartPerfClock(struct timespec* clock, void* log)
{
    int status = EINVAL;

    if (NULL == clock)
    {
        OsConfigLogError(log, "StartPerfClock called with an invalid argument");
        return status;
    }

    if (0 != (status = clock_gettime(CLOCK_MONOTONIC, clock)))
    {
        OsConfigLogError(log, "StartPerfClock: clock_gettime failed with %d (%d)", status, errno);
    }

    return status;
}

long StopPerfClock(struct timespec* clock, void* log)
{
    struct timespec end = {0};
    int status = 0;
    long milliSeconds = -1;

    if (NULL == clock)
    {
        OsConfigLogError(log, "StopPerfClock called with an invalid argument");
        return milliSeconds;
    }

    if (0 == (status = clock_gettime(CLOCK_MONOTONIC, &end)))
    {
        if (end.tv_sec < clock->tv_sec)
        {
            OsConfigLogError(log, "StopPerfClock: clock_gettime returned an earlier time than expected (%ld seconds earlier)", clock->tv_sec - end.tv_sec);
        }
        else
        {
            milliSeconds = ((end.tv_sec - clock->tv_sec) * 1000) + (((end.tv_nsec > clock->tv_nsec) ? (end.tv_nsec - clock->tv_nsec) : 0) / 1000);
        }
    }
    else
    {
        OsConfigLogError(log, "StopPerfClock: clock_gettime failed with %d (%d)", status, errno);
    }

    OsConfigLogInfo(log, "StopPerfClock: ### %ld milliseconds", milliSeconds);

    return milliSeconds;
}