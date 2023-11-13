// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef SSHUTILS_H
#define SSHUTILS_H

// Include CommonUtils.h in the target source before including this header

#ifdef __cplusplus
extern "C"
{
#endif

int CheckOnlyApprovedMacAlgorithmsAreUsed(const char** macs, unsigned int numberOfMacs, char** reason, void* log);
int CheckAppropriateCiphersForSsh(const char** ciphers, unsigned int numberOfCiphers, char** reason, void* log);
int CheckSshOptionIsSet(const char* option, const char* expectedValue, char** actualValue, char** reason, void* log);
int CheckSshClientAliveInterval(char** reason, void* log);
int CheckSshLoginGraceTime(char** reason, void* log);
int SetSshOption(const char* option, const char* value, void* log);
int SetSshWarningBanner(unsigned int desiredBannerFileAccess, const char* bannerText, void* log);

#ifdef __cplusplus
}
#endif

#endif // SSHUTILS_H