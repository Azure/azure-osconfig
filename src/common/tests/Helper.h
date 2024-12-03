#ifndef HELPER_H
#define HELPER_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
int BackupSshdConfigTest(char const* c);
void SwapGlobalSshServerConfigs(const char** config, const char** backup, const char** remediation);
int GetSshdVersionTest(int *major, int *minor);
int IsSshConfigIncludeSupportedTest(void);
int SaveRemediationToSshdConfigTest(void);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HELPER_H
