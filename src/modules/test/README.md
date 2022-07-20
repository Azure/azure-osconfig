# modulestest
The modulestest application is OSConfig’s module testing framework, it allows module authors to write test recipes which in turn, drive testing of the module functionality and conformity (against the published MIM) of the Management Module.

# Prerequisites
Although modulestest does not require any runtime dependencies, generating the test recipe configuration (testplate.json) does require the `jq` application to be installed when building OSConfig. When installed a `testplate.json` is placed in the `modulestest` directory which includes the test recipe configurations for every module which includes a [MIM](https://github.com/Azure/azure-osconfig/tree/main/src/modules/mim) and [test recipe](https://github.com/Azure/azure-osconfig/tree/main/src/modules/test/recipes).

# Usage
1. `modulestest` or `modulestest testplate.json` (equivalent) tests all modules which have [test recipe](https://github.com/Azure/azure-osconfig/tree/main/src/modules/test/recipes) and [MIM](https://github.com/Azure/azure-osconfig/tree/main/src/modules/mim) defined.
2. `moduletest <module name>` Example: `modulestest tpm` will perform the [test recipe](https://github.com/Azure/azure-osconfig/tree/main/src/modules/test/recipes) defined for the _tpm_ module.
3. `moduletest <module.so>` Example: `modulestest /azure-osconfig/build/modules/tpm/src/so/tpm.so` performs a basic test on the module ONLY, this ensures the MMI interface is properly defined. No payloads are sent to modules, we simply load the module, ensure _MMIGetInfo_ contains the correct module definitions.
4. `moduletest <module.so> <mim.json> <recipe.json>` Example: `modulestest "/azure-osconfig/build/modules/tpm/src/so/tpm.so" "/azure-osconfig/src/modules/mim/tpm.json" "/azure-osconfig/src/modules/test/recipes/TpmTests.json"` Equivalent to #2