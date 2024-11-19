// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

static const char* g_etcPamdCommonPassword = "/etc/pam.d/common-password";
static const char* g_etcSecurityPwQualityConf = "/etc/security/pwquality.conf";
static const char* g_etcPamdSystemAuth = "/etc/pam.d/system-auth";
static const char* g_pamUnixSo = "pam_unix.so";
static const char* g_remember = "remember";

static char* FindPamModule(const char* pamModule, void* log)
{
    const char* paths[] = {
        "/usr/lib/x86_64-linux-gnu/security/%s",
        "/usr/lib/security/%s",
        "/lib/security/%s",
        "/lib64/security/%s",
        "/lib/x86_64-linux-gnu/security/%s" };
    int numPaths = ARRAY_SIZE(paths);
    char* result = NULL;
    int status = 0, i = 0;

    if (NULL == pamModule)
    {
        OsConfigLogError(log, "FindPamModule: invalid argument");
        return NULL;
    }

    for (i = 0; i < numPaths; i++)
    {
        if (NULL != (result = FormatAllocateString(paths[i], pamModule)))
        {
            if (0 == (status = CheckFileExists(result, NULL, log)))
            {
                break;
            }
            else
            {
                FREE_MEMORY(result);
            }
        }
        else
        {
            OsConfigLogError(log, "FindPamModule: out of memory");
            break;
        }
    }

    if (result)
    {
        OsConfigLogInfo(log, "FindPamModule: the PAM module '%s' is present on this system as '%s'", pamModule, result);
    }
    else
    {
        OsConfigLogError(log, "FindPamModule: the PAM module '%s' is not present on this system", pamModule);
    }

    return result;
}

