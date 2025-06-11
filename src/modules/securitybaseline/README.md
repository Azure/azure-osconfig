# SecurityBaseline

## Introduction

SecurityBaseline is an [OSConfig Management Module](../../../docs/modules.md) that audits and remediates the [Azure Security Baseline for Linux](https://learn.microsoft.com/en-us/azure/governance/policy/samples/guest-configuration-baseline-linux).

The Module Interface Model (MIM) for Security Baseline is at [src/modules/mim/securitybaseline.json](../mim/securitybaseline.json). This MIM implements a single MIM component, `SecurityBaseline`. This component contains sets (pairs or triplets) of reported and desired MIM objects for the individual baseline checks with names that follow the respective check descriptions. For example:

 Check description | Reported MIM object  | Desired MIM object
-----|-----|-----
Ensure nodev option set on home partition | `auditEnsureNodevOptionOnHomePartition` | `remediateEnsureNodevOptionOnHomePartition`
Ensure users own their home directories | `auditEnsureUsersOwnTheirHomeDirectories` | `remediatesEnsureUsersOwnTheirHomeDirectories`

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
