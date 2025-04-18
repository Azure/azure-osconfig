// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

int StartPerfClock(PerfClock* clock, OsConfigLogHandle log)
{
    int status = EINVAL;

    if (NULL == clock)
    {
        OsConfigLogError(log, "StartPerfClock called with an clock invalid argument");
        return status;
    }

    memset(clock, 0, sizeof(PerfClock));

    if (0 != (status = clock_gettime(CLOCK_MONOTONIC, &(clock->start))))
    {
        OsConfigLogError(log, "StartPerfClock: clock_gettime failed with %d (%d)", status, errno);
    }

    return status;
}

int StopPerfClock(PerfClock* clock, OsConfigLogHandle log)
{
    int status = EINVAL;

    if (NULL == clock)
    {
        OsConfigLogError(log, "StopPerfClock called with an invalid clock argument");
        return status;
    }

    if (0 == (status = clock_gettime(CLOCK_MONOTONIC, &(clock->stop))))
    {
        if (clock->stop.tv_sec < clock->start.tv_sec)
        {
            OsConfigLogError(log, "StopPerfClock: clock_gettime returned an earlier time than expected (%ld seconds earlier)",
                clock->start.tv_sec - clock->stop.tv_sec);

            memset(clock, 0, sizeof(PerfClock));

            status = ENOENT;
        }
    }
    else
    {
        OsConfigLogError(log, "StopPerfClock: clock_gettime failed with %d (%d)", status, errno);
    }

    return status;
}

long GetPerfClockTime(PerfClock* clock, OsConfigLogHandle log)
{
    long seconds = 0;
    long nanoseconds = 0;
    long microseconds = -1;

    if ((NULL == clock) || (0 == clock->stop.tv_sec))
    {
        OsConfigLogError(log, "GetPerfClockTime called with an invalid clock argument");
        return microseconds;
    }

    seconds = clock->stop.tv_sec - clock->start.tv_sec;
    nanoseconds = clock->stop.tv_nsec - clock->start.tv_nsec;

    if (nanoseconds < 0)
    {
        seconds -= 1;
        nanoseconds += 1000000000;
    }

    microseconds = (seconds * 1000000) + (long)(((float)nanoseconds / 1000.0) + 0.5);

    return microseconds;
}

void LogPerfClock(PerfClock* clock, const char* componentName, const char* objectName, int objectResult, long limit, OsConfigLogHandle log)
{
    long microseconds = -1;

    if ((NULL == clock) || (NULL == componentName))
    {
        OsConfigLogError(log, "LogPerfClock called with an invalid argument");
        return;
    }

    microseconds = GetPerfClockTime(clock, log);

    if (NULL != objectName)
    {
        if (0 == objectResult)
        {
            OsConfigLogInfo(log, "%s.%s completed in %ld microseconds", componentName, objectName, microseconds);
        }
        else
        {
            OsConfigLogInfo(log, "%s.%s failed in %ld microseconds with %d", componentName, objectName, microseconds, objectResult);
        }

        if (microseconds > limit)
        {
            OsConfigLogError(log, "%s.%s completion time of %ld microseconds is longer than %ld microseconds",
                componentName, objectName, microseconds, limit);
        }
    }
    else
    {
        OsConfigLogInfo(log, "%s completed in %.2f seconds (%ld microseconds)", componentName, microseconds / 1000000.0, microseconds);

        if (microseconds > limit)
        {
            OsConfigLogError(log, "%s completion time of %ld microseconds is longer than %.2f minutes (%ld microseconds)",
                componentName, microseconds, limit / 60000000.0, limit);
        }
    }
}
