// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PNPAGENT_H
#define PNPAGENT_H

#ifdef __cplusplus
extern "C"
{
#endif

int InitializeAgent(const char* connectionString);
void AgentDoWork(void);
void CloseAgent(void);
void ScheduleRefreshConnection(void);

#ifdef __cplusplus
}
#endif

#endif // PNPAGENT_H
