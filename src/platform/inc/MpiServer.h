// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MPI_API_H
#define MPI_API_H

#ifdef __cplusplus
extern "C"
{
#endif

void MpiApiInitialize(void);
void MpiApiShutdown(void);

int LoadModules(void);
void UnloadModules(void);

#ifdef __cplusplus
}
#endif

#endif // MPI_API_H