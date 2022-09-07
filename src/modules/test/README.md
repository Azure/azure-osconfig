# ModulesTest
ModulesTest (binary name: modulestest) is OSConfig's functional module test utility, it allows module authors to write Test Recipes which in turn, drive testing of the module functionality and conformity (against the published MIM) of the Management Module.

`modulestest` uses the [Google Test](https://github.com/google/googletest/) framework and registers tests dynamically based on the given test recipes provided. As a result, there are many flags exposed to customize `modulestest` functionality. See [Usage](#usage) for usage examples.

Every module that includes a [test recipe](https://github.com/Azure/azure-osconfig/tree/main/src/modules/test/recipes) and [MIM](https://github.com/Azure/azure-osconfig/tree/main/src/modules/mim) is automatically included in the test configuration (testplate.json).
# Prerequisites
Although `modulestest` does not require any runtime dependencies, generating the Test Recipe configuration (testplate.json) does require the [jq](https://github.com/stedolan/jq) application to be installed when building OSConfig.

# Test Naming Composition
Authored test recipes use the following test naming composition when registering with the Google Test framework. This is useful diagnostics as the name is used in test reports. `ModulesTest` is the test suite name and is further composed with the `ModuleName` (defined by the directory/file name), `ComponentName` and `ObjectName` (`<null>` is used if either are empty strings).

## Example - Test Naming Convention
For example, the following test recipe (sample Tpm recipe) will create the following test names:
 * `ModulesTest.tpm.Tpm.tpmStatus`
 * `ModulesTest.tpm.<null>.tpmStatus`
 * `ModulesTest.tpm.<null>.<null>`
```
[
  {
    "ComponentName": "Tpm",
    "ObjectName": "tpmStatus",
    "Desired": false,
    "ExpectedResult": 0,
    "WaitSeconds": 0
  },
  {
    "ComponentName": "",
    "ObjectName": "tpmStatus",
    "Desired": false,
    "ExpectedResult": 22,
    "WaitSeconds": 0
  },
  {
    "ComponentName": "",
    "ObjectName": "",
    "Desired": false,
    "ExpectedResult": 22,
    "WaitSeconds": 0
  }
]
```

See [Googletest Primer - Simple Tests](https://google.github.io/googletest/primer.html#simple-tests) for more details on test suites and test naming.

# Usage
1. `modulestest` or `modulestest testplate.json` (equivalent) tests all modules which have [test recipe](https://github.com/Azure/azure-osconfig/tree/main/src/modules/test/recipes) and [MIM](https://github.com/Azure/azure-osconfig/tree/main/src/modules/mim) defined.
2. `moduletest <module name>` Example: `modulestest tpm` will perform the [test recipe](https://github.com/Azure/azure-osconfig/tree/main/src/modules/test/recipes) defined for the _tpm_ module.
3. `moduletest <module.so>` Example: `modulestest /azure-osconfig/build/modules/tpm/src/so/tpm.so` performs a basic test on the module only, this ensures the MMI interface is properly defined. No payloads are sent to modules, we simply load the module, ensure _MMIGetInfo_ returns correctly.
4. `moduletest <module.so> <mim.json> <recipe.json>` Example: `modulestest "/azure-osconfig/build/modules/tpm/src/so/tpm.so" "/azure-osconfig/src/modules/mim/tpm.json" "/azure-osconfig/src/modules/test/recipes/TpmTests.json"` Equivalent to #2

Supports any of the `-gtest_*` flags. See `--help` for additional usage details.
 * `--gtest_list_tests`
 * `--gtest_filter=POSTIVE_PATTERNS[-NEGATIVE_PATTERNS]`

# Authoring Test Recipes
See *Testing* in [docs/modules.md](https://github.com/Azure/azure-osconfig/blob/main/docs/modules.md#13-testing) for more information on authoring Test Recipes (see [src/modules/test/recipes/](https://github.com/Azure/azure-osconfig/tree/main/src/modules/test/recipes) for current recipes).

# Known limitations
  - Each object payload in a Test Recipe is currently executed in its own module session (MmiGetInfo and MmiOpen are executed before this payload and MmiClose after it). This is a temporary limitation that will be removed in the future (so that all objects payloads in a recipe will be executed into one module session).