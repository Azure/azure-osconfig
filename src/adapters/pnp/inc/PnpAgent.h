// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PNPAGENT_H
#define PNPAGENT_H

#ifdef __cplusplus
extern "C"
{
#endif

void ScheduleRefreshConnection(void);
bool RefreshMpiClientSession(bool* platformAlreadyRunning);

#ifdef __cplusplus
}
#endif

#endif // PNPAGENT_H
