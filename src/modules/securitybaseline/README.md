# SecurityBaseline 

## Introduction

SecurityBaseline is an [OSConfig Management Module](../../../docs/modules.md) that audits and remediates the [Linux Security Baseline](https://learn.microsoft.com/en-us/azure/governance/policy/samples/guest-configuration-baseline-linux) from the Azure Compute Security Baselines.

The Module Interface Model (MIM) for Security Baseline is at [src/modules/mim/securitybaseline.json](../mim/securitybaseline.json). This MIM implements a single MIM component, `SecurityBaseline`. This component contains two global reported and desired MIM objects, `auditSecurityBaseline` and `remediateSecurityBaseline` that audit and remediate respectively the entire baseline, and also contains several more pairs of reported and desired MIM objects for the individuals checks with names that follow the respective check descriptions. For example:

 Check description | Reported MIM object  | Desired MIM object
-----|-----|-----
Ensure nodev option set on home partition | `auditEnsureNodevOptionOnHomePartition` | `remediateEnsureNodevOptionOnHomePartition`
Ensure users own their home directories | `auditEnsureUsersOwnTheirHomeDirectories` | `remediatesEnsureUsersOwnTheirHomeDirectories`

The SecurityBaseline module implementation is done for all Linux distributions that OSConfig targets today: Ubuntu 18.04, Ubuntu 20.04, Debian 10, Debian 11. In a future release the implementation can be expanded to other distros. 

The full set of reported MIM objects for audit is fully implemented. 

Remediation is incomplete for 102 remaining desired MIM objects. All these remaining objects are already plugged into unit-tests, functional tests with test recipe and to all management channels OSConfig supports: [Azure Policy](https://learn.microsoft.com/en-us/azure/governance/policy/overview) via [AutoManage Machine Configuration](https://learn.microsoft.com/en-us/azure/governance/machine-configuration/) and the [Universal NRP](../../adapters/mc/), GitOps, Digital Twins via [IoT Hub](https://learn.microsoft.com/en-us/azure/iot-hub/), Local Management, etc. All that remains are implementations for these checks.

The implementation of the checks follows a rule where there are general utility check functions added to [commonutils](../../common/commonutils/) libraries which are then simply invoked from the [SecurityBaseline module implementation](src/lib/). This will allow us to reuse those checks for other security baseline imlementations in the future.

For example there are functions in [commonutils](../../common/commonutils/) that check and set file access:

```C
int CheckFileAccess(const char* fileName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, char** reason, void* log);
int SetFileAccess(const char* fileName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, void* log);
```

which then get invoked from several check implementations in [src/lib/securitybaseline.c](src/lib/SecurityBaseline.c), such as for example `AuditEnsurePermissionsOnEtcIssue` and `RemediateEnsurePermissionsOnEtcIssue`.

Note that for the remaining remediation checks there are missing set counterparts to the check functions in [commonutils](../../common/commonutils/) such as for example `CheckFileSystemMountingOption`, `CheckSystemAccountsAreNonLogin`, etc. 
                                                                                                                                                                          
## Building the module

Follow the instructions in the main [README.md](../../../README.md) how to install prerequisites and how to build OSConfig. The SecurityBaseline module is built with the rest of OSConfig.

## Testing the module

### Unit tests

From the build directory, run the unit-tests for the Security Baseline module with:

```bash
sudo modules/securitybaseline/tests/securitybaselinetests
```

Run the unit-tests for the [commonutils](../../common/commonutils/) libraries (that the SecurityBaseline module is using) with:

```bash
sudo common/tests/commontests
```

Or run all the unit-tests for the entire OSConfig project with:

```bash
sudo ctest
```

### Functional tests

The test recipe for the module is at [src/modules/test/recipes/SecurityBaselineTests.json](../test/recipes/SecurityBaselineTests.json).

From the build directory, run the test recipe with:

```bash
sudo modules/test/moduletest ../src/modules/test/recipes/SecurityBaselineTests.json
```

### Testing at runtime with OSConfig and RC/DC

See instructions in the main [README.md](../../../README.md) on how to enable local management.

When local management is enabled we can use the desired configuration (DC) file at `/etc/osconfig/osconfig_desired.json` and the reported configuration (RC) file at `/etc/osconfig/osconfig_reported.json` to test all desired and reported objects for Security Baseline.

## Continuing implementation

The remediation checks that remain to be fully implemented can be found in [src/lib/SecurityBaseline.c](src/lib/SecurityBaseline.c).

There the MIM object names constants are listed:

```C
static const char* g_remediateEnsureAllAccountsHavePasswordsObject = "remediateEnsureAllAccountsHavePasswords";
...
static const char* g_remediateEnsureUsersOwnTheirHomeDirectoriesObject = "remediateEnsureUsersOwnTheirHomeDirectories";
```

And then later the placeholder check functions that need to be completed:

```C
static int RemediateEnsureAllAccountsHavePasswords(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}
...
static int RemediateEnsureUsersOwnTheirHomeDirectories(char* value)
{
    UNUSED(value);
    return 0; //TODO: add remediation respecting all existing patterns
}
```

By returning 0 (success) these empty placeholder checks do not flag any error in the functional recipe tests. Try turning one to a non-zero value (error) and the respective functional test recipe check will fail, etc. 

### Example 

An example of a completed check, `auditEnsureAuditdServiceIsRunning` and `remediateEnsureAuditdServiceIsRunning` in [src/lib/SecurityBaseline.c](src/lib/SecurityBaseline.c):

```C
static char* AuditEnsureAuditdServiceIsRunning(void)
{
    return CheckIfDaemonActive(g_auditd, SecurityBaselineGetLog()) ? 
        DuplicateString(g_pass) : FormatAllocateString("Service '%s' is not running", g_auditd);
}
```

```C
static int RemediateEnsureAuditdServiceIsRunning(char* value)
{
    UNUSED(value);
    return (0 == InstallPackage(g_auditd, SecurityBaselineGetLog()) &&
        EnableAndStartDaemon(g_auditd, SecurityBaselineGetLog())) ? 0 : ENOENT;
}
```

These simple functions invoke functions like `CheckIfDaemonActive` and `InstallPackage` that are implemented in [commonutils](../../common/commonutils/).

Remember, we want to separate the bulk of generic check implementations from this security baseline so that they could be reused in the future for the implementations of other baselines.

Last but not least, make sure to follow [CONTRIBUTING](../../../CONTRIBUTING.md).