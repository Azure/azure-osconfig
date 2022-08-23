# ModulesTest
ModulesTest (binary name: modulestest) is OSConfig's functional module test utility, it allows module authors to write Test Recipes which in turn, drive testing of the module functionality and conformity (against the published MIM) of the Management Module.

# Prerequisites
Although `modulestest` does not require any runtime dependencies, generating the Test Recipe configuration (testplate.json) does require the [jq](https://github.com/stedolan/jq) application to be installed when building OSConfig.

# Usage
1. `modulestest` or `modulestest testplate.json` (equivalent) tests all modules which have [test recipe](https://github.com/Azure/azure-osconfig/tree/main/src/modules/test/recipes) and [MIM](https://github.com/Azure/azure-osconfig/tree/main/src/modules/mim) defined.
2. `moduletest <module name>` Example: `modulestest tpm` will perform the [test recipe](https://github.com/Azure/azure-osconfig/tree/main/src/modules/test/recipes) defined for the _tpm_ module.
3. `moduletest <module.so>` Example: `modulestest /azure-osconfig/build/modules/tpm/src/so/tpm.so` performs a basic test on the module only, this ensures the MMI interface is properly defined. No payloads are sent to modules, we simply load the module, ensure _MMIGetInfo_ returns correctly.
4. `moduletest <module.so> <mim.json> <recipe.json>` Example: `modulestest "/azure-osconfig/build/modules/tpm/src/so/tpm.so" "/azure-osconfig/src/modules/mim/tpm.json" "/azure-osconfig/src/modules/test/recipes/TpmTests.json"` Equivalent to #2

# Authoring Test Recipes
See *Testing* in [docs/modules.md](https://github.com/Azure/azure-osconfig/blob/main/docs/modules.md#13-testing) for more information on authoring Test Recipes (see [src/modules/test/recipes/](https://github.com/Azure/azure-osconfig/tree/main/src/modules/test/recipes) for current recipes).

# Known limitations
  - Each object payload in a Test Recipe is currently executed in its own module session (MmiGetInfo and MmiOpen are executed before this payload and MmiClose after it). This is a temporary limitation that will be removed in the future (so that all objects payloads in a recipe will be executed into one module session).