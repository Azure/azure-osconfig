// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

static const char* g_etcPamdCommonPassword = "/etc/pam.d/common-password";
static const char* g_etcSecurityPwQualityConf = "/etc/security/pwquality.conf";
static const char* g_etcPamdSystemAuth = "/etc/pam.d/system-auth";
static const char* g_remember = "remember";

int CheckEnsurePasswordReuseIsLimited(int remember, char** reason, void* log)
{
    int status = ENOENT;

    if (0 == CheckFileExists(g_etcPamdCommonPassword, NULL, log))
    {
        // On Debian-based systems '/etc/pam.d/common-password' is expected to exist
        status = ((0 == CheckLineFoundNotCommentedOut(g_etcPamdCommonPassword, '#', g_remember, NULL, log)) &&
            (0 == CheckIntegerOptionFromFileLessOrEqualWith(g_etcPamdCommonPassword, g_remember, '=', remember, reason, log))) ? 0 : ENOENT;
    }
    else if (0 == CheckFileExists(g_etcPamdSystemAuth, NULL, log))
    {
        // On Red Hat-based systems '/etc/pam.d/system-auth' is expected to exist
        status = ((0 == CheckLineFoundNotCommentedOut(g_etcPamdSystemAuth, '#', g_remember, NULL, log)) &&
            (0 == CheckIntegerOptionFromFileLessOrEqualWith(g_etcPamdSystemAuth, g_remember, '=', remember, reason, log))) ? 0 : ENOENT;
    }
    else
    {
        OsConfigCaptureReason(reason, "Neither '%s' or '%s' found, unable to check for '%s' option being set",
            g_etcPamdCommonPassword, g_etcPamdSystemAuth, g_remember);
    }

    return status;
}

static int IsPamModulePresent(const char* pamModule, void* log)
{
    const char* paths[] = {"/usr/lib/x86_64-linux-gnu/security/%s", "/lib/security/%s", "/usr/lib/security/%s", "/lib64/security/%s"};
    int numPaths = ARRAY_SIZE(paths);
    char* pamPath = NULL;
    int i = 0, status = 0;

    if (NULL == pamModule)
    {
        OsConfigLogError(log, "IsPamModulePresent: invalid argument");
        return EINVAL;
    }

    for (i = 0; i < numPaths; i++)
    {
        if (NULL != (pamPath = FormatAllocateString(paths[i], pamModule)))
        {
            status = CheckFileExists(pamPath, NULL, log);
            FREE_MEMORY(pamPath);
            if (0 == status)
            {
                break;
            }
        }
        else
        {
            OsConfigLogError(log, "IsPamModulePresent: out of memory");
            status = EINVAL;
            break;
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "IsPamModulePresent: the PAM module '%s' is not present on this system", pamModule);
    }

    return status;
}

static void EnsurePamModulePackagesAreInstalled(void* log)
{
    const char* pamPackages[] = {"pam", "libpam-modules", "pam_pwquality", "libpam-pwquality", "libpam-cracklib"};
    int numPamPackages = ARRAY_SIZE(pamPackages);
    int i = 0;

    for (i = 0; i < numPamPackages; i++)
    {
        InstallPackage(pamPackages[i], log);
    }
}

