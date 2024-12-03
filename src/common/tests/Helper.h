#ifndef HELPER_H
#define HELPER_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
int BackupSshdConfigTest(char const* c);
void SwapGlobalSshServerConfigs(const char** config, const char** backup, const char** remediation);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HELPER_H
