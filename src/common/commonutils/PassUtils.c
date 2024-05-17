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
        // On Debian-based systems 'etc/pam.d/common-password' is expected to exist
        status = CheckIntegerOptionFromFileLessOrEqualWith(g_etcPamdCommonPassword, g_remember, '=', remember, reason, log);
    }
    else if (0 == CheckFileExists(g_etcPamdSystemAuth, NULL, log))
    {
        // On Red Hat-based systems '/etc/pam.d/system-auth' is expected to exist
        status = CheckIntegerOptionFromFileLessOrEqualWith(g_etcPamdSystemAuth, g_remember, '=', remember, reason, log);
    }
    else
    {
        OsConfigCaptureReason(reason, "Neither '%s' or '%s' found, unable to check for '%s' option being set",
            g_etcPamdCommonPassword, g_etcPamdSystemAuth, g_remember);
    }

    return status;
}

int SetEnsurePasswordReuseIsLimited(int remember, void* log)
{
    const char* etcPamdCommonPasswordCopy = "/etc/pam.d/~common-password.copy";
    const char* etcPamdSystemAuthCopy = "/etc/pam.d/~system-auth.copy";
    const char* etcPamdCommonPasswordTemplate = "password required pam_unix.so sha512 shadow %s=%d\n";
    const char* etcPamdSystemAuthTemplate = "password required pam_pwcheck.so nullok %s=%d\n";
    char* newline = NULL;
    char* original = NULL;
    int status = 0;

    if (0 == (status = CheckEnsurePasswordReuseIsLimited(remember, NULL, log)))
    {
        OsConfigLogInfo(log, "SetEnsurePasswordReuseIsLimited: '%s' is already set to %d in '%s'", g_remember, remember, g_etcPamdCommonPassword);
        return 0;
    }

    if (0 == CheckFileExists(g_etcPamdCommonPassword, NULL, log))
    {
        if (NULL != (newline = FormatAllocateString(etcPamdCommonPasswordTemplate, g_remember, remember)))
        {
            if (NULL != (original = LoadStringFromFile(g_etcPamdCommonPassword, false, log)))
            {
                if (SavePayloadToFile(etcPamdCommonPasswordCopy, original, strlen(original), log))
                {
                    if (0 == (status = ReplaceMarkedLinesInFile(etcPamdCommonPasswordCopy, g_remember, newline, '#', log)))
                    {
                        if (0 != (status = rename(etcPamdCommonPasswordCopy, g_etcPamdCommonPassword)))
                        {
                            OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: rename('%s' to '%s') failed with %d",
                                etcPamdCommonPasswordCopy, g_etcPamdCommonPassword, errno);
                            status = (0 == errno) ? ENOENT : errno;
                        }
                    }

                    remove(etcPamdCommonPasswordCopy);
                }
                else
                {
                    OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: failed saving copy of '%s' to temp file '%s",
                        g_etcPamdCommonPassword, etcPamdCommonPasswordCopy);
                    status = EPERM;
                }

                FREE_MEMORY(original);
            }
            else
            {
                OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: failed reading '%s", g_etcPamdCommonPassword);
                status = ENOENT;
            }

            FREE_MEMORY(newline);
        }
        else
        {
            
            OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: out of memory");
            status = ENOMEM;
        }
    }

    if (0 == CheckFileExists(g_etcPamdSystemAuth, NULL, log))
    {
        if (NULL != (newline = FormatAllocateString(etcPamdSystemAuthTemplate, g_remember, remember)))
        {
            if (NULL != (original = LoadStringFromFile(g_etcPamdSystemAuth, false, log)))
            {
                if (SavePayloadToFile(etcPamdSystemAuthCopy, original, strlen(original), log))
                {
                    if (0 == (status = ReplaceMarkedLinesInFile(etcPamdSystemAuthCopy, g_remember, newline, '#', log)))
                    {
                        if (0 != (status = rename(etcPamdSystemAuthCopy, g_etcPamdSystemAuth)))
                        {
                            OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: rename('%s' to '%s') failed with %d",
                                etcPamdSystemAuthCopy, g_etcPamdSystemAuth, errno);
                            status = (0 == errno) ? ENOENT : errno;
                        }
                    }

                    remove(etcPamdSystemAuthCopy);
                }
                else
                {
                    OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: failed saving copy of '%s' to temp file '%s",
                        g_etcPamdSystemAuth, etcPamdSystemAuthCopy);
                    status = EPERM;
                }

                FREE_MEMORY(original);

            }
            else
            {
                OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: failed reading '%s", g_etcPamdSystemAuth);
                status = EACCES;
            }

            FREE_MEMORY(newline);
        }
        else
        {
            OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: out of memory");
            status = ENOMEM;
        }
    }

    FREE_MEMORY(newline);

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
    // For /etc/pam.d/login:
    //
    // 'auth required pam_tally2.so file=/var/log/tallylog onerr=fail audit silent deny=5 unlock_time=900 even_deny_root'
    //
    // For /etc/pam.d/system-auth and /etc/pam.d/password-auth:
    //
    // 'auth required [default=die] pam_faillock.so preauth silent audit deny=3 unlock_time=900 even_deny_root'
    //
    // Where:
    //
    // - 'auth': specifies that the module is invoked during authentication
    // - 'required': the module is essential for authentication to proceed
    // - '[default=die]': sets the default behavior if the module fails (e.g., due to too many failed login attempts), then the authentication process will terminate immediately
    // - 'pam_tally2.so': the PAM pam_tally2 module, which maintains a count of attempted accesses during the authentication process
    // - 'pam_faillock.so': the PAM_faillock module, which maintains a list of failed authentication attempts per user
    // - 'file=/var/log/tallylog': the default log file used to keep login counts
    // - 'onerr=fail': if an error occurs (e.g., unable to open a file), return with a PAM error code
    // - 'audit': generate an audit record for this event
    // - 'silent': do not display any error messages
    // - 'deny=5': deny access if the tally (failed login attempts) for this user exceeds 5 times
    // - 'unlock_time=900': allow access after 900 seconds (15 minutes) following a failed attempt

    const char* pamTally2Line = "auth required pam_tally2.so file=/var/log/tallylog onerr=fail audit silent deny=5 unlock_time=900 even_deny_root\n";
    const char* pamFailLockLine = "auth required [default=die] pam_faillock.so preauth silent audit deny=3 unlock_time=900 even_deny_roo\nt";
    const char* etcPamdLogin = "/etc/pam.d/login";
    const char* etcPamdPasswordAuth = "/etc/pam.d/password-auth";
    const char* etcPamdLoginCopy = "/etc/pam.d/~login.copy";
    const char* g_etcPamdSystemAuthCopy = "/etc/pam.d/~system-auth.copy";
    const char* etcPamdPasswordAuthCopy = "/etc/pam.d/~password-auth.copy";
    const char* marker = "auth";
    char* original = NULL;
    int status = ENOENT, _status = 0;

    if (0 == CheckFileExists(g_etcPamdSystemAuth, NULL, log))
    {
        if (NULL != (original = LoadStringFromFile(g_etcPamdSystemAuth, false, log)))
        {
            if (SavePayloadToFile(g_etcPamdSystemAuthCopy, original, strlen(original), log))
            {
                if (0 == (status = ReplaceMarkedLinesInFile(g_etcPamdSystemAuthCopy, marker, pamFailLockLine, '#', log)))
                {
                    if (0 != (status = rename(g_etcPamdSystemAuthCopy, g_etcPamdSystemAuth)))
                    {
                        OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: rename('%s' to '%s') failed with %d", g_etcPamdSystemAuthCopy, g_etcPamdSystemAuth, errno);
                        status = (0 == errno) ? ENOENT : errno;
                    }
                }

                remove(g_etcPamdSystemAuthCopy);
            }
            else
            {
                OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed saving copy of '%s' to temp file '%s", g_etcPamdSystemAuth, g_etcPamdSystemAuthCopy);
                status = EPERM;
            }

            FREE_MEMORY(original);
        }
        else
        {
            OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed reading '%s", g_etcPamdSystemAuth);
            status = ENOENT;
        }
    }
    
    if (0 == CheckFileExists(etcPamdPasswordAuth, NULL, log))
    {
        if (NULL != (original = LoadStringFromFile(etcPamdPasswordAuth, false, log)))
        {
            if (SavePayloadToFile(etcPamdPasswordAuthCopy, original, strlen(original), log))
            {
                if (0 == (_status = ReplaceMarkedLinesInFile(etcPamdPasswordAuthCopy, marker, pamFailLockLine, '#', log)))
                {
                    if (0 != (_status = rename(etcPamdPasswordAuthCopy, etcPamdPasswordAuth)))
                    {
                        OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: rename('%s' to '%s') failed with %d", etcPamdPasswordAuthCopy, etcPamdPasswordAuth, errno);
                        _status = (0 == errno) ? ENOENT : errno;
                    }
                }

                remove(etcPamdPasswordAuthCopy);
            }
            else
            {
                OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed saving copy of '%s' to temp file '%s", etcPamdPasswordAuth, etcPamdPasswordAuthCopy);
                _status = EPERM;
            }

            FREE_MEMORY(original);
        }
        else
        {
            OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed reading '%s", etcPamdPasswordAuth);
            _status = ENOENT;
        }

        if (_status && (0 == status))
        {
            status = _status;
        }
    }

    /*
    [2024-05-17 14:38:12] [main.c:526] Step 208 of 434
    [2024-05-17 14:38:12] [main.c:536] Running desired test 'SecurityBaseline.remediateEnsureLockoutForFailedPasswordAttempts'
    [2024-05-17 14:38:12] [FileUtils.c:285] CheckFileExists: file '/etc/pam.d/system-auth' is not found
    [2024-05-17 14:38:12] [FileUtils.c:285] CheckFileExists: file '/etc/pam.d/password-auth' is not found
    [2024-05-17 14:38:12] [FileUtils.c:280] CheckFileExists: file '/etc/pam.d/login' exists
    [2024-05-17 14:38:12] [Asb.c:5199] AsbMmiSet(SecurityBaseline, remediateEnsureLockoutForFailedPasswordAttempts, , 0) returning 2
    [2024-05-17 14:38:12] [SecurityBaseline.c:160] MmiSet(0x77c3e1b1d0a0, SecurityBaseline, remediateEnsureLockoutForFailedPasswordAttempts, , 0) returning 2
    [2024-05-17 14:38:12] [main.c:458] [ERROR] Assertion failed, expected result '0', actual '2'
    */

    if (0 == CheckFileExists(etcPamdLogin, NULL, log))
    {
        if (NULL != (original = LoadStringFromFile(etcPamdLogin, false, log)))
        {
            if (SavePayloadToFile(etcPamdLoginCopy, original, strlen(original), log))
            {
                if (0 == (_status = ReplaceMarkedLinesInFile(etcPamdLoginCopy, marker, pamTally2Line, '#', log)))
                {
                    if (0 != (_status = rename(etcPamdLoginCopy, etcPamdLogin)))
                    {
                        OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: rename('%s' to '%s') failed with %d", etcPamdLoginCopy, etcPamdLogin, errno);
                        _status = (0 == errno) ? ENOENT : errno;
                    }
                }

                remove(etcPamdLoginCopy);
            }
            else
            {
                OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed saving copy of '%s' to temp file '%s", etcPamdLogin, etcPamdLoginCopy);
                _status = EPERM;
            }

            FREE_MEMORY(original);
        }
        else
        {
            OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed reading '%s", etcPamdLogin);
            _status = ENOENT;
        }

        if (_status && (0 == status))
        {
            status = _status;
        }
    }

    return status;
}