int SetEnsurePasswordReuseIsLimited(int remember, void* log)
{
    // This configuration line is used in the PAM (Pluggable Authentication Module) configuration
    // to set the number of previous passwords to remember in order to prevent password reuse.
    //
    // Where:
    //
    // - 'password required': specifies that the password module is required for authentication
    // - 'pam_unix.so': the PAM module responsible for traditional Unix authentication
    // - 'sha512': indicates that the SHA-512 hashing algorithm shall be used to hash passwords
    //- 'shadow': specifies that the password information shall be stored in the /etc/shadow file
    //- 'remember=n': sets the number of previous passwords to remember to prevent password reuse
    //- 'retry=3': the number of times a user can retry entering their password before failing
    //
    // An alternative is:
    //
    // const char* endsHereIfSucceedsTemplate = "password sufficient pam_unix.so sha512 shadow %s=%d retry=3\n";
    //
    // Where 'sufficient' says that if this module succeeds other modules are not invoked. 
    // While 'required'  says that if this module fails, authentication fails.

    const char* endsHereIfFailsTemplate = "password required %s sha512 shadow %s=%d retry=3\n";
    const char* pamUnixSo = "pam_unix.so";
    char* newline = NULL;
    int status = 0, _status = 0;

    EnsurePamModulePackagesAreInstalled(log);

    if (0 != IsPamModulePresent(pamUnixSo, log))
    {
        return ENOENT;
    }
    else if (NULL == (newline = FormatAllocateString(endsHereIfFailsTemplate, pamUnixSo, g_remember, remember)))
    {
        OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: out of memory");
        return ENOMEM;
    }

    if (0 == CheckFileExists(g_etcPamdSystemAuth, NULL, log))
    {
        status = ReplaceMarkedLinesInFile(g_etcPamdSystemAuth, g_remember, newline, '#', true, log);
    }

    if (0 == CheckFileExists(g_etcPamdCommonPassword, NULL, log))
    {
        if ((0 != (_status = ReplaceMarkedLinesInFile(g_etcPamdCommonPassword, g_remember, newline, '#', true, log))) && (0 == status))
        {
            status = _status;
        }
    }

    FREE_MEMORY(newline);

    OsConfigLogInfo(log, "SetEnsurePasswordReuseIsLimited(%d) complete with %d", remember, status);

    return status;
}

int CheckLockoutForFailedPasswordAttempts(const char* fileName, const char* pamSo, char commentCharacter, char** reason, void* log)
{
    const char* auth = "auth";
    const char* required = "required";
    FILE* fileHandle = NULL;
    char* line = NULL;
    char* authValue = NULL;
    int deny = INT_ENOENT;
    int unlockTime = INT_ENOENT;
    long lineMax = sysconf(_SC_LINE_MAX);
    int status = ENOENT;

    if ((NULL == fileName) || (NULL == pamSo))
    {
        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: invalid arguments");
        return EINVAL;
    }
    else if (0 != CheckFileExists(fileName, reason, log))
    {
        // CheckFileExists logs
        return ENOENT;
    }
    else if (NULL == (line = malloc(lineMax + 1)))
    {
        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: out of memory");
        return ENOMEM;
    }
    else
    {
        memset(line, 0, lineMax + 1);
    }

    if (NULL == (fileHandle = fopen(fileName, "r")))
    {
        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: cannot read from '%s'", fileName);
        status = EACCES;
    }
    else
    {
        status = ENOENT;

        while (NULL != fgets(line, lineMax + 1, fileHandle))
        {
            // Example of valid lines: 
            //
            // 'auth required pam_tally2.so onerr=fail audit silent deny=5 unlock_time=900' in /etc/pam.d/login
            // 'auth required pam_faillock.so preauth silent audit deny=3 unlock_time=900' in /etc/pam.d/system-auth

            if ((commentCharacter == line[0]) || (EOL == line[0]))
            {
                status = 0;
                continue;
            }
            else if ((NULL != strstr(line, auth)) && (NULL != strstr(line, pamSo)) && 
                (NULL != (authValue = GetStringOptionFromBuffer(line, auth, ' ', log))) && (0 == strcmp(authValue, required)) && FreeAndReturnTrue(authValue) &&
                (0 <= (deny = GetIntegerOptionFromBuffer(line, "deny", '=', log))) && (deny <= 5) &&
                (0 < (unlockTime = GetIntegerOptionFromBuffer(line, "unlock_time", '=', log))))
            {
                OsConfigLogInfo(log, "CheckLockoutForFailedPasswordAttempts: '%s %s %s' found uncommented with 'deny' set to %d and 'unlock_time' set to %d in '%s'",
                    auth, required, pamSo, deny, unlockTime, fileName);
                OsConfigCaptureSuccessReason(reason, "'%s %s %s' found uncommented with 'deny' set to %d and 'unlock_time' set to %d in '%s'",
                    auth, required, pamSo, deny, unlockTime, fileName);
                
                status = 0;
                break;
            }
            else
            {
                status = ENOENT;
            }

            memset(line, 0, lineMax + 1);
        }
        
        if (status)
        {
            if (INT_ENOENT == deny)
            {
                OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: 'deny' not found in '%s' for '%s'", fileName, pamSo);
                OsConfigCaptureReason(reason, "'deny' not found in '%s' for '%s'", fileName, pamSo);
            }
            else
            {
                OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: 'deny' found set to %d in '%s' for '%s' instead of a value between 0 and 5", 
                    deny, fileName, pamSo);
                OsConfigCaptureReason(reason, "'deny' found set to %d in '%s' for '%s' instead of a value between 0 and 5", deny, fileName, pamSo);
            }
        
            if (INT_ENOENT == unlockTime)
            {
                OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: 'unlock_time' not found in '%s' for '%s'", fileName, pamSo);
                OsConfigCaptureReason(reason, "'unlock_time' not found in '%s' for '%s'", fileName, pamSo);
            }
            else
            {
                OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: 'unlock_time' found set to %d in '%s' for '%s' instead of a positive value",
                    unlockTime, fileName, pamSo);
                OsConfigCaptureReason(reason, "'unlock_time' found set to %d in '%s' for '%s' instead of a positive value", unlockTime, fileName, pamSo);
            }
        }

        fclose(fileHandle);
    }

    FREE_MEMORY(line);

    return status;
}

