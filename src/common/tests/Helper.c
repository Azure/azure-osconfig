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

int GetSshdVersionTest(int *major, int *minor)
{
    return GetSshdVersion(major, minor, NULL);
}

int IsSshConfigIncludeSupportedTest(void)
{
    return IsSshConfigIncludeSupported(NULL);
}

int SaveRemediationToSshdConfigTest(void)
{
    return SaveRemediationToSshdConfig(NULL);
}