int CheckEnsurePasswordReuseIsLimited(int remember, char** reason, void* log)
{
    int status = ENOENT;
    char* pamModule = NULL;

    if (0 == CheckFileExists(g_etcPamdCommonPassword, NULL, log))
    {
        // On Debian-based systems '/etc/pam.d/common-password' is expected to exist
        status = ((0 == CheckLineFoundNotCommentedOut(g_etcPamdCommonPassword, '#', g_remember, reason, log)) &&
            (0 == CheckIntegerOptionFromFileLessOrEqualWith(g_etcPamdCommonPassword, g_remember, '=', remember, reason, log))) ? 0 : ENOENT;
    }
    else if (0 == CheckFileExists(g_etcPamdSystemAuth, NULL, log))
    {
        // On Red Hat-based systems '/etc/pam.d/system-auth' is expected to exist
        status = ((0 == CheckLineFoundNotCommentedOut(g_etcPamdSystemAuth, '#', g_remember, reason, log)) &&
            (0 == CheckIntegerOptionFromFileLessOrEqualWith(g_etcPamdSystemAuth, g_remember, '=', remember, reason, log))) ? 0 : ENOENT;
    }
    else
    {
        OsConfigCaptureReason(reason, "Neither '%s' or '%s' found, unable to check for '%s' option being set",
            g_etcPamdCommonPassword, g_etcPamdSystemAuth, g_remember);
    }

    if (status)
    {
        if (NULL == (pamModule = FindPamModule(g_pamUnixSo, log)))
        {
            OsConfigCaptureReason(reason, "The PAM module '%s' is not available. Automatic remediation is not possible", g_pamUnixSo);
        }
        FREE_MEMORY(pamModule);
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
    // - 'shadow': specifies that the password information shall be stored in the /etc/shadow file
    // - 'remember=n': sets the number of previous passwords to remember to prevent password reuse
    // - 'retry=3': the number of times a user can retry entering their password before failing
    //
    // An alternative is:
    //
    // const char* endsHereIfSucceedsTemplate = "password sufficient pam_unix.so sha512 shadow %s=%d retry=3\n";
    //
    // Where 'sufficient' says that if this module succeeds other modules are not invoked.
    // While 'required'  says that if this module fails, authentication fails.

    const char* endsHereIfFailsTemplate = "password required %s sha512 shadow %s=%d retry=3\n";
    char* pamModulePath = NULL;
    char* newline = NULL;
    int status = 0, _status = 0;

    EnsurePamModulePackagesAreInstalled(log);

    if (NULL == (pamModulePath = FindPamModule(g_pamUnixSo, log)))
    {
        OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: cannot proceed without %s being present", g_pamUnixSo);
        return ENOENT;
    }

    if (NULL != (newline = FormatAllocateString(endsHereIfFailsTemplate, pamModulePath, g_remember, remember)))
    {
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
    }
    else
    {
        OsConfigLogError(log, "SetEnsurePasswordReuseIsLimited: out of memory");
        status = ENOMEM;
    }

    FREE_MEMORY(newline);
    FREE_MEMORY(pamModulePath);

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
            // 'auth required pam_tally2.so onerr=fail audit silent deny=5 unlock_time=900'
            // 'auth required pam_faillock.so preauth silent audit deny=3 unlock_time=900'
            // 'auth required pam_tally.so onerr=fail deny=3 unlock_time=900'

            if ((commentCharacter == line[0]) || (EOL == line[0]))
            {
                status = 0;
                continue;
            }
            else if ((NULL != strstr(line, auth)) && (NULL != strstr(line, pamSo)) &&
                (NULL != (authValue = GetStringOptionFromBuffer(line, auth, ' ', log))) && (0 == strcmp(authValue, required)) &&
                (0 <= (deny = GetIntegerOptionFromBuffer(line, "deny", '=', log))) && (deny <= 5) &&
                (0 < (unlockTime = GetIntegerOptionFromBuffer(line, "unlock_time", '=', log))))
            {
                OsConfigLogInfo(log, "CheckLockoutForFailedPasswordAttempts: '%s %s %s' found uncommented with 'deny' set to %d and 'unlock_time' set to %d in '%s'",
                    auth, required, pamSo, deny, unlockTime, fileName);
                OsConfigCaptureSuccessReason(reason, "'%s %s %s' found uncommented with 'deny' set to %d and 'unlock_time' set to %d in '%s'",
                    auth, required, pamSo, deny, unlockTime, fileName);

                status = 0;
                FREE_MEMORY(authValue);
                break;
            }
            else
            {
                FREE_MEMORY(authValue);
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
    // 'auth required pam_faillock.so preauth silent audit deny=3 unlock_time=900 even_deny_root'
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
    // - 'file=/var/log/tallylog': the default log file used to keep login counts
    // - 'onerr=fail': if an error occurs (e.g., unable to open a file), return with a PAM error code
    // - 'audit': generate an audit record for this event
    // - 'silent': do not display any error messages
    // - 'deny=5': deny access if the tally (failed login attempts) for this user exceeds 5 times
    // - 'unlock_time=900': allow access after 900 seconds (15 minutes) following a failed attempt

    const char* pamFailLockLineTemplate = "auth required %s preauth silent audit deny=3 unlock_time=900 even_deny_root\n";
    const char* pamTally2LineTemplate = "auth required %s file=/var/log/tallylog onerr=fail audit silent deny=5 unlock_time=900 even_deny_root\n";
    const char* pamTallyDenyLineTemplate = "auth required %s onerr=fail deny=3 unlock_time=900\nauth required %s\n";
    const char* pamFaillockSo = "pam_faillock.so";
    const char* pamTally2So = "pam_tally2.so";
    const char* pamTallySo = "pam_tally.so";
    const char* pamDenySo = "pam_deny.so";
    const char* pamConfigurations[] = {"/etc/pam.d/login", "/etc/pam.d/system-auth", "/etc/pam.d/password-auth", "/etc/pam.d/common-auth"};
    int numPamConfigurations = ARRAY_SIZE(pamConfigurations);
    char* pamModulePath = NULL;
    char* pamModulePath2 = NULL;
    char* line = NULL;
    int i = 0, status = 0, _status = 0;

    EnsurePamModulePackagesAreInstalled(log);

    for (i = 0; i < numPamConfigurations; i++)
    {
        if (0 == CheckFileExists(pamConfigurations[i], NULL, log))
        {
            if (NULL != (pamModulePath = FindPamModule(pamFaillockSo, log)))
            {
                if (NULL != (line = FormatAllocateString(pamFailLockLineTemplate, pamModulePath)))
                {
                    _status = ReplaceMarkedLinesInFile(pamConfigurations[i], pamFaillockSo, line, '#', true, log);
                    FREE_MEMORY(line);
                }
                else
                {
                    _status = ENOMEM;
                }

                FREE_MEMORY(pamModulePath);
            }
            else if (NULL != (pamModulePath = FindPamModule(pamTally2So, log)))
            {
                if (NULL != (line = FormatAllocateString(pamTally2LineTemplate, pamModulePath)))
                {
                    _status = ReplaceMarkedLinesInFile(pamConfigurations[i], pamTally2So, line, '#', true, log);
                    FREE_MEMORY(line);
                }
                else
                {
                    _status = ENOMEM;
                }

                FREE_MEMORY(pamModulePath);
            }
            else if ((NULL != (pamModulePath = FindPamModule(pamTallySo, log))) &&
                (NULL != (pamModulePath2 = FindPamModule(pamDenySo, log))))
            {
                if (NULL != (line = FormatAllocateString(pamTallyDenyLineTemplate, pamModulePath, pamModulePath2)))
                {
                    _status = ReplaceMarkedLinesInFile(pamConfigurations[i], pamTallySo, line, '#', true, log);
                    FREE_MEMORY(line);
                }
                else
                {
                    _status = ENOMEM;
                }

                FREE_MEMORY(pamModulePath);
                FREE_MEMORY(pamModulePath2);
            }

            if (_status && (_status != status))
            {
                status = _status;
            }

            if (ENOMEM == status)
            {
                OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: out of memory");
                break;
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
                ((NULL != strstr(line, pamPwQualitySo)) || (NULL != strstr(line, pamCrackLibSo)) || (NULL != strstr(line, g_pamUnixSo))))
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
    bool pamUnixSoExists = false;
    char* pamModulePath = NULL;
    char* pamModulePath2 = NULL;
    char* pamModulePath3 = NULL;
    int i = 0, status = 0, _status = 0;
    char* line = NULL;

    if (0 == CheckFileExists(g_etcPamdCommonPassword, NULL, log))
    {
        EnsurePamModulePackagesAreInstalled(log);

        pamPwQualitySoExists = (NULL != (pamModulePath = FindPamModule(pamPwQualitySo, log))) ? true : false;
        pamCrackLibSoExists = (NULL != (pamModulePath2 = FindPamModule(pamCrackLibSo, log))) ? true : false;
        pamUnixSoExists = (NULL != (pamModulePath3 = FindPamModule(g_pamUnixSo, log))) ? true : false;

        if (pamPwQualitySoExists || pamCrackLibSoExists || pamUnixSoExists)
        {
            if (NULL != (line = FormatAllocateString(etcPamdCommonPasswordLineTemplate,
                pamPwQualitySoExists ? pamModulePath : (pamCrackLibSoExists ? pamModulePath2 : pamModulePath3),
                retry, minlen, lcredit, ucredit, ocredit, dcredit)))
            {
                status = ReplaceMarkedLinesInFile(g_etcPamdCommonPassword, pamPwQualitySoExists ? pamPwQualitySo : (pamCrackLibSoExists ? pamCrackLibSo : g_pamUnixSo), line, '#', true, log);
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

        FREE_MEMORY(pamModulePath);
        FREE_MEMORY(pamModulePath2);
        FREE_MEMORY(pamModulePath3);
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