int SetLockoutForFailedPasswordAttempts(void* log)
{
    // These configuration lines are used in the PAM (Pluggable Authentication Module) settings to count
    // number of attempted accesses and lock user accounts after a specified number of failed login attempts.
    //
    // For etc/pam.d/login, /etc/pam.d/system-auth and /etc/pam.d/password-auth when pam_faillock.so exists:
    //
    // 'auth required [default=die] pam_faillock.so preauth silent audit deny=3 unlock_time=900 even_deny_root'
    //
    // For etc/pam.d/login, /etc/pam.d/system-auth and /etc/pam.d/password-auth when pam_faillock.so does not exist and pam_tally2.so exists:
    //
    // 'auth required pam_tally2.so file=/var/log/tallylog onerr=fail audit silent deny=5 unlock_time=900 even_deny_root'
    //
    // Otherwise, if pam_tally.so and  pam_deny.so exist:
    //
    // 'auth required pam_tally.so onerr=fail deny=3 unlock_time=900\nauth required pam_deny.so\n'
    //
    // Where:
    //
    // - 'auth': specifies that the module is invoked during authentication
    // - 'required': the module is essential for authentication to proceed
    // - '[default=die]': sets the default behavior if the module fails (e.g., due to too many failed login attempts), then the authentication process will terminate immediately
    // - 'file=/var/log/tallylog': the default log file used to keep login counts
    // - 'onerr=fail': if an error occurs (e.g., unable to open a file), return with a PAM error code
    // - 'audit': generate an audit record for this event
    // - 'silent': do not display any error messages
    // - 'deny=5': deny access if the tally (failed login attempts) for this user exceeds 5 times
    // - 'unlock_time=900': allow access after 900 seconds (15 minutes) following a failed attempt

    const char* pamFailLockLine = "auth required [default=die] pam_faillock.so preauth silent audit deny=3 unlock_time=900 even_deny_root\n";
    const char* pamTally2Line = "auth required pam_tally2.so file=/var/log/tallylog onerr=fail audit silent deny=5 unlock_time=900 even_deny_root\n";
    const char* pamTallyDenyLine = "auth required pam_tally.so onerr=fail deny=3 unlock_time=900\nauth required pam_deny.so\n";
    const char* pamFaillockSo = "pam_faillock.so";
    const char* pamTally2So = "pam_tally2.so";
    const char* pamTallySo = "pam_tally.so";
    const char* pamDenySo = "pam_deny.so";
    const char* pamConfigurations[] = {"/etc/pam.d/login", "/etc/pam.d/system-auth", "/etc/pam.d/password-auth", "/etc/pam.d/common-auth"};
    int numPamConfigurations = ARRAY_SIZE(pamConfigurations);
    int i = 0, status = 0, _status = 0;

    EnsurePamModulePackagesAreInstalled(log);

    for (i = 0; i < numPamConfigurations; i++)
    {
        if (0 == CheckFileExists(pamConfigurations[i], NULL, log))
        {
            if (0 == IsPamModulePresent(pamFaillockSo, log))
            {
                _status = ReplaceMarkedLinesInFile(pamConfigurations[i], pamFaillockSo, pamFailLockLine, '#', true, log);
            }
            else if (0 == IsPamModulePresent(pamTally2So, log))
            {
                _status = ReplaceMarkedLinesInFile(pamConfigurations[i], pamTally2So, pamTally2Line, '#', true, log);
            }
            else if ((0 == IsPamModulePresent(pamTallySo, log)) && (0 == IsPamModulePresent(pamDenySo, log)))
            {
                _status = ReplaceMarkedLinesInFile(pamConfigurations[i], pamTallySo, pamTallyDenyLine, '#', true, log);
            }

            if (_status && (0 == status))
            {
                status = _status;
            }
        }
    }

    return status;
}