static int CheckRequirementsForCommonPassword(int retry, int minlen, int dcredit, int ucredit, int ocredit, int lcredit, char** reason, void* log)
{
    const char* pamPwQualitySo = "pam_pwquality.so";
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
            else if ((NULL != strstr(line, password)) && (NULL != strstr(line, requisite)) && (NULL != strstr(line, pamPwQualitySo)))
            {
                found = true;
                
                if ((retry == (retryOption = GetIntegerOptionFromBuffer(line, "retry", '=', log))) &&
                    (minlen == (minlenOption = GetIntegerOptionFromBuffer(line, "minlen", '=', log))) &&
                    (dcredit == (dcreditOption = GetIntegerOptionFromBuffer(line, "dcredit", '=', log))) &&
                    (ucredit == (ucreditOption = GetIntegerOptionFromBuffer(line, "ucredit", '=', log))) &&
                    (ocredit == (ocreditOption = GetIntegerOptionFromBuffer(line, "ocredit", '=', log))) &&
                    (lcredit == (lcreditOption = GetIntegerOptionFromBuffer(line, "lcredit", '=', log))))
                {
                    OsConfigLogInfo(log, "CheckRequirementsForCommonPassword: '%s' contains uncommented '%s %s %s' with "
                        "the expected password creation requirements (retry: %d, minlen: %d, dcredit: %d, ucredit: %d, ocredit: %d, lcredit: %d)", 
                        g_etcPamdCommonPassword, password, requisite, pamPwQualitySo, retryOption, minlenOption, 
                        dcreditOption, ucreditOption, ocreditOption, lcreditOption);
                    OsConfigCaptureSuccessReason(reason, "'%s' contains uncommented '%s %s %s' with the expected password creation requirements "
                        "(retry: %d, minlen: %d, dcredit: %d, ucredit: %d, ocredit: %d, lcredit: %d)", g_etcPamdCommonPassword, password, requisite,
                        pamPwQualitySo, retryOption, minlenOption, dcreditOption, ucreditOption, ocreditOption, lcreditOption);
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
        OsConfigLogError(log, "CheckRequirementsForCommonPassword: '%s' does not contain a line '%s %s %s' with retry, minlen, dcredit, ucredit, ocredit, lcredit password creation options",
            g_etcPamdCommonPassword, password, requisite, pamPwQualitySo);
        OsConfigCaptureReason(reason, "'%s' does not contain a line '%s %s %s' with retry, minlen, dcredit, ucredit, ocredit, lcredit password creation options",
            g_etcPamdCommonPassword, password, requisite, pamPwQualitySo);
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
        if (comment != buffer[0])
        {
            OsConfigLogError(log, "CheckPasswordRequirementFromBuffer: '%s' is set to correct value %d in '%s' but is commented out", option, value, fileName);
            OsConfigCaptureReason(reason, "'%s' is set to correct value %d in '%s' but is commented out", option, value, fileName);
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
        OsConfigLogError(log, "CheckPasswordRequirementFromBuffer: '%s' is set to %d instead of %d in '%s'", option, value, desired, fileName);
        OsConfigCaptureReason(reason, "'%s' is set to %d instead of %d in '%s'", option, value, desired, fileName);
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

int SetPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, void* log)
{
    // These lines are used for password creation requirements configuration.
    //
    // A single line for /etc/pam.d/common-password:
    //
    // 'password requisite pam_pwquality.so retry=3 minlen=14 lcredit=-1 ucredit=-1 ocredit=-1 dcredit=-1'
    //
    // Separate lines for /etc/security/pwquality.conf:
    //
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
    
    const char* etcPamdCommonPasswordLineTemplate = "password requisite pam_pwquality.so retry=%d minlen=%d lcredit=%d ucredit=%d ocredit=%d dcredit=%d\n";
    const char* etcSecurityPwQualityConfLineTemplate = "%s = %d\n";
    const char* etcPamdCommonPasswordMarker = "pam_pwquality.so";
    const char* etcPamdCommonPasswordCopy = "/etc/pam.d/~common-password.copy";
    const char* etcSecurityPwQualityConfCopy = "/etc/security/~pwquality.conf.copy";
    const char* entries[] = { "minclass", "dcredit", "ucredit", "ocredit", "lcredit" };
    int numEntries = ARRAY_SIZE(entries);
    char* original = NULL;
    char* line = NULL;
    int i = 0;
    int status = ENOENT, _status = ENOENT;

    if (0 == (status = CheckPasswordCreationRequirements(retry, minlen, minclass, lcredit, dcredit, ucredit, ocredit, NULL, log)))
    {
        OsConfigLogInfo(log, "SetPasswordCreationRequirements: nothing to remediate");
        return 0;
    }

    if (0 == CheckFileExists(g_etcPamdCommonPassword, NULL, log))
    {
        if (NULL != (line = FormatAllocateString(etcPamdCommonPasswordLineTemplate, retry, minlen, lcredit, ucredit, ocredit, dcredit)))
        {
            if (NULL != (original = LoadStringFromFile(g_etcPamdCommonPassword, false, log)))
            {
                if (SavePayloadToFile(etcPamdCommonPasswordCopy, original, strlen(original), log))
                {
                    if (0 == (status = ReplaceMarkedLinesInFile(etcPamdCommonPasswordCopy, etcPamdCommonPasswordMarker, line, '#', log)))
                    {
                        if (0 != (status = rename(etcPamdCommonPasswordCopy, g_etcPamdCommonPassword)))
                        {
                            OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: rename('%s' to '%s') failed with %d", 
                                etcPamdCommonPasswordCopy, g_etcPamdCommonPassword, errno);
                            status = (0 == errno) ? ENOENT : errno;
                        }
                    }

                    remove(etcPamdCommonPasswordCopy);
                }
                else
                {
                    OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed saving copy of '%s' to temp file '%s", 
                        g_etcPamdCommonPassword, etcPamdCommonPasswordCopy);
                    status = EPERM;
                }

                FREE_MEMORY(original);
            }
            else
            {
                OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed reading '%s", g_etcPamdCommonPassword);
                status = EACCES;
            }

            FREE_MEMORY(line);
        }
        else
        {
            OsConfigLogError(log, "SetPasswordCreationRequirements: out of memory when allocating new line for '%s'", g_etcPamdCommonPassword);
        }
    }

    if (0 == CheckFileExists(g_etcSecurityPwQualityConf, NULL, log))
    {
        for (i = 0; i < numEntries; i++)
        {
            if (NULL != (line = FormatAllocateString(etcSecurityPwQualityConfLineTemplate, entries[i])))
            {
                if (NULL != (original = LoadStringFromFile(g_etcSecurityPwQualityConf, false, log)))
                {
                    if (SavePayloadToFile(etcSecurityPwQualityConfCopy, original, strlen(original), log))
                    {
                        if (0 == (_status = ReplaceMarkedLinesInFile(etcSecurityPwQualityConfCopy, entries[i], line, '#', log)))
                        {
                            if (0 != (status = rename(etcSecurityPwQualityConfCopy, g_etcSecurityPwQualityConf)))
                            {
                                OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: rename('%s' to '%s') failed with %d", 
                                    etcSecurityPwQualityConfCopy, g_etcSecurityPwQualityConf, errno);
                                _status = (0 == errno) ? ENOENT : errno;
                            }
                        }

                        remove(etcSecurityPwQualityConfCopy);
                    }

                    FREE_MEMORY(original);
                }
                else
                {
                    OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed reading '%s", g_etcSecurityPwQualityConf);
                    status = EACCES;
                }

                FREE_MEMORY(line);
            }
            else
            {
                OsConfigLogError(log, "SetPasswordCreationRequirements: out of memory when allocating new line for '%s'", g_etcSecurityPwQualityConf);
            }
        }

        if (_status && (0 == status))
        {
            status = _status;
        }
    }

    return status;
}