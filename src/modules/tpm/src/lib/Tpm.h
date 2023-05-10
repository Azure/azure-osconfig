// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TPM_H
#define TPM_H

void TpmInitialize(void);
void TpmShutdown(void);

MMI_HANDLE TpmMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void TpmMmiClose(MMI_HANDLE clientSession);
int TpmMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int TpmMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int TpmMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
void TpmMmiFree(MMI_JSON_STRING payload);

#endif // TPM_H