static int CheckRequirementsForCommonPassword(int retry, int minlen, int dcredit, int ucredit, int ocredit, int lcredit, char** reason, void* log)
{
    const char* pamPwQualitySo = "pam_pwquality.so";
    const char* pamCrackLibSo = "pam_cracklib.so";
    const char* password = "password";
    const char* requisite = "requisite";
    int retryOption = 0;
    int minlenOption = 0;
    int dcreditOption = 0;
    int ucreditOption = 0;
    int ocreditOption = 0;
    int lcreditOption = 0;
    char commentCharacter = '#';
    FILE* fileHandle = NULL;
    char* line = NULL;
    long lineMax = sysconf(_SC_LINE_MAX);
    bool found = false;
    int status = ENOENT;

    if (false == FileExists(g_etcPamdCommonPassword))
    {
        OsConfigLogError(log, "CheckRequirementsForCommonPassword: '%s' does not exist", g_etcPamdCommonPassword);
        OsConfigCaptureReason(reason, "%s' does not exist", g_etcPamdCommonPassword);
        return ENOENT;
    }
    else if (NULL == (line = malloc(lineMax + 1)))
    {
        OsConfigLogError(log, "CheckRequirementsForCommonPassword: out of memory");
        return ENOMEM;
    }
    else
    {
        memset(line, 0, lineMax + 1);
    }

    if (NULL == (fileHandle = fopen(g_etcPamdCommonPassword, "r")))
    {
        OsConfigLogError(log, "CheckRequirementsForCommonPassword: cannot read from '%s'", g_etcPamdCommonPassword);
        OsConfigCaptureReason(reason, "Cannot read from '%s'", g_etcPamdCommonPassword);
        status = EACCES;
    }
    else
    {
        status = ENOENT;

        while (NULL != fgets(line, lineMax + 1, fileHandle))
        {
            // Example of valid line: 
            // 'password requisite pam_pwquality.so retry=3 minlen=14 lcredit=-1 ucredit=1 ocredit=-1 dcredit=-1'
            
            if ((commentCharacter == line[0]) || (EOL == line[0]))
            {
                status = 0;
                continue;
            }
            else if ((NULL != strstr(line, password)) && (NULL != strstr(line, requisite)) && 
                ((NULL != strstr(line, pamPwQualitySo)) || (NULL != strstr(line, pamCrackLibSo))))
            {
                found = true;
                
                if ((retry == (retryOption = GetIntegerOptionFromBuffer(line, "retry", '=', log))) &&
                    (minlen == (minlenOption = GetIntegerOptionFromBuffer(line, "minlen", '=', log))) &&
                    (dcredit == (dcreditOption = GetIntegerOptionFromBuffer(line, "dcredit", '=', log))) &&
                    (ucredit == (ucreditOption = GetIntegerOptionFromBuffer(line, "ucredit", '=', log))) &&
                    (ocredit == (ocreditOption = GetIntegerOptionFromBuffer(line, "ocredit", '=', log))) &&
                    (lcredit == (lcreditOption = GetIntegerOptionFromBuffer(line, "lcredit", '=', log))))
                {
                    OsConfigLogInfo(log, "CheckRequirementsForCommonPassword: '%s' contains uncommented '%s %s' with "
                        "the expected password creation requirements (retry: %d, minlen: %d, dcredit: %d, ucredit: %d, ocredit: %d, lcredit: %d)", 
                        g_etcPamdCommonPassword, password, requisite, retryOption, minlenOption, dcreditOption, ucreditOption, ocreditOption, lcreditOption);
                    OsConfigCaptureSuccessReason(reason, "'%s' contains uncommented '%s %s' with the expected password creation requirements "
                        "(retry: %d, minlen: %d, dcredit: %d, ucredit: %d, ocredit: %d, lcredit: %d)", g_etcPamdCommonPassword, password, requisite,
                        retryOption, minlenOption, dcreditOption, ucreditOption, ocreditOption, lcreditOption);
                    status = 0;
                    break;
                }
                else
                {
                    if (INT_ENOENT == retryOption)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'retry' is missing", g_etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'retry' is missing", g_etcPamdCommonPassword);
                    }
                    else if (retryOption != retry)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'retry' is set to %d instead of %d",
                            g_etcPamdCommonPassword, retryOption, retry);
                        OsConfigCaptureReason(reason, "In '%s' 'retry' is set to %d instead of %d", g_etcPamdCommonPassword, retryOption, retry);
                    }

                    if (INT_ENOENT == minlenOption)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'minlen' is missing", g_etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'minlen' is missing", g_etcPamdCommonPassword);
                    }
                    else if (minlenOption != minlen)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'minlen' is set to %d instead of %d",
                            g_etcPamdCommonPassword, minlenOption, minlen);
                        OsConfigCaptureReason(reason, "In '%s' 'minlen' is set to %d instead of %d", g_etcPamdCommonPassword, minlenOption, minlen);
                    }

                    if (INT_ENOENT == dcreditOption)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'dcredit' is missing", g_etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'dcredit' is missing", g_etcPamdCommonPassword);
                    }
                    else if (dcreditOption != dcredit)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'dcredit' is set to '%d' instead of %d",
                            g_etcPamdCommonPassword, dcreditOption, dcredit);
                        OsConfigCaptureReason(reason, "In '%s' 'dcredit' is set to '%d' instead of %d", g_etcPamdCommonPassword, dcreditOption, dcredit);
                    }

                    if (INT_ENOENT == ucreditOption)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'ucredit' missing", g_etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'ucredit' missing", g_etcPamdCommonPassword);
                    }
                    else if (ucreditOption != ucredit)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'ucredit' set to '%d' instead of %d",
                            g_etcPamdCommonPassword, ucreditOption, ucredit);
                        OsConfigCaptureReason(reason, "In '%s' 'ucredit' set to '%d' instead of %d", g_etcPamdCommonPassword, ucreditOption, ucredit);
                    }

                    if (INT_ENOENT == ocreditOption)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'ocredit' missing", g_etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'ocredit' missing", g_etcPamdCommonPassword);
                    }
                    else if (ocreditOption != ocredit)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'ocredit' set to '%d' instead of %d",
                            g_etcPamdCommonPassword, ocreditOption, ocredit);
                        OsConfigCaptureReason(reason, "In '%s' 'ocredit' set to '%d' instead of %d", g_etcPamdCommonPassword, ocreditOption, ocredit);
                    }

                    if (INT_ENOENT == lcreditOption)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'lcredit' missing", g_etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'lcredit' missing", g_etcPamdCommonPassword);
                    }
                    else if (lcreditOption != lcredit)
                    {
                        OsConfigLogError(log, "CheckRequirementsForCommonPassword: in '%s' 'lcredit' set to '%d' instead of %d",
                            g_etcPamdCommonPassword, lcreditOption, lcredit);
                        OsConfigCaptureReason(reason, "In '%s' 'lcredit' set to '%d' instead of %d", g_etcPamdCommonPassword, lcreditOption, lcredit);
                    }
                    status = ENOENT;
                    break;
                }
            }

            memset(line, 0, lineMax + 1);
        }

        fclose(fileHandle);
    }

    FREE_MEMORY(line);

    if (false == found)
    {
        OsConfigLogError(log, "CheckRequirementsForCommonPassword: '%s' does not contain a line '%s %s' with retry, minlen, dcredit, ucredit, ocredit, lcredit password creation options",
            g_etcPamdCommonPassword, password, requisite);
        OsConfigCaptureReason(reason, "'%s' does not contain a line '%s %s' with retry, minlen, dcredit, ucredit, ocredit, lcredit password creation options",
            g_etcPamdCommonPassword, password, requisite);
        status = ENOENT;
    }

    return status;
}

