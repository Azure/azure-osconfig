
This folder contains the artifacts and scripts used by our engineering systems (build, test, package).

[![E2E](https://github.com/Azure/azure-osconfig/actions/workflows/e2e.yml/badge.svg)](https://github.com/Azure/azure-osconfig/actions/workflows/e2e.yml)
[![Module Test](https://github.com/Azure/azure-osconfig/actions/workflows/module-test.yml/badge.svg)](https://github.com/Azure/azure-osconfig/actions/workflows/module-test.yml)

* `debian` - Debian packaging scripts
* `docker/<os>-<arch>` - Build containers for supported operating systems and architectures
  * `amd64`
  * `arm` (armv7)
  * `arm64` (aarch64)
* `pipeline` - Azure Pipelines definitions
* `e2e`
  * `cloudtest` - backup of 1ES managed CloudTest image definitions
  * `terraform` - terraform modules for provisioning Azure resources for E2E tests

> See [Actions](https://github.com/Azure/azure-osconfig/actions) for more details and the status of our [workflows](https://github.com/Azure/azure-osconfig/tree/main/.github/workflows).