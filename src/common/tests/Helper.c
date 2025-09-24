#include "../commonutils/SshUtils.c"

int BackupSshdConfigTest(char const* c)
{
    return BackupSshdConfig(c, NULL);
}

void SwapGlobalSshServerConfigs(const char** config, const char** backup, const char** remediation)
{
    const char* temp = g_sshServerConfiguration;
    g_sshServerConfiguration = *config;
    *config = temp;
    temp = g_sshServerConfigurationBackup;
    g_sshServerConfigurationBackup = *backup;
    *backup = temp;
    temp = g_osconfigRemediationConf;
    g_osconfigRemediationConf = *remediation;
    *remediation = temp;
}