static int CheckPasswordRequirementFromBuffer(const char* buffer, const char* option, const char* fileName, char separator, char comment, int desired, char** reason, void* log)
{
    int value = INT_ENOENT;
    int status = ENOENT;

    if ((NULL == buffer) || (NULL == option) || (NULL == fileName))
    {
        OsConfigLogError(log, "CheckPasswordRequirementFromBuffer: invalid arguments");
        return INT_ENOENT;
    }

    if (desired == (value = GetIntegerOptionFromBuffer(buffer, option, separator, log)))
    {
        if (comment == buffer[0])
        {
            OsConfigLogError(log, "CheckPasswordRequirementFromBuffer: '%s' is set to correct value %d in '%s' but it's commented out", option, value, fileName);
            OsConfigCaptureReason(reason, "'%s' is set to correct value %d in '%s' but it's commented out", option, value, fileName);
        }
        else
        {
            OsConfigLogInfo(log, "CheckPasswordRequirementFromBuffer: '%s' is set to correct value %d in '%s'", option, value, fileName);
            OsConfigCaptureSuccessReason(reason, "'%s' is set to correct value %d in '%s'", option, value, fileName);
            status = 0;
        }
    }
    else
    {
        if (comment == buffer[0])
        {
            OsConfigLogError(log, "CheckPasswordRequirementFromBuffer: '%s' is set to %d instead of %d in '%s' and it's commented out", option, value, desired, fileName);
            OsConfigCaptureReason(reason, "'%s' is set to %d instead of %d in '%s' and it's commented out", option, value, desired, fileName);
        }
        else
        {
            OsConfigLogError(log, "CheckPasswordRequirementFromBuffer: '%s' is set to %d instead of %d in '%s'", option, value, desired, fileName);
            OsConfigCaptureReason(reason, "'%s' is set to %d instead of %d in '%s'", option, value, desired, fileName);
        }
    }

    return status;
}

static int CheckRequirementsForPwQualityConf(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, char** reason, void* log)
{
    FILE* fileHandle = NULL;
    char* line = NULL;
    long lineMax = sysconf(_SC_LINE_MAX);
    int status = ENOENT, _status = ENOENT;

    if (false == FileExists(g_etcSecurityPwQualityConf))
    {
        OsConfigLogError(log, "CheckRequirementsForPwQualityConf: '%s' does not exist", g_etcSecurityPwQualityConf);
        OsConfigCaptureReason(reason, "'%s' does not exist", g_etcSecurityPwQualityConf);
        return ENOENT;
    }
    else if (NULL == (line = malloc(lineMax + 1)))
    {
        OsConfigLogError(log, "CheckRequirementsForPwQualityConf: out of memory");
        return ENOMEM;
    }
    else
    {
        memset(line, 0, lineMax + 1);
    }

    if (NULL == (fileHandle = fopen(g_etcSecurityPwQualityConf, "r")))
    {
        OsConfigLogError(log, "CheckRequirementsForPwQualityConf: cannot read from '%s'", g_etcSecurityPwQualityConf);
        OsConfigCaptureReason(reason, "Cannot read from '%s'", g_etcSecurityPwQualityConf);
        status = EACCES;
    }
    else
    {
        status = ENOENT;

        while (NULL != fgets(line, lineMax + 1, fileHandle))
        {
            // Example of typical lines coming by default commented out: 
            //
            //# retry = 3
            //# minlen = 8
            //# minclass = 0
            //# dcredit = 0
            //# ucredit = 0
            //# lcredit = 0
            //# ocredit = 0

            if (NULL != strstr(line, "retry"))
            {
                _status = CheckPasswordRequirementFromBuffer(line, "retry", g_etcSecurityPwQualityConf, '=', '#', retry, reason, log);
            }
            else if (NULL != strstr(line, "minlen"))
            {
                _status = CheckPasswordRequirementFromBuffer(line, "minlen", g_etcSecurityPwQualityConf, '=', '#', minlen, reason, log);
            }
            else if (NULL != strstr(line, "minclass"))
            {
                _status = CheckPasswordRequirementFromBuffer(line, "minclass", g_etcSecurityPwQualityConf, '=', '#', minclass, reason, log);
            }
            else if (NULL != strstr(line, "dcredit"))
            {
                _status = CheckPasswordRequirementFromBuffer(line, "dcredit", g_etcSecurityPwQualityConf, '=', '#', dcredit, reason, log);
            }
            else if (NULL != strstr(line, "ucredit"))
            {
                _status = CheckPasswordRequirementFromBuffer(line, "ucredit", g_etcSecurityPwQualityConf, '=', '#', ucredit, reason, log);
            }
            else if (NULL != strstr(line, "lcredit"))
            {
                _status = CheckPasswordRequirementFromBuffer(line, "lcredit", g_etcSecurityPwQualityConf, '=', '#', lcredit, reason, log);
            }
            else if (NULL != strstr(line, "ocredit"))
            {
                _status = CheckPasswordRequirementFromBuffer(line, "ocredit", g_etcSecurityPwQualityConf, '=', '#', ocredit, reason, log);
            }

            if (_status && (0 == status))
            {
                status = _status;
            }

            memset(line, 0, lineMax + 1);
        }

        fclose(fileHandle);
    }

    FREE_MEMORY(line);

    return status;
}

int CheckPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, char** reason, void* log)
{
    int status = ENOENT;

    if (0 == CheckFileExists(g_etcPamdCommonPassword, NULL, log))
    {
        status = CheckRequirementsForCommonPassword(retry, minlen, dcredit, ucredit, ocredit, lcredit, reason, log);
    }
    else if (0 == CheckFileExists(g_etcSecurityPwQualityConf, NULL, log))
    {
        status = CheckRequirementsForPwQualityConf(retry, minlen, minclass, dcredit, ucredit, ocredit, lcredit, reason, log);
    }
    else
    {
        OsConfigLogError(log, "CheckPasswordCreationRequirements: neither '%s' or '%s' exist", g_etcPamdCommonPassword, g_etcSecurityPwQualityConf);
        OsConfigCaptureReason(reason, "Neither '%s' or '%s' exist", g_etcPamdCommonPassword, g_etcSecurityPwQualityConf);
    }

    return status;
}

typedef struct PASSWORD_CREATION_REQUIREMENTS
{
    const char* name;
    int value;
} PASSWORD_CREATION_REQUIREMENTS;

int SetPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, void* log)
{
    // These lines are used for password creation requirements configuration.
    //
    // A single line for /etc/pam.d/common-password when pam_pwquality.so is present:
    //
    // 'password requisite pam_pwquality.so retry=3 minlen=14 lcredit=-1 ucredit=-1 ocredit=-1 dcredit=-1'
    //
    //  Otherwise a single line for /etc/pam.d/common-password when pam_cracklib.so is present:
    //
    // 'password required pam_cracklib.so retry=3 minlen=14 dcredit=-1 ucredit=-1 ocredit=-1 lcredit=-1'
    //
    // Separate lines for /etc/security/pwquality.conf:
    //
    // 'retry = 3'
    // 'minlen = 14'
    // 'minclass = 4' 
    // 'dcredit = -1'
    // 'ucredit = -1'
    // 'ocredit = -1' 
    // 'lcredit = -1'
    //
    // Where:
    //
    // - password requisite pam_pwquality.so: the pam_pwquality module is required during password authentication
    // - retry: the user will be prompted at most this times to enter a valid password before an error is returned
    // - minlen: the minlen parameter sets the minimum acceptable length for a password to 14 characters
    // - minclass: the minimum number of character types that must be used (e.g., uppercase, lowercase, digits, other)
    // - lcredit: the minimum number of lowercase letters required in the password (negative means no requirement)
    // - ucredit: the minimum number of uppercase letters required in the password (negative means no requirement)
    // - ocredit: the minimum number of other (non-alphanumeric) characters required in the password (negative means none)
    // - dcredit: the minimum number of digits required in the password  (negative means no requirement)
    
    const char* etcPamdCommonPasswordLineTemplate = "password requisite %s retry=%d minlen=%d lcredit=%d ucredit=%d ocredit=%d dcredit=%d\n";
    const char* etcSecurityPwQualityConfLineTemplate = "%s = %d\n";
    const char* pamPwQualitySo = "pam_pwquality.so";
    const char* pamCrackLibSo = "pam_cracklib.so";
    PASSWORD_CREATION_REQUIREMENTS entries[] = {{"retry", 0}, {"minlen", 0}, {"minclass", 0}, {"dcredit", 0}, {"ucredit", 0}, {"ocredit", 0}, {"lcredit", 0}};
    int numEntries = ARRAY_SIZE(entries);
    bool pamPwQualitySoExists = false;
    bool pamCrackLibSoExists = false;
    int i = 0, status = 0, _status = 0;
    char* line = NULL;

    if (0 == (status = CheckPasswordCreationRequirements(retry, minlen, minclass, lcredit, dcredit, ucredit, ocredit, NULL, log)))
    {
        OsConfigLogInfo(log, "SetPasswordCreationRequirements: nothing to remediate");
        return 0;
    }

    if (0 == CheckFileExists(g_etcPamdCommonPassword, NULL, log))
    {
        EnsurePamModulePackagesAreInstalled(log);

        pamPwQualitySoExists = (0 == IsPamModulePresent(pamPwQualitySo, log)) ? true : false;
        pamCrackLibSoExists = (0 == IsPamModulePresent(pamCrackLibSo, log)) ? true : false;

        if (pamPwQualitySoExists || pamCrackLibSoExists)
        {
            if (NULL != (line = FormatAllocateString(etcPamdCommonPasswordLineTemplate, pamPwQualitySoExists ? pamPwQualitySo : pamCrackLibSo, 
                retry, minlen, lcredit, ucredit, ocredit, dcredit)))
            {
                status = ReplaceMarkedLinesInFile(g_etcPamdCommonPassword, pamPwQualitySoExists ? pamPwQualitySo : pamCrackLibSo, line, '#', true, log);
                FREE_MEMORY(line);
            }
            else
            {
                OsConfigLogError(log, "SetPasswordCreationRequirements: out of memory when allocating new line for '%s'", g_etcPamdCommonPassword);
            }
        }
        else
        {
            status = ENOENT;
        }
    }

    if (0 == CheckFileExists(g_etcSecurityPwQualityConf, NULL, log))
    {
        entries[0].value = retry;
        entries[1].value = minlen;
        entries[2].value = minclass;
        entries[3].value = dcredit;
        entries[4].value = ucredit;
        entries[5].value = ocredit;
        entries[6].value = lcredit;
        
        for (i = 0; i < numEntries; i++)
        {
            if (NULL != (line = FormatAllocateString(etcSecurityPwQualityConfLineTemplate, entries[i].name, entries[i].value)))
            {
                _status = ReplaceMarkedLinesInFile(g_etcSecurityPwQualityConf, entries[i].name, line, '#', true, log);
                FREE_MEMORY(line);
            }
            else
            {
                OsConfigLogError(log, "SetPasswordCreationRequirements: out of memory when allocating new line for '%s'", g_etcSecurityPwQualityConf);
            }
        }
    }

    if ((0 == _status) || (_status && (0 == status)))
    {
        status = _status;
    }

    return status;
}